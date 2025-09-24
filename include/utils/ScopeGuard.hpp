// Copyright 2025 Jakub Kijek
// Licensed under the MIT License.
// See LICENSE.md file in the project root for full license information.

#pragma once

#include "TypeTraits.hpp"
#include <type_traits>
#include <utility>

namespace frank {
namespace utils {
template <typename Callback>
concept ScopeGuardCallback = NoArgCallable<Callback> && ReturnsVoid<Callback>
                             && NothrowDestructible<Callback>;

template <ScopeGuardCallback Callback>
class ScopeGuard {
private:
    Callback m_callback;
    bool     m_active;

public:
    ScopeGuard()                             = delete;
    ScopeGuard(const ScopeGuard&)            = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard& operator=(ScopeGuard&)       = delete;

    explicit ScopeGuard(Callback&& cb)
        : m_callback(std::forward<Callback>(cb))
        , m_active(true) { }

    ScopeGuard(ScopeGuard&& other)
        : m_callback(std::move(other.m_callback))
        , m_active(other.m_active) {
        other.m_active = false;
    }

    ~ScopeGuard() noexcept {
        if (m_active) {
            m_callback();
        }
    }

    void dismiss() noexcept { m_active = false; }
};
}
}
