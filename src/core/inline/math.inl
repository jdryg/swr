#ifndef CORE_MATH_H
#error "Must be included from math.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline int32_t core_absi32(int32_t x)
{
	return x < 0 ? -x : x;
}

static inline int32_t core_maxi32(int32_t a, int32_t b)
{
	return a > b ? a : b;
}

static inline int32_t core_mini32(int32_t a, int32_t b)
{
	return a < b ? a : b;
}

static inline int32_t core_max3i32(int32_t a, int32_t b, int32_t c)
{
	return core_maxi32(a, core_maxi32(b, c));
}

static inline int32_t core_min3i32(int32_t a, int32_t b, int32_t c)
{
	return core_mini32(a, core_mini32(b, c));
}

static inline int32_t core_roundDown(int32_t x, int32_t y)
{
	return (x / y) * y;
}

static inline int32_t core_roundUp(int32_t x, int32_t y)
{
	return ((x / y) + ((x % y) != 0 ? 1 : 0)) * y;
}

static inline float core_floorf(float x)
{
	return math_api->floorf(x);
}

static inline float core_ceilf(float x)
{
	return math_api->ceilf(x);
}

#ifdef __cplusplus
}
#endif
