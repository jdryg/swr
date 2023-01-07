#ifndef CORE_MATH_H
#define CORE_MATH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

static int32_t core_absi32(int32_t x);
static int32_t core_maxi32(int32_t a, int32_t b);
static int32_t core_mini32(int32_t a, int32_t b);
static int32_t core_max3i32(int32_t a, int32_t b, int32_t c);
static int32_t core_min3i32(int32_t a, int32_t b, int32_t c);

#ifdef __cplusplus
}
#endif

#endif // CORE_MATH_H

#include "inline/math.inl"
