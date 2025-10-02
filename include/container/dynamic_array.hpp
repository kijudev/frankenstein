// Copyright 2025 Jakub Kijek
// Licensed under the MIT License.
// See LICENSE.md file in the project root for full license information.

#include <algorithm>
#include <cassert>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>

#include "../internal/scope_guard.hpp"
#include "../internal/type_traits.hpp"
#include "../macro/assert.hpp"

namespace frank {
template <typename T, typename Allocator = std::allocator<T>>
    requires std::copy_constructible<T> && std::move_constructible<T>
             && std::destructible<T>
class DynamicArray {
public:
    using value_type      = T;
    using reference       = T&;
    using const_reference = const T&;
    using size_type       = size_t;
    using difference_type = std::ptrdiff_t;
    using allocator_type  = Allocator;
    using pointer         = std::allocator_traits<Allocator>::pointer;
    using const_pointer   = std::allocator_traits<Allocator>::const_pointer;

    using iterator               = T*;
    using const_iterator         = const T*;
    using reverse_iterator       = std::reverse_iterator<T*>;
    using const_reverse_iterator = std::reverse_iterator<const T*>;
    using iterator_category      = std::contiguous_iterator_tag;

private:
    // The Impl struct stores all the data of the DynamicArray.
    // It also houses some utility functions for allocating, deallocating
    // memory, consucting and destructing items. Impl enables EBO for the
    // Allocator type. All the methods in Impl are unsafe, there is no bound
    // checking, nullptr checking etc.
    struct Impl : public Allocator {
        // Points to the first item
        pointer first {nullptr};

        // Points one past the last item
        pointer last {nullptr};

        // Points at the end of the memory block
        pointer capacity {nullptr};

        ~Impl() noexcept(std::is_nothrow_destructible_v<T>) {
            if (!is_null()) {
                destroy_self();
                deallocate_self();
            }
        };

        Impl() noexcept(std::is_nothrow_constructible_v<Allocator>)
            : Allocator() { }

        Impl(const Allocator& a) noexcept(
            (std::is_nothrow_constructible_v<Allocator, decltype(a)>))
            : Allocator(a) { };

        Impl(Allocator&& a) noexcept(
            std::is_nothrow_move_constructible_v<Allocator>)
            : Allocator(std::move_if_noexcept(a)) { };

        Impl(const Impl& other) = delete;
        Impl(Impl&& other)      = delete;

        Impl& operator=(const Impl& other) = delete;
        Impl& operator=(Impl&& other)      = delete;

        bool is_null() const noexcept { return first == nullptr; }

        void swap_without_allocator(Impl& other) noexcept {
            std::swap(first, other.first);
            std::swap(last, other.last);
            std::swap(capacity, other.capacity);
        }

        void swap_with_allocator(Impl& other) noexcept {
            std::swap(*this, other);
        }

        void init_self(size_type sz) {
            FRANK_ASSERT(sz != 0);

            first    = std::allocator_traits<Allocator>::allocate(*this, sz);
            last     = first;
            capacity = first;

            std::advance(capacity, sz);
        }

        void init_self_null() noexcept {
            first    = nullptr;
            last     = nullptr;
            capacity = nullptr;
        }

        void deallocate_self() noexcept {
            std::allocator_traits<Allocator>::deallocate(
                static_cast<Allocator&>(*this),
                first,
                std::distance(first, capacity));
        }

        template <typename... Args>
        void construct_item(pointer p, Args&&... args) noexcept(
            std::is_nothrow_constructible_v<T, Args...>) {
            FRANK_ASSERT(is_valid_pointer(p));

            std::allocator_traits<Allocator>::construct(
                static_cast<Allocator&>(*this), p, std::forward<Args>(args)...);
        }

        void destroy_self() noexcept(std::is_nothrow_destructible_v<T>) {
            destroy_range(first, last);
        }

        void destroy_range(pointer a, pointer b) noexcept(
            std::is_nothrow_destructible_v<T>) {
            FRANK_ASSERT(is_valid_range(a, b));

            if constexpr (!std::is_trivially_destructible_v<T>) {
                for (; a != b; ++a) {
                    std::allocator_traits<Allocator>::destroy(
                        static_cast<Allocator&>(*this), a);
                }
            }
        }

        void
        destroy_item(pointer p) noexcept(std::is_nothrow_destructible_v<T>) {
            FRANK_ASSERT(is_valid_pointer(p));

            if constexpr (!std::is_trivially_destructible_v<T>) {
                std::allocator_traits<Allocator>::destroy(
                    static_cast<Allocator&>(*this), p);
            }
        }

