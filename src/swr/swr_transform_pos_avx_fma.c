#include "swr.h"
#include "swr_p.h"

#define SWR_VEC_MATH_AVX
#define SWR_VEC_MATH_FMA
#include "swr_vec_math.h"

void swrTransformPos2fTo2iAVX_FMA(uint32_t n, const float* posf, int32_t* posi, const float* mtx)
{
	const float* src = posf;
	int32_t* dst = posi;

	const vec8f m01 = vec8f_fromFloat8(mtx[0], mtx[1], mtx[0], mtx[1], mtx[0], mtx[1], mtx[0], mtx[1]);
	const vec8f m23 = vec8f_fromFloat8(mtx[2], mtx[3], mtx[2], mtx[3], mtx[2], mtx[3], mtx[2], mtx[3]);
	const vec8f m45 = vec8f_fromFloat8(mtx[4], mtx[5], mtx[4], mtx[5], mtx[4], mtx[5], mtx[4], mtx[5]);

	const uint32_t numIter = n >> 3;
	for (uint32_t i = 0; i < numIter; ++i) {
		const vec8f src_xy0_xy1_xy2_xy3 = vec8f_fromFloat8vu(&src[0]);
		const vec8f src_xy4_xy5_xy6_xy7 = vec8f_fromFloat8vu(&src[8]);

		const vec8f src_x00_x11_x22_x33 = vec8f_permute(src_xy0_xy1_xy2_xy3, VEC4_SHUFFLE_XXZZ);
		const vec8f src_y00_y11_y22_y33 = vec8f_permute(src_xy0_xy1_xy2_xy3, VEC4_SHUFFLE_YYWW);
		const vec8f src_x44_x55_x66_x77 = vec8f_permute(src_xy4_xy5_xy6_xy7, VEC4_SHUFFLE_XXZZ);
		const vec8f src_y44_y55_y66_y77 = vec8f_permute(src_xy4_xy5_xy6_xy7, VEC4_SHUFFLE_YYWW);

		const vec8f dst_xy0_xy1_xy2_xy3 = vec8f_madd(src_x00_x11_x22_x33, m01, vec8f_madd(src_y00_y11_y22_y33, m23, m45));
		const vec8f dst_xy4_xy5_xy6_xy7 = vec8f_madd(src_x44_x55_x66_x77, m01, vec8f_madd(src_y44_y55_y66_y77, m23, m45));

		vec8i_toInt8vu(vec8i_fromVec8f(dst_xy0_xy1_xy2_xy3), &dst[0]);
		vec8i_toInt8vu(vec8i_fromVec8f(dst_xy4_xy5_xy6_xy7), &dst[8]);

		src += 16;
		dst += 16;
	}

	uint32_t rem = n & 7;
	if (rem >= 4) {
		const vec8f src_xy0_xy1_xy2_xy3 = vec8f_fromFloat8vu(&src[0]);

		const vec8f src_x00_x11_x22_x33 = vec8f_permute(src_xy0_xy1_xy2_xy3, VEC4_SHUFFLE_XXZZ);
		const vec8f src_y00_y11_y22_y33 = vec8f_permute(src_xy0_xy1_xy2_xy3, VEC4_SHUFFLE_YYWW);

		const vec8f dst_xy0_xy1_xy2_xy3 = vec8f_madd(src_x00_x11_x22_x33, m01, vec8f_madd(src_y00_y11_y22_y33, m23, m45));

		vec8i_toInt8vu(vec8i_fromVec8f(dst_xy0_xy1_xy2_xy3), &dst[0]);

		src += 8;
		dst += 8;
		rem -= 4;
	}

	if (rem >= 2) {
		const vec4f src_x0_y0_x1_y1 = vec4f_fromFloat4vu(&src[0]);

		const vec4f src_x0_x0_x1_x1 = vec4f_shuffle(src_x0_y0_x1_y1, src_x0_y0_x1_y1, VEC4_SHUFFLE_XXZZ);
		const vec4f src_y0_y0_y1_y1 = vec4f_shuffle(src_x0_y0_x1_y1, src_x0_y0_x1_y1, VEC4_SHUFFLE_YYWW);

		const vec4f dst_x0_y0_x1_y1 = vec4f_madd(src_x0_x0_x1_x1, vec4f_fromVec8f_low(m01), vec4f_madd(src_y0_y0_y1_y1, vec4f_fromVec8f_low(m23), vec4f_fromVec8f_low(m45)));

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
