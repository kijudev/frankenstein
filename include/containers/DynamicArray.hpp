// Copyright 2025 Jakub Kijek
// Licensed under the MIT License.
// See LICENSE.md file in the project root for full license information.

#include <algorithm>
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

#include "../utils/ScopeGuard.hpp"

namespace frank {
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
    Allocator m_allocator {Allocator()};
    pointer   m_first {nullptr}, m_last {nullptr}, m_capacity {nullptr};

    // =========================================================================
    // Destructor, constructors, assigment operators, lexical operators
    // =========================================================================
public:
    ~DynamicArray() {
        impl_destroy_range(m_first, m_last);
        impl_deallocate(m_first, m_capacity);
    }

    DynamicArray() noexcept = default;

    explicit DynamicArray(const Allocator& a) noexcept
        : m_allocator(a) { }

    explicit DynamicArray(size_type sz, const Allocator& a = Allocator())
        : m_allocator(a) {
        if (sz == 0) [[unlikely]] {
            return;
        }

        m_first    = impl_allocate(sz);
        m_last     = m_first;
        m_capacity = m_first + sz;
    }

    DynamicArray(const DynamicArray& other)
        : DynamicArray(
              other,
              AT::select_on_container_copy_construction(other.m_allocator)) { }

    DynamicArray(const DynamicArray& other, const Allocator& a)
        : m_allocator(a) {
        if (other.is_empty()) [[unlikely]] {
            return;
        }

        size_type sz = other.size();
        m_first      = impl_allocate(sz);
        m_last       = m_first + sz;
        m_capacity   = m_first + sz;

        impl_copy_range_raw(other.m_first, other.m_last, m_first);
    }

    DynamicArray(DynamicArray&& other) noexcept
        : m_allocator(std::move(other.m_allocator))
        , m_first(other.m_first)
        , m_last(other.m_last)
        , m_capacity(other.m_capacity) {
        other.m_first    = nullptr;
        other.m_last     = nullptr;
        other.m_capacity = nullptr;
    }

    DynamicArray(DynamicArray&& other, const Allocator& a) noexcept
        : m_allocator(a)
        , m_first(other.m_first)
        , m_last(other.m_last)
        , m_capacity(other.m_capacity) {
        other.m_first    = nullptr;
        other.m_last     = nullptr;
        other.m_capacity = nullptr;
    }

    DynamicArray(std::initializer_list<T> il, const Allocator& a = Allocator())
        : DynamicArray(il.begin(), il.end(), a) { }

    template <std::input_iterator It>
    DynamicArray(It first, It last, const Allocator& a = Allocator())
        : m_allocator(a) {
        assign_range(first, last);
    }

    DynamicArray& operator=(const DynamicArray& other) {
        if (*this == other) [[unlikely]] {
            return *this;
        }

        DynamicArray temp(other);
        swap(temp);
        return *this;
    }

    DynamicArray& operator=(DynamicArray&& other) noexcept(
        std::is_nothrow_destructible_v<T>
        && std::is_nothrow_move_assignable_v<T>) {
        if (this == &other) {
            return *this;
        }

        impl_destroy_range(m_first, m_last);
        impl_deallocate(m_first, m_capacity);

        m_allocator = std::move(other.m_allocator);
        m_first     = other.m_first;
        m_last      = other.m_last;
        m_capacity  = other.m_capacity;

        other.m_capacity = nullptr;
        other.m_first    = nullptr;
        other.m_last     = nullptr;

        return *this;
    }

    bool operator==(const DynamicArray& other) const {
        if (m_first == other.m_first) {
            return true;
        };

        if (size() != other.size()) {
            return false;
        }

        if (is_empty() && other.is_empty()) {
            return true;
        }

        if constexpr (std::equality_comparable<T>) {
            for (size_type i = 0; i < size(); ++i) {
                if (m_first[i] != other.m_first[i]) {
                    return false;
                }
            }
        }

        return true;
    }

    bool operator!=(const DynamicArray& other) const {
        if (m_first == other.m_first) {
            return false;
        }

        if (size() != other.size()) {
            return true;
        }

        if (is_empty() && other.is_empty()) {
            return false;
        }

        if constexpr (std::equality_comparable<T>) {
            for (size_type i = 0; i < size(); ++i) {
                if (m_first[i] == other.m_first[i]) {
                    return false;
                }
            }
        }

        return true;
    }

