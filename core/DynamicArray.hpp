// Copyright 2025 Jakub Kijek
// Licensed under the MIT License.
// See LICENSE.md file in the project root for full license information.

#include "TypeTraits.hpp"
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <limits>
#include <memory>
#include <stdexcept>

namespace core {
template <class T, class Allocator = std::allocator<T>>
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

    template <class It>
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
};
}
