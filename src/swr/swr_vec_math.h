#ifndef SWR_SWR_VEC_MATH_H
#define SWR_SWR_VEC_MATH_H

#include <stdint.h>
#include <stdbool.h>
#include <smmintrin.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VEC4_SHUFFLE_MASK(d0_a, d1_a, d2_b, d3_b) (((d3_b) << 6) | ((d2_b) << 4) | ((d1_a) << 2) | ((d0_a)))

typedef enum vec4_shuffle_mask
{
	VEC4_SHUFFLE_XXXX = VEC4_SHUFFLE_MASK(0, 0, 0, 0),
	VEC4_SHUFFLE_YYYY = VEC4_SHUFFLE_MASK(1, 1, 1, 1),
	VEC4_SHUFFLE_ZZZZ = VEC4_SHUFFLE_MASK(2, 2, 2, 2),
	VEC4_SHUFFLE_WWWW = VEC4_SHUFFLE_MASK(3, 3, 3, 3),
	VEC4_SHUFFLE_XYXY = VEC4_SHUFFLE_MASK(0, 1, 0, 1),
	VEC4_SHUFFLE_XZXZ = VEC4_SHUFFLE_MASK(0, 2, 0, 2),
	VEC4_SHUFFLE_YWYW = VEC4_SHUFFLE_MASK(1, 3, 1, 3),
	VEC4_SHUFFLE_ZWZW = VEC4_SHUFFLE_MASK(2, 3, 2, 3),
} vec4_shuffle_mask;

typedef struct vec4f
{
#if defined(SWR_VEC_MATH_SSE2) || defined(SWR_VEC_MATH_SSSE3) || defined(SWR_VEC_MATH_SSE41)
	__m128 m_XMM;
#else
	float m_Elem[4];
#endif
} vec4f;

typedef struct vec4i
{
#if defined(SWR_VEC_MATH_SSE2) || defined(SWR_VEC_MATH_SSSE3) || defined(SWR_VEC_MATH_SSE41)
	__m128i m_IMM;
#else
	int32_t m_Elem[4];
#endif
} vec4i;

static vec4f vec4f_zero(void);
static vec4f vec4f_fromFloat(float x);
static vec4f vec4f_fromVec4i(vec4i x);
static vec4f vec4f_fromFloat4(float x0, float x1, float x2, float x3);
static vec4f vec4f_fromRGBA8(uint32_t rgba8);
static uint32_t vec4f_toRGBA8(vec4f x);
static vec4f vec4f_add(vec4f a, vec4f b);
static vec4f vec4f_sub(vec4f a, vec4f b);
static vec4f vec4f_mul(vec4f a, vec4f b);
static vec4f vec4f_floor(vec4f x);
static vec4f vec4f_ceil(vec4f x);
static vec4f vec4f_madd(vec4f a, vec4f b, vec4f c);
static float vec4f_getX(vec4f a);
static float vec4f_getY(vec4f a);
static float vec4f_getZ(vec4f a);
static float vec4f_getW(vec4f a);
static vec4f vec4f_getXXXX(vec4f x);
static vec4f vec4f_getYYYY(vec4f x);
static vec4f vec4f_getZZZZ(vec4f x);
static vec4f vec4f_getWWWW(vec4f x);
static vec4f vec4f_getXYXY(vec4f x);
static vec4f vec4f_getYWYW(vec4f x);
static vec4f vec4f_getZWZW(vec4f x);

static vec4i vec4i_zero(void);
static vec4i vec4i_fromInt(int32_t x);
static vec4i vec4i_fromVec4f(vec4f x);
static vec4i vec4i_fromInt4(int32_t x0, int32_t x1, int32_t x2, int32_t x3);
static vec4i vec4i_fromInt4va(const int32_t* arr);
static void vec4i_toInt4vu(vec4i x, int32_t* arr);
static void vec4i_toInt4va(vec4i x, int32_t* arr);
static void vec4i_toInt4va_masked(vec4i x, vec4i mask, int32_t* buffer);
static void vec4i_toInt4va_maskedInv(vec4i x, vec4i maskInv, int32_t* buffer);
static void vec4i_toInt4vu_maskedInv(vec4i x, vec4i maskInv, int32_t* buffer);
static int32_t vec4i_toInt(vec4i x);
static vec4i vec4i_add(vec4i a, vec4i b);
static vec4i vec4i_sub(vec4i a, vec4i b);
static vec4i vec4i_mullo(vec4i a, vec4i b);
static vec4i vec4i_and(vec4i a, vec4i b);
static vec4i vec4i_or(vec4i a, vec4i b);
static vec4i vec4i_or3(vec4i a, vec4i b, vec4i c);
static vec4i vec4i_andnot(vec4i a, vec4i b);
static vec4i vec4i_xor(vec4i a, vec4i b);
static vec4i vec4i_sar(vec4i x, uint32_t shift);
static vec4i vec4i_sal(vec4i x, uint32_t shift);
static vec4i vec4i_cmplt(vec4i a, vec4i b);
static vec4i vec4i_packR32G32B32A32_to_RGBA8(vec4i r, vec4i g, vec4i b, vec4i a);
static bool vec4i_anyNegative(vec4i x);
static bool vec4i_allNegative(vec4i x);
static uint32_t vec4i_getSignMask(vec4i x);

#ifdef __cplusplus
}
#endif

#if defined(SWR_VEC_MATH_SSE2)
#include "inline/swr_vec_math_sse2.inl"
#elif defined(SWR_VEC_MATH_SSSE3)
#include "inline/swr_vec_math_ssse3.inl"
#elif defined(SWR_VEC_MATH_SSE41)
#include "inline/swr_vec_math_sse41.inl"
#else
#include "inline/swr_vec_math_ref.inl"
#endif

#endif // SWR_SWR_VEC_MATH_H
