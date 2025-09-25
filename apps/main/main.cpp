// Copyright 2025 Jakub Kijek
// Licensed under the MIT License.
// See LICENSE.md file in the project root for full license information.

#include "containers/DynamicArray.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

template <typename T>
void print_dynamic_array(const frank::DynamicArray<T>& da) {
    std::cout << "==== core::DynamicArray ====\n";
    for (size_t i = 0; i < da.size(); ++i) {
        std::cout << i << " -> " << da[i] << "\n";
    }
}

int main() {
    frank::DynamicArray ns = {1, 2, 3, 4, 5, 6};

    print_dynamic_array(ns);

    ns.pop_back();
    ns.pop_back();

    print_dynamic_array(ns);

    ns.shrink_fit();

    print_dynamic_array(ns);
}
