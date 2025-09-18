// Copyright 2025 Jakub Kijek
// Licensed under the MIT License.
// See LICENSE.md file in the project root for full license information.

#pragma once

#include "./ScopeGuard.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace core {
template <class T, class Allocator = std::allocator<T>>
class DynamicArray {
private:
    using AT = std::allocator_traits<Allocator>;

public:
    using value_type      = T;
    using reference       = T&;
    using const_reference = const T&;
    using size_type       = size_t;
    using difference_type = std::make_signed<size_t>::type;
    using allocator_type  = Allocator;
    using pointer         = typename AT::pointer;
    using const_pointer   = typename AT::const_pointer;

    using iterator               = T*;
    using const_iterator         = const T*;
    using reverse_iterator       = std::reverse_iterator<T*>;
    using const_reverse_iterator = std::reverse_iterator<const T*>;

private:
    Allocator m_alloc;
    pointer   m_first, m_last, m_capacity;

public:
    // =========================================================================
    // Constructor
    // =========================================================================

    DynamicArray()
        : m_alloc(Allocator())
        , m_first(nullptr)
        , m_last(nullptr)
        , m_capacity(nullptr) { };

    ~DynamicArray() {
        if (m_first != nullptr) [[likely]] {
            m_destroy_range(m_first, m_last);
            m_deallocate(m_first, m_capacity);
        }
    }

    explicit DynamicArray(const Allocator& alloc)
        : m_alloc(alloc) { }
    DynamicArray(size_type size, const Allocator& alloc = Allocator())
        : m_alloc(alloc)
        , m_first(nullptr)
        , m_last(nullptr)
        , m_capacity(nullptr) {
        resize(size);
    }

    DynamicArray(const DynamicArray& other)
        : m_alloc(AT::select_on_copy_container_construction(other.m_alloc))
        , m_first(nullptr)
        , m_last(nullptr)
        , m_capacity(nullptr) {
        resize(std::distance(other.begin(), other.end()));
        m_copy_range(other.begin(), other.end(), m_first);
    }

    DynamicArray(const DynamicArray& other, const Allocator& alloc)
        : DynamicArray(other.begin(), other.end(), alloc) { }

    DynamicArray(DynamicArray&& other) noexcept
        : m_alloc(std::move(other.m_alloc))
        , m_first(other.m_first)
        , m_last(other.m_last)
        , m_capacity(other.m_capacity) {
        other.m_first    = nullptr;
        other.m_last     = nullptr;
        other.m_capacity = nullptr;
    }

    DynamicArray(DynamicArray&& other, const Allocator& alloc)
        : m_alloc(alloc)
        , m_first(nullptr)
        , m_last(nullptr)
        , m_capacity(nullptr) {
        if (m_alloc == other.m_alloc) {
            s_swap(*this, other);
        } else {
            resize(other.size());
            m_move_range(other.begin(), other.end(), m_first);
        }
    };

    DynamicArray(
        std::initializer_list<T> il, const Allocator& alloc = Allocator())
        : DynamicArray(il.begin(), il.end(), alloc) { }

    template <class It>
    DynamicArray(It first, It last, const Allocator& alloc = Allocator())
        : m_alloc(alloc)
        , m_first(nullptr)
        , m_last(nullptr)
        , m_capacity(nullptr) {
        assign(first, last);
    }

    // =========================================================================
    // Operators
    // =========================================================================

    DynamicArray& operator=(const DynamicArray& other) { }

    DynamicArray& operator=(DynamicArray&& other) { }

public:
    // =========================================================================
    // Iterator
    // =========================================================================

    iterator       begin() noexcept { return m_first; }
    const_iterator begin() const noexcept { return m_first; }
    iterator       end() noexcept { return m_last; }
    const_iterator end() const noexcept { return m_last; }

    reverse_iterator       rbegin() noexcept { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    reverse_iterator       rend() noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }

