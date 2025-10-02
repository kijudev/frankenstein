// Copyright 2025 Jakub Kijek
// Licensed under the MIT License.
// See LICENSE.md file in the project root for full license information.

#include <iterator>
#include <memory>
#include <type_traits>

namespace frank {
template <typename T, typename Allocator = std::allocator<T>>
class List {
public:
    using value_type      = T;
    using reference       = T&;
    using const_reference = const T&;
    using size_type       = size_t;
    using difference_type = std::ptrdiff_t;
    using allocator_type  = Allocator;
    using pointer         = std::allocator_traits<Allocator>::pointer;
    using const_pointer   = std::allocator_traits<Allocator>::const_pointer;

    using iterator               = T*;
    using const_iterator         = const T*;
    using reverse_iterator       = std::reverse_iterator<T*>;
    using const_reverse_iterator = std::reverse_iterator<const T*>;
    using iterator_category      = std::bidirectional_iterator_tag;

private:
    struct Node {
        Node*      prev;
        Node*      next;
        value_type value;
    };

    struct Impl : public Allocator {
        Node* head {nullptr};
        Node* tail {nullptr};

        ~Impl() { }

        Impl(const Allocator&& a) noexcept(
            std::is_nothrow_constructible_v<Allocator, decltype(a)>)
            : Allocator(a) { }

        Impl(Allocator&& a) noexcept(
            std::is_nothrow_constructible_v<Allocator, decltype(a)>)
            : Allocator(std::move(a)) { }

        Impl()            = delete;
        Impl(const Impl&) = delete;
        Impl(Impl&&)      = delete;

        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&)      = delete;
    };
};
}
