// <mwrap/concurrentqueue/macros.hpp> — moodycamel's headers with their macros,
// configured as mcpplibs.mwrap.concurrentqueue is (<mwrap/concurrentqueue/config.hpp>).
// Include it, before `import mcpplibs.mwrap.concurrentqueue;`, for what a
// mapping cannot be: the syntax macros (MOODYCAMEL_NOEXCEPT,
// MOODYCAMEL_DELETE_FUNCTION, MOODYCAMEL_ALIGNAS, MOODYCAMEL_TRY / CATCH /
// THROW …) and `#if` tests of the configuration (MOODYCAMEL_EXCEPTIONS_ENABLED,
// MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED). The declarations it brings are the
// very entities the module exports. While a macro is defined it expands even
// inside `mcpplibs::mwrap::concurrentqueue::macros::NAME`.
#pragma once
#include <mwrap/concurrentqueue/config.hpp>
#include <concurrentqueue.h>
#include <blockingconcurrentqueue.h>
#include <lightweightsemaphore.h>