        void move_range_right(pointer a, pointer b, size_type n) noexcept(
            std::is_nothrow_move_constructible_v<T>) {
            FRANK_ASSERT(is_valid_range(a, b));

            if constexpr (std::is_trivially_move_constructible_v<T>) {
                std::memmove(
                    static_cast<void*>(a + n),
                    static_cast<const void*>(a),
                    std::distance(a, b) * sizeof(T));
            } else {
                std::move_backward(a, b, b + n);
            }
        }

        void move_range_left(pointer a, pointer b, size_type n) noexcept(
            std::is_nothrow_move_constructible_v<T>) {
            FRANK_ASSERT(is_valid_range(a, b));

            if constexpr (std::is_trivially_move_constructible_v<T>) {
                std::memmove(
                    static_cast<void*>(a - n),
                    static_cast<const void*>(a),
                    std::distance(a, b) * sizeof(T));
            } else {
                std::move(a, b, a - n);
            }
        }
        inline void advance(size_type n) {
            FRANK_ASSERT(n != 0);
            std::advance(last, n);
        }

        inline void prev(size_type n) {
            FRANK_ASSERT(n != 0);
            last = std::prev(last, n);
        }

        [[nodiscard]] inline bool is_valid_pointer(pointer p) noexcept {
            return std::distance(first, p) >= 0
                   && std::distance(p, capacity) > 0;
        }

        [[nodiscard]] inline bool
        is_valid_range(pointer a, pointer b) noexcept {
            return std::distance(a, b) >= 0 && std::distance(first, a) >= 0
                   && std::distance(a, capacity) >= 0
                   && std::distance(first, b) >= 0
                   && std::distance(b, capacity) >= 0;
        }
    };

    Impl impl;

public:
    ~DynamicArray() = default;

    DynamicArray() noexcept(std::is_nothrow_constructible_v<Impl>)
        : impl() { }

    explicit DynamicArray(const Allocator& a) noexcept(
        std::is_nothrow_constructible_v<Impl, decltype(a)>)
        : impl(a) { }

    explicit DynamicArray(size_type sz, const Allocator& a = Allocator())
        : impl(a) {
        grow(sz);
    }

    DynamicArray(const DynamicArray& other)
        : DynamicArray(
              other,
              std::allocator_traits<T>::select_on_container_copy_construction(
                  static_cast<Allocator>(other.impl))) { }

    DynamicArray(const DynamicArray& other, const Allocator& a = Allocator())
        : impl(a) {
        grow(other.size());
        copy_range(other.impl.first, other.impl.last, impl.first);
    }

    DynamicArray(DynamicArray&& other) noexcept(
        std::is_nothrow_move_constructible_v<Allocator>) {
        impl.swap(other.impl);
    }

    DynamicArray(DynamicArray&& other, const Allocator& a) noexcept(
        std::is_nothrow_constructible_v<Impl, decltype(a)>)
        : impl(a) {
        impl.swap_without_allocator(other.impl);
    }

    DynamicArray(std::initializer_list<T> il, const Allocator& a = Allocator())
        : impl(a) {
        assign(il);
    }

    template <typename It>
        requires std::input_iterator<It>
                 && std::convertible_to<std::iter_value_t<It>, T>
    DynamicArray(It a, It b) {
        assign(a, b);
    }

    DynamicArray& operator=(DynamicArray other) {
        if constexpr (std::allocator_traits<
                          Allocator>::propagate_on_container_copy_assignment) {
            impl.swap(other.impl);
        } else {
            impl.swap_without_allocator(other.impl);
        }

        return *this;
    }

    DynamicArray& operator=(DynamicArray&& other) noexcept(
        std::is_nothrow_destructible_v<T>
        && (std::allocator_traits<
                Allocator>::propagate_on_container_move_assigment ?
                std::is_nothrow_move_constructible_v<Allocator> :
                true)) {
        if constexpr (std::allocator_traits<
                          Allocator>::propagate_on_container_move_assigment) {
            impl.swap(other.impl);
        } else {
            impl.swap_without_allocator(other.impl);
        }

        if (!other.is_null()) {
            other.impl.destroy_self();
            other.impl.deallocate_self();
        }

        other.impl.init_self_null();

        return *this;
    }

    bool operator==(const DynamicArray& other) const noexcept {
        return size() == other.size()
               && std::equal(cbegin(), cend(), other.cbegin());
    }

