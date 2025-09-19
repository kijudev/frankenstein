// Copyright 2025 Jakub Kijek
// Licensed under the MIT License.
// See LICENSE.md file in the project root for full license information.

#include "ScopeGuard.hpp"
#include "TypeTraits.hpp"
#include <concepts>
#include <cstddef>
#include <cstring>
#include <exception>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>

namespace core {
template <typename T, typename Allocator = std::allocator<T>>
class DynamicArray {
    // =========================================================================
    // Typedefs, members
    // =========================================================================
private:
    using AT = std::allocator_traits<Allocator>;

public:
    using value_type      = T;
    using reference       = T&;
    using const_reference = const T&;
    using size_type       = size_t;
    using difference_type = std::ptrdiff_t;
    using allocator_type  = Allocator;
    using pointer         = typename AT::pointer;
    using const_pointer   = typename AT::const_pointer;

    using iterator               = T*;
    using const_iterator         = const T*;
    using reverse_iterator       = std::reverse_iterator<T*>;
    using const_reverse_iterator = std::reverse_iterator<const T*>;
    using iterator_category      = std::random_access_iterator_tag;

private:
    Allocator m_allocator;
    pointer   m_first, m_last, m_capacity;

    // =========================================================================
    // Destructor, constructors, assigment operators, lexical operators
    // =========================================================================
public:
    ~DynamicArray() { }

    DynamicArray() { }

    explicit DynamicArray(const Allocator& a) { }

    explicit DynamicArray(size_type size, const Allocator& a = Allocator()) { }

    DynamicArray(const DynamicArray& other) { }

    DynamicArray(const DynamicArray& other, const Allocator& a) { }

    DynamicArray(DynamicArray&& other) { }

    DynamicArray(DynamicArray&& other, const Allocator& a) { }

    DynamicArray(std::initializer_list<T> il, const Allocator& a) { }

    template <typename It>
    DynamicArray(It first, It last, const Allocator& a = Allocator()) { }

    DynamicArray operator=(const DynamicArray& other) { }

    DynamicArray operator=(DynamicArray&& other) { }

    bool operator==(const DynamicArray& other) const
        requires std::equality_comparable<T>
    { }

    bool operator!=(const DynamicArray& other) const
        requires std::equality_comparable<T>
    { }

    bool operator<(const DynamicArray& other) const
        requires std::totally_ordered<T>
    { }

    bool operator<=(const DynamicArray& other) const
        requires std::totally_ordered<T>
    { }

    bool operator>(const DynamicArray& other) const
        requires std::totally_ordered<T>
    { }

    bool operator>=(const DynamicArray& other) const
        requires std::totally_ordered<T>
    { }

    // =========================================================================
    // Iterator
    // =========================================================================
public:
    iterator       begin() noexcept { return m_first; }
    const_iterator begin() const noexcept { return m_first; }
    iterator       end() noexcept { return m_last; }
    const_iterator end() const noexcept { return m_last; }

    reverse_iterator       rbegin() noexcept { return m_last; }
    const_reverse_iterator rbegin() const noexcept { return m_last; }

    reverse_iterator       rend() noexcept { return m_first; }
    const_reverse_iterator rend() const noexcept { return m_first; }

    const_iterator         cbegin() const noexcept { return m_first; }
    const_iterator         cend() const noexcept { return m_last; }
    const_reverse_iterator crbegin() const noexcept { return m_last; }
    const_reverse_iterator crend() const noexcept { return m_first; }

    // =========================================================================
    // Info
    // =========================================================================
public:
    [[nodiscard]] bool is_empty() const noexcept { return size() == 0; }

    [[nodiscard]] bool is_full() const noexcept { return size() = capacity(); }

    [[nodiscard]] size_type capacity() const noexcept {
        return std::distance(m_first, m_capacity);
    }

    [[nodiscard]] constexpr size_type max_capacity() const noexcept {
        return max_capacity();
    }

    [[nodiscard]] size_type size() const noexcept {
        return std::distance(m_first, m_last);
    }

    [[nodiscard]] constexpr size_type max_size() const noexcept {
        if constexpr (core_type::HasMaxSize<Allocator>) {
            return AT::max_size();
        } else {
            return std::numeric_limits<size_type>::max();
        }
    }

    [[nodiscard]] Allocator allocator() const noexcept {
        return std::min(
            AT::max_size(m_allocator), std::numeric_limits<size_type>::max());
    }

    // =========================================================================
    // Access
    // =========================================================================

    reference operator[](size_type i) {
        return const_cast<reference>(
            static_cast<const DynamicArray&>(*this).impl_bound_checked_at(i));
    }
    const_reference operator[](size_type i) const {
        return impl_bound_checked_at(i);
    }

    reference at(size_type i) {
        return const_cast<reference>(
            static_cast<const DynamicArray&>(*this).impl_bound_checked_at(i));
    }
    const_reference at(size_type i) const { return impl_bound_checked_at(i); }

