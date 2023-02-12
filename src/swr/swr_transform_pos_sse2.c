#include "swr.h"
#include "swr_p.h"

#define SWR_VEC_MATH_SSE2
#include "swr_vec_math.h"

#if 1
// More complex one-time setup and fewer shuffles inside the loop.
void swrTransformPos2fTo2iSSE2(uint32_t n, const float* posf, int32_t* posi, const float* mtx)
{
	const float* src = posf;
	int32_t* dst = posi;

	const vec4f m0101 = vec4f_fromFloat4(mtx[0], mtx[1], mtx[0], mtx[1]);
	const vec4f m2323 = vec4f_fromFloat4(mtx[2], mtx[3], mtx[2], mtx[3]);
	const vec4f m4545 = vec4f_fromFloat4(mtx[4], mtx[5], mtx[4], mtx[5]);

	const uint32_t numIter = n >> 2;
	for (uint32_t i = 0; i < numIter; ++i) {
		const vec4f src_x0_y0_x1_y1 = vec4f_fromFloat4vu(&src[0]);
		const vec4f src_x2_y2_x3_y3 = vec4f_fromFloat4vu(&src[4]);

		const vec4f src_x0_x0_x1_x1 = vec4f_shuffle(src_x0_y0_x1_y1, src_x0_y0_x1_y1, VEC4_SHUFFLE_XXZZ);
		const vec4f src_y0_y0_y1_y1 = vec4f_shuffle(src_x0_y0_x1_y1, src_x0_y0_x1_y1, VEC4_SHUFFLE_YYWW);
		const vec4f src_x2_x2_x3_x3 = vec4f_shuffle(src_x2_y2_x3_y3, src_x2_y2_x3_y3, VEC4_SHUFFLE_XXZZ);
		const vec4f src_y2_y2_y3_y3 = vec4f_shuffle(src_x2_y2_x3_y3, src_x2_y2_x3_y3, VEC4_SHUFFLE_YYWW);

		const vec4f dst_x0_y0_x1_y1 = vec4f_madd(src_x0_x0_x1_x1, m0101, vec4f_madd(src_y0_y0_y1_y1, m2323, m4545));
		const vec4f dst_x2_y2_x3_y3 = vec4f_madd(src_x2_x2_x3_x3, m0101, vec4f_madd(src_y2_y2_y3_y3, m2323, m4545));

		vec4i_toInt4vu(vec4i_fromVec4f(dst_x0_y0_x1_y1), &dst[0]);
		vec4i_toInt4vu(vec4i_fromVec4f(dst_x2_y2_x3_y3), &dst[4]);

		src += 8;
		dst += 8;
	}

	uint32_t rem = n & 3;
	if (rem >= 2) {
		const vec4f src_x0_y0_x1_y1 = vec4f_fromFloat4vu(&src[0]);

		const vec4f src_x0_x0_x1_x1 = vec4f_shuffle(src_x0_y0_x1_y1, src_x0_y0_x1_y1, VEC4_SHUFFLE_XXZZ);
		const vec4f src_y0_y0_y1_y1 = vec4f_shuffle(src_x0_y0_x1_y1, src_x0_y0_x1_y1, VEC4_SHUFFLE_YYWW);

		const vec4f dst_x0_y0_x1_y1 = vec4f_madd(src_x0_x0_x1_x1, m0101, vec4f_madd(src_y0_y0_y1_y1, m2323, m4545));

		vec4i_toInt4vu(vec4i_fromVec4f(dst_x0_y0_x1_y1), &dst[0]);

		src += 4;
		dst += 4;
		rem -= 2;
	}

	if (rem) {
		dst[0] = (int32_t)(mtx[0] * src[0] + mtx[2] * src[1] + mtx[4]);
		dst[1] = (int32_t)(mtx[1] * src[0] + mtx[3] * src[1] + mtx[5]);
	}
}
#else
// Simpler one-time setup, more shuffles inside the loop
void swrTransformPos2fTo2iSSE2(uint32_t n, const float* posf, int32_t* posi, const float* mtx)
{
	const float* src = posf;
	int32_t* dst = posi;

	const vec4f m0 = vec4f_fromFloat(mtx[0]);
	const vec4f m1 = vec4f_fromFloat(mtx[1]);
	const vec4f m2 = vec4f_fromFloat(mtx[2]);
	const vec4f m3 = vec4f_fromFloat(mtx[3]);
	const vec4f m4 = vec4f_fromFloat(mtx[4]);
	const vec4f m5 = vec4f_fromFloat(mtx[5]);

	const uint32_t numIter = n >> 2;
	for (uint32_t i = 0; i < numIter; ++i) {
		const vec4f src_x0_y0_x1_y1 = vec4f_fromFloat4vu(&src[0]);
		const vec4f src_x2_y2_x3_y3 = vec4f_fromFloat4vu(&src[4]);

		const vec4f src_x0_x1_x2_x3 = vec4f_shuffle(src_x0_y0_x1_y1, src_x2_y2_x3_y3, VEC4_SHUFFLE_XZXZ);
		const vec4f src_y0_y1_y2_y3 = vec4f_shuffle(src_x0_y0_x1_y1, src_x2_y2_x3_y3, VEC4_SHUFFLE_YWYW);

		const vec4f dst_x0_x1_x2_x3 = vec4f_madd(m0, src_x0_x1_x2_x3, vec4f_madd(m2, src_y0_y1_y2_y3, m4));
		const vec4f dst_y0_y1_y2_y3 = vec4f_madd(m1, src_x0_x1_x2_x3, vec4f_madd(m3, src_y0_y1_y2_y3, m5));

		const vec4f dst_x0_x1_y0_y1 = vec4f_shuffle(dst_x0_x1_x2_x3, dst_y0_y1_y2_y3, VEC4_SHUFFLE_XYXY);
		const vec4f dst_x2_x3_y2_y3 = vec4f_shuffle(dst_x0_x1_x2_x3, dst_y0_y1_y2_y3, VEC4_SHUFFLE_ZWZW);

		const vec4f dst_x0_y0_x1_y1 = vec4f_shuffle(dst_x0_x1_y0_y1, dst_x0_x1_y0_y1, VEC4_SHUFFLE_XZYW);
		const vec4f dst_x2_y2_x3_y3 = vec4f_shuffle(dst_x2_x3_y2_y3, dst_x2_x3_y2_y3, VEC4_SHUFFLE_XZYW);

		vec4i_toInt4vu(vec4i_fromVec4f(dst_x0_y0_x1_y1), &dst[0]);
		vec4i_toInt4vu(vec4i_fromVec4f(dst_x2_y2_x3_y3), &dst[4]);

		src += 8;
		dst += 8;
	}

	const uint32_t rem = n & 3;
	switch (rem) {
	case 3: {
		dst[0] = (int32_t)(mtx[0] * src[0] + mtx[2] * src[1] + mtx[4]);
		dst[1] = (int32_t)(mtx[1] * src[0] + mtx[3] * src[1] + mtx[5]);
		src += 2;
		dst += 2;
	} // fallthrough
	case 2: {
		dst[0] = (int32_t)(mtx[0] * src[0] + mtx[2] * src[1] + mtx[4]);
		dst[1] = (int32_t)(mtx[1] * src[0] + mtx[3] * src[1] + mtx[5]);
		src += 2;
		dst += 2;
	} // fallthrough
	case 1: {
		dst[0] = (int32_t)(mtx[0] * src[0] + mtx[2] * src[1] + mtx[4]);
		dst[1] = (int32_t)(mtx[1] * src[0] + mtx[3] * src[1] + mtx[5]);
		src += 2;
		dst += 2;
	} // fallthrough
	case 0:
	default:
		break;
	}
}
#endif
