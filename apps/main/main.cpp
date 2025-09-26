// Copyright 2025 Jakub Kijek
// Licensed under the MIT License.
// See LICENSE.md file in the project root for full license information.

#include "containers/DynamicArray.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    frank::DynamicArrayOld ns = {1, 2, 3, 4};
    std::cout << sizeof(ns) << "\n";
}
