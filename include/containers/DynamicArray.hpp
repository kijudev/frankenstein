// Copyright 2025 Jakub Kijek
// Licensed under the MIT License.
// See LICENSE.md file in the project root for full license information.

#include "utils/ScopeGuard.hpp"
#include <cstddef>
#include <iterator>
#include <memory>
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
            : Allocator(a) {};

        Impl(Allocator&& a) noexcept
            : Allocator(std::move_if_noexcept(a)) {};

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
            }

            first    = std::allocator_traits<Allocator>::allocate(*this, sz);
            last     = first;
            capacity = first + sz;
        }

        void init_self_null() noexcept {
            first    = nullptr;
            last     = nullptr;
            capacity = nullptr;
        }

        void deallocate_self() noexcept {
            std::allocator_traits<Allocator>::deallocate(
                *this, first, last - first);
        }

        template <typename... Args>
        void construct_item(pointer p, Args&&... args) noexcept(
            std::is_nothrow_constructible_v<T, Args...>) {
            std::allocator_traits<Allocator>::construct(
                *this, p, std::forward(args)...);
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
    };
};
}
