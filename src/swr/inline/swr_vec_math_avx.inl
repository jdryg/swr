#ifndef SWR_SWR_VEC_MATH_H
#error "Must be included from swr_vec_math.h"
#endif

#define VEC4F(xmm_reg) (vec4f){ .m_XMM = (xmm_reg) }

static __forceinline vec4f vec4f_zero(void)
{
	return VEC4F(_mm_setzero_ps());
}

static __forceinline vec4f vec4f_fromFloat(float x)
{
	return VEC4F(_mm_set_ps1(x));
}

static __forceinline vec4f vec4f_fromVec4i(vec4i x)
{
	return VEC4F(_mm_cvtepi32_ps(x.m_IMM));
}

static __forceinline vec4f vec4f_fromFloat4(float x0, float x1, float x2, float x3)
{
	return VEC4F(_mm_set_ps(x3, x2, x1, x0));
}

static __forceinline vec4f vec4f_fromRGBA8(uint32_t rgba8)
{
	const __m128i imm_zero = _mm_setzero_si128();
	const __m128i imm_rgba8 = _mm_cvtsi32_si128(rgba8);
	const __m128i imm_rgba16 = _mm_unpacklo_epi8(imm_rgba8, imm_zero);
	const __m128i imm_rgba32 = _mm_unpacklo_epi16(imm_rgba16, imm_zero);
	return VEC4F(_mm_cvtepi32_ps(imm_rgba32));
}

static __forceinline uint32_t vec4f_toRGBA8(vec4f x)
{
	const __m128i imm_zero = _mm_setzero_si128();
	const __m128i imm_rgba32 = _mm_cvtps_epi32(x.m_XMM);
	const __m128i imm_rgba16 = _mm_packs_epi32(imm_rgba32, imm_zero);
	const __m128i imm_rgba8 = _mm_packus_epi16(imm_rgba16, imm_zero);
	return (uint32_t)_mm_cvtsi128_si32(imm_rgba8);
}

static __forceinline vec4f vec4f_add(vec4f a, vec4f b)
{
	return VEC4F(_mm_add_ps(a.m_XMM, b.m_XMM));
}

static __forceinline vec4f vec4f_sub(vec4f a, vec4f b)
{
	return VEC4F(_mm_sub_ps(a.m_XMM, b.m_XMM));
}

static __forceinline vec4f vec4f_mul(vec4f a, vec4f b)
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

static __forceinline vec4f vec4f_madd(vec4f a, vec4f b, vec4f c)
{
#if 1
	return VEC4F(_mm_fmadd_ps(a.m_XMM, b.m_XMM, c.m_XMM));
#else
	return VEC4F(_mm_add_ps(c.m_XMM, _mm_mul_ps(a.m_XMM, b.m_XMM)));
#endif
}

static __forceinline float vec4f_getX(vec4f a)
{
	return _mm_cvtss_f32(a.m_XMM);
}

static __forceinline float vec4f_getY(vec4f a)
{
	return _mm_cvtss_f32(_mm_shuffle_ps(a.m_XMM, a.m_XMM, _MM_SHUFFLE(1, 1, 1, 1)));
}

static __forceinline float vec4f_getZ(vec4f a)
{
	return _mm_cvtss_f32(_mm_shuffle_ps(a.m_XMM, a.m_XMM, _MM_SHUFFLE(2, 2, 2, 2)));
}

static __forceinline float vec4f_getW(vec4f a)
{
	return _mm_cvtss_f32(_mm_shuffle_ps(a.m_XMM, a.m_XMM, _MM_SHUFFLE(3, 3, 3, 3)));
}

#define VEC4F_GET_FUNC(swizzle) \
static __forceinline vec4f vec4f_get##swizzle(vec4f x) \
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

static __forceinline vec4i vec4i_zero(void)
{
	return VEC4I(_mm_setzero_si128());
}

static __forceinline vec4i vec4i_fromInt(int32_t x)
{
	return VEC4I(_mm_set1_epi32(x));
}

