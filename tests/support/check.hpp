// A small checker for the tests. Not gtest: with GCC 16.1, a translation unit
// that includes <cxxabi.h> (gtest does) cannot instantiate the queue's
// thread-exit path from the module (docs/usage.md, "GCC and <cxxabi.h>").
#pragma once
#include <cstdio>
#include <cstdlib>
#include <source_location>

namespace check {
inline int failures = 0;
inline void that(bool ok, const char* what, std::source_location at = std::source_location::current()) {
    if (ok) return;
    ++failures;
    std::fprintf(stderr, "%s:%u: check failed: %s\n", at.file_name(), static_cast<unsigned>(at.line()), what);
}
template <class Test>
void run(const char* name, Test test) {
    const int before = failures;
    test();
    std::printf("[%s] %s\n", failures == before ? "  OK  " : "FAILED", name);
}
inline int result() {
    std::printf("%s (%d failed checks)\n", failures ? "FAILED" : "all passed", failures);
    return failures ? 1 : 0;
}
}  // namespace check

#define CHECK(...) ::check::that(static_cast<bool>(__VA_ARGS__), #__VA_ARGS__)
