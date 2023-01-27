#ifndef SWR_SWR_VEC_MATH_H
#error "Must be included from swr_vec_math.h"
#endif

#define VEC4F(xmm_reg) (vec4f){ .m_XMM = (xmm_reg) }

static inline vec4f vec4f_zero(void)
{
	return VEC4F(_mm_setzero_ps());
}

static inline vec4f vec4f_fromFloat(float x)
{
	return VEC4F(_mm_set_ps1(x));
}

static inline vec4f vec4f_fromVec4i(vec4i x)
{
	return VEC4F(_mm_cvtepi32_ps(x.m_IMM));
}

static inline vec4f vec4f_fromFloat4(float x0, float x1, float x2, float x3)
{
	return VEC4F(_mm_set_ps(x3, x2, x1, x0));
}

static inline vec4f vec4f_fromRGBA8(uint32_t rgba8)
{
	const __m128i imm_zero = _mm_setzero_si128();
	const __m128i imm_rgba8 = _mm_cvtsi32_si128(rgba8);
	const __m128i imm_rgba16 = _mm_unpacklo_epi8(imm_rgba8, imm_zero);
	const __m128i imm_rgba32 = _mm_unpacklo_epi16(imm_rgba16, imm_zero);
	return VEC4F(_mm_cvtepi32_ps(imm_rgba32));
}

static inline uint32_t vec4f_toRGBA8(vec4f x)
{
	const __m128i imm_zero = _mm_setzero_si128();
	const __m128i imm_rgba32 = _mm_cvtps_epi32(x.m_XMM);
	const __m128i imm_rgba16 = _mm_packs_epi32(imm_rgba32, imm_zero);
	const __m128i imm_rgba8 = _mm_packus_epi16(imm_rgba16, imm_zero);
	return (uint32_t)_mm_cvtsi128_si32(imm_rgba8);
}

static inline vec4f vec4f_add(vec4f a, vec4f b)
{
	return VEC4F(_mm_add_ps(a.m_XMM, b.m_XMM));
}

static inline vec4f vec4f_sub(vec4f a, vec4f b)
{
	return VEC4F(_mm_sub_ps(a.m_XMM, b.m_XMM));
}

static inline vec4f vec4f_mul(vec4f a, vec4f b)
{
	return VEC4F(_mm_mul_ps(a.m_XMM, b.m_XMM));
}

// http://dss.stephanierct.com/DevBlog/?p=8
static vec4f vec4f_floor(vec4f x)
{
	return VEC4F(_mm_round_ps(x.m_XMM, _MM_FROUND_FLOOR));
}

// http://dss.stephanierct.com/DevBlog/?p=8
static vec4f vec4f_ceil(vec4f x)
{
	return VEC4F(_mm_round_ps(x.m_XMM, _MM_FROUND_CEIL));
}

static inline vec4f vec4f_madd(vec4f a, vec4f b, vec4f c)
{
	return VEC4F(_mm_add_ps(c.m_XMM, _mm_mul_ps(a.m_XMM, b.m_XMM)));
}

