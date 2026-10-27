# concurrentqueue

> moodycamel::ConcurrentQueue, the lock-free multi-producer multi-consumer queue, through C++23 modules.

**English** · [简体中文](README.zh.md) · [Upstream](https://github.com/cameron314/concurrentqueue) · [Releases](https://github.com/mcpplibs/concurrentqueue-m/releases) · [mcpp](https://github.com/mcpp-community/mcpp) · [Index](https://github.com/mcpplibs/mcpp-index)

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)
[![mcpp-community](https://img.shields.io/badge/mcpp--community-mwrap-blue)](https://github.com/mcpp-community)
[![CI](https://github.com/mcpplibs/concurrentqueue-m/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/mcpplibs/concurrentqueue-m/actions/workflows/ci.yml)

The upstream API is unchanged: `moodycamel::ConcurrentQueue`,
`BlockingConcurrentQueue`, `ProducerToken`, `ConsumerToken` and
`LightweightSemaphore` are the upstream entities of the three headers,
templates over your element and `Traits` types. The headers are
`compat.concurrentqueue` 1.0.5. The value macros are constants. Upstream's
own `tests/unittests` (all 45 tests) pass on every CI run, built against the
module instead of the headers.

## Modules

- `import mcpplibs.mwrap.concurrentqueue;`: the queues, tokens, traits, semaphore and `swap` in `moodycamel`, re-exporting the one below
- `import mcpplibs.mwrap.concurrentqueue.macros;`: `MOODYCAMEL_EXCEPTIONS_ENABLED`, `MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED` … in `mcpplibs::mwrap::concurrentqueue::macros`
- `import mcpplibs.mwrap.concurrentqueue.c_api;`: the C API (`moodycamel_cq_*`, `moodycamel_bcq_*`), with the `c-api` feature

> More Modules API -> [Usage and API](docs/usage.md)

## Getting started

```sh
mcpp new pipeline --template mcpplibs.mwrap.concurrentqueue@1.0.5
cd pipeline
mcpp run
```

The default template runs a producer-consumer pipeline. Producer threads
enqueue numbers in bulk through their tokens. Consumers wait on a blocking
queue with a timeout and count primes.

```toml
[dependencies]
"mcpplibs.mwrap.concurrentqueue" = "1.0.5"
```

```cpp
import std;
import mcpplibs.mwrap.concurrentqueue;

int main() {
    moodycamel::ConcurrentQueue<std::function<void()>> tasks;
    std::jthread worker([&] { tasks.enqueue([] { std::println("ran on the main thread"); }); });
    worker.join();
    std::function<void()> task;
    while (tasks.try_dequeue(task)) task();
}
```

| Feature | Effect |
|---|---|
| `c-api` | upstream's C API: compat.concurrentqueue's sources and the `c_api` module |
| `no-thread-local` | `MOODYCAMEL_NO_THREAD_LOCAL`: no `thread_local`, implicit producers not recycled at thread exit |

## Configuration and macros

Configuration macros must be set before the module is built. The module
uses upstream's defaults, with `MOODYCAMEL_NOEXCEPT_CTOR` / `_ASSIGN` spelled
as the equivalent type traits so that GCC accepts the interface
([config.hpp](include/mwrap/concurrentqueue/config.hpp)). Per-queue settings
(block size, index sizes, allocation) are the `Traits` argument, as upstream
documents. The syntax macros (`MOODYCAMEL_NOEXCEPT`,
`MOODYCAMEL_DELETE_FUNCTION`, `MOODYCAMEL_TRY` …) stay in
`<mwrap/concurrentqueue/macros.hpp>`, included before the imports. With GCC,
a unit that includes `<cxxabi.h>` (gtest does) cannot use the queue's
thread-exit path: use `no-thread-local` there. Details:
[docs/usage.md](docs/usage.md).

## Build and testing

Linux, macOS and Windows CI build the modules and run the tests:
multi-producer multi-consumer traffic, tokens, bulk operations, timed waits,
user types and traits, the semaphore, the C API and header mixing. CI also
runs the export audit ([tools/check_exports.py](tools/check_exports.py): every
declaration exported or listed with its reason, every macro mapped or kept)
and upstream's `tests/unittests` against the module
([tools/run_conformance.py](tools/run_conformance.py)). A separate job
scaffolds the default template as an installed package and runs it.

## License

[Apache-2.0](LICENSE) for the wrapper, by mcpp-community ([NOTICE](NOTICE)).
concurrentqueue remains under the Simplified BSD license or the Boost Software
License, and its semaphore under zlib
([upstream/concurrentqueue/LICENSE.md](upstream/concurrentqueue/LICENSE.md),
[README](upstream/concurrentqueue/README.md)).
