// Copyright 2025 Jakub Kijek
// Licensed under the MIT License.
// See LICENSE.md file in the project root for full license information.

#pragma once

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace core_type {

template <class T>
concept NoArgCallable = requires(T&& t) {
    { t() };
    { std::forward<T>(t)() };
};

template <class T>
concept ReturnsVoid = requires(T&& t) {
    { t() };
    { std::forward<T>(t)() } -> std::same_as<void>;
};

template <class T>
concept NothrowDestructible = std::is_nothrow_destructible_v<T>;

template <class Allocator>
concept HasMaxSize = requires(const Allocator& a) {
    { a.max_size() } -> std::convertible_to<std::size_t>;
};
} // namespace core_utils
