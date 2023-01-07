#ifndef CORE_MATH_H
#error "Must be included from math.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

static int32_t core_absi32(int32_t x)
{
	return x < 0 ? -x : x;
}

static int32_t core_maxi32(int32_t a, int32_t b)
{
	return a > b ? a : b;
}

static int32_t core_mini32(int32_t a, int32_t b)
{
	return a < b ? a : b;
}

static int32_t core_max3i32(int32_t a, int32_t b, int32_t c)
{
	return core_maxi32(a, core_maxi32(b, c));
}

static int32_t core_min3i32(int32_t a, int32_t b, int32_t c)
{
	return core_mini32(a, core_mini32(b, c));
}

#ifdef __cplusplus
}
#endif
