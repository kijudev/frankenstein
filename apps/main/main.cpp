// Copyright 2025 Jakub Kijek
// Licensed under the MIT License.
// See LICENSE.md file in the project root for full license information.

#include "../../core/ScopeGuard.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    core::ScopeGuard guard(
        [&]() { std::cout << "Hello from ScopeGuard v2\n"; });

    std::cout << "Hello from main\n";
}
