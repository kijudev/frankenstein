#include <cstddef>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../include/containers/DynamicArray.hpp"
#include "doctest/doctest.h"

TEST_CASE("Testing push_back") {
    frank::containers::DynamicArray<int> ns;

    for (size_t i = 0; i < 1000; ++i) {
        ns.push_back(i);
    }

    CHECK(ns.size() == 1000);
    CHECK(ns[0] == 0);
    CHECK(ns[999] == 999);
}