    // =========================================================================
    // Iterator
    // =========================================================================
public:
    iterator       begin() noexcept { return m_first; }
    const_iterator begin() const noexcept { return m_first; }
    iterator       end() noexcept { return m_last; }
    const_iterator end() const noexcept { return m_last; }

    reverse_iterator rbegin() noexcept { return reverse_iterator(m_last); }
    const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(m_last);
    }

    reverse_iterator       rend() noexcept { return reverse_iterator(m_first); }
    const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(m_first);
    }

    const_iterator         cbegin() const noexcept { return m_first; }
    const_iterator         cend() const noexcept { return m_last; }
    const_reverse_iterator crbegin() const noexcept {
        return const_reverse_iterator(m_last);
    }
    const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator(m_first);
    }

    // =========================================================================
    // Info
    // =========================================================================
public:
    [[nodiscard]] inline bool is_empty() const noexcept { return size() == 0; }

    [[nodiscard]] inline bool is_full() const noexcept {
        return size() == capacity();
    }

    [[nodiscard]] inline size_type capacity() const noexcept {
        return std::distance(m_first, m_capacity);
    }

    [[nodiscard]] inline constexpr size_type max_capacity() const noexcept {
        return max_size();
    }

    [[nodiscard]] inline size_type size() const noexcept {
        return std::distance(m_first, m_last);
    }

    [[nodiscard]] inline constexpr size_type max_size() const noexcept {
        if constexpr (utils::HasMaxSize<Allocator>) {
            return AT::max_size();
        } else {
            return std::numeric_limits<size_type>::max();
        }
    }

    [[nodiscard]] Allocator allocator() const noexcept { return m_allocator; }

    // =========================================================================
    // Access
    // =========================================================================
public:
    reference       operator[](size_type i) { return m_first[i]; }
    const_reference operator[](size_type i) const { return m_first[i]; }

    reference at(size_type i) {
        impl_index_bound_check(i);
        return m_first[i];
    }
    const_reference at(size_type i) const {
        impl_index_bound_check(i);
        return m_first[i];
    }

    reference front() {
        impl_empty_check();
        return *m_first;
    }
    const_reference front() const {
        impl_empty_check();
        return *m_first;
    }

    reference back() {
        impl_empty_check();
        return m_last[-1];
    }

    const_reference back() const {
        impl_empty_check();
        return m_last[-1];
    }

    // =========================================================================
    // Modifiers
    // =========================================================================
