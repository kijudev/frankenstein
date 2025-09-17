// Copyright 2025 Jakub Kijek
// Licensed under the MIT License.
// See LICENSE.md file in the project root for full license information.

#include "../../core/DynamicArray.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>

int main() {
  core::DynamicArray<int> numbers;
  numbers.push_back(1);
  numbers.push_back(2);
  numbers.push_back(3);
}