static __forceinline vec4i vec4i_fromVec4f(vec4f x)
{
	return VEC4I(_mm_cvtps_epi32(x.m_XMM));
}

static __forceinline vec4i vec4i_fromInt4(int32_t x0, int32_t x1, int32_t x2, int32_t x3)
{
	return VEC4I(_mm_set_epi32(x3, x2, x1, x0));
}

static __forceinline vec4i vec4i_fromInt4va(const int32_t* arr)
{
	return VEC4I(_mm_load_si128((const __m128i*)arr));
}

static __forceinline void vec4i_toInt4vu(vec4i x, int32_t* arr)
{
	_mm_storeu_si128((__m128i*)arr, x.m_IMM);
}

static __forceinline void vec4i_toInt4va(vec4i x, int32_t* arr)
{
	_mm_store_si128((__m128i*)arr, x.m_IMM);
}

static __forceinline void vec4i_toInt4va_masked(vec4i x, vec4i mask, int32_t* buffer)
{
	const __m128i old = _mm_load_si128((const __m128i*)buffer);
	const __m128i oldMasked = _mm_andnot_si128(mask.m_IMM, old);
	const __m128i newMasked = _mm_and_si128(mask.m_IMM, x.m_IMM);
	const __m128i final = _mm_or_si128(oldMasked, newMasked);
	_mm_store_si128((__m128i*)buffer, final);
}

static __forceinline void vec4i_toInt4va_maskedInv(vec4i x, vec4i maskInv, int32_t* buffer)
{
	const __m128i old = _mm_load_si128((const __m128i*)buffer);
	const __m128i oldMasked = _mm_and_si128(maskInv.m_IMM, old);
	const __m128i newMasked = _mm_andnot_si128(maskInv.m_IMM, x.m_IMM);
	const __m128i final = _mm_or_si128(oldMasked, newMasked);
	_mm_store_si128((__m128i*)buffer, final);
}

static __forceinline void vec4i_toInt4vu_maskedInv(vec4i x, vec4i maskInv, int32_t* buffer)
{
	const __m128i old = _mm_lddqu_si128((const __m128i*)buffer);
	const __m128i oldMasked = _mm_and_si128(maskInv.m_IMM, old);
	const __m128i newMasked = _mm_andnot_si128(maskInv.m_IMM, x.m_IMM);
	const __m128i final = _mm_or_si128(oldMasked, newMasked);
	_mm_storeu_si128((__m128i*)buffer, final);
}

static __forceinline int32_t vec4i_toInt(vec4i x)
{
	return _mm_cvtsi128_si32(x.m_IMM);
}

static __forceinline vec4i vec4i_add(vec4i a, vec4i b)
{
	return VEC4I(_mm_add_epi32(a.m_IMM, b.m_IMM));
}

static __forceinline vec4i vec4i_sub(vec4i a, vec4i b)
{
	return VEC4I(_mm_sub_epi32(a.m_IMM, b.m_IMM));
}

static __forceinline vec4i vec4i_mullo(vec4i a, vec4i b)
{
	return VEC4I(_mm_mullo_epi32(a.m_IMM, b.m_IMM));
}

static __forceinline vec4i vec4i_and(vec4i a, vec4i b)
{
	return VEC4I(_mm_and_si128(a.m_IMM, b.m_IMM));
}

static __forceinline vec4i vec4i_or(vec4i a, vec4i b)
{
	return VEC4I(_mm_or_si128(a.m_IMM, b.m_IMM));
}

static __forceinline vec4i vec4i_or3(vec4i a, vec4i b, vec4i c)
{
	return VEC4I(_mm_or_si128(a.m_IMM, _mm_or_si128(b.m_IMM, c.m_IMM)));
}

static __forceinline vec4i vec4i_andnot(vec4i a, vec4i b)
{
	return VEC4I(_mm_andnot_si128(a.m_IMM, b.m_IMM));
}

static __forceinline vec4i vec4i_xor(vec4i a, vec4i b)
{
	return VEC4I(_mm_xor_si128(a.m_IMM, b.m_IMM));
}

