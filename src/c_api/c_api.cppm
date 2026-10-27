// mcpplibs.mwrap.concurrentqueue.c_api — upstream's flat C API over both
// queues (c_api/concurrentqueue.h: moodycamel_cq_*, moodycamel_bcq_*), with
// the `c-api` feature, which builds compat.concurrentqueue's two C API
// sources. Import it by name: the root does not re-export it.
module;
// A static library on Windows: no dllimport (compat.concurrentqueue defines
// it for its own sources and consumers alike).
#ifndef MOODYCAMEL_STATIC
#define MOODYCAMEL_STATIC
#endif
#include <c_api/concurrentqueue.h>

export module mcpplibs.mwrap.concurrentqueue.c_api;

// types (3)
export using ::MoodycamelCQHandle;
export using ::MoodycamelBCQHandle;
export using ::MoodycamelValue;

// functions (10)
export using ::moodycamel_cq_create;
export using ::moodycamel_cq_destroy;
export using ::moodycamel_cq_enqueue;
export using ::moodycamel_cq_try_dequeue;
export using ::moodycamel_cq_size_approx;
export using ::moodycamel_bcq_create;
export using ::moodycamel_bcq_destroy;
export using ::moodycamel_bcq_enqueue;
export using ::moodycamel_bcq_wait_dequeue;
export using ::moodycamel_bcq_try_dequeue;
