// Copyright 2025 Jakub Kijek
// Licensed under the MIT License.
// See LICENSE.md file in the project root for full license information.

#include "../internal/scope_guard.hpp"
#include "../macro/assert.hpp"
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
        : item(std::forward<Args>(args)...) { }

    ListNode(const ListNode&) = delete;
    ListNode(ListNode&&)      = delete;

    ListNode& operator=(const ListNode&) = delete;
    ListNode& operator=(ListNode&&)      = delete;
};

template <typename T, typename Allocator = std::allocator<ListNode<T>>>
class List {
private:
    using Node = ListNode<T>;

public:
    using value_type      = T;
    using reference       = T&;
    using const_reference = const T&;
    using size_type       = size_t;
    using difference_type = std::ptrdiff_t;
    using allocator_type  = Allocator;
    using pointer         = std::allocator_traits<Allocator>::pointer;
    using const_pointer   = std::allocator_traits<Allocator>::const_pointer;

    using iterator               = Node*;
    using const_iterator         = const Node*;
    using reverse_iterator       = std::reverse_iterator<Node*>;
    using const_reverse_iterator = std::reverse_iterator<const Node*>;
    using iterator_category      = std::bidirectional_iterator_tag;

private:
    struct Impl : public Allocator {
        Node* head {nullptr};
        Node* tail {nullptr};

        ~Impl() noexcept(std::is_nothrow_destructible_v<T>) {
            while (head != nullptr) {
                Node* next = head->next;

                destroy_node(head);
                deallocate_node(head);

                head = next;
            }
        }

        Impl(const Allocator& a) noexcept(
            std::is_nothrow_constructible_v<Allocator, decltype(a)>)
            : Allocator(a) { }

        Impl()
            : Allocator() { };

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
                    static_cast<Allocator&>(*this),
                    node,
                    std::forward<Args>(args)...);
            } else {
                internal::ScopeGuard guard([&]() {
                    std::allocator_traits<Allocator>::deallocate(
                        static_cast<Allocator&>(*this), node, 1);
                });

                std::allocator_traits<Allocator>::construct(
                    static_cast<Allocator&>(*this),
                    node,
                    std::forward<Args>(args)...);

                guard.dismiss();
            }
        }

        void destroy_node(Node* node) noexcept(
            std::is_nothrow_destructible_v<Node>) {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                std::allocator_traits<Allocator>::destroy(
                    static_cast<Allocator&>(*this), node);
            }
        }

        void deallocate_node(Node* node) noexcept {
            std::allocator_traits<Allocator>::deallocate(
                static_cast<Allocator&>(*this), node, 1);
        }
    };

    Impl impl;

public:
    ~List() = default;

    List() { }

public:
    reference head() noexcept {
        FRANK_ASSERT(!is_null());
        return impl.head->item;
    }

public:
    [[nodiscard]] bool is_null() { return impl.is_null(); }

public:
    iterator push_back(const T& item) noexcept { return emplace_back(item); }

    template <typename... Args>
    iterator emplace_back(Args&&... args) {
        Node* node = impl.allocate_node();
        impl.construct_node(node, std::forward<Args>(args)...);

        if (is_null()) {
            impl.head = node;
            impl.tail = node;

            return node;
        }

        impl.tail->next = node;
        node->prev      = impl.tail;
        impl.tail       = node;

        return node;
    }
};
}
