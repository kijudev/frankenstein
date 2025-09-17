// Copyright 2025 Jakub Kijek
// Licensed under the MIT License.
// See LICENSE.md file in the project root for full license information.

#pragma once

#include "./TypeTraits.hpp"
#include <type_traits>
#include <utility>

namespace core {
namespace impl {
template <class T>
struct is_scope_guard_callback_t
    : public core_utils::and_t<core_utils::is_noarg_callable_t<T>,
                               core_utils::returns_void_t<T>,
                               std::is_nothrow_destructible<T>> {};
} // namespace impl

template <class Callback,
          class = typename std::enable_if<
              impl::is_scope_guard_callback_t<Callback>::value>::type>
class ScopeGuard;

template <class Callback> ScopeGuard<Callback> MakeScopeGuard(Callback &&cb);

template <class Callback> class ScopeGuard<Callback> final {
private:
  Callback m_callback;
  bool m_active;

public:
  ScopeGuard() = delete;
  ScopeGuard(const ScopeGuard &) = delete;
  ScopeGuard &operator=(const ScopeGuard &) = delete;
  ScopeGuard &operator=(ScopeGuard &) = delete;

  explicit ScopeGuard(Callback &&cb)
      : m_callback(std::forward<Callback>(cb)), m_active(true) {}

  ScopeGuard(ScopeGuard &&other)
      : m_callback(std::move(other.m_callback)), m_active(other.m_active) {
    other.m_active = false;
  }

  ~ScopeGuard() noexcept {
    if (m_active) {
      m_callback();
    }
  }

  void dismiss() noexcept { m_active = false; }
};

template <class Callback> ScopeGuard<Callback> MakeScopeGuard(Callback &&cb) {
  return ScopeGuard<Callback>(std::forward<Callback>(cb));
}
} // namespace core
