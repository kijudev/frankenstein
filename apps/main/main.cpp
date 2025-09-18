// Copyright 2025 Jakub Kijek
// Licensed under the MIT License.
// See LICENSE.md file in the project root for full license information.

#include "../../core/DynamicArray.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    std::vector        ns1 = {1, 2};
    core::DynamicArray ns2 = {5, 6};

    ns2.assign(ns1.begin(), ns1.end());

    for (size_t i = 0; i < ns2.size(); ++i) {
        std::cout << i << " -> " << ns2[i] << "\n";
    }
}
