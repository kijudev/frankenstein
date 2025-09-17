#include "../../core/ScopeGuard.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>

int main() {
  int n = 42;

  auto g = core::MakeScopeGuard([&]() { std::cout << n << "\n "; });

  std::cout << "Hello World!\n";
  g.dismiss();
}