    const_iterator         cbegin() const noexcept { return m_first; }
    const_iterator         cend() const noexcept { return m_last; }
    const_reverse_iterator crbegin() const noexcept {
        return const_reverse_iterator(end());
    }
    const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator(begin());
    }

    // =========================================================================
    // Access
    // =========================================================================

    reference       operator[](size_type idx) { return m_first[idx]; }
    const_reference operator[](size_type idx) const { return m_first[idx]; }

    const_reference at(size_type idx) const {
        if (idx >= size()) [[unlikely]] {
            throw std::out_of_range("DynamicArray => Index out of range.");
        }

        return m_first[idx];
    }

    reference at(size_type idx) {
        auto const& const_this = *this;
        return const_cast<reference>(const_this.at(idx));
    }

    const_reference front() const {
        if (is_empty()) [[unlikely]] {
            throw std::out_of_range("DynamicArray => Array is empty.");
        }

        return *m_first;
    }

    reference front() {
        auto const& const_this = *this;
        return const_cast<reference>(const_this.front());
    }

    const_reference back() const {
        if (is_empty()) [[unlikely]] {
            throw std::out_of_range("DynamicArray => Array is empty.");
        }

        return m_last[-1];
    }

    reference back() {
        auto const& const_this = *this;
        return const_cast<reference>(const_this.back());
    }

    pointer       data() noexcept { return m_first; }
    const_pointer data() const noexcept { return m_first; }

    // =========================================================================
    // Info
    // =========================================================================

    size_type size() const noexcept { return size_type(m_last - m_first); }

    size_type max_size() const noexcept {
        return std::numeric_limits<size_type>::max();
    }

    size_type capacity() const noexcept {
        return size_type(m_capacity - m_first);
    }

    bool is_empty() const noexcept { return m_first == m_last; }

    // =========================================================================
    // Modifiers
    // =========================================================================

    template <class... Args>
    reference emplace_back(Args&&... args) {
        if (m_last == m_capacity) {
            resize(m_calc_growth());
        }

        m_construct_item(m_last, std::forward<Args>(args)...);
        ++m_last;
        return back();
    }

    void push_back(const T& item) {
        if (m_last == m_capacity) {
            resize(m_calc_growth());
        }

        m_construct_item(m_last, item);
        ++m_last;
    }

    void push_back(T&& item) {
        if (m_last == m_capacity) {
            resize(m_calc_growth());
        }

        m_construct_item(m_last, std::move(item));
        ++m_last;
    }

    void pop_back() {
        if (m_first == nullptr) [[unlikely]] {
            throw std::out_of_range(
                "DynamicArray => Cannot pop an item if the "
                "DynamicArray is empty.");
        }

        --m_last;
        m_destroy_item(m_last);
    }

    template <
        class It,
        class Category = typename std::iterator_traits<It>::iterator_category>
    void assign(It first, It last) {
        size_type new_size = std::distance(first, last);
        clear();

        if (new_size > size()) {
            pointer new_first    = m_allocate(new_size);
            pointer new_last     = new_first;
            pointer new_capacity = new_first + new_size;

            if (m_first != nullptr) [[likely]] {
                m_deallocate(m_first, m_capacity);
            }

            m_first    = new_first;
            m_last     = new_last;
            m_capacity = new_capacity;
        }

        assign(first, last, Category());
    }

private:
    template <class ForwardIterator>
    void assign(
        ForwardIterator first,
        ForwardIterator last,
        std::forward_iterator_tag) {
        m_copy_range_any(first, last, m_first);
        m_last = m_capacity;
    }

    template <class InputIterator>
    void
    assign(InputIterator first, InputIterator last, std::input_iterator_tag) {
        for (; first != last; ++first) {
            emplace_back(*first);
        }
    }

public:
    void clear() {
        m_destroy_range(m_first, m_last);
        m_last = m_first;
    }

    void reserve(size_type n) {
        if (size() + n > capacity()) {
            resize(size() + n);
        }
    }

    void resize(size_type n) {
        if (n == 0) [[unlikely]] {
            clear();
            m_deallocate(m_first, m_capacity);

            m_first    = nullptr;
            m_last     = nullptr;
            m_capacity = nullptr;

            return;
        }

        if (n == size()) [[unlikely]] {
            return;
        }

        pointer new_first    = m_allocate(n);
        pointer new_last     = n > size() ? new_first + size() : new_first + n;
        pointer new_capacity = new_first + n;

        auto guard
            = MakeScopeGuard([&]() { m_deallocate(new_first, new_capacity); });

        if (n > size()) {
            if (m_first != nullptr) [[likely]] {
                m_move_range(m_first, m_last, new_first);
                m_deallocate(m_first, m_capacity);
            }
        } else {
            if (m_first != nullptr) [[likely]] {
                m_destroy_range(m_first + n, m_last);
                m_move_range(m_first, m_first + n, new_first);
                m_deallocate(m_first, m_capacity);
            }
        }

        m_first    = new_first;
        m_last     = new_last;
        m_capacity = new_capacity;
        guard.dismiss();
    }

