// Copyright 2025 Jakub Kijek
// Licensed under the MIT License.
// See LICENSE.md file in the project root for full license information.

#pragma once

#include <type_traits>

namespace core_utils {

template <class T, class = void>
struct is_noarg_callable_t : public std::false_type {};

template <class T>
struct is_noarg_callable_t<T, decltype(std::declval<T &&>()())>
    : public std::true_type {};

template <class T>
struct returns_void_t
    : public std::is_same<void, decltype(std::declval<T &&>()())> {};

template <class A, class B, class... Rest>
struct and_t : public and_t<A, and_t<B, Rest...>> {};

template <class A, class B>
struct and_t<A, B> : public std::conditional<A::value, B, A>::type {};
} // namespace core_utils
