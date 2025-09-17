#pragma once

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
template <class T, class Allocator = std::allocator<T>> class DynamicArray {
private:
  using AT = std::allocator_traits<Allocator>;

public:
  using value_type = T;
  using reference = T &;
  using const_reference = const T &;
  using size_type = size_t;
  using difference_type = std::make_signed<size_t>::type;
  using allocator_type = Allocator;
  using pointer = typename AT::pointer;
  using const_pointer = typename AT::const_pointer;

  using iterator = T *;
  using const_iterator = const T *;
  using reverse_iterator = std::reverse_iterator<T *>;
  using const_reverse_iterator = std::reverse_iterator<const T *>;

private:
  Allocator m_alloc;
  pointer m_first, m_last, m_capacity;

public:
  DynamicArray()
      : m_alloc(Allocator()), m_first(nullptr), m_last(nullptr),
        m_capacity(nullptr) {};

  ~DynamicArray() {
    if (m_first != nullptr) [[likely]] {
      m_destroy_range(m_first, m_last);
      m_deallocate(m_first, m_capacity);
    }
  }

  explicit DynamicArray(const Allocator &alloc) : m_alloc(alloc) {}
  DynamicArray(size_type size, const Allocator &alloc = Allocator())
      : m_alloc(alloc) {
    resize(size);
  }

  DynamicArray(std::initializer_list<T> il,
               const Allocator &alloc = Allocator())
      : DynamicArray(il.begin(), il.end(), alloc) {}

  template <class It, class Category =
                          typename std::iterator_traits<It>::iterator_category>
  DynamicArray(It first, It last, const Allocator &alloc = Allocator())
      : DynamicArray(first, last, alloc, Category()) {}

  template <class ForwardIterator>
  DynamicArray(ForwardIterator first, ForwardIterator last,
               const Allocator &alloc, std::forward_iterator_tag)
      : m_alloc(alloc) {
    resize(std::distance(first, last));
    m_copy_range(first, last, m_first);
  }

  template <class InputIterator>
  DynamicArray(InputIterator first, InputIterator last, const Allocator &alloc,
               std::input_iterator_tag)
      : m_alloc(alloc) {
    for (; first != last; ++first) {
      emplace_back(*first);
    }
  }

  DynamicArray(const DynamicArray &other)
      : m_alloc(AT::select_on_copy_container_construction(other.m_alloc)) {
    resize(size_type(other.m_last - other.m_first));
    m_copy_range(other.m_first, other.m_last, m_first);
  }

  DynamicArray(const DynamicArray &other, const Allocator &alloc)
      : DynamicArray(other.begin(), other.end(), alloc) {}

  DynamicArray(DynamicArray &&other) noexcept
      : m_alloc(std::move(other.m_alloc)), m_first(other.m_first),
        m_last(other.m_last), m_capacity(other.m_capacity) {
    other.m_first = nullptr;
    other.m_last = nullptr;
    other.m_capacity = nullptr;
  }

  DynamicArray(DynamicArray &&other, const Allocator &alloc) : m_alloc(alloc) {
    if (m_alloc == other.m_alloc) {
      s_swap(*this, other);
    } else {
      resize(other.size());
      m_move_range(other.begin(), other.end(), m_first);
    }
  };

public:
  iterator begin() noexcept { return m_first; }
  const_iterator begin() const noexcept { return m_first; }
  iterator end() noexcept { return m_last; }
  const_iterator end() const noexcept { return m_last; }

  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }
  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }

  const_iterator cbegin() const noexcept { return m_first; }
  const_iterator cend() const noexcept { return m_last; }
  const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator(end());
  }
  const_reverse_iterator crend() const noexcept {
    return const_reverse_iterator(begin());
  }

  reference operator[](size_type idx) { return m_first[idx]; }
  const_reference operator[](size_type idx) const { return m_first[idx]; }

  const_reference at(size_type idx) const {
    if (idx >= size()) [[unlikely]] {
      throw std::out_of_range("DynamicArray => Index out of range.");
    }

    return m_first[idx];
  }

  reference at(size_type idx) {
    auto const &const_this = *this;
    return const_cast<reference>(const_this.at(idx));
  }

  const_reference front() const {
    if (is_empty()) [[unlikely]] {
      throw std::out_of_range("DynamicArray => Array is empty.");
    }

    return *m_first;
  }

  reference front() {
    auto const &const_this = *this;
    return const_cast<reference>(const_this.front());
  }

  const_reference back() const {
    if (is_empty()) [[unlikely]] {
      throw std::out_of_range("DynamicArray => Array is empty.");
    }

    return m_last[-1];
  }

  reference back() {
    auto const &const_this = *this;
    return const_cast<reference>(const_this.back());
  }

  pointer data() noexcept { return m_first; }
  const_pointer data() const noexcept { return m_first; }

  size_type size() const noexcept { return size_type(m_last - m_first); }
  size_type max_size() const noexcept {
    return std::numeric_limits<size_type>::max();
  }

  size_type capacity() const noexcept {
    return size_type(m_capacity - m_first);
  }

  bool is_empty() const noexcept { return m_first == m_last; }

  void reserve(size_type n) { resize(size() + n); }

  void resize(size_type n) {
    if (n <= size()) {
      return;
    }

    pointer new_begin = m_allocate(n);
    pointer new_end = new_begin + size();
    pointer new_capacity = new_begin + n;

    if (m_first != nullptr) [[likely]] {
      m_move_range(m_first, m_last, new_begin);
      m_deallocate(m_first, m_capacity);
    }

    m_first = new_begin;
    m_last = new_end;
    m_capacity = new_capacity;
  }

  template <class... Args> reference emplace_back(Args &&...args) {
    if (m_last == m_capacity) {
      resize(m_calc_growth());
    }

    m_construct_item(m_last, std::forward<Args>(args)...);
    ++m_last;
    return back();
  }

  void push_back(const T &item) {
    if (m_last == m_capacity) {
      resize(m_calc_growth());
    }

    m_construct_item(m_last, item);
    ++m_last;
  }

  void push_back(T &&item) {
    if (m_last == m_capacity) {
      resize(m_calc_growth());
    }

    m_construct_item(m_last, std::move(item));
    ++m_last;
  }

