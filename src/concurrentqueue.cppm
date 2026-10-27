// mcpplibs.mwrap.concurrentqueue — moodycamel::ConcurrentQueue, the lock-free
// multi-producer multi-consumer queue, with its blocking variant and
// lightweight semaphore, as a C++ module. Every exported name is the upstream
// entity of <concurrentqueue.h>, <blockingconcurrentqueue.h> and
// <lightweightsemaphore.h>, unchanged; the root also re-exports the macro
// mappings. The headers are compat.concurrentqueue's, configured by
// <mwrap/concurrentqueue/config.hpp>. The C API is its own module,
// mcpplibs.mwrap.concurrentqueue.c_api, with the `c-api` feature (mcpp's
// scanner admits no import under a feature test, so the root cannot
// re-export it conditionally).
//
// moodycamel::details is the queue's implementation and is not exported. Its
// `static` (TU-local, not exportable) helpers that are not templates over
// arbitrary types are adapted in mcpplibs::mwrap::concurrentqueue::details
// under the same names, each calling upstream's body (src/details.cpp).
// tools/check_exports.py audits both lists against the headers.
module;
#include "module_prologue.hpp"  // GCC: the exposures of upstream's static helpers
#include <mwrap/concurrentqueue/config.hpp>
#include <concurrentqueue.h>
#include <blockingconcurrentqueue.h>
#include <lightweightsemaphore.h>

export module mcpplibs.mwrap.concurrentqueue;

export import mcpplibs.mwrap.concurrentqueue.macros;

export namespace moodycamel {
// <concurrentqueue.h>
using moodycamel::ConcurrentQueue;
using moodycamel::ConcurrentQueueDefaultTraits;
using moodycamel::ProducerToken;
using moodycamel::ConsumerToken;
using moodycamel::ConcurrentQueueTests;  // forward-declared, befriended by the queue: upstream's test class
// <blockingconcurrentqueue.h>
using moodycamel::BlockingConcurrentQueue;
// <lightweightsemaphore.h>
using moodycamel::LightweightSemaphore;
// every overload: ConcurrentQueue, BlockingConcurrentQueue, ProducerToken,
// ConsumerToken, ConcurrentQueue::ImplicitProducerKVP
using moodycamel::swap;
}  // namespace moodycamel

// The TU-local helpers of moodycamel::details, adapted (implementation
// details; upstream's own tests call circular_less_than).
export namespace mcpplibs::mwrap::concurrentqueue::details {
using thread_id_t = ::moodycamel::details::thread_id_t;
thread_id_t thread_id();
bool likely(bool x);
bool unlikely(bool x);
std::size_t hash_thread_id(thread_id_t id);
// Templates over the unsigned integer types: one overload per type.
bool circular_less_than(unsigned char a, unsigned char b);
bool circular_less_than(unsigned short a, unsigned short b);
bool circular_less_than(unsigned int a, unsigned int b);
bool circular_less_than(unsigned long a, unsigned long b);
bool circular_less_than(unsigned long long a, unsigned long long b);
unsigned char ceil_to_pow_2(unsigned char x);
unsigned short ceil_to_pow_2(unsigned short x);
unsigned int ceil_to_pow_2(unsigned int x);
unsigned long ceil_to_pow_2(unsigned long x);
unsigned long long ceil_to_pow_2(unsigned long long x);
}  // namespace mcpplibs::mwrap::concurrentqueue::details

// GCC binds a call that does not depend on the queue's T when it reads the
// template, and with it the specialization of upstream's `static` function
// template the call names (details::swap_relaxed on the std::uint32_t,
// thread-id and std::size_t members swap() exchanges), but emits that
// specialization's body only where a member that calls it is instantiated.
// An importer instantiating swap() would then reference a TU-local
// specialization that no object defines. Instantiating the queues' members
// here, once, puts those bodies in the module interface, and every importer
// emits its own copy (tests/api_test.cpp swaps the queues and tokens).
namespace mcpplibs::mwrap::concurrentqueue::instantiation {
[[maybe_unused]] void bind_non_dependent_helpers() {
    moodycamel::ConcurrentQueue<int> a, b;
    moodycamel::ProducerToken producer(a);
    moodycamel::ConsumerToken consumer(a);
    int items[2] = {1, 2}, item = 0;
    a.enqueue(item);
    a.enqueue(producer, item);
    a.try_enqueue(item);
    a.try_enqueue(producer, item);
    a.enqueue_bulk(items, 2);
    a.enqueue_bulk(producer, items, 2);
    a.try_enqueue_bulk(items, 2);
    a.try_enqueue_bulk(producer, items, 2);
    a.try_dequeue(item);
    a.try_dequeue(consumer, item);
    a.try_dequeue_non_interleaved(item);
    a.try_dequeue_from_producer(producer, item);
    a.try_dequeue_bulk(items, 2);
    a.try_dequeue_bulk(consumer, items, 2);
    a.try_dequeue_bulk_from_producer(producer, items, 2);
    (void)a.size_approx();
    a.swap(b);
    swap(a, b);
    moodycamel::ConcurrentQueue<int> moved(std::move(a));
    b = std::move(moved);
    moodycamel::BlockingConcurrentQueue<int> c, d;
    moodycamel::ProducerToken blocking_producer(c);
    moodycamel::ConsumerToken blocking_consumer(c);
    c.enqueue(item);
    c.enqueue(blocking_producer, item);
    c.try_enqueue(item);
    c.enqueue_bulk(items, 2);
    c.try_enqueue_bulk(blocking_producer, items, 2);
    c.try_dequeue(item);
    c.try_dequeue(blocking_consumer, item);
    c.wait_dequeue(item);
    c.wait_dequeue(blocking_consumer, item);
    c.wait_dequeue_timed(item, 0);
    c.wait_dequeue_timed(blocking_consumer, item, 0);
    c.wait_dequeue_bulk(items, 2);
    c.wait_dequeue_bulk_timed(blocking_consumer, items, 2, 0);
    c.try_dequeue_bulk(items, 2);
    (void)c.size_approx();
    c.swap(d);
    swap(c, d);
    moodycamel::swap(producer, blocking_producer);
    moodycamel::swap(consumer, blocking_consumer);
}
}  // namespace mcpplibs::mwrap::concurrentqueue::instantiation
