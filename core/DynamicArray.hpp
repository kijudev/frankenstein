#pragma once

#include <algorithm>
#include <cassert>
#include <cstring>
#include <exception>
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
  pointer m_begin, m_end, m_capacity;

public:
  DynamicArray()
      : m_alloc(Allocator()), m_begin(nullptr), m_end(nullptr),
        m_capacity(nullptr) {};

  ~DynamicArray() {
    if (m_begin != nullptr) [[likely]] {
      d_destroy_range(m_begin, m_end);
      d_deallocate(m_begin, m_capacity);
    }
  }

  explicit DynamicArray(const Allocator &alloc) : m_alloc(alloc) {}
  DynamicArray(size_type size, const Allocator &alloc = Allocator())
      : m_alloc(alloc) {
    reserve_exact(size);
  }

  DynamicArray(const DynamicArray &other)
      : m_alloc(AT::select_on_copy_container_construction(other.m_alloc)) {
    reserve_exact(size_type(other.m_end - other.m_begin));
    d_copy_range(other.m_begin, other.m_end, m_begin);
  }

  DynamicArray(DynamicArray &&other) noexcept
      : m_alloc(std::move(other.m_alloc)), m_begin(other.m_begin),
        m_end(other.m_end), m_capacity(other.m_capacity) {
    other.m_begin = nullptr;
    other.m_end = nullptr;
    other.m_capacity = nullptr;
  };

public:
  iterator begin() noexcept { return m_begin; }
  const_iterator begin() const noexcept { return m_begin; }
  iterator end() noexcept { return m_end; }
  const_iterator end() const noexcept { return m_end; }

  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }
  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }

  const_iterator cbegin() const noexcept { return m_begin; }
  const_iterator cend() const noexcept { return m_end; }
  const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator(end());
  }
  const_reverse_iterator crend() const noexcept {
    return const_reverse_iterator(begin());
  }

  reference operator[](size_type idx) { return m_begin[idx]; }
  const_reference operator[](size_type idx) const { return m_begin[idx]; }

  const_reference at(size_type idx) const {
    if (idx >= size()) [[unlikely]] {
      throw std::out_of_range("DynamicArray => Index out of range.");
    }

    return m_begin[idx];
  }

  reference at(size_type idx) {
    auto const &const_this = *this;
    return const_cast<reference>(const_this.at(idx));
  }

  const_reference front() const {
    if (is_empty()) [[unlikely]] {
      throw std::out_of_range("DynamicArray => Array is empty.");
    }

    return *m_begin;
  }

  reference front() {
    auto const &const_this = *this;
    return const_cast<reference>(const_this.front());
  }

  const_reference back() const {
    if (is_empty()) [[unlikely]] {
      throw std::out_of_range("DynamicArray => Array is empty.");
    }

    return m_end[-1];
  }

  reference back() {
    auto const &const_this = *this;
    return const_cast<reference>(const_this.back());
  }

  pointer data() noexcept { return m_begin; }
  const_pointer data() const noexcept { return m_begin; }

  size_type size() const noexcept { return size_type(m_end - m_begin); }
  size_type max_size() const noexcept {
    return std::numeric_limits<size_type>::max();
  }

  size_type capacity() const noexcept {
    return size_type(m_capacity - m_begin);
  }

  bool is_empty() const noexcept { return m_begin == m_end; }

  size_type reserve_aligned(size_type min_capacity) {
    // TODO
    reserve_exact(min_capacity);
    return min_capacity;
  }

  void reserve_exact(size_type target_capacity) {
    if (target_capacity <= size()) {
      return;
    }

    pointer new_begin = d_allocate(target_capacity);
    pointer new_end = new_begin + size();
    pointer new_capacity = new_begin + target_capacity;

    if (m_begin != nullptr) [[likely]] {
      d_move_range(m_begin, m_end, new_begin);
      d_deallocate(m_begin, m_capacity);
    }

    m_begin = new_begin;
    m_end = new_end;
    m_capacity = new_capacity;
  }

  template <class... Args> reference emplace_back(Args &&...args) {
    if (m_end == m_capacity) {
      reserve_exact(m_calc_growth());
    }

    d_construct_item(m_end, std::forward<Args>(args)...);
    ++m_end;
    return back();
  }

  void push_back(const T &item) {
    if (m_end == m_capacity) {
      reserve_exact(m_calc_growth());
    }

    d_construct_item(m_end, item);
    ++m_end;
  }

  void push_back(T &&item) {
    if (m_end == m_capacity) {
      reserve_exact(m_calc_growth());
    }

    d_construct_item(m_end, std::move(item));
    ++m_end;
  }

private:
  pointer d_allocate(size_type size) { return AT::allocate(m_alloc, size); }

  void d_deallocate(pointer begin, pointer end) {
    AT::deallocate(m_alloc, begin, size_type(end - begin));
  }

  void d_destroy_range(pointer begin, pointer end) noexcept {
    if constexpr (!std::is_trivially_destructible_v<T>) {
      for (; end - begin >= 4; begin += 4) {
        AT::destroy(m_alloc, begin);
        AT::destroy(m_alloc, begin + 1);
        AT::destroy(m_alloc, begin + 2);
        AT::destroy(m_alloc, begin + 3);
      }

      for (; begin != end; ++begin) {
        AT::destroy(m_alloc, begin);
      }
    }
  }

  void d_destroy_item(pointer item) noexcept {
    if constexpr (!std::is_trivially_destructible_v<T>) {
      AT::destroy(m_alloc, item);
    }
  }

  template <class... Args> void d_construct_item(pointer ptr, Args &&...args) {
    AT::construct(m_alloc, ptr, std::forward<Args>(args)...);
  }

  void d_move_range(pointer src_begin, pointer src_end, pointer dest) {
    if constexpr (std::is_trivially_copyable_v<T>) {
      std::memcpy(dest, src_begin, size_type(src_end - src_begin) * sizeof(T));
    } else {
      std::uninitialized_move(src_begin, src_end, dest);
    }
  };

  void d_copy_range(pointer src_begin, pointer src_end, pointer dest) {
    if constexpr (std::is_trivially_copyable_v<T>) {
      std::memcpy(dest, src_begin, size_type(src_end - src_begin) * sizeof(T));
    } else {
      std::uninitialized_copy(src_begin, src_end, dest);
    }
  }

  static void s_swap(DynamicArray &a, DynamicArray &b) noexcept {
    std::swap(a.m_begin, b.m_begin);
    std::swap(a.m_end, b.m_end);
    std::swap(a.m_capacity, b.m_capacity);
  }

  size_type m_calc_growth() { return capacity() == 0 ? 1 : capacity() * 2; }
};

} // namespace core
