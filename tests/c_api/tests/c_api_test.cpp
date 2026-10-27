// Upstream's C API (c_api/concurrentqueue.h) through
// import mcpplibs.mwrap.concurrentqueue.c_api, and the queue through gtest:
// with no-thread-local the module has no thread_local, so a translation unit
// that includes <cxxabi.h> (gtest does) instantiates it with GCC too.
#include <gtest/gtest.h>

import std;
import mcpplibs.mwrap.concurrentqueue;
import mcpplibs.mwrap.concurrentqueue.c_api;

namespace macros = mcpplibs::mwrap::concurrentqueue::macros;

TEST(CApi, ConcurrentQueue) {
    MoodycamelCQHandle q = nullptr;
    ASSERT_TRUE(moodycamel_cq_create(&q));
    int values[3] = {1, 2, 3};
    for (int& v : values) ASSERT_TRUE(moodycamel_cq_enqueue(q, &v));
    EXPECT_EQ(moodycamel_cq_size_approx(q), 3u);
    MoodycamelValue out = nullptr;
    int sum = 0;
    while (moodycamel_cq_try_dequeue(q, &out)) sum += *static_cast<int*>(out);
    EXPECT_EQ(sum, 6);
    EXPECT_TRUE(moodycamel_cq_destroy(q));
}

TEST(CApi, BlockingQueueAcrossThreads) {
    MoodycamelBCQHandle q = nullptr;
    ASSERT_TRUE(moodycamel_bcq_create(&q));
    static int values[1000];
    std::jthread producer([&] {
        for (int i = 0; i < 1000; ++i) {
            values[i] = i;
            moodycamel_bcq_enqueue(q, &values[i]);
        }
    });
    long sum = 0;
    for (int i = 0; i < 1000; ++i) {
        MoodycamelValue out = nullptr;
        ASSERT_TRUE(moodycamel_bcq_wait_dequeue(q, &out));
        sum += *static_cast<int*>(out);
    }
    producer.join();
    MoodycamelValue none = nullptr;
    EXPECT_FALSE(moodycamel_bcq_try_dequeue(q, &none));
    EXPECT_EQ(sum, 999L * 1000 / 2);
    EXPECT_TRUE(moodycamel_bcq_destroy(q));
}

// The feature defines MOODYCAMEL_NO_THREAD_LOCAL for its consumers too (so the
// headers, where a unit includes them, agree with the module): set it aside
// to read the mapped constant.
#ifndef MOODYCAMEL_NO_THREAD_LOCAL
#error "no-thread-local defines MOODYCAMEL_NO_THREAD_LOCAL for consumers"
#endif
#pragma push_macro("MOODYCAMEL_NO_THREAD_LOCAL")
#undef MOODYCAMEL_NO_THREAD_LOCAL
static_assert(macros::MOODYCAMEL_NO_THREAD_LOCAL && !macros::MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED);
#pragma pop_macro("MOODYCAMEL_NO_THREAD_LOCAL")

TEST(NoThreadLocal, QueueBesideGtest) {
    moodycamel::ConcurrentQueue<std::string> q;
    std::vector<std::jthread> threads;
    for (int t = 0; t < 4; ++t)
        threads.emplace_back([&, t] {
            for (int i = 0; i < 100; ++i) q.enqueue(std::to_string(t * 1000 + i));
        });
    threads.clear();
    std::string s;
    int n = 0;
    while (q.try_dequeue(s)) ++n;
    EXPECT_EQ(n, 400);
}