static __forceinline vec4i vec4i_sar(vec4i x, uint32_t shift)
{
	return VEC4I(_mm_srai_epi32(x.m_IMM, shift));
}

static __forceinline vec4i vec4i_sal(vec4i x, uint32_t shift)
{
	return VEC4I(_mm_slli_epi32(x.m_IMM, shift));
}

static __forceinline vec4i vec4i_cmplt(vec4i a, vec4i b)
{
	return VEC4I(_mm_cmplt_epi32(a.m_IMM, b.m_IMM));
}

static __forceinline vec4i vec4i_packR32G32B32A32_to_RGBA8(vec4i r, vec4i g, vec4i b, vec4i a)
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

static __forceinline bool vec4i_anyNegative(vec4i x)
{
	return (_mm_movemask_epi8(x.m_IMM) & 0x8888) != 0;
}

static __forceinline bool vec4i_allNegative(vec4i x)
{
#if 0
	return (_mm_movemask_epi8(x.m_IMM) & 0x8888) == 0x8888;
#else
	return (_mm_movemask_ps(_mm_castsi128_ps(x.m_IMM)) == 0x0F);
#endif
}

static __forceinline uint32_t vec4i_getSignMask(vec4i x)
{
	return _mm_movemask_ps(_mm_castsi128_ps(x.m_IMM));
}

