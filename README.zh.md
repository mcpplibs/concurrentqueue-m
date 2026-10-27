# concurrentqueue

> 通过 C++23 模块使用 moodycamel::ConcurrentQueue（无锁的多生产者多消费者队列）。

[English](README.md) · **简体中文** · [上游](https://github.com/cameron314/concurrentqueue) · [发布](https://github.com/mcpplibs/concurrentqueue-m/releases) · [mcpp](https://github.com/mcpp-community/mcpp) · [索引](https://github.com/mcpplibs/mcpp-index)

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)
[![mcpp-community](https://img.shields.io/badge/mcpp--community-mwrap-blue)](https://github.com/mcpp-community)
[![CI](https://github.com/mcpplibs/concurrentqueue-m/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/mcpplibs/concurrentqueue-m/actions/workflows/ci.yml)

上游 API 保持不变：`moodycamel::ConcurrentQueue`、`BlockingConcurrentQueue`、
`ProducerToken`、`ConsumerToken`、`LightweightSemaphore` 就是三个头文件里的上游
实体，是以你的元素类型和 `Traits` 类型为参数的模板。头文件由
`compat.concurrentqueue` 1.0.5 提供，表示值的宏映射为常量。每次 CI 都会把上游自带的
`tests/unittests`（共 45 个测试）改为基于本模块构建（而不是头文件），并且全部通过。

## 模块

- `import mcpplibs.mwrap.concurrentqueue;`：`moodycamel` 中的队列、令牌、traits、信号量和 `swap`，并重新导出下面这个模块
- `import mcpplibs.mwrap.concurrentqueue.macros;`：`MOODYCAMEL_EXCEPTIONS_ENABLED`、`MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED` 等，位于 `mcpplibs::mwrap::concurrentqueue::macros`
- `import mcpplibs.mwrap.concurrentqueue.c_api;`：C API（`moodycamel_cq_*`、`moodycamel_bcq_*`），需要 `c-api` 特性

> 更多模块 API -> [用法和 API](docs/usage.md)

## 快速开始

```sh
mcpp new pipeline --template mcpplibs.mwrap.concurrentqueue@1.0.5
cd pipeline
mcpp run
```

默认模板运行一条生产者-消费者流水线：生产者线程通过各自的令牌批量入队数字，
消费者在阻塞队列上带超时地等待，并统计其中的素数。

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

| 特性 | 作用 |
|---|---|
| `c-api` | 上游的 C API：compat.concurrentqueue 中的源文件和 `c_api` 模块 |
| `no-thread-local` | `MOODYCAMEL_NO_THREAD_LOCAL`：不使用 `thread_local`，线程退出时不回收隐式生产者 |

## 配置与宏

配置宏必须在模块构建之前设定。模块使用上游默认配置，只是把
`MOODYCAMEL_NOEXCEPT_CTOR` / `_ASSIGN` 写成等价的类型特征，这样 GCC 才能接受这个
接口（[config.hpp](include/mwrap/concurrentqueue/config.hpp)）。单个队列的设置
（块大小、索引大小、内存分配）按上游文档通过 `Traits` 参数传入。语法类的宏
（`MOODYCAMEL_NOEXCEPT`、`MOODYCAMEL_DELETE_FUNCTION`、`MOODYCAMEL_TRY` 等）保留在
`<mwrap/concurrentqueue/macros.hpp>` 中，需要在 import 之前包含。使用 GCC 时，包含了
`<cxxabi.h>` 的翻译单元（gtest 会包含它）不能使用队列在线程退出时的回收路径，
这时请启用 `no-thread-local`。详见 [docs/usage.md](docs/usage.md)。

## 构建与测试

Linux、macOS 和 Windows 的 CI 会构建模块并运行测试，覆盖：多生产者多消费者流量、
令牌、批量操作、带超时的等待、用户类型与 traits、信号量、C API，以及头文件与模块
混用。CI 还会运行导出审计（[tools/check_exports.py](tools/check_exports.py)：每个
声明都已导出或注明原因，每个宏都已映射或保留为宏），并基于本模块运行上游的
`tests/unittests`（[tools/run_conformance.py](tools/run_conformance.py)）。另一个任务
把默认模板当作已安装的包生成出来并运行。

## 许可证

封装部分采用 [Apache-2.0](LICENSE)，作者为 mcpp-community（[NOTICE](NOTICE)）。
concurrentqueue 仍采用 Simplified BSD 许可证或 Boost Software License，其中的
信号量部分采用 zlib 许可证
（[upstream/concurrentqueue/LICENSE.md](upstream/concurrentqueue/LICENSE.md)、
[README](upstream/concurrentqueue/README.md)）。
