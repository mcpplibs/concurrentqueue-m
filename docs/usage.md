# Using mcpplibs.mwrap.concurrentqueue

`import mcpplibs.mwrap.concurrentqueue;` declares moodycamel::ConcurrentQueue 1.0.5 as `<concurrentqueue.h>`, `<blockingconcurrentqueue.h>` and `<lightweightsemaphore.h>` do. The exported names are the upstream entities in namespace `moodycamel`, so a translation unit may include the headers too. The headers are `compat.concurrentqueue` 1.0.5's.

## Modules

| Module | What it holds |
|---|---|
| `mcpplibs.mwrap.concurrentqueue` | `moodycamel::ConcurrentQueue`, `BlockingConcurrentQueue`, `ConcurrentQueueDefaultTraits`, `ProducerToken`, `ConsumerToken`, `LightweightSemaphore`, every `swap` overload, and `ConcurrentQueueTests` (the class the queue befriends for upstream's tests); the adapted helpers of `moodycamel::details` (below); re-exports the macros module |
| `mcpplibs.mwrap.concurrentqueue.macros` | the value macros, in `mcpplibs::mwrap::concurrentqueue::macros` |
| `mcpplibs.mwrap.concurrentqueue.c_api` | with the `c-api` feature: upstream's C API, `moodycamel_cq_*` and `moodycamel_bcq_*` with `MoodycamelCQHandle`, `MoodycamelBCQHandle`, `MoodycamelValue`. Import it by name: mcpp's scanner admits no import under a feature test, so the root cannot re-export it. |

The queues are templates over the element type and a `Traits` type (block size, index sizes, `malloc` / `free` …). Both are the importer's own types, instantiated in the importer as with the headers.

## Features

| Feature | Effect |
|---|---|
| `c-api` | builds compat.concurrentqueue's two C API sources and the `c_api` module |
| `no-thread-local` | upstream's `MOODYCAMEL_NO_THREAD_LOCAL`, for the module and its consumers: thread ids from `std::this_thread`, no `thread_local`, so implicit producers are not recycled at thread exit (as upstream always builds on ARM and with Clang in GNU mode) |

Neither is on by default.

## Configuration

Configuration macros must be set before the module is built. The module is built with upstream's defaults, plus one adjustment, in `<mwrap/concurrentqueue/config.hpp>`. Upstream defines `MOODYCAMEL_NOEXCEPT_CTOR(type, valueType, expr)` as `noexcept(expr)`, an expression that names `details::deref_noexcept`, a `static` function. In a template argument (`details::nomove_if<…>` in `enqueue_bulk`), that makes the queue expose a TU-local entity, which GCC rejects in a module interface. config.hpp spells the two macros with the type traits that give the same answers: `std::is_nothrow_constructible<T, decltype(*it)>` and `std::is_nothrow_assignable<T&, T&&>`. This uses upstream's own configuration point: both macros are defined by concurrentqueue.h only when `MOODYCAMEL_NOEXCEPT` is not. Without exceptions, upstream's definitions stand.

Per-queue configuration is the `Traits` argument. It needs no macro and works through the module unchanged:

```cpp
struct SmallBlocks : moodycamel::ConcurrentQueueDefaultTraits {
    static constexpr std::size_t BLOCK_SIZE = 4;
};
moodycamel::ConcurrentQueue<Job, SmallBlocks> queue;
```

## Macros

A named module cannot export a macro. `mcpplibs::mwrap::concurrentqueue::macros` holds the value macros, under their original names, as `inline constexpr bool`. Each is true exactly when the macro is defined where the module is built.

- Derived by the headers: `MOODYCAMEL_EXCEPTIONS_ENABLED`, `MOODYCAMEL_CPP11_THREAD_LOCAL_SUPPORTED` (implicit producers are recycled at thread exit; upstream enables it for GCC 4.8+ and MSVC 2015+, but not for Clang in GNU mode, which reports `__GNUC__` 4.2, nor on ARM), and `MOODYCAMEL_LIGHTWEIGHTSEMAPHORE_MONOTONIC` (timed waits on `CLOCK_MONOTONIC`, glibc 2.30+).
- The configuration inputs, as the module was built: `MOODYCAMEL_NO_THREAD_LOCAL`, `MOODYCAMEL_QUEUE_INTERNAL_DEBUG`, `MCDBGQ_USE_RELACY`, `MCDBGQ_TRACKMEM`, `MCDBGQ_NOLOCKFREE_FREELIST`, `MCDBGQ_USEDEBUGFREELIST`, `MCDBGQ_NOLOCKFREE_IMPLICITPRODBLOCKINDEX`, `MCDBGQ_NOLOCKFREE_IMPLICITPRODHASH`.

The syntax macros stay macros, in `<mwrap/concurrentqueue/macros.hpp>`. That header is config.hpp plus the three headers, so it is the configuration the module has:

| Macro | Why |
|---|---|
| `MOODYCAMEL_NOEXCEPT`, `MOODYCAMEL_NOEXCEPT_CTOR`, `MOODYCAMEL_NOEXCEPT_ASSIGN` | a specifier; expressions over an argument expression |
| `MOODYCAMEL_DELETE_FUNCTION` | `= delete` |
| `MOODYCAMEL_ALIGNAS`, `MOODYCAMEL_ALIGNOF`, `MOODYCAMEL_ALIGNED_TYPE_LIKE` | alignment specifiers and operators |
| `MOODYCAMEL_TRY`, `MOODYCAMEL_CATCH`, `MOODYCAMEL_RETHROW`, `MOODYCAMEL_THROW`, `MOODYCAMEL_CONSTEXPR_IF` | control flow |
| `MOODYCAMEL_MAYBE_UNUSED`, `MOODYCAMEL_NO_TSAN`, `MOODYCAMEL_THREADLOCAL` | attributes, a storage class |
| `MOODYCAMEL_EXPORT` | the C API's linkage annotation |

Include the side header before the imports. While a macro is defined it expands even inside a qualified name, so `macros::MOODYCAMEL_EXCEPTIONS_ENABLED` is reachable there only with the macro set aside (`#pragma push_macro`, see [tests/header_mix_test.cpp](../tests/header_mix_test.cpp)). To mix a unit that includes the headers with the module, include `<mwrap/concurrentqueue/macros.hpp>` rather than `<concurrentqueue.h>` alone, so both see one configuration.

## moodycamel::details

`moodycamel::details` is the queue's implementation and is not exported. Its `static` (TU-local) helpers cannot be exported. Those that are not templates over arbitrary types are adapted in `mcpplibs::mwrap::concurrentqueue::details`, with the same names, and each calls upstream's body: `thread_id`, `thread_id_t`, `likely`, `unlikely`, `hash_thread_id`, and `circular_less_than` and `ceil_to_pow_2`, with one overload per unsigned integer type. `align_for`, `swap_relaxed`, `nomove` and `deref_noexcept` are templates over any type. The queue instantiates upstream's own, so they are not adapted. [tools/check_exports.py](../tools/check_exports.py) lists every name of the namespace with its reason.

## GCC

GCC 16.1 has three module behaviours the wrapper works with:

- The queue's member templates call upstream's `static` helpers, so they expose TU-local entities from the global module fragment. GCC accepts this as an extension, with one warning per template. The module silences that warning (src/module_prologue.hpp), and importers instantiate the templates normally.
- GCC binds a call that does not depend on the queue's `T` when it reads the template, and with it the specialization of a `static` function template (`details::swap_relaxed` on the queue's counters). It emits that body only where a member that calls it is instantiated. The module instantiates the queues' members once, so the bodies are in its interface and every importer emits its own. Without this, `swap` failed to link.
- A translation unit that includes `<cxxabi.h>` (gtest does) and instantiates the queue's thread-exit path fails with GCC: "conflicting 'noexcept' specifier for imported declaration `__cxa_thread_atexit`". This is GCC's handling of a function-local `thread_local` from an imported global module fragment. Clang has neither thread_local recycling nor the error. Keep such code in a unit without `<cxxabi.h>`, or use the `no-thread-local` feature, as [tests/c_api](../tests/c_api) does with gtest.

## Checks

- `mcpp test`: the queues through the module (the module is the only source of the queue's declarations):
  - multi-producer multi-consumer traffic on threads, implicit and with tokens and bulk operations, checking that every item arrives once and in per-producer order;
  - bounded `try_enqueue`;
  - move-only, throwing and over-aligned element types;
  - a counting `Traits`;
  - `swap` and moves;
  - `try_dequeue_from_producer`;
  - the blocking queue's waits with and without timeouts;
  - `LightweightSemaphore`;
  - the adapted helpers;
  - the constants;
  - the side header mixed with the import.
- `tests/c_api` (its own project, with `c-api` and `no-thread-local`): the C API from threads, and gtest beside the queue.
- `python3 tools/check_exports.py`: every declaration of `moodycamel`, every name of `moodycamel::details`, the C API and every macro, against the pinned headers.
- `python3 tools/run_conformance.py`: upstream's `tests/unittests` (all 45 registered tests, including the C API and `corealgos.h` tests), built against the module instead of the headers. The tool documents the mechanical edits and applies them to the pinned tarball's files. The run must end with upstream's "All tests passed.".
