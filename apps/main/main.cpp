#include "../../core/DynamicArray.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>

int main() {
  core::DynamicArray<int> numbers;

  numbers.push_back(1);
  numbers.push_back(2);
  numbers.push_back(3);

  std::cout << numbers[0] << ", " << numbers[1] << ", " << numbers[2] << "\n";

  numbers.pop_back();
  std::cout << numbers.size() << "\n";

  return 0;
}
