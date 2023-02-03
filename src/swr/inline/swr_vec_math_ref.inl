#ifndef SWR_SWR_VEC_MATH_H
#error "Must be included from swr_vec_math.h"
#endif

#define VEC4F(e0, e1, e2, e3) (vec4f){ .m_Elem[0] = (e0), .m_Elem[1] = (e1), .m_Elem[2] = (e2), .m_Elem[3] = (e3) }

static inline vec4f vec4f_zero(void)
{
	return VEC4F(0.0f, 0.0f, 0.0f, 0.0f);
}

static inline vec4f vec4f_fromFloat(float x)
{
	return VEC4F(x, x, x, x);
}

static inline vec4f vec4f_fromVec4i(vec4i x)
{
	return VEC4F((float)x.m_Elem[0], (float)x.m_Elem[1], (float)x.m_Elem[2], (float)x.m_Elem[3]);
}

static inline vec4f vec4f_fromFloat4(float x0, float x1, float x2, float x3)
{
	return VEC4F(x0, x1, x2, x3);
}

static inline vec4f vec4f_fromRGBA8(uint32_t rgba8)
{
	return VEC4F((float)((rgba8 & 0xFF000000u) >> 24), (float)((rgba8 & 0x00FF0000u) >> 16), (float)((rgba8 & 0x0000FF00u) >> 8), (float)((rgba8 & 0x000000FFu) >> 0));
}

static inline uint32_t vec4f_toRGBA8(vec4f x)
{
	return 0
		| (((uint32_t)x.m_Elem[0] << 24) & 0xFF000000u)
		| (((uint32_t)x.m_Elem[1] << 16) & 0x00FF0000u)
		| (((uint32_t)x.m_Elem[2] << 8) & 0x0000FF00u)
		| (((uint32_t)x.m_Elem[3] << 0) & 0x000000FFu)
		;
}

static inline vec4f vec4f_add(vec4f a, vec4f b)
{
	return VEC4F(a.m_Elem[0] + b.m_Elem[0], a.m_Elem[1] + b.m_Elem[1], a.m_Elem[2] + b.m_Elem[2], a.m_Elem[3] + b.m_Elem[3]);
}

static inline vec4f vec4f_sub(vec4f a, vec4f b)
{
	return VEC4F(a.m_Elem[0] - b.m_Elem[0], a.m_Elem[1] - b.m_Elem[1], a.m_Elem[2] - b.m_Elem[2], a.m_Elem[3] - b.m_Elem[3]);
}

static inline vec4f vec4f_mul(vec4f a, vec4f b)
{
	return VEC4F(a.m_Elem[0] * b.m_Elem[0], a.m_Elem[1] * b.m_Elem[1], a.m_Elem[2] * b.m_Elem[2], a.m_Elem[3] * b.m_Elem[3]);
}

static vec4f vec4f_floor(vec4f x)
{
	return VEC4F(core_floorf(x.m_Elem[0]), core_floorf(x.m_Elem[1]), core_floorf(x.m_Elem[2]), core_floorf(x.m_Elem[3]));
}

static vec4f vec4f_ceil(vec4f x)
{
	return VEC4F(core_ceilf(x.m_Elem[0]), core_ceilf(x.m_Elem[1]), core_ceilf(x.m_Elem[2]), core_ceilf(x.m_Elem[3]));
}

static inline vec4f vec4f_madd(vec4f a, vec4f b, vec4f c)
{
	return VEC4F(a.m_Elem[0] * b.m_Elem[0] + c.m_Elem[0], a.m_Elem[1] * b.m_Elem[1] + c.m_Elem[1], a.m_Elem[2] * b.m_Elem[2] + c.m_Elem[2], a.m_Elem[3] * b.m_Elem[3] + c.m_Elem[3]);
}

static inline float vec4f_getX(vec4f a)
{
	return a.m_Elem[0];
}

static inline float vec4f_getY(vec4f a)
{
	return a.m_Elem[1];
}

