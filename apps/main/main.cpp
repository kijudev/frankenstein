// Copyright 2025 Jakub Kijek
// Licensed under the MIT License.
// See LICENSE.md file in the project root for full license information.

#include "../../include/container/list.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    frank::List<int> numbers;

    numbers.push_back(1);
    numbers.push_back(2);
    numbers.push_back(3);

    std::cout << numbers.head() << "\n";
}
