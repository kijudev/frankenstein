// Copyright 2025 Jakub Kijek
// Licensed under the MIT License.
// See LICENSE.md file in the project root for full license information.

#pragma once

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace core_type {

template <class T, class = void>
struct IsNoArgCallable : public std::false_type { };

template <class T>
struct IsNoArgCallable<T, decltype(std::declval<T&&>()())>
    : public std::true_type { };

template <class T>
struct ReturnsVoid
    : public std::is_same<void, decltype(std::declval<T&&>()())> { };

template <class A, class B, class... Rest>
struct And : public And<A, And<B, Rest...>> { };

template <class A, class B>
struct And<A, B> : public std::conditional<A::value, B, A>::type { };

template <class Allocator>
concept HasMaxSize = requires(const Allocator& a) {
    { a.max_size() } -> std::convertible_to<std::size_t>;
};
} // namespace core_utils
