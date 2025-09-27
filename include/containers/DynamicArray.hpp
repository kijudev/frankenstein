// Copyright 2025 Jakub Kijek
// Licensed under the MIT License.
// See LICENSE.md file in the project root for full license information.

#include "utils/ScopeGuard.hpp"
#include "utils/TypeTraits.hpp"
#include <cstddef>
#include <cstring>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>

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

        ~Impl() noexcept(std::is_nothrow_destructible_v<T>) {
            if (!is_null()) {
                destroy_self();
                deallocate_self();
            }
        };

        Impl() noexcept(std::is_nothrow_constructible_v<Allocator>)
            : Allocator() { }

        Impl(const Allocator& a) noexcept(
            std::is_nothrow_constructible_v<Allocator, decltype(a)>)
            : Allocator(a) { };

        Impl(Allocator&& a) noexcept
            : Allocator(std::move_if_noexcept(a)) { };

        Impl(const Impl& other) = delete;
        Impl(Impl&& other)      = delete;

        Impl& operator=(const Impl& other) = delete;
        Impl& operator=(Impl&& other)      = delete;

        inline bool is_null() noexcept { return first == nullptr; }

        void swap(Impl& other) noexcept {
            swap_data(other);
            swap_allocator(other);
        }

        void swap_data(Impl& other) noexcept {
            std::swap(first, other.first);
            std::swap(last, other.last);
            std::swap(capacity, other.capacity);
        }

        void swap_allocator(Impl& other) noexcept {
            std::swap(
                static_cast<Allocator&>(*this), static_cast<Allocator&>(other));
        }

        void set(pointer f, pointer l, pointer c) noexcept {
            first    = f;
            last     = l;
            capacity = c;
        }

        void reset(size_type sz) noexcept(std::is_nothrow_destructible_v<T>) {
            destroy_self();
            deallocate_self();

            utils::ScopeGuard rollback([&]() { init_self_null(); });
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

        void init_self_null() noexcept {
            first    = nullptr;
            last     = nullptr;
            capacity = nullptr;
        }

        void deallocate_self() noexcept {
            std::allocator_traits<Allocator>::deallocate(
                *this, first, std::distance(first, capacity));
        }

        template <typename... Args>
        void construct_item(pointer p, Args&&... args) noexcept(
            std::is_nothrow_constructible_v<T, Args...>) {
            std::allocator_traits<Allocator>::construct(
                *this, p, std::forward<Args>(args)...);
        }

        void destroy_self() noexcept(std::is_nothrow_destructible_v<T>) {
            destroy_range(first, last);
        }

        void destroy_range(pointer a, pointer b) noexcept(
            std::is_nothrow_destructible_v<T>) {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                for (; a != b; ++a) {
                    std::allocator_traits<Allocator>::destroy(*this, a);
                }
            }
        }

        void
        destroy_item(pointer p) noexcept(std::is_nothrow_destructible_v<T>) {
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

    DynamicArray() noexcept
        : impl() { }

    explicit DynamicArray(const Allocator& a) noexcept(
        std::is_nothrow_constructible_v<Impl, decltype(a)>)
        : impl(a) { }

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
        return impl.is_null() ? std::nullopt :
                                std::optional<reference>(*impl.first);
    }

    [[nodiscard]] std::optional<const_reference> front() const noexcept {
        return impl.is_null() ? std::nullopt :
                                std::optional<const_reference>(*impl.first);
    }

    reference       front_unsafe() noexcept { return *impl.first; }
    const_reference front_unsafe() const noexcept { return *impl.first; }

    [[nodiscard]] std::optional<reference> back() noexcept {
        return impl.is_null() ? std::nullopt :
                                std::optional<reference>(impl.last[-1]);
    }

    [[nodiscard]] std::optional<const_reference> back() const noexcept {
        return impl.is_null() ? std::nullopt :
                                std::optional<const_reference>(impl.last[-1]);
    }

    reference       back_unsafe() noexcept { return impl.last[-1]; }
    const_reference back_unsafe() const noexcept { return impl.last[-1]; }

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
        if constexpr (utils::HasMaxSize<Allocator>) {
            return std::allocator_traits<Allocator>::max_size();
        } else {
            return std::numeric_limits<size_type>::max();
        }
    }

    [[nodiscard]] Allocator allocator() const noexcept {
        return static_cast<Allocator>(impl);
    }

    [[nodiscard]] std::optional<T*> data() noexcept {
        return impl.is_null() ? std::nullopt : std::to_address(impl.first);
    }

    [[nodiscard]] std::optional<const T*> data() const noexcept {
        return impl.is_null() ? std::nullopt : std::to_address(impl.first);
    }

    T*       data_unsafe() noexcept { return std::to_address(impl.first); }
    const T* data_unsafe() const noexcept {
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

    void grow(size_type sz) { }

    void shrink_to_fit() { }

    void shrink(size_type sz) { }

private:
    void copy_range(pointer a, pointer b, pointer dest) noexcept(
        std::is_nothrow_copy_constructible_v<T>) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(
                static_cast<void*>(dest),
                static_cast<void*>(a),
                std::distance(a, b));
        } else {
            std::uninitialized_copy(a, b, dest);
        }
    }

    void move_range(pointer a, pointer b, pointer dest) noexcept(
        std::is_nothrow_move_constructible_v<T>) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(
                static_cast<void*>(dest),
                static_cast<void*>(a),
                std::distance(a, b));
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