private:
    // =========================================================================
    // Helpers
    // =========================================================================

    // Allocation can always throw
    pointer m_allocate(size_type n) { return AT::allocate(m_alloc, n); }

    // Deallocation MUST not throw
    // https://en.cppreference.com/w/cpp/named_req/Allocator
    // =========================================================================
    void m_deallocate(pointer first, pointer last) noexcept {
        AT::deallocate(m_alloc, first, std::distance(first, last));
    }

    // Assuming the destructor of T does not throw
    // Same as STL
    void m_destroy_range(pointer first, pointer last) noexcept {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (; last - first >= 4; first += 4) {
                AT::destroy(m_alloc, first);
                AT::destroy(m_alloc, first + 1);
                AT::destroy(m_alloc, first + 2);
                AT::destroy(m_alloc, first + 3);
            }

            for (; first != last; ++first) {
                AT::destroy(m_alloc, first);
            }
        }
    }

    // Assuming the destructor of T does not throw
    // Same as STL
    void m_destroy_item(pointer item) noexcept {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            AT::destroy(m_alloc, item);
        }
    }

    // Conidionally does not throw
    void m_init_range(pointer first, pointer last) noexcept(
        std::is_nothrow_default_constructible_v<T>) {

        // Initializing to an empty memory block is only possible if T
        // is_trivially_copyable
        // https://en.cppreference.com/w/cpp/string/byte/memset.html
        // =================================================================
        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memset(first, 0, std::distance(first, last) * sizeof(T));
        } else {
            if constexpr (std::is_nothrow_default_constructible_v<T>) {
                for (; first != last; ++first) {
                    AT::construct(m_alloc, first);
                }
            } else {
                pointer current = first;

                try {
                    for (; current != last; ++current) {
                        AT::construct(m_alloc, current);
                    }
                } catch (...) {
                    if constexpr (!std::is_trivially_destructible_v<T>) {
                        for (pointer ptr = first; ptr != current; ++ptr) {
                            AT::destroy(m_alloc, ptr);
                        }
                    }

                    throw;
                }
            }
        }
    }

    // Coditionally does not throw
    template <class... Args>
    void m_construct_item(pointer ptr, Args&&... args) noexcept(
        std::is_nothrow_constructible_v<T>) {
        AT::construct(m_alloc, ptr, std::forward<Args>(args)...);
    }

    // Coditionally does not throw
    void
    m_move_range(pointer src_first, pointer src_last, pointer dest) noexcept(
        std::is_nothrow_move_constructible_v<T>) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(
                dest,
                src_first,
                std::distance(src_first, src_last) * sizeof(T));
        } else {
            std::uninitialized_move(src_first, src_last, dest);
        }
    };

    // Coditionally does not throw
    template <class It>
    void m_copy_range_any(It src_first, It src_last, pointer dest) noexcept(
        std::is_nothrow_copy_constructible_v<T>) {
        if constexpr (
            std::is_trivially_copyable_v<T>
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

    // Coditionally does not throw
    void
    m_copy_range(pointer src_first, pointer src_last, pointer dest) noexcept(
        std::is_nothrow_copy_constructible_v<T>) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(
                dest,
                src_first,
                std::distance(src_first, src_last) * sizeof(T));
        } else {
            std::uninitialized_copy(src_first, src_last, dest);
        }
    }

    size_type m_calc_growth() noexcept {
        if (capacity() == 0) {
            return 1;
        } else if (capacity() <= 1024) {
            return capacity() * 2;
        } else {
            return capacity() * 3 / 2;
        }
    }

    static void s_swap(DynamicArray& a, DynamicArray& b) noexcept {
        std::swap(a.m_first, b.m_first);
        std::swap(a.m_last, b.m_last);
        std::swap(a.m_capacity, b.m_capacity);
    }
};

} // namespace core
