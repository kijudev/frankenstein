#pragma once

#include <utility>

#ifdef FRANK_DISABLE_NOEXCEPT
#define FRANK_NOEXCEPT
#else
#define FRANK_NOEXCEPT noexcept
#endif

#ifdef FRANK_DISABLE_NOEXCEPT
#define FRANK_NOEXCEPT_METHOD
#else
#define FRANK_NOEXCEPT_METHOD(ClassType, MethodType)                           \
    noexcept(std::declval<ClassType>().MethodType())
#endif
