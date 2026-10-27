// The value macros, at compile time: the macros module alone.
#include <climits>  // the C library's version macros (__GLIBC__), for the expectations below

import mcpplibs.mwrap.concurrentqueue.macros;
namespace macros = mcpplibs::mwrap::concurrentqueue::macros;
// A C++ build with exceptions: try / catch, not the exception-free spelling.
static_assert(macros::MOODYCAMEL_EXCEPTIONS_ENABLED);
// Upstream's defaults: no debugging switches, thread-locals allowed.
static_assert(!macros::MOODYCAMEL_NO_THREAD_LOCAL && !macros::MOODYCAMEL_QUEUE_INTERNAL_DEBUG);
static_assert(!macros::MCDBGQ_USE_RELACY && !macros::MCDBGQ_TRACKMEM && !macros::MCDBGQ_NOLOCKFREE_FREELIST);
static_assert(!macros::MCDBGQ_USEDEBUGFREELIST && !macros::MCDBGQ_NOLOCKFREE_IMPLICITPRODBLOCKINDEX && !macros::MCDBGQ_NOLOCKFREE_IMPLICITPRODHASH);
// Derived: implicit producers are recycled at thread exit through C++11
// thread_local where upstream's test admits it (not Clang in GNU mode, which
// reports __GNUC__ 4.2, nor ARM). The condition is upstream's, as it is.
#if (!defined(_MSC_VER) || _MSC_VER >= 1900) && (!defined(__MINGW32__) && !defined(__MINGW64__) || !defined(__WINPTHREADS_VERSION)) && \
    (!defined(__GNUC__) || __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)) && !defined(__arm__) && !defined(_M_ARM) && \
    !defined(__aarch64__) && !defined(__MVS__)
static_assert(macros::MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED);
#else
static_assert(!macros::MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED);
#endif
// CLOCK_MONOTONIC timed waits: glibc 2.30 and later (with _GNU_SOURCE, which C++ implies).
#if defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 30))
static_assert(macros::MOODYCAMEL_LIGHTWEIGHTSEMAPHORE_MONOTONIC);
#elif !defined(__linux__)
static_assert(!macros::MOODYCAMEL_LIGHTWEIGHTSEMAPHORE_MONOTONIC);
#endif
int main() { return 0; }
