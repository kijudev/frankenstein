#pragma once

#include <cassert>

#ifdef FRANK_DISABLE_ASSERT
#define FRANK_ASSERT
#else
#define FRANK_ASSERT(cond) assert(cond)
#endif
