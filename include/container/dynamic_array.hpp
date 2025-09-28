// Copyright 2025 Jakub Kijek
// Licensed under the MIT License.
// See LICENSE.md file in the project root for full license information.

#include <cassert>
#include <cstddef>
#include <cstring>
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
#include "../macro/noexcept.hpp"

namespace frank {
template <typename T, typename Allocator = std::allocator<T>>
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

        ~Impl() FRANK_NOEXCEPT(std::is_nothrow_destructible_v<T>) {
            if (!is_null()) {
                destroy_self();
                deallocate_self();
            }
        };

        Impl() FRANK_NOEXCEPT(std::is_nothrow_constructible_v<Allocator>)
            : Allocator() { }

        Impl(const Allocator& a) FRANK_NOEXCEPT(
            std::is_nothrow_constructible_v<Allocator, decltype(a)>)
            : Allocator(a) { };

        Impl(Allocator&& a) FRANK_NOEXCEPT
            : Allocator(std::move_if_noexcept(a)) { };

        Impl(const Impl& other) = delete;
        Impl(Impl&& other)      = delete;

        Impl& operator=(const Impl& other) = delete;
        Impl& operator=(Impl&& other)      = delete;

        inline bool is_null() const FRANK_NOEXCEPT { return first == nullptr; }

        void swap(Impl& other) FRANK_NOEXCEPT {
            swap_data(other);
            swap_allocator(other);
        }

        void swap_data(Impl& other) FRANK_NOEXCEPT {
            std::swap(first, other.first);
            std::swap(last, other.last);
            std::swap(capacity, other.capacity);
        }

        void swap_allocator(Impl& other) FRANK_NOEXCEPT {
            std::swap(
                static_cast<Allocator&>(*this), static_cast<Allocator&>(other));
        }

        void set(pointer f, pointer l, pointer c) FRANK_NOEXCEPT {
            first    = f;
            last     = l;
            capacity = c;
        }

        void reset(size_type sz)
            FRANK_NOEXCEPT(std::is_nothrow_destructible_v<T>) {
            destroy_self();
            deallocate_self();

            internal::ScopeGuard rollback([&]() { init_self_null(); });
            init_self(sz);
            rollback.dismiss();
        }

        void init_self(size_type sz) {
            if (sz == 0) [[unlikely]] {
                init_self_null();
                return;
            }

            first    = std::allocator_traits<Allocator>::allocate(*this, sz);
            last     = first;
            capacity = first;
            std::advance(capacity, sz);
        }

        void init_self_null() FRANK_NOEXCEPT {
            first    = nullptr;
            last     = nullptr;
            capacity = nullptr;
        }

        void deallocate_self() FRANK_NOEXCEPT {
            std::allocator_traits<Allocator>::deallocate(
                *this, first, std::distance(first, capacity));
        }

        template <typename... Args>
        void construct_item(pointer p, Args&&... args)
            FRANK_NOEXCEPT(std::is_nothrow_constructible_v<T, Args...>) {
            std::allocator_traits<Allocator>::construct(
                *this, p, std::forward<Args>(args)...);
        }

        void destroy_self() FRANK_NOEXCEPT(std::is_nothrow_destructible_v<T>) {
            destroy_range(first, last);
        }

        void destroy_range(pointer a, pointer b)
            FRANK_NOEXCEPT(std::is_nothrow_destructible_v<T>) {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                for (; a != b; ++a) {
                    std::allocator_traits<Allocator>::destroy(*this, a);
                }
            }
        }

        void destroy_item(pointer p)
            FRANK_NOEXCEPT(std::is_nothrow_destructible_v<T>) {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                std::allocator_traits<Allocator>::destroy(*this, p);
            }
        }

        inline void advance() { std::advance(last, 1); }
        inline void advance_by(size_type n) { std::advance(last, n); }

        inline void prev() { std::prev(last, 1); }
        inline void prev_by(size_type n) { std::prev(last, n); }
    };

    Impl impl;