public:
    template <typename... Args>
    reference emplace_back(Args&&... args) {
        if (is_full()) {
            grow(impl_calc_growth());
        }

        impl_construct_item(m_last, std::forward<Args>(args)...);
        ++m_last;

        return m_last[-1];
    }

    void push_back(const T& item) { emplace_back(item); }
    void push_back(T&& item) { emplace_back(std::move(item)); }

    template <typename... Args>
    iterator emplace(const_iterator pos, Args&&... args) {
        impl_iterator_bound_check(pos);
        if (is_full()) {
            grow(impl_calc_growth());
        }

        impl_move_segment_up(pos, m_last, 1);
        impl_construct_item(pos, std::forward<Args>(args)...);

        ++m_last;
        return const_cast<iterator>(pos);
    }

    iterator insert_item(const_iterator pos, const T& item) {
        return emplace(pos, item);
    }

    iterator insert_item(const_iterator pos, T&& item) {
        return emplace(pos, std::move(item));
    }

    iterator insert_fill(const_iterator pos, size_type count, const T& item) {
        impl_iterator_bound_check(pos);
        if (count == 0) [[unlikely]] {
            return const_cast<iterator>(pos);
        }

        if (size() + count > capacity()) {
            grow(size() + count);
        }

        if (pos != end()) [[likely]] {
            impl_move_segment_up(pos, m_last, count);
        }

        for (size_type i = count; i != 0; --i) {
            impl_construct_item(pos + i - 1, item);
        }

        m_last += count;
        return const_cast<iterator>(pos);
    }

    template <std::input_iterator It>
    iterator insert_range(const_iterator pos, It first, It last) {
        impl_iterator_bound_check(pos);
        if (first == last) [[unlikely]] {
            return const_cast<iterator>(pos);
        }

        size_type count = std::distance(first, last);

        // TODO
        // Can opimize to reduce the number of long jump memmove
        if (size() + count > capacity()) {
            grow(size() + count);
        }

        if (pos != end()) [[likely]] {
            impl_move_segment_up(pos, m_last, count);
        }

        impl_copy_range_iterator<It>(first, last, pos);

        m_last += count;
        return static_cast<iterator>(pos);
    }

    iterator insert_range(const_iterator pos, std::initializer_list<T> il) {
        return insert_range(pos, il.begin(), il.end());
    }

    void assign_fill(size_type count, const T& item) {
        if (count == 0) [[unlikely]] {
            return;
        }

        if (count > capacity()) {
            pointer new_first    = impl_allocate(count);
            pointer new_last     = new_first;
            pointer new_capacity = new_first + count;

            if constexpr (std::is_nothrow_constructible_v<T, const T&>) {
                for (; new_last != new_capacity; ++new_last) {
                    impl_construct_item(new_last, item);
                }
            } else {
                utils::ScopeGuard guard([&]() {
                    impl_destroy_range(new_first, new_last);
                    impl_deallocate(new_first, new_capacity);
                });

                for (; new_last != new_capacity; ++new_last) {
                    impl_construct_item(new_last, item);
                }

                guard.dismiss();
            }

            std::swap(m_first, new_first);
            std::swap(m_last, new_last);
            std::swap(m_capacity, new_capacity);

            if (new_first != nullptr) [[likely]] {
                impl_destroy_range(new_first, new_capacity);
                impl_deallocate(new_first, new_capacity);
            }

            return;
        }

        impl_destroy_range(m_first, m_last);
        m_last = m_first;

        if constexpr (std::is_nothrow_constructible_v<T, const T&>) {
            for (; m_last != m_first + count; ++m_last) {
                impl_construct_item(m_last, item);
            }
        } else {
            utils::ScopeGuard guard(
                [&]() { impl_destroy_range(m_first, m_last); });

            for (; m_last != m_first + count; ++m_last) {
                impl_construct_item(m_last, item);
            }

            guard.dismiss();
        }
    }

    template <std::input_iterator It>
    void assign_range(It first, It last) {
        if (first == last) [[unlikely]] {
            impl_destroy_range(m_first, m_last);
            m_last = m_first;
            return;
        }

        size_type count = std::distance(first, last);

        if (count > capacity()) {
            pointer new_first    = impl_allocate(count);
            pointer new_last     = new_first + count;
            pointer new_capacity = new_first + count;

            if constexpr (std::is_nothrow_copy_constructible_v<T>) {
                impl_copy_range_iterator<It>(first, last, new_first);
            } else {
                utils::ScopeGuard guard(
                    [&]() { impl_deallocate(new_first, new_last); });

                impl_copy_range_iterator<It>(first, last, new_first);

                guard.dismiss();
            }

            std::swap(m_first, new_first);
            std::swap(m_last, new_last);
            std::swap(m_capacity, new_capacity);

            if (new_first != nullptr) [[likely]] {
                impl_destroy_range(new_first, new_capacity);
                impl_deallocate(new_first, new_capacity);
            }

            return;
        }

        impl_destroy_range(m_first, m_last);
        m_last = m_first;

        impl_copy_range_iterator<It>(first, last, m_first);
        m_last += count;
        return;
    }

    void pop_back() {
        impl_empty_check();

        --m_last;
        impl_destroy_item(m_last);
    }

    iterator erase_item(const_iterator pos) noexcept(
        std::is_nothrow_destructible_v<T>
        && std::is_nothrow_move_constructible_v<T>) {
        impl_iterator_bound_check(pos);

        if (pos == end()) [[unlikely]] {
            --m_last;
            impl_destroy_item(m_last);
            return end();
        }

        impl_destroy_item(pos);
        impl_move_segment_down(pos + 1, m_last, 1);

        --m_last;
        return end();
    }

    iterator erase_range(const_iterator first, const_iterator last) noexcept(
        std::is_nothrow_destructible_v<T>
        && std::is_nothrow_move_constructible_v<T>) {
        impl_iterator_bound_check(first);
        impl_iterator_bound_check(last);

        if (first == last) [[unlikely]] {
            return const_cast<iterator>(first);
        }

        size_type count = std::distance(first, last);

        impl_destroy_range(first, last);
        impl_move_segment_down(last + 1, m_last, count);

        m_last -= count;
        return const_cast<iterator>(first);
    }

    void reserve(size_type sz) {
        if (size() + sz > capacity()) {
            grow(size() + sz);
        }
    }

    void shrink_fit() {
        if (is_full()) [[unlikely]] {
            return;
        }

        pointer new_first    = impl_allocate(size());
        pointer new_last     = new_first + size();
        pointer new_capacity = new_first + size();

        if constexpr (std::is_nothrow_copy_constructible_v<T>) {
            impl_move_range_raw(m_first, m_last, new_first);
        } else {
            utils::ScopeGuard guard(
                [&]() { impl_deallocate(new_first, new_capacity); });

            impl_move_range_raw(m_first, m_last, new_first);

            guard.dismiss();
        }

        std::swap(m_first, new_first);
        std::swap(m_last, new_last);
        std::swap(m_capacity, new_capacity);

        impl_deallocate(new_first, new_capacity);
    }

    void shrink_lossy(size_type sz) {
        if (sz >= size()) [[unlikely]] {
            throw std::out_of_range(
                "DynamicArray => Cannot shrink to a bigger or equal size.");
        }

        if (sz == 0) [[unlikely]] {
            impl_destroy_range(m_first, m_last);
            impl_deallocate(m_first, m_capacity);

            m_first    = nullptr;
            m_last     = nullptr;
            m_capacity = nullptr;
            return;
        }

        pointer new_first    = impl_allocate(size());
        pointer new_last     = new_first + sz;
        pointer new_capacity = new_first + sz;

        if constexpr (std::is_nothrow_copy_constructible_v<T>) {
            impl_move_range_raw(m_first, m_first + sz, new_first);
        } else {
            utils::ScopeGuard guard(
                [&]() { impl_deallocate(new_first, new_capacity); });

            impl_move_range_raw(m_first, m_first + sz, new_first);

            guard.dismiss();
        }

        std::swap(m_first, new_first);
        std::swap(m_last, new_last);
        std::swap(m_capacity, new_capacity);

        impl_destroy_range(new_first + sz, new_capacity);
        impl_deallocate(new_first, new_capacity);
    }

    void grow(size_type sz) {
        if (sz <= size()) [[unlikely]] {
            throw std::out_of_range(
                "DynamicArray => Cannot grow to a smaller or equal size.");
        }

        if (m_first == nullptr) {
            m_first    = impl_allocate(sz);
            m_last     = m_first;
            m_capacity = m_first + sz;
            return;
        }

        pointer new_first    = impl_allocate(sz);
        pointer new_last     = new_first + size();
        pointer new_capacity = new_first + sz;

        if constexpr (std::is_nothrow_copy_constructible_v<T>) {
            impl_move_range_raw(m_first, m_first + size(), new_first);
        } else {
            utils::ScopeGuard guard(
                [&]() { impl_deallocate(new_first, new_capacity); });

            impl_move_range_raw(m_first, m_first + size(), new_first);

            guard.dismiss();
        }

        std::swap(m_first, new_first);
        std::swap(m_last, new_last);
        std::swap(m_capacity, new_capacity);

        impl_deallocate(new_first, new_capacity);
    }

    void clear() noexcept(std::is_nothrow_destructible_v<T>) {
        impl_destroy_range(m_first, m_last);
        m_last = m_first;
    }

    void swap(DynamicArray& other) { impl_swap(other); }

    // =========================================================================
    // Impl
    // =========================================================================