#define VEC4F_GET_FUNC(swizzle) \
static inline vec4f vec4f_get##swizzle(vec4f x) \
{ \
	return (vec4f){ .m_XMM = _mm_shuffle_ps(x.m_XMM, x.m_XMM, (uint32_t)(VEC4_SHUFFLE_##swizzle)) }; \
}

VEC4F_GET_FUNC(XXXX)
VEC4F_GET_FUNC(YYYY)
VEC4F_GET_FUNC(ZZZZ)
VEC4F_GET_FUNC(WWWW)
VEC4F_GET_FUNC(XYXY)
VEC4F_GET_FUNC(ZWZW)

#undef VEC4F

#define VEC4I(imm_reg) (vec4i){ .m_IMM = (imm_reg) }

static inline vec4i vec4i_zero(void)
{
	return VEC4I(_mm_setzero_si128());
}

static inline vec4i vec4i_fromInt(int32_t x)
{
	return VEC4I(_mm_set1_epi32(x));
}

static inline vec4i vec4i_fromVec4f(vec4f x)
{
	return VEC4I(_mm_cvtps_epi32(x.m_XMM));
}

static inline vec4i vec4i_fromInt4(int32_t x0, int32_t x1, int32_t x2, int32_t x3)
{
	return VEC4I(_mm_set_epi32(x3, x2, x1, x0));
}

static inline vec4i vec4i_fromInt4va(const int32_t* arr)
{
	return VEC4I(_mm_load_si128((const __m128i*)arr));
}

static inline void vec4i_toInt4vu(vec4i x, int32_t* arr)
{
	_mm_storeu_si128((__m128i*)arr, x.m_IMM);
}

static inline void vec4i_toInt4va(vec4i x, int32_t* arr)
{
	_mm_store_si128((__m128i*)arr, x.m_IMM);
}

static inline void vec4i_toInt4va_masked(vec4i x, vec4i mask, int32_t* buffer)
{
#if 0
	_mm_maskmoveu_si128(x.m_IMM, mask.m_IMM, (char*)buffer);
#else
	const __m128i old = _mm_load_si128((const __m128i*)buffer);
	const __m128i oldMasked = _mm_andnot_si128(mask.m_IMM, old);
	const __m128i newMasked = _mm_and_si128(mask.m_IMM, x.m_IMM);
	const __m128i final = _mm_or_si128(oldMasked, newMasked);
	_mm_store_si128((__m128i*)buffer, final);
#endif
}

static inline void vec4i_toInt4va_maskedInv(vec4i x, vec4i maskInv, int32_t* buffer)
{
#if 0
	static const uint32_t ones[] = { UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX };
	const __m128i imm_ones = _mm_load_si128((const __m128i*)ones);
	const __m128i imm_mask = _mm_xor_si128(maskInv.m_IMM, imm_ones);
	_mm_maskmoveu_si128(x.m_IMM, imm_mask, (char*)buffer);
#else
	const __m128i old = _mm_load_si128((const __m128i*)buffer);
	const __m128i oldMasked = _mm_and_si128(maskInv.m_IMM, old);
	const __m128i newMasked = _mm_andnot_si128(maskInv.m_IMM, x.m_IMM);
	const __m128i final = _mm_or_si128(oldMasked, newMasked);
	_mm_store_si128((__m128i*)buffer, final);
#endif
}

static inline int32_t vec4i_toInt(vec4i x)
{
	return _mm_cvtsi128_si32(x.m_IMM);
}

static inline vec4i vec4i_add(vec4i a, vec4i b)
{
	return VEC4I(_mm_add_epi32(a.m_IMM, b.m_IMM));
}

static inline vec4i vec4i_sub(vec4i a, vec4i b)
{
	return VEC4I(_mm_sub_epi32(a.m_IMM, b.m_IMM));
}

static inline vec4i vec4i_mullo(vec4i a, vec4i b)
{
	return VEC4I(_mm_mullo_epi32(a.m_IMM, b.m_IMM));
}

static inline vec4i vec4i_and(vec4i a, vec4i b)
{
	return VEC4I(_mm_and_si128(a.m_IMM, b.m_IMM));
}

static inline vec4i vec4i_or(vec4i a, vec4i b)
{
	return VEC4I(_mm_or_si128(a.m_IMM, b.m_IMM));
}

static inline vec4i vec4i_or3(vec4i a, vec4i b, vec4i c)
{
	return VEC4I(_mm_or_si128(a.m_IMM, _mm_or_si128(b.m_IMM, c.m_IMM)));
}

static inline vec4i vec4i_andnot(vec4i a, vec4i b)
{
	return VEC4I(_mm_andnot_si128(a.m_IMM, b.m_IMM));
}

static inline vec4i vec4i_xor(vec4i a, vec4i b)
{
	return VEC4I(_mm_xor_si128(a.m_IMM, b.m_IMM));
}

static inline vec4i vec4i_sar(vec4i x, uint32_t shift)
{
	return VEC4I(_mm_srai_epi32(x.m_IMM, shift));
}

static inline vec4i vec4i_sal(vec4i x, uint32_t shift)
{
	return VEC4I(_mm_slli_epi32(x.m_IMM, shift));
}

static inline vec4i vec4i_cmplt(vec4i a, vec4i b)
{
	return VEC4I(_mm_cmplt_epi32(a.m_IMM, b.m_IMM));
}

static inline vec4i vec4i_packR32G32B32A32_to_RGBA8(vec4i r, vec4i g, vec4i b, vec4i a)
{
	const __m128i mask = _mm_set_epi8(15, 11, 7, 3, 14, 10, 6, 2, 13, 9, 5, 1, 12, 8, 4, 0);

	// Pack into uint8_t
	// (uint8_t){ r0, r1, r2, r3, g0, g1, g2, g3, b0, b1, b2, b3, a0, a1, a2, a3 }
	const __m128i imm_r0123_g0123_b0123_a0123_u8 = _mm_packus_epi16(
		_mm_packs_epi32(r.m_IMM, g.m_IMM), _mm_packs_epi32(b.m_IMM, a.m_IMM)
	);
	const __m128i imm_rgba_p0123_u8 = _mm_shuffle_epi8(imm_r0123_g0123_b0123_a0123_u8, mask);
	return VEC4I(imm_rgba_p0123_u8);
}

static inline bool vec4i_anyNegative(vec4i x)
{
	return (_mm_movemask_epi8(x.m_IMM) & 0x8888) != 0;
}

static inline bool vec4i_allNegative(vec4i x)
{
	return (_mm_movemask_epi8(x.m_IMM) & 0x8888) == 0x8888;
}

static inline uint32_t vec4i_getSignMask(vec4i x)
{
	return _mm_movemask_ps(_mm_castsi128_ps(x.m_IMM));
}

#define VEC4I_GET_FUNC(swizzle) \
static inline vec4i vec4i_get##swizzle(vec4i x) \
{ \
	return (vec4i){ .m_IMM = _mm_shuffle_epi32(x.m_IMM, (uint32_t)(VEC4_SHUFFLE_##swizzle)) }; \
}

VEC4I_GET_FUNC(XXXX);
VEC4I_GET_FUNC(YYYY);
VEC4I_GET_FUNC(ZZZZ);
VEC4I_GET_FUNC(WWWW);
VEC4I_GET_FUNC(XYXY);
VEC4I_GET_FUNC(ZWZW);

#undef VEC4I
