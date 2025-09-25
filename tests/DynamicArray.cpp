#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "containers/DynamicArray.hpp"
#include "doctest/doctest.h"
#include <cstddef>
#include <string>

TEST_CASE("Constructors") {
    SUBCASE("Deafult") {
        frank::DynamicArray<int> ns;

        CHECK(ns.size() == 0);
        CHECK(ns.capacity() == 0);
    }

    SUBCASE("Initiliazer list") {
        frank::DynamicArray<int> ns({42, 69, 2137});

        CHECK(ns.size() == 3);
        CHECK(ns.capacity() == 3);
        CHECK(ns[0] == 42);
        CHECK(ns[1] == 69);
        CHECK(ns[2] == 2137);
    }

    SUBCASE("Copy - trivially copyable types") {
        frank::DynamicArray<int> nsa({42, 69, 2137});
        frank::DynamicArray<int> nsb(nsa);

        CHECK(nsb.size() == 3);
        CHECK(nsb.capacity() == 3);
        CHECK(nsb[0] == 42);
        CHECK(nsb[1] == 69);
        CHECK(nsb[2] == 2137);
    }

    SUBCASE("Copy - nontrivially copyable types") {
        frank::DynamicArray nsa(
            {std::string("42"), std::string("69"), std::string("2137")});
        frank::DynamicArray nsb(nsa);

        CHECK(nsb.size() == 3);
        CHECK(nsb.capacity() == 3);
        CHECK(nsb[0] == "42");
        CHECK(nsb[1] == "69");
        CHECK(nsb[2] == "2137");
    }

    SUBCASE("Move - trivially copyable types") {
        frank::DynamicArray<int> nsa({42, 69, 2137});
        frank::DynamicArray<int> nsb(std::move(nsa));

        CHECK(nsb.size() == 3);
        CHECK(nsb.capacity() == 3);
        CHECK(nsb[0] == 42);
        CHECK(nsb[1] == 69);
        CHECK(nsb[2] == 2137);

        CHECK(nsa.size() == 0);
        CHECK(nsa.capacity() == 0);
    }

    SUBCASE("Move - nontrivially copyable types") {
        frank::DynamicArray nsa(
            {std::string("42"), std::string("69"), std::string("2137")});
        frank::DynamicArray nsb(std::move(nsa));

        CHECK(nsb.size() == 3);
        CHECK(nsb.capacity() == 3);
        CHECK(nsb[0] == "42");
        CHECK(nsb[1] == "69");
        CHECK(nsb[2] == "2137");

        CHECK(nsa.size() == 0);
        CHECK(nsa.capacity() == 0);
    }
}

TEST_CASE("push_back") {
    frank::DynamicArray<int> ns;

    for (size_t i = 0; i < 1000; ++i) {
        ns.push_back(i);
    }

    CHECK(ns.size() == 1000);
    CHECK(ns[0] == 0);
    CHECK(ns[999] == 999);
}
