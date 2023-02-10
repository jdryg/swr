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
	static const float xmm_ones[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	const __m128i i = _mm_cvttps_epi32(x.m_XMM);
	const __m128 fi = _mm_cvtepi32_ps(i);
	const __m128 igx = _mm_cmpgt_ps(fi, x.m_XMM);
	const __m128 j = _mm_and_ps(igx, _mm_load_ps(&xmm_ones[0]));
	return VEC4F(_mm_sub_ps(fi, j));
}

// http://dss.stephanierct.com/DevBlog/?p=8
static vec4f vec4f_ceil(vec4f x)
{
	static const float xmm_ones[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	const __m128i i = _mm_cvttps_epi32(x.m_XMM);
	const __m128 fi = _mm_cvtepi32_ps(i);
	const __m128 igx = _mm_cmplt_ps(fi, x.m_XMM);
	const __m128 j = _mm_and_ps(igx, _mm_load_ps(&xmm_ones[0]));
	return VEC4F(_mm_add_ps(fi, j));
}

static inline vec4f vec4f_madd(vec4f a, vec4f b, vec4f c)
{
	return VEC4F(_mm_add_ps(c.m_XMM, _mm_mul_ps(a.m_XMM, b.m_XMM)));
}

static inline float vec4f_getX(vec4f a)
{
	return _mm_cvtss_f32(a.m_XMM);
}

static inline float vec4f_getY(vec4f a)
{
	return _mm_cvtss_f32(_mm_shuffle_ps(a.m_XMM, a.m_XMM, _MM_SHUFFLE(1, 1, 1, 1)));
}

static inline float vec4f_getZ(vec4f a)
{
	return _mm_cvtss_f32(_mm_shuffle_ps(a.m_XMM, a.m_XMM, _MM_SHUFFLE(2, 2, 2, 2)));
}

static inline float vec4f_getW(vec4f a)
{
	return _mm_cvtss_f32(_mm_shuffle_ps(a.m_XMM, a.m_XMM, _MM_SHUFFLE(3, 3, 3, 3)));
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
	_mm_maskmoveu_si128(x.m_IMM, _mm_xor_si128(maskInv.m_IMM, _mm_set1_epi32(-1)), (char*)buffer);
#else
	const __m128i old = _mm_load_si128((const __m128i*)buffer);
	const __m128i oldMasked = _mm_and_si128(maskInv.m_IMM, old);
	const __m128i newMasked = _mm_andnot_si128(maskInv.m_IMM, x.m_IMM);
	const __m128i final = _mm_or_si128(oldMasked, newMasked);
	_mm_store_si128((__m128i*)buffer, final);
#endif
}

static void vec4i_toInt4vu_maskedInv(vec4i x, vec4i maskInv, int32_t* buffer)
{
#if 0
	_mm_maskmoveu_si128(x.m_IMM, _mm_xor_si128(maskInv.m_IMM, _mm_set1_epi32(-1)), (char*)buffer);
#else
	const __m128i old = _mm_loadu_si128((const __m128i*)buffer);
	const __m128i oldMasked = _mm_and_si128(maskInv.m_IMM, old);
	const __m128i newMasked = _mm_andnot_si128(maskInv.m_IMM, x.m_IMM);
	const __m128i final = _mm_or_si128(oldMasked, newMasked);
	_mm_storeu_si128((__m128i*)buffer, final);
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
#if 1
	// https://fgiesen.wordpress.com/2016/04/03/sse-mind-the-gap/
	// even and odd lane products
	const __m128i evnp = _mm_mul_epu32(a.m_IMM, b.m_IMM);
	const __m128i odda = _mm_srli_epi64(a.m_IMM, 32);
	const __m128i oddb = _mm_srli_epi64(b.m_IMM, 32);
	const __m128i oddp = _mm_mul_epu32(odda, oddb);

	// merge results
	const __m128i evn_mask = _mm_setr_epi32(-1, 0, -1, 0);
	const __m128i evn_result = _mm_and_si128(evnp, evn_mask);
	const __m128i odd_result = _mm_slli_epi64(oddp, 32);

	return VEC4I(_mm_or_si128(evn_result, odd_result));
#else
	const __m128i tmp1 = _mm_mul_epu32(a.m_IMM, b.m_IMM);
	const __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(a.m_IMM, 4), _mm_srli_si128(b.m_IMM, 4));
	return (vec4i) { .m_IMM = _mm_unpacklo_epi32(_mm_shuffle_epi32(tmp1, _MM_SHUFFLE(0, 0, 2, 0)), _mm_shuffle_epi32(tmp2, _MM_SHUFFLE(0, 0, 2, 0))) };
#endif
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