private:
    inline void impl_iterator_bound_check(const_iterator it) const {
        if (it < m_first || it > m_last) [[unlikely]] {
            throw std::out_of_range("DynamicArray => Invalid iterator.");
        }
    }

    inline void impl_index_bound_check(size_type i) const {
        if (i >= size()) [[unlikely]] {
            throw std::out_of_range("DynamicArray => Index out of range.");
        }
    }

    inline void impl_empty_check() const {
        if (is_empty()) [[unlikely]] {
            throw std::out_of_range("DynamicArray => Array is empty.");
        }
    }

    void impl_move_segment_up(
        pointer   first,
        pointer   last,
        size_type offset) noexcept(std::is_nothrow_move_constructible_v<T>) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            impl_memmove(first, last, last + offset);
        } else {
            std::move_backward(first, last, last + offset);
        }
    }

    void impl_move_segment_down(
        pointer   first,
        pointer   last,
        size_type offset) noexcept(std::is_nothrow_move_constructible_v<T>) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            impl_memmove(first, last, first - offset);
        } else {
            std::move(first, last, first - offset);
        }
    }

    [[nodiscard]] pointer impl_allocate(size_type sz) {
        return AT::allocate(m_allocator, sz);
    }

    void impl_deallocate(pointer first, pointer last) noexcept {
        AT::deallocate(m_allocator, first, std::distance(first, last));
    }

    void impl_destroy_range(pointer first, pointer last) noexcept(
        std::is_nothrow_destructible_v<T>) {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (; first != last; ++first) {
                AT::destroy(m_allocator, first);
            }
        }
    }

    void
    impl_destroy_item(pointer p) noexcept(std::is_nothrow_destructible_v<T>) {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            AT::destroy(m_allocator, p);
        }
    }

    template <typename... Args>
    void impl_construct_item(pointer p, Args&&... args) noexcept(
        std::is_nothrow_constructible_v<T, Args...>) {
        AT::construct(m_allocator, p, std::forward<Args>(args)...);
    }

    void impl_move_range_raw(
        const_pointer src_first,
        const_pointer src_last,
        pointer       dest) noexcept(std::is_nothrow_move_constructible_v<T>) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            impl_memcpy(src_first, src_last, dest);
        } else {
            std::uninitialized_move(src_first, src_last, dest);
        }
    }

    // Optimization of std::uninilialized_copy for trivially copyable
    // types is not guaranteed by the standard
    // https://en.cppreference.com/w/cpp/memory/uninitialized_copy.html
    // The optimization is implemented manually
    template <std::input_iterator It>
    void
    impl_copy_range_iterator(It src_first, It src_last, pointer dest) noexcept(
        std::is_nothrow_copy_constructible_v<T>) {
        if constexpr (
            std::is_trivially_copyable_v<T> && std::contiguous_iterator<It>
            && std::
                is_same_v<typename std::iterator_traits<It>::value_type, T>) {
            impl_memcpy(src_first, src_last, dest);
        } else {
            std::uninitialized_copy(src_first, src_last, dest);
        }
    }

    // Optimization of std::uninilialized_copy for trivially copyable
    // types is not guaranteed by the standard
    // https://en.cppreference.com/w/cpp/memory/uninitialized_copy.html
    // The optimization is implemented manually
    void impl_copy_range_raw(
        const_pointer src_first,
        const_pointer src_last,
        pointer       dest) noexcept(std::is_nothrow_copy_constructible_v<T>) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            impl_memcpy(src_first, src_last, dest);
        } else {
            std::uninitialized_copy(src_first, src_last, dest);
        }
    }

    inline void impl_memmove(
        const_pointer src_first,
        const_pointer src_last,
        pointer       dest) noexcept {
        std::memmove(
            dest, src_first, std::distance(src_first, src_last) * sizeof(T));
    }

    inline void impl_memcpy(
        const_pointer src_first,
        const_pointer src_last,
        pointer       dest) noexcept {
        std::memcpy(
            dest, src_first, std::distance(src_first, src_last) * sizeof(T));
    }

    inline size_type impl_calc_growth() noexcept {
        return capacity() == 0 ? 1 : capacity() * 2;
    }

    inline void impl_swap(DynamicArray& other) noexcept {
        std::swap(m_first, other.m_first);
        std::swap(m_last, other.m_last);
        std::swap(m_capacity, other.m_capacity);
    }
};
}