public:
    ~DynamicArray() = default;

    DynamicArray() FRANK_NOEXCEPT : impl() { }

    explicit DynamicArray(const Allocator& a)
        FRANK_NOEXCEPT(std::is_nothrow_constructible_v<Impl, decltype(a)>)
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

    DynamicArray(const DynamicArray& other, const Allocator& a)
        : impl(a) {
        grow(other.size());
        copy_range(other.impl.first, other.impl.last, impl.first);
    }

    DynamicArray(DynamicArray&& other) FRANK_NOEXCEPT { impl.swap(other.impl); }

    DynamicArray(DynamicArray&& other, const Allocator& a) FRANK_NOEXCEPT
        : impl(a) {
        impl.swap_data(other.impl);
    }

    DynamicArray& operator=(DynamicArray other) {
        if constexpr (std::allocator_traits<
                          T>::propagate_on_container_copy_assignment) {
            impl.swap(other.impl);
        } else {
            impl.swap_data(other.impl);
        }

        return *this;
    }

public:
    iterator begin() FRANK_NOEXCEPT { return std::to_address(impl.first); }
    const_iterator begin() const FRANK_NOEXCEPT {
        return std::to_address(impl.first);
    }

    iterator       end() FRANK_NOEXCEPT { return std::to_address(impl.last); }
    const_iterator end() const FRANK_NOEXCEPT {
        return std::to_address(impl.last);
    }

    reverse_iterator rbegin() FRANK_NOEXCEPT { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const FRANK_NOEXCEPT {
        return const_reverse_iterator(end());
    }

    reverse_iterator rend() FRANK_NOEXCEPT { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const FRANK_NOEXCEPT {
        return const_reverse_iterator(begin());
    }

    const_iterator cbegin() const FRANK_NOEXCEPT {
        return std::to_address(impl.first);
    }
    const_iterator cend() const FRANK_NOEXCEPT {
        return std::to_address(impl.last);
    }

    const_reverse_iterator crbegin() const FRANK_NOEXCEPT {
        return const_reverse_iterator(end());
    }
    const_reverse_iterator crend() const FRANK_NOEXCEPT {
        return const_reverse_iterator(begin());
    }

    reference operator[](size_type idx) FRANK_NOEXCEPT {
        return impl.first[idx];
    }
    const_reference operator[](size_type idx) const FRANK_NOEXCEPT {
        return impl.first[idx];
    }

    [[nodiscard]] std::optional<reference> at(size_type idx) FRANK_NOEXCEPT {
        return is_idx_valid(idx) ? std::optional<reference>(impl.first[idx]) :
                                   std::nullopt;
    }

    [[nodiscard]] std::optional<const_reference>
    at(size_type idx) const FRANK_NOEXCEPT {
        return is_idx_valid(idx) ?
                   std::optional<const_reference>(impl.first[idx]) :
                   std::nullopt;
    }

    [[nodiscard]] std::optional<reference> front() FRANK_NOEXCEPT {
        return is_null() ? std::nullopt : std::optional<reference>(*impl.first);
    }

    [[nodiscard]] std::optional<const_reference> front() const FRANK_NOEXCEPT {
        return is_null() ? std::nullopt :
                           std::optional<const_reference>(*impl.first);
    }

    reference       front_unsafe() FRANK_NOEXCEPT { return *impl.first; }
    const_reference front_unsafe() const FRANK_NOEXCEPT { return *impl.first; }

    [[nodiscard]] std::optional<reference> back() FRANK_NOEXCEPT {
        return is_null() ? std::nullopt :
                           std::optional<reference>(impl.last[-1]);
    }

    [[nodiscard]] std::optional<const_reference> back() const FRANK_NOEXCEPT {
        return is_null() ? std::nullopt :
                           std::optional<const_reference>(impl.last[-1]);
    }

    reference       back_unsafe() FRANK_NOEXCEPT { return impl.last[-1]; }
    const_reference back_unsafe() const FRANK_NOEXCEPT { return impl.last[-1]; }

    [[nodiscard]] bool is_null() const FRANK_NOEXCEPT { return impl.is_null(); }

    [[nodiscard]] bool is_empty() const FRANK_NOEXCEPT {
        return impl.first == impl.last;
    }

    [[nodiscard]] bool is_full() const FRANK_NOEXCEPT {
        return impl.last == impl.capacity;
    }

    [[nodiscard]] size_type size() const FRANK_NOEXCEPT {
        return std::distance(impl.first, impl.last);
    }

    [[nodiscard]] size_type capacity() const FRANK_NOEXCEPT {
        return std::distance(impl.first, impl.capacity);
    }

    [[nodiscard]] constexpr size_type max_size() const FRANK_NOEXCEPT {
        if constexpr (internal::HasMaxSize<Allocator>) {
            return std::allocator_traits<Allocator>::max_size();
        } else {
            return std::numeric_limits<size_type>::max();
        }
    }

    [[nodiscard]] Allocator allocator() const FRANK_NOEXCEPT {
        return static_cast<Allocator>(impl);
    }

    [[nodiscard]] std::optional<T*> data() FRANK_NOEXCEPT {
        return is_null() ? std::nullopt : std::to_address(impl.first);
    }

    [[nodiscard]] std::optional<const T*> data() const FRANK_NOEXCEPT {
        return is_null() ? std::nullopt : std::to_address(impl.first);
    }

    T* data_unsafe() FRANK_NOEXCEPT { return std::to_address(impl.first); }
    const T* data_unsafe() const FRANK_NOEXCEPT {
        return std::to_address(impl.first);
    }

public:
    template <typename... Args>
    void emplace_back(Args&&... args) {
        if (is_full()) {
            grow(calc_next_capacity());
        }

        impl.construct_item(impl.last, std::forward<Args>(args)...);
        impl.advance();
    }

    void push_back(const T& item) { emplace_back(item); }

    void push_back(T&& item) { emplace_back(std::move(item)); }

    void pop_back() FRANK_NOEXCEPT(FRANK_NOEXCEPT_METHOD(Impl, destroy_item)) {
        FRANK_ASSERT(!is_empty());

        impl.prev();
        impl.destroy_item(impl.last);
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
            impl.swap_data(new_impl);
            return;
        }

        move_range(impl.first, impl.last, new_impl.first);
        new_impl.advance_by(size());
        impl.swap_data(new_impl);
    }

    void shrink_to_fit() { shrink(size()); }

    void shrink(size_type sz) {
        FRANK_ASSERT(sz < capacity());
        FRANK_ASSERT(sz >= size());

        Impl new_impl(static_cast<Allocator>(impl));
        new_impl.init_self(sz);

        move_range(impl.first, impl.last, new_impl.first);

        impl.swap(new_impl);
    }

private:
    void copy_range(pointer a, pointer b, pointer dest)
        FRANK_NOEXCEPT(std::is_nothrow_copy_constructible_v<T>) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(
                static_cast<void*>(dest),
                static_cast<void*>(a),
                std::distance(a, b));
        } else {
            std::uninitialized_copy(a, b, dest);
        }
    }

    void move_range(pointer a, pointer b, pointer dest)
        FRANK_NOEXCEPT(std::is_nothrow_move_constructible_v<T>) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(
                static_cast<void*>(dest),
                static_cast<void*>(a),
                std::distance(a, b));
        } else {
            std::uninitialized_move(a, b, dest);
        }
    }

    [[nodiscard]] inline bool
    is_iterator_valid(const_iterator i) FRANK_NOEXCEPT {
        return (std::to_address(i) >= std::to_address(impl.first))
               && (std::to_address(i) <= std::to_address(impl.last));
    }

    [[nodiscard]] inline bool is_idx_valid(size_type idx) FRANK_NOEXCEPT {
        return idx < size();
    }

    [[nodiscard]] inline size_type calc_next_capacity() FRANK_NOEXCEPT {
        return is_empty() ? 1 : capacity() * 2;
    }
};
}
