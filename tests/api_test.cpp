// moodycamel's queues through the module: multi-producer multi-consumer
// traffic on threads (every item exactly once, per-producer order kept),
// tokens, bulk operations, bounded try_enqueue, user types (move-only,
// throwing, over-aligned) and Traits, swap and move, the blocking queue's
// waits with and without timeouts, and the lightweight semaphore — spelled as
// upstream's README documents them.
#include "support/check.hpp"

import std;
import mcpplibs.mwrap.concurrentqueue;

using moodycamel::BlockingConcurrentQueue;
using moodycamel::ConcurrentQueue;
using moodycamel::ConsumerToken;
using moodycamel::LightweightSemaphore;
using moodycamel::ProducerToken;

namespace {

constexpr int producers = 4, consumers = 4, per_producer = 20000;

struct Item {
    int producer = -1, seq = -1;
};

// Every item dequeued once; each consumer sees each producer's items in order.
void check_traffic(const std::vector<std::vector<Item>>& seen) {
    std::vector<std::vector<char>> got(producers, std::vector<char>(per_producer, 0));
    for (const auto& one : seen) {
        std::vector<int> last(producers, -1);
        for (const Item& item : one) {
            if (item.producer < 0 || item.producer >= producers || item.seq < 0 || item.seq >= per_producer) {
                CHECK(!"an item that was never enqueued");
                return;
            }
            CHECK(item.seq > last[item.producer]);  // per-producer FIFO
            last[item.producer] = item.seq;
            CHECK(got[item.producer][item.seq]++ == 0);  // not dequeued twice
        }
    }
    std::size_t lost = 0;
    for (const auto& p : got) lost += static_cast<std::size_t>(std::ranges::count(p, 0));
    CHECK(lost == 0);
}

void ConcurrentQueue_MpmcImplicitProducers() {
    ConcurrentQueue<Item> q;
    std::atomic<int> done{0}, taken{0};
    std::vector<std::vector<Item>> seen(consumers);
    {
        std::vector<std::jthread> threads;
        for (int p = 0; p < producers; ++p)
            threads.emplace_back([&, p] {
                for (int i = 0; i < per_producer; ++i) CHECK(q.enqueue(Item{p, i}));
                done.fetch_add(1);
            });
        for (int c = 0; c < consumers; ++c)
            threads.emplace_back([&, c] {
                Item item;
                while (taken.load() < producers * per_producer)
                    if (q.try_dequeue(item)) {
                        seen[c].push_back(item);
                        taken.fetch_add(1);
                    }
            });
    }
    Item rest;
    CHECK(!(q.try_dequeue(rest)));
    check_traffic(seen);
}

void ConcurrentQueue_MpmcTokensAndBulk() {
    ConcurrentQueue<Item> q(64);
    std::atomic<int> done{0}, taken{0};
    std::vector<std::vector<Item>> seen(consumers);
    {
        std::vector<std::jthread> threads;
        for (int p = 0; p < producers; ++p)
            threads.emplace_back([&, p] {
                ProducerToken token(q);
                std::array<Item, 100> batch;
                for (int i = 0; i < per_producer; i += static_cast<int>(batch.size())) {
                    for (std::size_t k = 0; k < batch.size(); ++k) batch[k] = Item{p, i + static_cast<int>(k)};
                    CHECK(q.enqueue_bulk(token, batch.begin(), batch.size()));
                }
                done.fetch_add(1);
            });
        for (int c = 0; c < consumers; ++c)
            threads.emplace_back([&, c] {
                ConsumerToken token(q);
                std::array<Item, 37> out;
                while (taken.load() < producers * per_producer) {
                    const std::size_t n = q.try_dequeue_bulk(token, out.begin(), out.size());
                    seen[c].insert(seen[c].end(), out.begin(), out.begin() + static_cast<std::ptrdiff_t>(n));
                    taken.fetch_add(static_cast<int>(n));
                }
            });
    }
    check_traffic(seen);
}

void ConcurrentQueue_TryEnqueueIsBounded() {
    // try_enqueue never allocates: with no pre-allocated room it fails.
    ConcurrentQueue<int> empty(0);
    CHECK(!(empty.try_enqueue(1)));
    ConcurrentQueue<int> q(ConcurrentQueue<int>::BLOCK_SIZE);
    std::size_t accepted = 0;
    while (q.try_enqueue(static_cast<int>(accepted))) ++accepted;
    CHECK((accepted) >= (ConcurrentQueue<int>::BLOCK_SIZE));
    CHECK(q.enqueue(-1));  // enqueue allocates
    CHECK((q.size_approx()) == (accepted + 1));
    int v = 0;
    CHECK(q.try_dequeue(v));
    CHECK((v) == (0));
}

void ConcurrentQueue_UserTypes() {
    // Move-only.
    ConcurrentQueue<std::unique_ptr<std::string>> owned;
    owned.enqueue(std::make_unique<std::string>("moved"));
    std::unique_ptr<std::string> out;
    CHECK(owned.try_dequeue(out));
    CHECK((*out) == ("moved"));
    // A braced element.
    ConcurrentQueue<std::pair<int, std::string>> pairs;
    pairs.enqueue({7, "seven"});
    std::pair<int, std::string> p;
    CHECK(pairs.try_dequeue(p));
    CHECK((p.second) == ("seven"));
    // Over-aligned.
    struct alignas(64) Wide {
        std::array<std::uint64_t, 8> lanes{};
    };
    ConcurrentQueue<Wide> wide;
    for (std::uint64_t i = 0; i < 1000; ++i) {
        Wide w;
        w.lanes.fill(i);
        wide.enqueue(w);
    }
    Wide w;
    for (std::uint64_t i = 0; i < 1000; ++i) {
        CHECK(wide.try_dequeue(w));
        CHECK((reinterpret_cast<std::uintptr_t>(&w) % 64) == (0u));
        CHECK((w.lanes[7]) == (i));
    }
}

struct Throwing {
    static inline int copies_left = 0;
    int value = 0;
    Throwing() = default;
    explicit Throwing(int v) : value(v) {}
    Throwing(const Throwing& o) : value(o.value) {
        if (--copies_left < 0) throw std::runtime_error("copy");
    }
    Throwing& operator=(const Throwing& o) {
        if (--copies_left < 0) throw std::runtime_error("assign");
        value = o.value;
        return *this;
    }
};

void ConcurrentQueue_ExceptionsLeaveTheQueueConsistent() {
    static_assert(mcpplibs::mwrap::concurrentqueue::macros::MOODYCAMEL_EXCEPTIONS_ENABLED);
    ConcurrentQueue<Throwing> q;
    Throwing one(1), two(2);
    Throwing::copies_left = 1;
    CHECK(q.enqueue(one));
    { bool thrown = false; try { q.enqueue(two); } catch (const std::runtime_error&) { thrown = true; } CHECK(thrown); }  // the element's copy throws: nothing is enqueued
    CHECK((q.size_approx()) == (1u));
    Throwing::copies_left = 100;
    Throwing out;
    CHECK(q.try_dequeue(out));
    CHECK((out.value) == (1));
    CHECK(!(q.try_dequeue(out)));
}

// Traits: a small block size and allocation through the traits' malloc/free.
struct CountingTraits : moodycamel::ConcurrentQueueDefaultTraits {
    static constexpr std::size_t BLOCK_SIZE = 4;
    static constexpr std::size_t EXPLICIT_INITIAL_INDEX_SIZE = 4;
    static constexpr std::size_t IMPLICIT_INITIAL_INDEX_SIZE = 4;
    static inline std::atomic<int> live{0};
    static void* malloc(std::size_t size) {
        ++live;
        return std::malloc(size);
    }
    static void free(void* p) {
        if (p) --live;
        std::free(p);
    }
};

void ConcurrentQueue_TraitsSwapAndMove() {
    {
        ConcurrentQueue<int, CountingTraits> a, b;
        CHECK((CountingTraits::live.load()) > (0));
        for (int i = 0; i < 100; ++i) a.enqueue(i);  // many 4-element blocks
        b.enqueue(-1);
        swap(a, b);  // moodycamel::swap, by argument-dependent lookup
        int v = 0;
        CHECK(a.try_dequeue(v));
        CHECK((v) == (-1));
        CHECK(b.try_dequeue(v));
        CHECK((v) == (0));
        ConcurrentQueue<int, CountingTraits> c(std::move(b));
        CHECK((c.size_approx()) == (99u));
        ProducerToken t1(c), t2(c);
        moodycamel::swap(t1, t2);
        CHECK(t1.valid() && t2.valid());
        ConsumerToken k1(c), k2(c);
        moodycamel::swap(k1, k2);
        CHECK(c.try_dequeue(k1, v));
        CHECK((v) == (1));
    }
    CHECK((CountingTraits::live.load()) == (0));  // every block freed through the traits
    static_assert(ConcurrentQueue<int, CountingTraits>::BLOCK_SIZE == 4);
    static_assert(ConcurrentQueue<int>::BLOCK_SIZE == moodycamel::ConcurrentQueueDefaultTraits::BLOCK_SIZE);
    CHECK(ConcurrentQueue<int>::is_lock_free() || !ConcurrentQueue<int>::is_lock_free());
}

void ConcurrentQueue_ProducerThatExitsAndTheDequeueFromProducer() {
    ConcurrentQueue<int> q;
    ProducerToken token(q);
    for (int i = 0; i < 10; ++i) q.enqueue(token, i);
    // A thread's implicit producer outlives the thread.
    std::jthread([&] { q.enqueue(100); }).join();
    int v = 0;
    CHECK(q.try_dequeue_from_producer(token, v));
    CHECK((v) == (0));
    std::vector<int> rest;
    while (q.try_dequeue(v)) rest.push_back(v);
    CHECK((rest.size()) == (10u));
    CHECK(std::ranges::find(rest, 100) != rest.end());
}

// ── BlockingConcurrentQueue ──────────────────────────────────────────────

void BlockingConcurrentQueue_WaitDequeueAcrossThreads() {
    // Consumers block in wait_dequeue_timed until everything is taken. The
    // queue is not globally ordered, so a sentinel item could overtake real
    // ones: the consumers count instead.
    BlockingConcurrentQueue<Item> q;
    std::vector<std::vector<Item>> seen(consumers);
    std::atomic<int> taken{0};
    {
        std::vector<std::jthread> threads;
        for (int c = 0; c < consumers; ++c)
            threads.emplace_back([&, c] {
                ConsumerToken token(q);
                Item item;
                while (taken.load() < producers * per_producer)
                    if (q.wait_dequeue_timed(token, item, std::chrono::milliseconds(1))) {
                        seen[c].push_back(item);
                        taken.fetch_add(1);
                    }
            });
        for (int p = 0; p < producers; ++p)
            threads.emplace_back([&, p] {
                ProducerToken token(q);
                for (int i = 0; i < per_producer; ++i) q.enqueue(token, Item{p, i});
            });
    }
    check_traffic(seen);
    // wait_dequeue blocks until an item arrives.
    std::jthread later([&] { q.enqueue(Item{0, 0}); });
    Item item;
    q.wait_dequeue(item);
    CHECK(item.producer == 0);
}

void BlockingConcurrentQueue_TimedWaits() {
    BlockingConcurrentQueue<int> q;
    int v = 0;
    auto start = std::chrono::steady_clock::now();
    CHECK(!(q.wait_dequeue_timed(v, std::chrono::milliseconds(30))));
    CHECK((std::chrono::steady_clock::now() - start) >= (std::chrono::milliseconds(25)));
    CHECK(!(q.wait_dequeue_timed(v, 1000)));  // microseconds
    std::jthread later([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        q.enqueue(42);
    });
    CHECK(q.wait_dequeue_timed(v, std::chrono::seconds(10)));
    CHECK((v) == (42));
    // Bulk waits.
    int many[] = {1, 2, 3, 4, 5};
    q.enqueue_bulk(many, 5);
    int out[8];
    CHECK(q.wait_dequeue_bulk(out, 8) == 5u);
    CHECK(q.wait_dequeue_bulk_timed(out, 8, std::chrono::milliseconds(5)) == 0u);
    ConsumerToken token(q);
    q.enqueue(9);
    CHECK(q.wait_dequeue_bulk_timed(token, out, 8, 1000000) == 1u);
    CHECK((out[0]) == (9));
    CHECK(!(q.try_dequeue(v)));
    CHECK((q.size_approx()) == (0u));
    BlockingConcurrentQueue<int> other;
    other.enqueue(5);
    swap(q, other);
    CHECK(q.try_dequeue(v));
    CHECK((v) == (5));
}

// ── LightweightSemaphore ─────────────────────────────────────────────────

void LightweightSemaphore_SignalWaitAndTimeouts() {
    LightweightSemaphore sema(2);
    CHECK((sema.availableApprox()) == (2u));
    CHECK(sema.tryWait());
    CHECK(sema.wait());
    CHECK(!(sema.tryWait()));
    CHECK(!(sema.wait(2000)));  // microseconds
    sema.signal(5);
    CHECK((sema.tryWaitMany(3)) == (3));
    CHECK((sema.waitMany(10)) == (2));
    CHECK(sema.waitMany(10, 1000) == 0);
    std::atomic<int> woken{0};
    {
        std::vector<std::jthread> waiters;
        for (int i = 0; i < 4; ++i)
            waiters.emplace_back([&] {
                sema.wait();
                ++woken;
            });
        sema.signal(4);
    }
    CHECK((woken.load()) == (4));
    static_assert(std::is_same_v<LightweightSemaphore::ssize_t, std::make_signed_t<std::size_t>>);
}

// ── the adapted helpers of moodycamel::details ───────────────────────────

void Details_AdaptedHelpers() {
    namespace d = mcpplibs::mwrap::concurrentqueue::details;
    CHECK(d::circular_less_than(std::uint32_t{0}, std::uint32_t{100}));
    CHECK(d::circular_less_than(std::uint32_t{0xFFFFFFF0u}, std::uint32_t{5}));  // wraps around
    CHECK(!(d::circular_less_than(std::uint16_t{5}, std::uint16_t{5})));
    CHECK(d::circular_less_than(std::uint64_t{1}, std::uint64_t{2}));
    CHECK(d::circular_less_than(std::uint8_t{250}, std::uint8_t{3}));
    CHECK((d::ceil_to_pow_2(std::size_t{33})) == (64u));
    CHECK((d::ceil_to_pow_2(std::uint8_t{3})) == (4u));
    CHECK((d::ceil_to_pow_2(std::uint32_t{1024})) == (1024u));
    CHECK(d::likely(true) && !d::unlikely(false));
    const d::thread_id_t here = d::thread_id();
    d::thread_id_t there{};
    std::jthread([&] { there = d::thread_id(); }).join();
    CHECK((here) != (there));
    CHECK((d::hash_thread_id(here)) == (d::hash_thread_id(d::thread_id())));
}

}  // namespace

