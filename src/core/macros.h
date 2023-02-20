#ifndef CORE_MACROS_H
#define CORE_MACROS_H

#include <stdint.h>

#ifndef CORE_CONFIG_DEBUG
#define CORE_CONFIG_DEBUG _DEBUG
#endif

#define CORE_PLATFORM_WINDOWS 1

#define CORE_COUNTOF(x) (sizeof((x)) / sizeof((x)[0]))

#if CORE_CONFIG_DEBUG
#define CORE_CHECK(expr) if (!(expr)) { __debugbreak(); }
#else // CORE_CONFIG_DEBUG
#define CORE_CHECK(expr)
#endif // CORE_CONFIG_DEBUG

#endif // CORE_MACROS_H
