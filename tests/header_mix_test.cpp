// The headers and the import in one translation unit (the side header first,
// the canonical order): the module's names are the very same entities, the
// syntax macros work beside them, and the header's configuration equals the
// mapped constants. While a macro is defined it expands even inside a
// qualified name, so each mapped constant is read with its macro set aside.
#include <mwrap/concurrentqueue/macros.hpp>
#include "support/check.hpp"

import std;
import mcpplibs.mwrap.concurrentqueue;

namespace macros = mcpplibs::mwrap::concurrentqueue::macros;

#ifdef MOODYCAMEL_EXCEPTIONS_ENABLED
#pragma push_macro("MOODYCAMEL_EXCEPTIONS_ENABLED")
#undef MOODYCAMEL_EXCEPTIONS_ENABLED
static_assert(macros::MOODYCAMEL_EXCEPTIONS_ENABLED);
#pragma pop_macro("MOODYCAMEL_EXCEPTIONS_ENABLED")
#else
static_assert(!macros::MOODYCAMEL_EXCEPTIONS_ENABLED);
#endif
#ifdef MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED
#pragma push_macro("MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED")
#undef MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED
static_assert(macros::MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED);
#pragma pop_macro("MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED")
#else
static_assert(!macros::MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED);
#endif
#ifdef MOODYCAMEL_LIGHTWEIGHTSEMAPHORE_MONOTONIC
#pragma push_macro("MOODYCAMEL_LIGHTWEIGHTSEMAPHORE_MONOTONIC")
#undef MOODYCAMEL_LIGHTWEIGHTSEMAPHORE_MONOTONIC
static_assert(macros::MOODYCAMEL_LIGHTWEIGHTSEMAPHORE_MONOTONIC);
#pragma pop_macro("MOODYCAMEL_LIGHTWEIGHTSEMAPHORE_MONOTONIC")
#else
static_assert(!macros::MOODYCAMEL_LIGHTWEIGHTSEMAPHORE_MONOTONIC);
#endif

namespace {
// The syntax macros, as upstream's tests use them.
struct MOODYCAMEL_ALIGNAS(32) Aligned {
    int value = 0;
    Aligned() MOODYCAMEL_NOEXCEPT = default;
    explicit Aligned(int v) MOODYCAMEL_NOEXCEPT : value(v) {}
    Aligned(Aligned&&) MOODYCAMEL_NOEXCEPT = default;
    Aligned& operator=(Aligned&&) MOODYCAMEL_NOEXCEPT = default;
    Aligned(const Aligned&) MOODYCAMEL_DELETE_FUNCTION;
    Aligned& operator=(const Aligned&) MOODYCAMEL_DELETE_FUNCTION;
};
}  // namespace

void same_entities() {
    // The header's class template and the module's are one entity: an object
    // made with one is the other.
    moodycamel::ConcurrentQueue<Aligned> q;
    ::moodycamel::ConcurrentQueue<Aligned>& same = q;
    static_assert(std::is_same_v<decltype(same), moodycamel::ConcurrentQueue<Aligned>&>);
    static_assert(alignof(Aligned) == 32 && !std::is_copy_constructible_v<Aligned>);
    // The configured noexcept: a nothrow-movable element's bulk path.
    Aligned items[3] = {Aligned(1), Aligned(2), Aligned(3)};
    CHECK(q.enqueue_bulk(std::make_move_iterator(items), 3));
    Aligned out[3];
    CHECK(q.try_dequeue_bulk(out, 3) == 3u);
    CHECK(out[2].value == 3);
    // MOODYCAMEL_TRY / MOODYCAMEL_CATCH, upstream's exception spelling.
    bool caught = false;
    MOODYCAMEL_TRY { MOODYCAMEL_THROW(std::runtime_error("x")); }
    MOODYCAMEL_CATCH(const std::runtime_error&) { caught = true; }
    CHECK(caught);
    // The blocking queue and the semaphore through both spellings.
    moodycamel::BlockingConcurrentQueue<int> b;
    b.enqueue(5);
    int v = 0;
    CHECK(b.wait_dequeue_timed(v, 1000));
    CHECK(v == 5);
    moodycamel::LightweightSemaphore s(1);
    CHECK(s.tryWait());
    // The adapted helper agrees with the header's own.
    CHECK(mcpplibs::mwrap::concurrentqueue::details::circular_less_than(3u, 7u) == moodycamel::details::circular_less_than(3u, 7u));
    CHECK(mcpplibs::mwrap::concurrentqueue::details::ceil_to_pow_2(17u) == moodycamel::details::ceil_to_pow_2(17u));
}

int main() {
    check::run("HeaderMix.SameEntities", same_entities);
    return check::result();
}