int main() {
    check::run("ConcurrentQueue.MpmcImplicitProducers", ConcurrentQueue_MpmcImplicitProducers);
    check::run("ConcurrentQueue.MpmcTokensAndBulk", ConcurrentQueue_MpmcTokensAndBulk);
    check::run("ConcurrentQueue.TryEnqueueIsBounded", ConcurrentQueue_TryEnqueueIsBounded);
    check::run("ConcurrentQueue.UserTypes", ConcurrentQueue_UserTypes);
    check::run("ConcurrentQueue.ExceptionsLeaveTheQueueConsistent", ConcurrentQueue_ExceptionsLeaveTheQueueConsistent);
    check::run("ConcurrentQueue.TraitsSwapAndMove", ConcurrentQueue_TraitsSwapAndMove);
    check::run("ConcurrentQueue.ProducerThatExitsAndTheDequeueFromProducer", ConcurrentQueue_ProducerThatExitsAndTheDequeueFromProducer);
    check::run("BlockingConcurrentQueue.WaitDequeueAcrossThreads", BlockingConcurrentQueue_WaitDequeueAcrossThreads);
    check::run("BlockingConcurrentQueue.TimedWaits", BlockingConcurrentQueue_TimedWaits);
    check::run("LightweightSemaphore.SignalWaitAndTimeouts", LightweightSemaphore_SignalWaitAndTimeouts);
    check::run("Details.AdaptedHelpers", Details_AdaptedHelpers);
    return check::result();
}