    auto operator<=>(const DynamicArray& other) const
        noexcept(noexcept(std::lexicographical_compare_three_way(
            std::declval<const_iterator>(),
            std::declval<const_iterator>(),
            std::declval<const_iterator>(),
            std::declval<const_iterator>(),
            std::compare_three_way {})))
        requires std::three_way_comparable<T>
    {
        return std::lexicographical_compare_three_way(
            cbegin(),
            cend(),
            other.cbegin(),
            other.cend(),
            std::compare_three_way {});
    }

public:
    iterator       begin() noexcept { return std::to_address(impl.first); }
    const_iterator begin() const noexcept {
        return std::to_address(impl.first);
    }

    iterator       end() noexcept { return std::to_address(impl.last); }
    const_iterator end() const noexcept { return std::to_address(impl.last); }

    reverse_iterator       rbegin() noexcept { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    reverse_iterator       rend() noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }

    const_iterator cbegin() const noexcept {
        return std::to_address(impl.first);
    }
    const_iterator cend() const noexcept { return std::to_address(impl.last); }

    const_reverse_iterator crbegin() const noexcept {
        return const_reverse_iterator(end());
    }
    const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator(begin());
    }

    reference operator[](size_type idx) noexcept { return impl.first[idx]; }
    const_reference operator[](size_type idx) const noexcept {
        return impl.first[idx];
    }

    [[nodiscard]] std::optional<reference> at(size_type idx) noexcept {
        return is_idx_valid(idx) ? std::optional<reference>(impl.first[idx]) :
                                   std::nullopt;
    }

    [[nodiscard]] std::optional<const_reference>
    at(size_type idx) const noexcept {
        return is_idx_valid(idx) ?
                   std::optional<const_reference>(impl.first[idx]) :
                   std::nullopt;
    }

    [[nodiscard]] std::optional<reference> front() noexcept {
        return is_null() ? std::nullopt : std::optional<reference>(*impl.first);
    }

    [[nodiscard]] std::optional<const_reference> front() const noexcept {
        return is_null() ? std::nullopt :
                           std::optional<const_reference>(*impl.first);
    }

    reference       front_unsafe() noexcept { return *impl.first; }
    const_reference front_unsafe() const noexcept { return *impl.first; }

    [[nodiscard]] std::optional<reference> back() noexcept {
        return is_null() ? std::nullopt :
                           std::optional<reference>(impl.last[-1]);
    }

    [[nodiscard]] std::optional<const_reference> back() const noexcept {
        return is_null() ? std::nullopt :
                           std::optional<const_reference>(impl.last[-1]);
    }

    reference       back_unsafe() noexcept { return impl.last[-1]; }
    const_reference back_unsafe() const noexcept { return impl.last[-1]; }

    [[nodiscard]] bool is_null() const noexcept { return impl.is_null(); }

    [[nodiscard]] bool is_empty() const noexcept {
        return impl.first == impl.last;
    }

    [[nodiscard]] bool is_full() const noexcept {
        return impl.last == impl.capacity;
    }

    [[nodiscard]] size_type size() const noexcept {
        return std::distance(impl.first, impl.last);
    }

    [[nodiscard]] size_type capacity() const noexcept {
        return std::distance(impl.first, impl.capacity);
    }

    [[nodiscard]] constexpr size_type max_size() const noexcept {
        if constexpr (internal::HasMaxSize<Allocator>) {
            return std::allocator_traits<Allocator>::max_size();
        } else {
            return std::numeric_limits<size_type>::max();
        }
    }

    [[nodiscard]] Allocator allocator() const noexcept {
        return static_cast<Allocator>(impl);
    }

    [[nodiscard]] std::optional<T*> data() noexcept {
        return is_null() ? std::nullopt : std::to_address(impl.first);
    }

    [[nodiscard]] std::optional<const T*> data() const noexcept {
        return is_null() ? std::nullopt : std::to_address(impl.first);
    }

    T*       data_unsafe() noexcept { return std::to_address(impl.first); }
    const T* data_unsafe() const noexcept {
        return std::to_address(impl.first);
    }

public:
    void push_back(const T& item) { emplace_back(item); }

    void push_back(T&& item) { emplace_back(std::move(item)); }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        if (is_full()) {
            grow(calc_next_capacity());
        }