    reference       unsafe_at(size_type i) noexcept { return m_first[i]; }
    const_reference unsafe_at(size_type i) const noexcept { return m_first[i]; }

    reference front() {
        return const_cast<reference>(
            static_cast<const DynamicArray&>(*this).impl_bound_checked_front());
    }
    const_reference front() const { return impl_bound_checked_front(); }

    reference       unsafe_front() noexcept { return *m_first; }
    const_reference unsafe_front() const noexcept { return *m_first; }

    reference back() {
        return const_cast<reference>(
            static_cast<const DynamicArray&>(*this).impl_bound_checked_back());
    }
    const_reference back() const { return impl_bound_checked_back(); }

    reference       unsafe_back() noexcept { return m_last[-1]; }
    const_reference unsafe_back() const noexcept { return m_last[-1]; }

    pointer       raw_data() noexcept { return m_first; }
    const_pointer raw_data() const noexcept { return m_first; }

    // =========================================================================
    // Modifiers
    // =========================================================================

    // =========================================================================
    // Impl
    // =========================================================================
private:
    inline const_reference impl_bound_checked_at(size_type i) const {
        if (i >= size()) [[unlikely]] {
            throw std::out_of_range("DynamicArray => Index out of range.");
        }

        return m_first[i];
    }

    inline const_reference impl_bound_checked_front() const {
        if (is_empty()) [[unlikely]] {
            throw std::out_of_range("DynamicArray => Array is empty.");
        }

        return *m_first;
    }

    inline const_reference impl_bound_checked_back() const {
        if (is_empty()) [[unlikely]] {
            throw std::out_of_range("DynamicArray => Array is empty.");
        }

        return m_last[-1];
    }

    template <typename... Args>
    void impl_construct_item(pointer p, Args&&... args) noexcept(
        std::is_nothrow_constructible_v<T, Args...>) {
        AT::construct(m_allocator, p, std::forward<Args>(args)...);
    }

    void impl_init_range(pointer first, pointer last) noexcept(
        std::is_nothrow_default_constructible_v<T>
        || std::is_trivially_default_constructible_v<T>) {
        if constexpr (std::is_trivially_default_constructible_v<T>) {
            std::memset(first, 0, std::distance(first, last) * sizeof(T));
        } else {
            if constexpr (
                std::is_nothrow_default_constructible_v<T>
                || std::is_trivially_destructible_v<T>) {
                for (; first != last; ++first) {
                    AT::construct(m_allocator, first);
                }
            } else {
                pointer    current = first;
                ScopeGuard guard([&]() {
                    for (pointer p = first; p != current; ++p) {
                        AT::destroy(m_allocator, p);
                    }
                });

                for (; current != last; ++current) {
                    AT::construct(m_allocator, current);
                }

                guard.dismiss();
            }
        }
    }

    void
    impl_move_range(pointer src_first, pointer src_last, pointer dest) noexcept(
        std::is_nothrow_move_constructible_v<T>) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(
                dest,
                std::to_address(src_first),
                std::distance(src_first, src_last) * sizeof(T));
        } else {
            std::uninitialized_move(src_first, src_last, dest);
        }
    }

    // Optimization of std::uninilialized_copy for trivially copyable
    // types is not guaranteed by the standard
    // https://en.cppreference.com/w/cpp/memory/uninitialized_copy.html
    // The optimization is implemented manually
    template <std::input_iterator It>
    void impl_copy_range_iterator(
        pointer src_first,
        pointer src_last,
        pointer dest) noexcept(std::is_nothrow_copy_constructible_v<T>) {
        if constexpr (
            std::is_trivially_copyable_v<T> && std::contiguous_iterator<It>
            && std::
                is_same_v<typename std::iterator_traits<It>::value_type, T>) {
            std::memcpy(
                dest,
                std::to_address(src_first),
                std::distance(src_first, src_last) * sizeof(T));

        } else {
            std::uninitialized_copy(src_first, src_last, dest);
        }
    }

    // Optimization of std::uninilialized_copy for trivially copyable
    // types is not guaranteed by the standard
    // https://en.cppreference.com/w/cpp/memory/uninitialized_copy.html
    // The optimization is implemented manually
    void impl_copy_range_raw(
        pointer src_first,
        pointer src_last,
        pointer dest) noexcept(std::is_nothrow_copy_constructible_v<T>) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(
                dest,
                src_first,
                std::distance(src_first, src_last) * sizeof(T));

        } else {
            std::uninitialized_copy(src_first, src_last, dest);
        }
    }

    inline size_type m_calc_growth() noexcept {
        return capacity() == 0 ?
                   2 :
                   (capacity() * sizeof(T) <= 0x1000 ? capacity() * 2 :
                                                       capacity() * 3 / 2);
    }

    inline void impl_swap(DynamicArray& other) noexcept {
        std::swap(m_first, other.m_first);
        std::swap(m_last, other.m_last);
        std::swap(m_capacity, other.m_capacity);
    }
};
}