static inline float vec4f_getZ(vec4f a)
{
	return a.m_Elem[2];
}

static inline float vec4f_getW(vec4f a)
{
	return a.m_Elem[3];
}

#define VEC4F_GET_FUNC(swizzle) \
static inline vec4f vec4f_get##swizzle(vec4f x) \
{ \
	const uint32_t id0 = (VEC4_SHUFFLE_##swizzle >> 0) & 0x03; \
	const uint32_t id1 = (VEC4_SHUFFLE_##swizzle >> 2) & 0x03; \
	const uint32_t id2 = (VEC4_SHUFFLE_##swizzle >> 4) & 0x03; \
	const uint32_t id3 = (VEC4_SHUFFLE_##swizzle >> 6) & 0x03; \
	return (vec4f){	\
		.m_Elem[0] = x.m_Elem[id0], \
		.m_Elem[1] = x.m_Elem[id1], \
		.m_Elem[2] = x.m_Elem[id2], \
		.m_Elem[3] = x.m_Elem[id3] \
	}; \
}

VEC4F_GET_FUNC(XXXX)
VEC4F_GET_FUNC(YYYY)
VEC4F_GET_FUNC(ZZZZ)
VEC4F_GET_FUNC(WWWW)
VEC4F_GET_FUNC(XYXY)
VEC4F_GET_FUNC(ZWZW)

#undef VEC4F

#define VEC4I(e0, e1, e2, e3) (vec4i){ .m_Elem[0] = (e0), .m_Elem[1] = (e1), .m_Elem[2] = (e2), .m_Elem[3] = (e3) }

static inline vec4i vec4i_zero(void)
{
	return VEC4I(0, 0, 0, 0);
}

static inline vec4i vec4i_fromInt(int32_t x)
{
	return VEC4I(x, x, x, x);
}

static inline vec4i vec4i_fromVec4f(vec4f x)
{
	return VEC4I((int32_t)x.m_Elem[0], (int32_t)x.m_Elem[1], (int32_t)x.m_Elem[2], (int32_t)x.m_Elem[3]);
}

static inline vec4i vec4i_fromInt4(int32_t x0, int32_t x1, int32_t x2, int32_t x3)
{
	return VEC4I(x0, x1, x2, x3);
}

static inline vec4i vec4i_fromInt4va(const int32_t* arr)
{
	return VEC4I(arr[0], arr[1], arr[2], arr[3]);
}

static inline void vec4i_toInt4vu(vec4i x, int32_t* arr)
{
	arr[0] = x.m_Elem[0];
	arr[1] = x.m_Elem[1];
	arr[2] = x.m_Elem[2];
	arr[3] = x.m_Elem[3];
}

static inline void vec4i_toInt4va(vec4i x, int32_t* arr)
{
	arr[0] = x.m_Elem[0];
	arr[1] = x.m_Elem[1];
	arr[2] = x.m_Elem[2];
	arr[3] = x.m_Elem[3];
}

static inline void vec4i_toInt4va_masked(vec4i x, vec4i mask, int32_t* buffer)
{
	buffer[0] = (int32_t)((~(uint32_t)mask.m_Elem[0] & (uint32_t)buffer[0]) | ((uint32_t)mask.m_Elem[0] & (uint32_t)x.m_Elem[0]));
	buffer[1] = (int32_t)((~(uint32_t)mask.m_Elem[1] & (uint32_t)buffer[1]) | ((uint32_t)mask.m_Elem[1] & (uint32_t)x.m_Elem[1]));
	buffer[2] = (int32_t)((~(uint32_t)mask.m_Elem[2] & (uint32_t)buffer[2]) | ((uint32_t)mask.m_Elem[2] & (uint32_t)x.m_Elem[2]));
	buffer[3] = (int32_t)((~(uint32_t)mask.m_Elem[3] & (uint32_t)buffer[3]) | ((uint32_t)mask.m_Elem[3] & (uint32_t)x.m_Elem[3]));
}

static inline void vec4i_toInt4va_maskedInv(vec4i x, vec4i maskInv, int32_t* buffer)
{
	buffer[0] = (int32_t)(((uint32_t)maskInv.m_Elem[0] & (uint32_t)buffer[0]) | (~(uint32_t)maskInv.m_Elem[0] & (uint32_t)x.m_Elem[0]));
	buffer[1] = (int32_t)(((uint32_t)maskInv.m_Elem[1] & (uint32_t)buffer[1]) | (~(uint32_t)maskInv.m_Elem[1] & (uint32_t)x.m_Elem[1]));
	buffer[2] = (int32_t)(((uint32_t)maskInv.m_Elem[2] & (uint32_t)buffer[2]) | (~(uint32_t)maskInv.m_Elem[2] & (uint32_t)x.m_Elem[2]));
	buffer[3] = (int32_t)(((uint32_t)maskInv.m_Elem[3] & (uint32_t)buffer[3]) | (~(uint32_t)maskInv.m_Elem[3] & (uint32_t)x.m_Elem[3]));
}

static inline int32_t vec4i_toInt(vec4i x)
{
	return x.m_Elem[0];
}

static inline vec4i vec4i_add(vec4i a, vec4i b)
{
	return VEC4I(a.m_Elem[0] + b.m_Elem[0], a.m_Elem[1] + b.m_Elem[1], a.m_Elem[2] + b.m_Elem[2], a.m_Elem[3] + b.m_Elem[3]);
}

static inline vec4i vec4i_sub(vec4i a, vec4i b)
{
	return VEC4I(a.m_Elem[0] - b.m_Elem[0], a.m_Elem[1] - b.m_Elem[1], a.m_Elem[2] - b.m_Elem[2], a.m_Elem[3] - b.m_Elem[3]);
}

static inline vec4i vec4i_mullo(vec4i a, vec4i b)
{
	return VEC4I(a.m_Elem[0] * b.m_Elem[0], a.m_Elem[1] * b.m_Elem[1], a.m_Elem[2] * b.m_Elem[2], a.m_Elem[3] * b.m_Elem[3]);
}

static inline vec4i vec4i_and(vec4i a, vec4i b)
{
	return VEC4I(a.m_Elem[0] & b.m_Elem[0], a.m_Elem[1] & b.m_Elem[1], a.m_Elem[2] & b.m_Elem[2], a.m_Elem[3] & b.m_Elem[3]);
}

static inline vec4i vec4i_or(vec4i a, vec4i b)
{
	return VEC4I(a.m_Elem[0] | b.m_Elem[0], a.m_Elem[1] | b.m_Elem[1], a.m_Elem[2] | b.m_Elem[2], a.m_Elem[3] | b.m_Elem[3]);
}

static inline vec4i vec4i_or3(vec4i a, vec4i b, vec4i c)
{
	return vec4i_or(a, vec4i_or(b, c));
}

static inline vec4i vec4i_andnot(vec4i a, vec4i b)
{
	return VEC4I((int32_t)(~(uint32_t)a.m_Elem[0] & (uint32_t)b.m_Elem[0]), (int32_t)(~(uint32_t)a.m_Elem[1] & (uint32_t)b.m_Elem[1]), (int32_t)(~(uint32_t)a.m_Elem[2] & (uint32_t)b.m_Elem[2]), (int32_t)(~(uint32_t)a.m_Elem[3] & (uint32_t)b.m_Elem[3]));
}

static inline vec4i vec4i_xor(vec4i a, vec4i b)
{
	return VEC4I(a.m_Elem[0] ^ b.m_Elem[0], a.m_Elem[1] ^ b.m_Elem[1], a.m_Elem[2] ^ b.m_Elem[2], a.m_Elem[3] ^ b.m_Elem[3]);
}

static inline vec4i vec4i_sar(vec4i x, uint32_t shift)
{
	return VEC4I(x.m_Elem[0] >> shift, x.m_Elem[1] >> shift, x.m_Elem[2] >> shift, x.m_Elem[3] >> shift);
}

static inline vec4i vec4i_sal(vec4i x, uint32_t shift)
{
	return VEC4I(x.m_Elem[0] << shift, x.m_Elem[1] << shift, x.m_Elem[2] << shift, x.m_Elem[3] << shift);
}

static inline vec4i vec4i_cmplt(vec4i a, vec4i b)
{
	return VEC4I((a.m_Elem[0] < b.m_Elem[0]) ? 0xFFFFFFFF : 0, (a.m_Elem[1] < b.m_Elem[1]) ? 0xFFFFFFFF : 0, (a.m_Elem[2] < b.m_Elem[2]) ? 0xFFFFFFFF : 0, (a.m_Elem[3] < b.m_Elem[3]) ? 0xFFFFFFFF : 0);
}

static inline vec4i vec4i_packR32G32B32A32_to_RGBA8(vec4i r, vec4i g, vec4i b, vec4i a)
{
	return VEC4I(
		((r.m_Elem[0] & 0xFF) << 24) | ((g.m_Elem[0] & 0xFF) << 16) | ((b.m_Elem[0] & 0xFF) << 8) | ((a.m_Elem[0] & 0xFF) << 0),
		((r.m_Elem[1] & 0xFF) << 24) | ((g.m_Elem[1] & 0xFF) << 16) | ((b.m_Elem[1] & 0xFF) << 8) | ((a.m_Elem[1] & 0xFF) << 0),
		((r.m_Elem[2] & 0xFF) << 24) | ((g.m_Elem[2] & 0xFF) << 16) | ((b.m_Elem[2] & 0xFF) << 8) | ((a.m_Elem[2] & 0xFF) << 0),
		((r.m_Elem[3] & 0xFF) << 24) | ((g.m_Elem[3] & 0xFF) << 16) | ((b.m_Elem[3] & 0xFF) << 8) | ((a.m_Elem[3] & 0xFF) << 0)
	);
}

static inline bool vec4i_anyNegative(vec4i x)
{
	return (x.m_Elem[0] | x.m_Elem[1] | x.m_Elem[2] | x.m_Elem[3]) < 0;
}

static inline bool vec4i_allNegative(vec4i x)
{
	return ((x.m_Elem[0] & x.m_Elem[1] & x.m_Elem[2] & x.m_Elem[3]) & 0x80000000) != 0;
}

static inline uint32_t vec4i_getSignMask(vec4i x)
{
	return 0
		| (x.m_Elem[0] < 0 ? 0x01 : 0)
		| (x.m_Elem[1] < 0 ? 0x02 : 0)
		| (x.m_Elem[2] < 0 ? 0x04 : 0)
		| (x.m_Elem[3] < 0 ? 0x08 : 0)
		;
}

#define VEC4I_GET_FUNC(swizzle) \
static inline vec4i vec4i_get##swizzle(vec4i x) \
{ \
	const uint32_t id0 = (VEC4_SHUFFLE_##swizzle >> 0) & 0x03; \
	const uint32_t id1 = (VEC4_SHUFFLE_##swizzle >> 2) & 0x03; \
	const uint32_t id2 = (VEC4_SHUFFLE_##swizzle >> 4) & 0x03; \
	const uint32_t id3 = (VEC4_SHUFFLE_##swizzle >> 6) & 0x03; \
	return (vec4i){	\
		.m_Elem[0] = x.m_Elem[id0], \
		.m_Elem[1] = x.m_Elem[id1], \
		.m_Elem[2] = x.m_Elem[id2], \
		.m_Elem[3] = x.m_Elem[id3] \
	}; \
}

VEC4I_GET_FUNC(XXXX);
VEC4I_GET_FUNC(YYYY);
VEC4I_GET_FUNC(ZZZZ);
VEC4I_GET_FUNC(WWWW);
VEC4I_GET_FUNC(XYXY);
VEC4I_GET_FUNC(ZWZW);

#undef VEC4I