#define VEC4I_GET_FUNC(swizzle) \
static __forceinline vec4i vec4i_get##swizzle(vec4i x) \
{ \
	return (vec4i){ .m_IMM = _mm_shuffle_epi32(x.m_IMM, (uint32_t)(VEC4_SHUFFLE_##swizzle)) }; \
}

VEC4I_GET_FUNC(XXXX);
VEC4I_GET_FUNC(YYYY);
VEC4I_GET_FUNC(ZZZZ);
VEC4I_GET_FUNC(WWWW);
VEC4I_GET_FUNC(XYXY);
VEC4I_GET_FUNC(ZWZW);

static __forceinline int32_t vec4i_getX(vec4i a)
{
	return _mm_cvtsi128_si32(a.m_IMM);
}

static __forceinline int32_t vec4i_getY(vec4i a)
{
	return _mm_cvtsi128_si32(_mm_shuffle_epi32(a.m_IMM, VEC4_SHUFFLE_YYYY));
}

static __forceinline int32_t vec4i_getZ(vec4i a)
{
	return _mm_cvtsi128_si32(_mm_shuffle_epi32(a.m_IMM, VEC4_SHUFFLE_ZZZZ));
}

static __forceinline int32_t vec4i_getW(vec4i a)
{
	return _mm_cvtsi128_si32(_mm_shuffle_epi32(a.m_IMM, VEC4_SHUFFLE_WWWW));
}

#undef VEC4I

#define VEC8F(ymm_reg) (vec8f){ .m_YMM = (ymm_reg) }

static __forceinline vec8f vec8f_zero(void)
{
	return VEC8F(_mm256_setzero_ps());
}

static __forceinline vec8f vec8f_fromFloat(float x)
{
	return VEC8F(_mm256_set1_ps(x));
}

static __forceinline vec8f vec8f_fromVec8i(vec8i x)
{
	return VEC8F(_mm256_cvtepi32_ps(x.m_YMM));
}

static __forceinline vec8f vec8f_fromFloat8(float x0, float x1, float x2, float x3, float x4, float x5, float x6, float x7)
{
	return VEC8F(_mm256_set_ps(x7, x6, x5, x4, x3, x2, x1, x0));
}

static __forceinline vec8f vec8f_add(vec8f a, vec8f b)
{
	return VEC8F(_mm256_add_ps(a.m_YMM, b.m_YMM));
}

static __forceinline vec8f vec8f_sub(vec8f a, vec8f b)
{
	return VEC8F(_mm256_sub_ps(a.m_YMM, b.m_YMM));
}

static __forceinline vec8f vec8f_mul(vec8f a, vec8f b)
{
	return VEC8F(_mm256_mul_ps(a.m_YMM, b.m_YMM));
}

static __forceinline vec8f vec8f_floor(vec8f x)
{
	return VEC8F(_mm256_round_ps(x.m_YMM, _MM_FROUND_FLOOR));
}

static __forceinline vec8f vec8f_ceil(vec8f x)
{
	return VEC8F(_mm256_round_ps(x.m_YMM, _MM_FROUND_CEIL));
}

static __forceinline vec8f vec8f_madd(vec8f a, vec8f b, vec8f c)
{
#if defined(SWR_VEC_MATH_FMA)
	return VEC8F(_mm256_fmadd_ps(a.m_YMM, b.m_YMM, c.m_YMM));
#else
	return VEC8F(_mm256_add_ps(c.m_YMM, _mm256_mul_ps(a.m_YMM, b.m_YMM)));
#endif
}

#undef VEC8F

#if defined(SWR_VEC_MATH_AVX2)
#define VEC8I(ymm_reg) (vec8i){ .m_YMM = (ymm_reg) }

static __forceinline vec8i vec8i_zero(void)
{
	return VEC8I(_mm256_setzero_si256());
}

static __forceinline vec8i vec8i_fromInt(int32_t x)
{
	return VEC8I(_mm256_set1_epi32(x));
}

static __forceinline vec8i vec8i_fromVec8f(vec8f x)
{
	return VEC8I(_mm256_cvtps_epi32(x.m_YMM));
}

static __forceinline vec8i vec8i_fromInt8(int32_t x0, int32_t x1, int32_t x2, int32_t x3, int32_t x4, int32_t x5, int32_t x6, int32_t x7)
{
	return VEC8I(_mm256_set_epi32(x7, x6, x5, x4, x3, x2, x1, x0));
}

static __forceinline vec8i vec8i_fromInt8va(const int32_t* arr)
{
	return VEC8I(_mm256_load_si256((const __m256i*)arr));
}

static __forceinline void vec8i_toInt8vu(vec8i x, int32_t* arr)
{
	_mm256_storeu_epi32(arr, x.m_YMM);
}

static __forceinline void vec8i_toInt8va(vec8i x, int32_t* arr)
{
	_mm256_store_si256((__m256i*)arr, x.m_YMM);
}

static __forceinline void vec8i_toInt8va_masked(vec8i x, vec8i mask, int32_t* buffer)
{
	const __m256i old = _mm256_load_si256((const __m256i*)buffer);
	const __m256i oldMasked = _mm256_andnot_si256(mask.m_YMM, old);
	const __m256i newMasked = _mm256_and_si256(mask.m_YMM, x.m_YMM);
	const __m256i final = _mm256_or_si256(oldMasked, newMasked);
	_mm256_store_si256((__m256i*)buffer, final);
}

static __forceinline void vec8i_toInt8va_maskedInv(vec8i x, vec8i maskInv, int32_t* buffer)
{
	const __m256i old = _mm256_load_si256((const __m256i*)buffer);
	const __m256i oldMasked = _mm256_and_si256(maskInv.m_YMM, old);
	const __m256i newMasked = _mm256_andnot_si256(maskInv.m_YMM, x.m_YMM);
	const __m256i final = _mm256_or_si256(oldMasked, newMasked);
	_mm256_store_si256((__m256i*)buffer, final);
}

static __forceinline void vec8i_toInt8vu_maskedInv(vec8i x, vec8i maskInv, int32_t* buffer)
{
	const __m256i old = _mm256_loadu_si256((const __m256i*)buffer);
	const __m256i oldMasked = _mm256_and_si256(maskInv.m_YMM, old);
	const __m256i newMasked = _mm256_andnot_si256(maskInv.m_YMM, x.m_YMM);
	const __m256i final = _mm256_or_si256(oldMasked, newMasked);
	_mm256_storeu_si256((__m256i*)buffer, final);
}

static __forceinline vec8i vec8i_add(vec8i a, vec8i b)
{
	return VEC8I(_mm256_add_epi32(a.m_YMM, b.m_YMM));
}

static __forceinline vec8i vec8i_sub(vec8i a, vec8i b)
{
	return VEC8I(_mm256_sub_epi32(a.m_YMM, b.m_YMM));
}

static __forceinline vec8i vec8i_mullo(vec8i a, vec8i b)
{
	return VEC8I(_mm256_mullo_epi32(a.m_YMM, b.m_YMM));
}

static __forceinline vec8i vec8i_and(vec8i a, vec8i b)
{
	return VEC8I(_mm256_and_si256(a.m_YMM, b.m_YMM));
}

static __forceinline vec8i vec8i_or(vec8i a, vec8i b)
{
	return VEC8I(_mm256_or_si256(a.m_YMM, b.m_YMM));
}

static __forceinline vec8i vec8i_or3(vec8i a, vec8i b, vec8i c)
{
	return vec8i_or(a, vec8i_or(b, c));
}

static __forceinline vec8i vec8i_andnot(vec8i a, vec8i b)
{
	return VEC8I(_mm256_andnot_si256(a.m_YMM, b.m_YMM));
}

static __forceinline vec8i vec8i_xor(vec8i a, vec8i b)
{
	return VEC8I(_mm256_xor_si256(a.m_YMM, b.m_YMM));
}

static __forceinline vec8i vec8i_sar(vec8i x, uint32_t shift)
{
	return VEC8I(_mm256_srai_epi32(x.m_YMM, shift));
}

static __forceinline vec8i vec8i_sal(vec8i x, uint32_t shift)
{
	return VEC8I(_mm256_slli_epi32(x.m_YMM, shift));
}

static __forceinline vec8i vec8i_packR32G32B32A32_to_RGBA8(vec8i r, vec8i g, vec8i b, vec8i a)
{
	// (uint16_t){
	//   r0, r1, r2, r3,
	//   g0, g1, g2, g3,
	//   r4, r5, r6, r7,
	//   g4, g5, g6, g7
	// }
	const __m256i r03_g03_r47_g47_u16 = _mm256_packs_epi32(r.m_YMM, g.m_YMM);
	
	// (uint16_t){
	//   b0, b1, b2, b3,
	//   a0, a1, a2, a3,
	//   b4, b5, b6, b7,
	//   a4, a5, a6, a7
	// }
	const __m256i b03_a03_b47_a47_u16 = _mm256_packs_epi32(b.m_YMM, a.m_YMM);

	// Pack into uint8_t
	// (uint8_t){ 
	//   r0, r1, r2, r3, 
	//   g0, g1, g2, g3, 
	//   b0, b1, b2, b3, 
	//   a0, a1, a2, a3, 
	//   r4, r5, r6, r7,
	//   g4, g5, g6, g7,
	//   b4, b5, b6, b7,
	//   a4, a5, a6, a7
	// };
	// 
	const __m256i r03_g03_b03_a03_r47_g47_b47_a47_u8 = _mm256_packus_epi16(r03_g03_r47_g47_u16, b03_a03_b47_a47_u16);

	static const uint8_t mask_u8[] = {
		0, 4, 8, 12,
		1, 5, 9, 13,
		2, 6, 10, 14,
		3, 7, 11, 15,
		
		0, 4, 8, 12,
		1, 5, 9, 13,
		2, 6, 10, 14,
		3, 7, 11, 15,
	};
	return VEC8I(_mm256_shuffle_epi8(r03_g03_b03_a03_r47_g47_b47_a47_u8, _mm256_load_si256((const __m256i*)mask_u8)));
}

static __forceinline bool vec8i_anyNegative(vec8i x)
{
	return vec8i_getSignMask(x) != 0;
}

static __forceinline bool vec8i_allNegative(vec8i x)
{
	return vec8i_getSignMask(x) == 0xFF;
}

static __forceinline uint32_t vec8i_getSignMask(vec8i x)
{
	return _mm256_movemask_ps(_mm256_castsi256_ps(x.m_YMM));
}

#undef VEC8I

#endif // SWR_VEC_MATH_AVX2