        impl.construct_item(impl.last, std::forward<Args>(args)...);
        impl.advance(1);
    }

    void pop_back() noexcept(std::is_nothrow_destructible_v<T>) {
        FRANK_ASSERT(!is_empty());

        impl.prev(1);
        impl.destroy_item(impl.last);
    }

    void insert(size_type idx, const T& item) noexcept(
        std::is_nothrow_move_constructible_v<T>
        && std::is_nothrow_constructible_v<T, T&&>) {
        emplace(idx, item);
    }

    void insert(size_type idx, T&& item) noexcept(
        std::is_nothrow_move_constructible_v<T>
        && std::is_nothrow_constructible_v<T, T&&>) {
        emplace(idx, std::move(item));
    }

    template <typename... Args>
    void emplace(size_type idx, Args&&... args) noexcept(
        std::is_nothrow_move_constructible_v<T>
        && std::is_nothrow_constructible_v<T, Args...>) {
        FRANK_ASSERT(is_idx_valid(idx));

        impl.move_range_right(impl.first + idx + 1, impl.last, 1);
        impl.construct_item(impl.first + idx, std::forward<Args>(args)...);
        impl.advance(1);
    }

    void assign(std::initializer_list<T> il) { assign(il.begin(), il.end()); }

    template <typename It>
        requires std::input_iterator<It>
                 && std::convertible_to<std::iter_value_t<It>, T>
    void assign(It a, It b) {
        impl.destroy_self();

        size_type size = std::distance(a, b);
        if (size > capacity()) {
            grow(size);
        }

        if constexpr (
            std::contiguous_iterator<It> && std::is_trivially_copyable_v<T>
            && std::is_trivially_copyable_v<std::iter_value_t<It>>
            && std::is_same_v<std::iter_value_t<It>, T>) {
            copy_range(
                reinterpret_cast<const_pointer>(a),
                reinterpret_cast<const_pointer>(b),
                impl.first);
        } else {
            std::copy(a, b, impl.first);
        }

        impl.advance(size);
    }

    void erase(size_type idx) noexcept(
        std::is_nothrow_destructible_v<T>
        && std::is_nothrow_move_constructible_v<T>) {
        FRANK_ASSERT(is_idx_valid(idx));

        if (idx == size() - 1) {
            pop_back();
            return;
        }

        impl.destroy_item(impl.first + idx);
        impl.move_range_left(impl.first + idx + 1, impl.last, 1);
        impl.prev(1);
    }

    void clear() noexcept(std::is_nothrow_destructible_v<T>) {
        impl.destroy_self();
        impl.last = impl.first;
    }

    void reserve(size_type sz) {
        FRANK_ASSERT(sz > size());
        if (sz <= capacity()) {
            return;
        }

        grow(sz);
    }

    void grow(size_type sz) {
        FRANK_ASSERT(sz > capacity());

        Impl new_impl(static_cast<Allocator>(impl));
        new_impl.init_self(sz);

        if (is_null()) [[unlikely]] {
            impl.swap_without_allocator(new_impl);
            return;
        }

        move_range(impl.first, impl.last, new_impl.first);
        new_impl.advance(size());

        impl.swap_without_allocator(new_impl);
        new_impl.last = new_impl.first;
    }

    void shrink_to_fit() { shrink(size()); }

    void shrink(size_type sz) {
        FRANK_ASSERT(sz < capacity());
        FRANK_ASSERT(sz >= size());

        Impl new_impl(static_cast<Allocator>(impl));
        new_impl.init_self(sz);

        move_range(impl.first, impl.last, new_impl.first);

        impl.swap(new_impl);
        new_impl.last = new_impl.first;
    }

private:
    void copy_range(const_pointer a, const_pointer b, pointer dest) noexcept(
        std::is_nothrow_copy_constructible_v<T>) {
        FRANK_ASSERT(std::distance(a, b) >= 0);

        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(
                static_cast<void*>(dest),
                static_cast<const void*>(a),
                std::distance(a, b) * sizeof(T));
        } else {
            std::uninitialized_copy(a, b, dest);
        }
    }

    void move_range(const_pointer a, const_pointer b, pointer dest) noexcept(
        std::is_nothrow_move_constructible_v<T>) {
        FRANK_ASSERT(std::distance(a, b) >= 0);

        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(
                static_cast<void*>(dest),
                static_cast<const void*>(a),
                std::distance(a, b) * sizeof(T));
        } else {
            std::uninitialized_move(a, b, dest);
        }
    }

    [[nodiscard]] inline bool is_iterator_valid(const_iterator i) noexcept {
        return (std::to_address(i) >= std::to_address(impl.first))
               && (std::to_address(i) <= std::to_address(impl.last));
    }

    [[nodiscard]] inline bool is_idx_valid(size_type idx) noexcept {
        return idx < size();
    }

    [[nodiscard]] inline size_type calc_next_capacity() noexcept {
        return is_empty() ? 1 : capacity() * 2;
    }
};
}
