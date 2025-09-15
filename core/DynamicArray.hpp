#pragma once

#include <iostream>
#include <iterator>
#include <memory>
#include <type_traits>

namespace core {
template <class T, class Allocator> class DynamicArray {
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
};
} // namespace core
