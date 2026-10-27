// <mwrap/concurrentqueue/config.hpp> — the configuration mcpplibs.mwrap.concurrentqueue
// is built with, applied before concurrentqueue.h. Every other configuration
// macro keeps upstream's default.
//
// MOODYCAMEL_NOEXCEPT_CTOR / MOODYCAMEL_NOEXCEPT_ASSIGN: upstream's
// configuration point (both are only defined by concurrentqueue.h when
// MOODYCAMEL_NOEXCEPT is not), here spelled with the type traits that give the
// same answer as upstream's `noexcept(expr)`. Upstream's expression names
// details::deref_noexcept, a `static` (TU-local) function; inside a template
// argument (details::nomove_if<…> in enqueue_bulk) that makes the class
// template's declaration expose a TU-local entity, which GCC rejects in a
// module interface. The traits name nothing TU-local.
//
//   noexcept(new (p) T(details::deref_noexcept(it)))  ==  std::is_nothrow_constructible<T, decltype(*it)>
//   noexcept(element = std::move(el))                 ==  std::is_nothrow_assignable<T&, T&&>
//
// Without exceptions (upstream's own test) upstream's definitions stand.
#pragma once
#include <type_traits>

#if !defined(MOODYCAMEL_NOEXCEPT) && !defined(MOODYCAMEL_EXCEPTIONS_ENABLED) && \
    ((defined(_MSC_VER) && defined(_CPPUNWIND)) || (defined(__GNUC__) && defined(__EXCEPTIONS)) || \
     (!defined(_MSC_VER) && !defined(__GNUC__)))
#define MOODYCAMEL_NOEXCEPT noexcept
#define MOODYCAMEL_NOEXCEPT_CTOR(type, valueType, expr) (::std::is_nothrow_constructible<type, valueType>::value)
#define MOODYCAMEL_NOEXCEPT_ASSIGN(type, valueType, expr) (::std::is_nothrow_assignable<type&, valueType>::value)
#endif
