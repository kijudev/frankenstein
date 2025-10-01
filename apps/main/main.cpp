// Copyright 2025 Jakub Kijek
// Licensed under the MIT License.
// See LICENSE.md file in the project root for full license information.

#include "../../include/container/dynamic_array.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    frank::DynamicArray<int> ns {1, 2, 3, 4, 5, 6};

    for (size_t i = 0; i < ns.size(); ++i) {
        std::cout << ns[i] << "\n";
    }
}
