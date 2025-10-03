// Copyright 2025 Jakub Kijek
// Licensed under the MIT License.
// See LICENSE.md file in the project root for full license information.

#include "../internal/scope_guard.hpp"
#include <iterator>
#include <memory>
#include <type_traits>

namespace frank {

template <typename T>
struct ListNode {
    ListNode* prev {nullptr};
    ListNode* next {nullptr};
    T         item;

    ~ListNode() = default;
    ListNode()  = delete;

    template <typename... Args>
    ListNode(Args&&... args) noexcept(
        std::is_nothrow_constructible_v<T, Args&&...>)
        : item(std::forward(args)...) { }

    ListNode(const ListNode&) = delete;
    ListNode(ListNode&&)      = delete;

    ListNode& operator=(const ListNode&) = delete;
    ListNode& operator=(ListNode&&)      = delete;
};

template <typename T, typename Allocator = std::allocator<ListNode<T>>>
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
    using Node = ListNode<T>;

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

        [[nodiscard]] inline bool is_null() noexcept { return head == nullptr; }

        [[nodiscard]] Node* allocate_node() {
            Node* node = std::allocator_traits<Allocator>::allocate(
                static_cast<Allocator&>(*this), 1);

            return node;
        }

        template <typename... Args>
        void construct_node(Node* node, Args&&... args) noexcept(
            std::is_nothrow_constructible_v<Node, Args&...>) {
            if constexpr (std::is_nothrow_constructible_v<Node, Args&...>) {
                std::allocator_traits<Allocator>::construct(
                    static_cast<Allocator&>(*this), std::forward(args)...);
            } else {
                internal::ScopeGuard guard([&]() {
                    std::allocator_traits<Allocator>::deallocate(
                        static_cast<Allocator&>(*this), node, 1);
                });

                std::allocator_traits<Allocator>::construct(
                    static_cast<Allocator&>(*this), std::forward(args)...);

                guard.dismiss();
            }
        }

        void destroy_node(Node* node) noexcept(
            std::is_nothrow_destructible_v<Node>) {
            std::allocator_traits<Allocator>(
                static_cast<Allocator&>(*this), node);
        }

        void deallocate_node(Node* node) noexcept {
            std::allocator_traits<Allocator>::deallocate(
                static_cast<Allocator&>(*this), node);
        }
    };
};
}