private:
  pointer m_allocate(size_type n) { return AT::allocate(m_alloc, n); }

  void m_deallocate(pointer first, pointer last) {
    AT::deallocate(m_alloc, first, size_type(last - first));
  }

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

  void m_destroy_item(pointer item) noexcept {
    if constexpr (!std::is_trivially_destructible_v<T>) {
      AT::destroy(m_alloc, item);
    }
  }

  template <class... Args> void m_construct_item(pointer ptr, Args &&...args) {
    AT::construct(m_alloc, ptr, std::forward<Args>(args)...);
  }

  void m_move_range(pointer src_first, pointer src_last, pointer dest) {
    if constexpr (std::is_trivially_copyable_v<T>) {
      std::memcpy(dest, src_first, size_type(src_last - src_first) * sizeof(T));
    } else {
      std::uninitialized_move(src_first, src_last, dest);
    }
  };

  void m_copy_range(pointer src_first, pointer src_last, pointer dest) {
    if constexpr (std::is_trivially_copyable_v<T>) {
      std::memcpy(dest, src_first, size_type(src_last - src_first) * sizeof(T));
    } else {
      std::uninitialized_copy(src_first, src_last, dest);
    }
  }

  static void s_swap(DynamicArray &a, DynamicArray &b) noexcept {
    std::swap(a.m_first, b.m_first);
    std::swap(a.m_last, b.m_last);
    std::swap(a.m_capacity, b.m_capacity);
  }

  size_type m_calc_growth() {
    if (capacity() == 0) {
      return 1;
    } else if (capacity() <= 1024) {
      return capacity() * 2;
    } else {
      return capacity() * 3 / 2;
    }
  }
};

} // namespace core
