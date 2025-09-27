#pragma once

#include <cassert>
#include <iostream>

#ifdef FRANK_DISABLE_ASSERT
#define FRANK_ASSERT_MSG
#else
#define FRANK_ASSERT_MSG(cond, msg)                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::cerr << "Assertion failed: " #cond << "\nMessage: " << msg    \
                      << "\nFile: " << __FILE__ << "\nLine: " << __LINE__      \
                      << std::endl;                                            \
            assert(cond);                                                      \
        }                                                                      \
    } while (0)
#endif
