// The adaptations of moodycamel::details' TU-local helpers: each calls
// upstream's own `static` function (or its instantiation for the type).
module;
#include <mwrap/concurrentqueue/config.hpp>
#include <concurrentqueue.h>

module mcpplibs.mwrap.concurrentqueue;

namespace mcpplibs::mwrap::concurrentqueue::details {

thread_id_t thread_id() { return ::moodycamel::details::thread_id(); }
bool likely(bool x) { return (::moodycamel::details::likely)(x); }
bool unlikely(bool x) { return (::moodycamel::details::unlikely)(x); }
std::size_t hash_thread_id(thread_id_t id) { return ::moodycamel::details::hash_thread_id(id); }
bool circular_less_than(unsigned char a, unsigned char b) { return ::moodycamel::details::circular_less_than(a, b); }
bool circular_less_than(unsigned short a, unsigned short b) { return ::moodycamel::details::circular_less_than(a, b); }
bool circular_less_than(unsigned int a, unsigned int b) { return ::moodycamel::details::circular_less_than(a, b); }
bool circular_less_than(unsigned long a, unsigned long b) { return ::moodycamel::details::circular_less_than(a, b); }
bool circular_less_than(unsigned long long a, unsigned long long b) { return ::moodycamel::details::circular_less_than(a, b); }
unsigned char ceil_to_pow_2(unsigned char x) { return ::moodycamel::details::ceil_to_pow_2(x); }
unsigned short ceil_to_pow_2(unsigned short x) { return ::moodycamel::details::ceil_to_pow_2(x); }
unsigned int ceil_to_pow_2(unsigned int x) { return ::moodycamel::details::ceil_to_pow_2(x); }
unsigned long ceil_to_pow_2(unsigned long x) { return ::moodycamel::details::ceil_to_pow_2(x); }
unsigned long long ceil_to_pow_2(unsigned long long x) { return ::moodycamel::details::ceil_to_pow_2(x); }

}  // namespace mcpplibs::mwrap::concurrentqueue::details
