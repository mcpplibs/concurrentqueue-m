// mcpplibs.mwrap.concurrentqueue.macros — the macros of moodycamel's headers
// that are values (a named module cannot export a macro), under their
// original names in mcpplibs::mwrap::concurrentqueue::macros, each an
// `inline constexpr bool` that is true exactly when the macro is defined
// where the module is built:
//
//   - what the headers derive: MOODYCAMEL_EXCEPTIONS_ENABLED,
//     MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED (implicit producers are
//     recycled at thread exit), MOODYCAMEL_LIGHTWEIGHTSEMAPHORE_MONOTONIC
//     (timed waits on CLOCK_MONOTONIC, glibc 2.30 and later);
//   - the configuration inputs they read, as the module was built (upstream's
//     defaults: all off): MOODYCAMEL_NO_THREAD_LOCAL,
//     MOODYCAMEL_QUEUE_INTERNAL_DEBUG and the MCDBGQ_* debugging switches.
//
// The syntax macros (MOODYCAMEL_NOEXCEPT, MOODYCAMEL_DELETE_FUNCTION,
// MOODYCAMEL_ALIGNAS, MOODYCAMEL_TRY / CATCH / RETHROW / THROW,
// MOODYCAMEL_THREADLOCAL …) cannot be values: they stay macros, in
// <mwrap/concurrentqueue/macros.hpp>. Per-queue configuration is the Traits
// template argument, which a module passes through as it is.
// tools/check_exports.py accounts for every macro.
module;
#include <mwrap/concurrentqueue/config.hpp>
#include <concurrentqueue.h>
#include <blockingconcurrentqueue.h>
#include <lightweightsemaphore.h>

namespace mwrap_concurrentqueue_capture {
#ifdef MOODYCAMEL_EXCEPTIONS_ENABLED
inline constexpr bool value_MOODYCAMEL_EXCEPTIONS_ENABLED = true;
#else
inline constexpr bool value_MOODYCAMEL_EXCEPTIONS_ENABLED = false;
#endif
#ifdef MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED
inline constexpr bool value_MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED = true;
#else
inline constexpr bool value_MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED = false;
#endif
#ifdef MOODYCAMEL_LIGHTWEIGHTSEMAPHORE_MONOTONIC
inline constexpr bool value_MOODYCAMEL_LIGHTWEIGHTSEMAPHORE_MONOTONIC = true;
#else
inline constexpr bool value_MOODYCAMEL_LIGHTWEIGHTSEMAPHORE_MONOTONIC = false;
#endif
#ifdef MOODYCAMEL_NO_THREAD_LOCAL
inline constexpr bool value_MOODYCAMEL_NO_THREAD_LOCAL = true;
#else
inline constexpr bool value_MOODYCAMEL_NO_THREAD_LOCAL = false;
#endif
#ifdef MOODYCAMEL_QUEUE_INTERNAL_DEBUG
inline constexpr bool value_MOODYCAMEL_QUEUE_INTERNAL_DEBUG = true;
#else
inline constexpr bool value_MOODYCAMEL_QUEUE_INTERNAL_DEBUG = false;
#endif
#ifdef MCDBGQ_USE_RELACY
inline constexpr bool value_MCDBGQ_USE_RELACY = true;
#else
inline constexpr bool value_MCDBGQ_USE_RELACY = false;
#endif
#ifdef MCDBGQ_TRACKMEM
inline constexpr bool value_MCDBGQ_TRACKMEM = true;
#else
inline constexpr bool value_MCDBGQ_TRACKMEM = false;
#endif
#ifdef MCDBGQ_NOLOCKFREE_FREELIST
inline constexpr bool value_MCDBGQ_NOLOCKFREE_FREELIST = true;
#else
inline constexpr bool value_MCDBGQ_NOLOCKFREE_FREELIST = false;
#endif
#ifdef MCDBGQ_USEDEBUGFREELIST
inline constexpr bool value_MCDBGQ_USEDEBUGFREELIST = true;
#else
inline constexpr bool value_MCDBGQ_USEDEBUGFREELIST = false;
#endif
#ifdef MCDBGQ_NOLOCKFREE_IMPLICITPRODBLOCKINDEX
inline constexpr bool value_MCDBGQ_NOLOCKFREE_IMPLICITPRODBLOCKINDEX = true;
#else
inline constexpr bool value_MCDBGQ_NOLOCKFREE_IMPLICITPRODBLOCKINDEX = false;
#endif
#ifdef MCDBGQ_NOLOCKFREE_IMPLICITPRODHASH
inline constexpr bool value_MCDBGQ_NOLOCKFREE_IMPLICITPRODHASH = true;
#else
inline constexpr bool value_MCDBGQ_NOLOCKFREE_IMPLICITPRODHASH = false;
#endif
}  // namespace mwrap_concurrentqueue_capture

// Every mapped name, so the definitions below are not rewritten.
#undef MOODYCAMEL_EXCEPTIONS_ENABLED
#undef MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED
#undef MOODYCAMEL_LIGHTWEIGHTSEMAPHORE_MONOTONIC
#undef MOODYCAMEL_NO_THREAD_LOCAL
#undef MOODYCAMEL_QUEUE_INTERNAL_DEBUG
#undef MCDBGQ_USE_RELACY
#undef MCDBGQ_TRACKMEM
#undef MCDBGQ_NOLOCKFREE_FREELIST
#undef MCDBGQ_USEDEBUGFREELIST
#undef MCDBGQ_NOLOCKFREE_IMPLICITPRODBLOCKINDEX
#undef MCDBGQ_NOLOCKFREE_IMPLICITPRODHASH

export module mcpplibs.mwrap.concurrentqueue.macros;

namespace c = mwrap_concurrentqueue_capture;

export namespace mcpplibs::mwrap::concurrentqueue::macros {

// derived by the headers
inline constexpr bool MOODYCAMEL_EXCEPTIONS_ENABLED = c::value_MOODYCAMEL_EXCEPTIONS_ENABLED;
inline constexpr bool MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED = c::value_MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED;
inline constexpr bool MOODYCAMEL_LIGHTWEIGHTSEMAPHORE_MONOTONIC = c::value_MOODYCAMEL_LIGHTWEIGHTSEMAPHORE_MONOTONIC;
// configuration inputs, as the module was built
inline constexpr bool MOODYCAMEL_NO_THREAD_LOCAL = c::value_MOODYCAMEL_NO_THREAD_LOCAL;
inline constexpr bool MOODYCAMEL_QUEUE_INTERNAL_DEBUG = c::value_MOODYCAMEL_QUEUE_INTERNAL_DEBUG;
inline constexpr bool MCDBGQ_USE_RELACY = c::value_MCDBGQ_USE_RELACY;
inline constexpr bool MCDBGQ_TRACKMEM = c::value_MCDBGQ_TRACKMEM;
inline constexpr bool MCDBGQ_NOLOCKFREE_FREELIST = c::value_MCDBGQ_NOLOCKFREE_FREELIST;
inline constexpr bool MCDBGQ_USEDEBUGFREELIST = c::value_MCDBGQ_USEDEBUGFREELIST;
inline constexpr bool MCDBGQ_NOLOCKFREE_IMPLICITPRODBLOCKINDEX = c::value_MCDBGQ_NOLOCKFREE_IMPLICITPRODBLOCKINDEX;
inline constexpr bool MCDBGQ_NOLOCKFREE_IMPLICITPRODHASH = c::value_MCDBGQ_NOLOCKFREE_IMPLICITPRODHASH;

}  // namespace mcpplibs::mwrap::concurrentqueue::macros
