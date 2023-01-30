#ifndef CORE_MATH_H
#define CORE_MATH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct core_math_api
{
	float (*floorf)(float x);
	float (*ceilf)(float x);
	float (*cosf)(float x);
	float (*sinf)(float x);
} core_math_api;

extern core_math_api* math_api;

static int32_t core_absi32(int32_t x);
static int32_t core_maxi32(int32_t a, int32_t b);
static int32_t core_mini32(int32_t a, int32_t b);
static int32_t core_max3i32(int32_t a, int32_t b, int32_t c);
static int32_t core_min3i32(int32_t a, int32_t b, int32_t c);
static uint16_t core_minu16(uint16_t a, uint16_t b);
static uint16_t core_maxu16(uint16_t a, uint16_t b);
static int32_t core_roundDown(int32_t x, int32_t y);
static int32_t core_roundUp(int32_t x, int32_t y);

static float core_floorf(float x);
static float core_ceilf(float x);
static float core_cosf(float x);
static float core_sinf(float x);
static float core_toRad(float deg);
static float core_toDeg(float rad);

#ifdef __cplusplus
}
#endif

#include "inline/math.inl"

#endif // CORE_MATH_H
