#pragma once

#ifdef FRANK_DISABLE_NOEXCEPT
#define FRANK_NOEXCEPT
#else
#define FRANK_NOEXCEPT noexcept
#endif
