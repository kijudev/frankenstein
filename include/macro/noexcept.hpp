#pragma once

#ifdef FRANK_DISABLE_NOEXCEPT
#define FRANK_NOEXCEPT
#else
#define FRANK_NOEXCEPT noexcept
#endif

#ifdef FRANK_DISABLE_NOEXCEPT
#define FRANK_NOEXCEPT_METHODS
#else
#define FRANK_NOEXCEPT_METHODS(Class, ...) (INTERNAL_NOEXCEPT_METHODS_)
#endif

#define FRANK_METHOD_NOEXCEPT(ClassType, MethodName)                           \
    noexcept(std::declval<ClassType>().MethodName())

// Variadic macro: ANDs all the method noexcepts together
#define FRANK_NOEXCEPT_ALL(ClassType, ...)                                     \
    (FRANK_NOEXCEPT_ALL_IMPL(ClassType, __VA_ARGS__))

// Internal helpers
#define FRANK_NOEXCEPT_ALL_IMPL(ClassType, ...)                                \
    FRANK_NOEXCEPT_ALL_EXPAND(ClassType, __VA_ARGS__)

#define FRANK_NOEXCEPT_ALL_EXPAND(ClassType, m1, ...)                          \
    FRANK_METHOD_NOEXCEPT(ClassType, m1)                                       \
    &&FRANK_NOEXCEPT_ALL_NEXT(ClassType, __VA_ARGS__)

#define FRANK_NOEXCEPT_ALL_NEXT(ClassType, m1, ...)                            \
    FRANK_METHOD_NOEXCEPT(ClassType, m1)                                       \
    &&FRANK_NOEXCEPT_ALL_NEXT2(ClassType, __VA_ARGS__)

#define FRANK_NOEXCEPT_ALL_NEXT2(ClassType, m1)                                \
    FRANK_METHOD_NOEXCEPT(ClassType, m1)
