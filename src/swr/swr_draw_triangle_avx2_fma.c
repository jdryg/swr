#include "swr.h"
#include "swr_p.h"
#include "../core/math.h"

#define SWR_VEC_MATH_AVX2
#define SWR_VEC_MATH_FMA
#include "swr_vec_math.h"

#define SWR_CONFIG_TREAT_ALL_TILES_AS_PARTIAL  1
#define SWR_CONFIG_SMALL_TRIANGLE_OPTIMIZATION 1

typedef struct swr_edge
{
	int32_t m_x0;
	int32_t m_y0;
	int32_t m_dx;
	int32_t m_dy;
} swr_edge;

typedef struct swr_vertex_attrib_data
{
	vec8f m_Val2;
	vec8f m_dVal02;
	vec8f m_dVal12;
} swr_vertex_attrib_data;

static __forceinline swr_edge swr_edgeInit(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
	return (swr_edge)
	{
		.m_x0 = x0,
		.m_y0 = y0,
		.m_dx = (y1 - y0),
		.m_dy = (x0 - x1),
	};
}

static __forceinline int32_t swr_edgeEval(swr_edge edge, int32_t x, int32_t y)
{
	return 0
		+ (x - edge.m_x0) * edge.m_dx
		+ (y - edge.m_y0) * edge.m_dy
		;
}

static __forceinline swr_vertex_attrib_data swr_vertexAttribInit(float v2, float dv02, float dv12)
{
	return (swr_vertex_attrib_data)
	{
		.m_Val2 = vec8f_fromFloat(v2),
		.m_dVal02 = vec8f_fromFloat(dv02),
		.m_dVal12 = vec8f_fromFloat(dv12)
	};
}

static __forceinline vec8f swr_vertexAttribEval(swr_vertex_attrib_data va, vec8f w0, vec8f w1)
{
	return vec8f_madd(va.m_dVal02, w0, vec8f_madd(va.m_dVal12, w1, va.m_Val2));
}

typedef struct swr_tile
{
	vec8i v_edge_dx[3];
	vec8i v_edge_dy[3];
	vec8f v_inv_area;
	int32_t blockMin_w[3];
} swr_tile;

static __forceinline void swr_drawTile8x8_partial_aligned(swr_tile tile, swr_vertex_attrib_data va_r, swr_vertex_attrib_data va_g, swr_vertex_attrib_data va_b, swr_vertex_attrib_data va_a, uint32_t* tileFB, uint32_t rowStride)
{
	const vec8i v_w0_row0 = vec8i_add(vec8i_fromInt(tile.blockMin_w[0]), tile.v_edge_dx[0]);
	const vec8i v_w1_row0 = vec8i_add(vec8i_fromInt(tile.blockMin_w[1]), tile.v_edge_dx[1]);
	const vec8i v_w2_row0 = vec8i_add(vec8i_fromInt(tile.blockMin_w[2]), tile.v_edge_dx[2]);

	const vec8i v_w0_row1 = vec8i_add(v_w0_row0, tile.v_edge_dy[0]);
	const vec8i v_w1_row1 = vec8i_add(v_w1_row0, tile.v_edge_dy[1]);
	const vec8i v_w2_row1 = vec8i_add(v_w2_row0, tile.v_edge_dy[2]);
	const vec8i v_w0_row2 = vec8i_add(v_w0_row1, tile.v_edge_dy[0]);
	const vec8i v_w1_row2 = vec8i_add(v_w1_row1, tile.v_edge_dy[1]);
	const vec8i v_w2_row2 = vec8i_add(v_w2_row1, tile.v_edge_dy[2]);
	const vec8i v_w0_row3 = vec8i_add(v_w0_row2, tile.v_edge_dy[0]);
	const vec8i v_w1_row3 = vec8i_add(v_w1_row2, tile.v_edge_dy[1]);
	const vec8i v_w2_row3 = vec8i_add(v_w2_row2, tile.v_edge_dy[2]);
	const vec8i v_w0_row4 = vec8i_add(v_w0_row3, tile.v_edge_dy[0]);
	const vec8i v_w1_row4 = vec8i_add(v_w1_row3, tile.v_edge_dy[1]);
	const vec8i v_w2_row4 = vec8i_add(v_w2_row3, tile.v_edge_dy[2]);
	const vec8i v_w0_row5 = vec8i_add(v_w0_row4, tile.v_edge_dy[0]);
	const vec8i v_w1_row5 = vec8i_add(v_w1_row4, tile.v_edge_dy[1]);
	const vec8i v_w2_row5 = vec8i_add(v_w2_row4, tile.v_edge_dy[2]);
	const vec8i v_w0_row6 = vec8i_add(v_w0_row5, tile.v_edge_dy[0]);
	const vec8i v_w1_row6 = vec8i_add(v_w1_row5, tile.v_edge_dy[1]);
	const vec8i v_w2_row6 = vec8i_add(v_w2_row5, tile.v_edge_dy[2]);
	const vec8i v_w0_row7 = vec8i_add(v_w0_row6, tile.v_edge_dy[0]);
	const vec8i v_w1_row7 = vec8i_add(v_w1_row6, tile.v_edge_dy[1]);
	const vec8i v_w2_row7 = vec8i_add(v_w2_row6, tile.v_edge_dy[2]);

	// Calculate the (inverse) pixel mask.
	// If any of the barycentric coordinates is negative, the pixel mask will 
	// be equal to 0xFFFFFFFF for that pixel. This mask is used to replace
	// the existing framebuffer values and the new values.
	const vec8i v_w_row0_or = vec8i_or3(v_w0_row0, v_w1_row0, v_w2_row0);
	const vec8i v_w_row1_or = vec8i_or3(v_w0_row1, v_w1_row1, v_w2_row1);
	const vec8i v_w_row2_or = vec8i_or3(v_w0_row2, v_w1_row2, v_w2_row2);
	const vec8i v_w_row3_or = vec8i_or3(v_w0_row3, v_w1_row3, v_w2_row3);
	const vec8i v_w_row4_or = vec8i_or3(v_w0_row4, v_w1_row4, v_w2_row4);
	const vec8i v_w_row5_or = vec8i_or3(v_w0_row5, v_w1_row5, v_w2_row5);
	const vec8i v_w_row6_or = vec8i_or3(v_w0_row6, v_w1_row6, v_w2_row6);
	const vec8i v_w_row7_or = vec8i_or3(v_w0_row7, v_w1_row7, v_w2_row7);

#if SWR_CONFIG_DISABLE_PIXEL_SHADERS
	const vec8i v_rgba8 = vec8i_fromInt(-1);

	// Row 0
	if (!vec8i_allNegative(v_w_row0_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row0_or, 31);
		vec8i_toInt8va_maskedInv(v_rgba8, v_notPixelMask, &tileFB[0]);
	}

	// Row 1
	if (!vec8i_allNegative(v_w_row1_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row1_or, 31);
		vec8i_toInt8va_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride]);
	}

	// Row 2
	if (!vec8i_allNegative(v_w_row2_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row2_or, 31);
		vec8i_toInt8va_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 2]);
	}

	// Row 3
	if (!vec8i_allNegative(v_w_row3_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row3_or, 31);
		vec8i_toInt8va_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 3]);
	}

	// Row 4
	if (!vec8i_allNegative(v_w_row4_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row4_or, 31);
		vec8i_toInt8va_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 4]);
	}

	// Row 5
	if (!vec8i_allNegative(v_w_row5_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row5_or, 31);
		vec8i_toInt8va_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 5]);
	}

	// Row 6
	if (!vec8i_allNegative(v_w_row6_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row6_or, 31);
		vec8i_toInt8va_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 6]);
	}

	// Row 7
	if (!vec8i_allNegative(v_w_row7_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row7_or, 31);
		vec8i_toInt8va_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 7]);
	}
#else // SWR_CONFIG_DISABLE_PIXEL_SHADERS
	const vec8f v_inv_area = tile.v_inv_area;

	// Row 0
	if (!vec8i_allNegative(v_w_row0_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row0_or, 31);
		const vec8f v_l0 = vec8f_mul(vec8f_fromVec8i(v_w0_row0), v_inv_area);
		const vec8f v_l1 = vec8f_mul(vec8f_fromVec8i(v_w1_row0), v_inv_area);
		const vec8f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec8f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec8f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec8f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec8i v_rgba8 = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8va_maskedInv(v_rgba8, v_notPixelMask, &tileFB[0]);
	}

	// Row 1
	if (!vec8i_allNegative(v_w_row1_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row1_or, 31);
		const vec8f v_l0 = vec8f_mul(vec8f_fromVec8i(v_w0_row1), v_inv_area);
		const vec8f v_l1 = vec8f_mul(vec8f_fromVec8i(v_w1_row1), v_inv_area);
		const vec8f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec8f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec8f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec8f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec8i v_rgba8 = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8va_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride]);
	}

	// Row 2
	if (!vec8i_allNegative(v_w_row2_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row2_or, 31);
		const vec8f v_l0 = vec8f_mul(vec8f_fromVec8i(v_w0_row2), v_inv_area);
		const vec8f v_l1 = vec8f_mul(vec8f_fromVec8i(v_w1_row2), v_inv_area);
		const vec8f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec8f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec8f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec8f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec8i v_rgba8 = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8va_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 2]);
	}

	// Row 3
	if (!vec8i_allNegative(v_w_row3_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row3_or, 31);
		const vec8f v_l0 = vec8f_mul(vec8f_fromVec8i(v_w0_row3), v_inv_area);
		const vec8f v_l1 = vec8f_mul(vec8f_fromVec8i(v_w1_row3), v_inv_area);
		const vec8f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec8f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec8f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec8f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec8i v_rgba8 = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8va_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 3]);
	}

	// Row 4
	if (!vec8i_allNegative(v_w_row4_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row4_or, 31);
		const vec8f v_l0 = vec8f_mul(vec8f_fromVec8i(v_w0_row4), v_inv_area);
		const vec8f v_l1 = vec8f_mul(vec8f_fromVec8i(v_w1_row4), v_inv_area);
		const vec8f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec8f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec8f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec8f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec8i v_rgba8 = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8va_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 4]);
	}

	// Row 5
	if (!vec8i_allNegative(v_w_row5_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row5_or, 31);
		const vec8f v_l0 = vec8f_mul(vec8f_fromVec8i(v_w0_row5), v_inv_area);
		const vec8f v_l1 = vec8f_mul(vec8f_fromVec8i(v_w1_row5), v_inv_area);
		const vec8f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec8f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec8f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec8f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec8i v_rgba8 = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8va_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 5]);
	}

	// Row 6
	if (!vec8i_allNegative(v_w_row6_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row6_or, 31);
		const vec8f v_l0 = vec8f_mul(vec8f_fromVec8i(v_w0_row6), v_inv_area);
		const vec8f v_l1 = vec8f_mul(vec8f_fromVec8i(v_w1_row6), v_inv_area);
		const vec8f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec8f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec8f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec8f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec8i v_rgba8 = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8va_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 6]);
	}

	// Row 7
	if (!vec8i_allNegative(v_w_row7_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row7_or, 31);
		const vec8f v_l0 = vec8f_mul(vec8f_fromVec8i(v_w0_row7), v_inv_area);
		const vec8f v_l1 = vec8f_mul(vec8f_fromVec8i(v_w1_row7), v_inv_area);
		const vec8f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec8f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec8f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec8f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec8i v_rgba8 = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8va_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 7]);
	}
#endif // SWR_CONFIG_DISABLE_PIXEL_SHADERS
}

static __forceinline void swr_drawTile8x8_partial_unaligned(swr_tile tile, swr_vertex_attrib_data va_r, swr_vertex_attrib_data va_g, swr_vertex_attrib_data va_b, swr_vertex_attrib_data va_a, uint32_t* tileFB, uint32_t rowStride)
{
	const vec8i v_w0_row0 = vec8i_add(vec8i_fromInt(tile.blockMin_w[0]), tile.v_edge_dx[0]);
	const vec8i v_w1_row0 = vec8i_add(vec8i_fromInt(tile.blockMin_w[1]), tile.v_edge_dx[1]);
	const vec8i v_w2_row0 = vec8i_add(vec8i_fromInt(tile.blockMin_w[2]), tile.v_edge_dx[2]);

	const vec8i v_w0_row1 = vec8i_add(v_w0_row0, tile.v_edge_dy[0]);
	const vec8i v_w1_row1 = vec8i_add(v_w1_row0, tile.v_edge_dy[1]);
	const vec8i v_w2_row1 = vec8i_add(v_w2_row0, tile.v_edge_dy[2]);
	const vec8i v_w0_row2 = vec8i_add(v_w0_row1, tile.v_edge_dy[0]);
	const vec8i v_w1_row2 = vec8i_add(v_w1_row1, tile.v_edge_dy[1]);
	const vec8i v_w2_row2 = vec8i_add(v_w2_row1, tile.v_edge_dy[2]);
	const vec8i v_w0_row3 = vec8i_add(v_w0_row2, tile.v_edge_dy[0]);
	const vec8i v_w1_row3 = vec8i_add(v_w1_row2, tile.v_edge_dy[1]);
	const vec8i v_w2_row3 = vec8i_add(v_w2_row2, tile.v_edge_dy[2]);
	const vec8i v_w0_row4 = vec8i_add(v_w0_row3, tile.v_edge_dy[0]);
	const vec8i v_w1_row4 = vec8i_add(v_w1_row3, tile.v_edge_dy[1]);
	const vec8i v_w2_row4 = vec8i_add(v_w2_row3, tile.v_edge_dy[2]);
	const vec8i v_w0_row5 = vec8i_add(v_w0_row4, tile.v_edge_dy[0]);
	const vec8i v_w1_row5 = vec8i_add(v_w1_row4, tile.v_edge_dy[1]);
	const vec8i v_w2_row5 = vec8i_add(v_w2_row4, tile.v_edge_dy[2]);
	const vec8i v_w0_row6 = vec8i_add(v_w0_row5, tile.v_edge_dy[0]);
	const vec8i v_w1_row6 = vec8i_add(v_w1_row5, tile.v_edge_dy[1]);
	const vec8i v_w2_row6 = vec8i_add(v_w2_row5, tile.v_edge_dy[2]);
	const vec8i v_w0_row7 = vec8i_add(v_w0_row6, tile.v_edge_dy[0]);
	const vec8i v_w1_row7 = vec8i_add(v_w1_row6, tile.v_edge_dy[1]);
	const vec8i v_w2_row7 = vec8i_add(v_w2_row6, tile.v_edge_dy[2]);

	// Calculate the (inverse) pixel mask.
	// If any of the barycentric coordinates is negative, the pixel mask will 
	// be equal to 0xFFFFFFFF for that pixel. This mask is used to replace
	// the existing framebuffer values and the new values.
	const vec8i v_w_row0_or = vec8i_or3(v_w0_row0, v_w1_row0, v_w2_row0);
	const vec8i v_w_row1_or = vec8i_or3(v_w0_row1, v_w1_row1, v_w2_row1);
	const vec8i v_w_row2_or = vec8i_or3(v_w0_row2, v_w1_row2, v_w2_row2);
	const vec8i v_w_row3_or = vec8i_or3(v_w0_row3, v_w1_row3, v_w2_row3);
	const vec8i v_w_row4_or = vec8i_or3(v_w0_row4, v_w1_row4, v_w2_row4);
	const vec8i v_w_row5_or = vec8i_or3(v_w0_row5, v_w1_row5, v_w2_row5);
	const vec8i v_w_row6_or = vec8i_or3(v_w0_row6, v_w1_row6, v_w2_row6);
	const vec8i v_w_row7_or = vec8i_or3(v_w0_row7, v_w1_row7, v_w2_row7);

#if SWR_CONFIG_DISABLE_PIXEL_SHADERS
	const vec8i v_rgba8 = vec8i_fromInt(-1);

	// Row 0
	if (!vec8i_allNegative(v_w_row0_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row0_or, 31);
		vec8i_toInt8vu_maskedInv(v_rgba8, v_notPixelMask, &tileFB[0]);
	}

	// Row 1
	if (!vec8i_allNegative(v_w_row1_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row1_or, 31);
		vec8i_toInt8vu_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride]);
	}

	// Row 2
	if (!vec8i_allNegative(v_w_row2_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row2_or, 31);
		vec8i_toInt8vu_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 2]);
	}

	// Row 3
	if (!vec8i_allNegative(v_w_row3_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row3_or, 31);
		vec8i_toInt8vu_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 3]);
	}

	// Row 4
	if (!vec8i_allNegative(v_w_row4_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row4_or, 31);
		vec8i_toInt8vu_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 4]);
	}

	// Row 5
	if (!vec8i_allNegative(v_w_row5_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row5_or, 31);
		vec8i_toInt8vu_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 5]);
	}

	// Row 6
	if (!vec8i_allNegative(v_w_row6_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row6_or, 31);
		vec8i_toInt8vu_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 6]);
	}

	// Row 7
	if (!vec8i_allNegative(v_w_row7_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row7_or, 31);
		vec8i_toInt8vu_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 7]);
	}
#else // SWR_CONFIG_DISABLE_PIXEL_SHADERS
	const vec8f v_inv_area = tile.v_inv_area;

	// Row 0
	if (!vec8i_allNegative(v_w_row0_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row0_or, 31);
		const vec8f v_l0 = vec8f_mul(vec8f_fromVec8i(v_w0_row0), v_inv_area);
		const vec8f v_l1 = vec8f_mul(vec8f_fromVec8i(v_w1_row0), v_inv_area);
		const vec8f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec8f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec8f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec8f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec8i v_rgba8 = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8vu_maskedInv(v_rgba8, v_notPixelMask, &tileFB[0]);
	}

	// Row 1
	if (!vec8i_allNegative(v_w_row1_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row1_or, 31);
		const vec8f v_l0 = vec8f_mul(vec8f_fromVec8i(v_w0_row1), v_inv_area);
		const vec8f v_l1 = vec8f_mul(vec8f_fromVec8i(v_w1_row1), v_inv_area);
		const vec8f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec8f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec8f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec8f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec8i v_rgba8 = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8vu_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride]);
	}

	// Row 2
	if (!vec8i_allNegative(v_w_row2_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row2_or, 31);
		const vec8f v_l0 = vec8f_mul(vec8f_fromVec8i(v_w0_row2), v_inv_area);
		const vec8f v_l1 = vec8f_mul(vec8f_fromVec8i(v_w1_row2), v_inv_area);
		const vec8f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec8f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec8f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec8f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec8i v_rgba8 = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8vu_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 2]);
	}

	// Row 3
	if (!vec8i_allNegative(v_w_row3_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row3_or, 31);
		const vec8f v_l0 = vec8f_mul(vec8f_fromVec8i(v_w0_row3), v_inv_area);
		const vec8f v_l1 = vec8f_mul(vec8f_fromVec8i(v_w1_row3), v_inv_area);
		const vec8f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec8f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec8f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec8f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec8i v_rgba8 = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8vu_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 3]);
	}

	// Row 4
	if (!vec8i_allNegative(v_w_row4_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row4_or, 31);
		const vec8f v_l0 = vec8f_mul(vec8f_fromVec8i(v_w0_row4), v_inv_area);
		const vec8f v_l1 = vec8f_mul(vec8f_fromVec8i(v_w1_row4), v_inv_area);
		const vec8f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec8f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec8f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec8f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec8i v_rgba8 = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8vu_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 4]);
	}

	// Row 5
	if (!vec8i_allNegative(v_w_row5_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row5_or, 31);
		const vec8f v_l0 = vec8f_mul(vec8f_fromVec8i(v_w0_row5), v_inv_area);
		const vec8f v_l1 = vec8f_mul(vec8f_fromVec8i(v_w1_row5), v_inv_area);
		const vec8f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec8f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec8f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec8f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec8i v_rgba8 = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8vu_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 5]);
	}

	// Row 6
	if (!vec8i_allNegative(v_w_row6_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row6_or, 31);
		const vec8f v_l0 = vec8f_mul(vec8f_fromVec8i(v_w0_row6), v_inv_area);
		const vec8f v_l1 = vec8f_mul(vec8f_fromVec8i(v_w1_row6), v_inv_area);
		const vec8f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec8f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec8f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec8f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec8i v_rgba8 = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8vu_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 6]);
	}

	// Row 7
	if (!vec8i_allNegative(v_w_row7_or)) {
		const vec8i v_notPixelMask = vec8i_sar(v_w_row7_or, 31);
		const vec8f v_l0 = vec8f_mul(vec8f_fromVec8i(v_w0_row7), v_inv_area);
		const vec8f v_l1 = vec8f_mul(vec8f_fromVec8i(v_w1_row7), v_inv_area);
		const vec8f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec8f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec8f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec8f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec8i v_rgba8 = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8vu_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 7]);
	}
#endif // SWR_CONFIG_DISABLE_PIXEL_SHADERS
}

static __forceinline void swr_drawTile8x8_full(swr_tile tile, swr_vertex_attrib_data va_r, swr_vertex_attrib_data va_g, swr_vertex_attrib_data va_b, swr_vertex_attrib_data va_a, uint32_t* tileFB, uint32_t rowStride)
{
#if SWR_CONFIG_DISABLE_PIXEL_SHADERS
	const vec8i v_rgba8 = vec8i_fromInt(-1);
	vec8i_toInt8va(v_rgba8, &tileFB[0]);
	vec8i_toInt8va(v_rgba8, &tileFB[rowStride]);
	vec8i_toInt8va(v_rgba8, &tileFB[rowStride * 2]);
	vec8i_toInt8va(v_rgba8, &tileFB[rowStride * 3]);
	vec8i_toInt8va(v_rgba8, &tileFB[rowStride * 4]);
	vec8i_toInt8va(v_rgba8, &tileFB[rowStride * 5]);
	vec8i_toInt8va(v_rgba8, &tileFB[rowStride * 6]);
	vec8i_toInt8va(v_rgba8, &tileFB[rowStride * 7]);
#else // SWR_CONFIG_DISABLE_PIXEL_SHADERS
	const vec8i v_w0_row0 = vec8i_add(vec8i_fromInt(tile.blockMin_w[0]), tile.v_edge_dx[0]);
	const vec8i v_w1_row0 = vec8i_add(vec8i_fromInt(tile.blockMin_w[1]), tile.v_edge_dx[1]);
	const vec8i v_edge0_dy = tile.v_edge_dy[0];
	const vec8i v_edge1_dy = tile.v_edge_dy[1];
	const vec8f v_inv_area = tile.v_inv_area;

	const vec8i v_w0_row1 = vec8i_add(v_w0_row0, v_edge0_dy);
	const vec8i v_w1_row1 = vec8i_add(v_w1_row0, v_edge1_dy);
	const vec8i v_w0_row2 = vec8i_add(v_w0_row1, v_edge0_dy);
	const vec8i v_w1_row2 = vec8i_add(v_w1_row1, v_edge1_dy);
	const vec8i v_w0_row3 = vec8i_add(v_w0_row2, v_edge0_dy);
	const vec8i v_w1_row3 = vec8i_add(v_w1_row2, v_edge1_dy);
	const vec8i v_w0_row4 = vec8i_add(v_w0_row3, v_edge0_dy);
	const vec8i v_w1_row4 = vec8i_add(v_w1_row3, v_edge1_dy);
	const vec8i v_w0_row5 = vec8i_add(v_w0_row4, v_edge0_dy);
	const vec8i v_w1_row5 = vec8i_add(v_w1_row4, v_edge1_dy);
	const vec8i v_w0_row6 = vec8i_add(v_w0_row5, v_edge0_dy);
	const vec8i v_w1_row6 = vec8i_add(v_w1_row5, v_edge1_dy);
	const vec8i v_w0_row7 = vec8i_add(v_w0_row6, v_edge0_dy);
	const vec8i v_w1_row7 = vec8i_add(v_w1_row6, v_edge1_dy);

	// Row 0
	{
		const vec8f v_l0 = vec8f_mul(vec8f_fromVec8i(v_w0_row0), v_inv_area);
		const vec8f v_l1 = vec8f_mul(vec8f_fromVec8i(v_w1_row0), v_inv_area);
		const vec8f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec8f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec8f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec8f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec8i v_rgba8 = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8va(v_rgba8, &tileFB[0]);
	}

	// Row 1
	{
		const vec8f v_l0 = vec8f_mul(vec8f_fromVec8i(v_w0_row1), v_inv_area);
		const vec8f v_l1 = vec8f_mul(vec8f_fromVec8i(v_w1_row1), v_inv_area);
		const vec8f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec8f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec8f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec8f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec8i v_rgba8 = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8va(v_rgba8, &tileFB[rowStride]);
	}

	// Row 2
	{
		const vec8f v_l0 = vec8f_mul(vec8f_fromVec8i(v_w0_row2), v_inv_area);
		const vec8f v_l1 = vec8f_mul(vec8f_fromVec8i(v_w1_row2), v_inv_area);
		const vec8f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec8f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec8f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec8f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec8i v_rgba8 = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8va(v_rgba8, &tileFB[rowStride * 2]);
	}

	// Row 3
	{
		const vec8f v_l0 = vec8f_mul(vec8f_fromVec8i(v_w0_row3), v_inv_area);
		const vec8f v_l1 = vec8f_mul(vec8f_fromVec8i(v_w1_row3), v_inv_area);
		const vec8f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec8f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec8f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec8f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec8i v_rgba8 = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8va(v_rgba8, &tileFB[rowStride * 3]);
	}

	// Row 4
	{
		const vec8f v_l0 = vec8f_mul(vec8f_fromVec8i(v_w0_row4), v_inv_area);
		const vec8f v_l1 = vec8f_mul(vec8f_fromVec8i(v_w1_row4), v_inv_area);
		const vec8f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec8f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec8f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec8f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec8i v_rgba8 = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8va(v_rgba8, &tileFB[rowStride * 4]);
	}

	// Row 5
	{
		const vec8f v_l0 = vec8f_mul(vec8f_fromVec8i(v_w0_row5), v_inv_area);
		const vec8f v_l1 = vec8f_mul(vec8f_fromVec8i(v_w1_row5), v_inv_area);
		const vec8f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec8f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec8f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec8f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec8i v_rgba8 = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8va(v_rgba8, &tileFB[rowStride * 5]);
	}

	// Row 6
	{
		const vec8f v_l0 = vec8f_mul(vec8f_fromVec8i(v_w0_row6), v_inv_area);
		const vec8f v_l1 = vec8f_mul(vec8f_fromVec8i(v_w1_row6), v_inv_area);
		const vec8f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec8f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec8f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec8f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec8i v_rgba8 = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8va(v_rgba8, &tileFB[rowStride * 6]);
	}

	// Row 7
	{
		const vec8f v_l0 = vec8f_mul(vec8f_fromVec8i(v_w0_row7), v_inv_area);
		const vec8f v_l1 = vec8f_mul(vec8f_fromVec8i(v_w1_row7), v_inv_area);
		const vec8f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec8f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec8f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec8f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec8i v_rgba8 = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8va(v_rgba8, &tileFB[rowStride * 7]);
	}
#endif // SWR_CONFIG_DISABLE_PIXEL_SHADERS
}

void swrDrawTriangleAVX2_FMA(swr_context* ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color0, uint32_t color1, uint32_t color2)
{
	// Make sure the triangle is CCW. If it's not swap points 1 and 2 to make it CCW.
	int32_t iarea = (x0 - x2) * (y1 - y0) - (x1 - x0) * (y0 - y2);
	if (iarea == 0) {
		// Degenerate triangle with 0 area.
		return;
	} else if (iarea < 0) {
		// Swap (x1, y1) <-> (x2, y2)
		{ int32_t tmp = x1; x1 = x2; x2 = tmp; }
		{ int32_t tmp = y1; y1 = y2; y2 = tmp; }
		{ uint32_t tmp = color1; color1 = color2; color2 = tmp; }
		iarea = -iarea;
	}

	// Compute triangle bounding box
	const int32_t bboxMinX = core_maxi32(core_min3i32(x0, x1, x2), 0);
	const int32_t bboxMinY = core_maxi32(core_min3i32(y0, y1, y2), 0);
	const int32_t bboxMaxX = core_mini32(core_max3i32(x0, x1, x2), (int32_t)(ctx->m_Width - 1));
	const int32_t bboxMaxY = core_mini32(core_max3i32(y0, y1, y2), (int32_t)(ctx->m_Height - 1));
	const int32_t bboxWidth = bboxMaxX - bboxMinX;
	const int32_t bboxHeight = bboxMaxY - bboxMinY;
	if (bboxWidth <= 0 || bboxHeight <= 0) {
		return;
	}

	// Prepare interpolated attributes
	const vec4f v_c0 = vec4f_fromRGBA8(color0);
	const vec4f v_c1 = vec4f_fromRGBA8(color1);
	const vec4f v_c2 = vec4f_fromRGBA8(color2);
	const vec4f v_c02 = vec4f_sub(v_c0, v_c2);
	const vec4f v_c12 = vec4f_sub(v_c1, v_c2);

	const swr_vertex_attrib_data va_r = swr_vertexAttribInit(vec4f_getX(v_c2), vec4f_getX(v_c02), vec4f_getX(v_c12));
	const swr_vertex_attrib_data va_g = swr_vertexAttribInit(vec4f_getY(v_c2), vec4f_getY(v_c02), vec4f_getY(v_c12));
	const swr_vertex_attrib_data va_b = swr_vertexAttribInit(vec4f_getZ(v_c2), vec4f_getZ(v_c02), vec4f_getZ(v_c12));
	const swr_vertex_attrib_data va_a = swr_vertexAttribInit(vec4f_getW(v_c2), vec4f_getW(v_c02), vec4f_getW(v_c12));

	// Barycentric coordinate normalization
	const vec8f v_inv_area = vec8f_fromFloat(1.0f / (float)iarea);

	// Triangle setup
	const swr_edge edge0 = swr_edgeInit(x2, y2, x1, y1);
	const swr_edge edge1 = swr_edgeInit(x0, y0, x2, y2);
	const swr_edge edge2 = swr_edgeInit(x1, y1, x0, y0);

	const vec8i v_pixelOffsets = vec8i_fromInt8(0, 1, 2, 3, 4, 5, 6, 7);
	const vec8i v_edge0_dx = vec8i_mullo(vec8i_fromInt(edge0.m_dx), v_pixelOffsets);
	const vec8i v_edge1_dx = vec8i_mullo(vec8i_fromInt(edge1.m_dx), v_pixelOffsets);
	const vec8i v_edge2_dx = vec8i_mullo(vec8i_fromInt(edge2.m_dx), v_pixelOffsets);
	const vec8i v_edge0_dy = vec8i_fromInt(edge0.m_dy);
	const vec8i v_edge1_dy = vec8i_fromInt(edge1.m_dy);
	const vec8i v_edge2_dy = vec8i_fromInt(edge2.m_dy);

	swr_tile tile = {
		.blockMin_w = { 0, 0, 0 },
		.v_edge_dx = { v_edge0_dx, v_edge1_dx, v_edge2_dx },
		.v_edge_dy = { v_edge0_dy, v_edge1_dy, v_edge2_dy },
		.v_inv_area = v_inv_area
	};

#if SWR_CONFIG_SMALL_TRIANGLE_OPTIMIZATION
	if (bboxWidth <= 8 && bboxHeight <= 8) {
		const int32_t tileX = (bboxMinX + 8 >= (int32_t)ctx->m_Width)
			? ctx->m_Width - 8
			: bboxMinX
			;

		const int32_t tileY = (bboxMinY + 8 >= (int32_t)ctx->m_Height)
			? ctx->m_Height - 8
			: bboxMinY
			;
		tile.blockMin_w[0] = swr_edgeEval(edge0, tileX, tileY);
		tile.blockMin_w[1] = swr_edgeEval(edge1, tileX, tileY);
		tile.blockMin_w[2] = swr_edgeEval(edge2, tileX, tileY);
		swr_drawTile8x8_partial_unaligned(tile, va_r, va_g, va_b, va_a, &ctx->m_FrameBuffer[tileX + tileY * ctx->m_Width], ctx->m_Width);
		return;
	}
#endif

	const int32_t bboxMinX_aligned = core_roundDown(bboxMinX, 64);
	const int32_t bboxMinY_aligned = core_roundDown(bboxMinY, 8);
	const int32_t bboxMaxX_aligned = core_roundUp(bboxMaxX, 64);
	const int32_t bboxMaxY_aligned = core_roundUp(bboxMaxY, 8);

	// Trivial reject/accept corner offsets relative to block min/max.
	const vec4i v_blockSize_m1 = vec4i_fromInt(8 - 1);
	const vec4i v_edge_dx = vec4i_fromInt4(edge0.m_dx, edge1.m_dx, edge2.m_dx, 0);
	const vec4i v_edge_dy = vec4i_fromInt4(edge0.m_dy, edge1.m_dy, edge2.m_dy, 0);
	const vec4i v_w_blockMax_dx = vec4i_mullo(v_edge_dx, v_blockSize_m1);
	const vec4i v_w_blockMax_dy = vec4i_mullo(v_edge_dy, v_blockSize_m1);

	const vec4i v_zero = vec4i_zero();
	const vec4i v_edge_dx_lt = vec4i_cmplt(v_edge_dx, v_zero);
	const vec4i v_edge_dy_lt = vec4i_cmplt(v_edge_dy, v_zero);

	const vec4i v_trivialRejectOffset = vec4i_add(
		vec4i_andnot(v_edge_dx_lt, v_w_blockMax_dx),
		vec4i_andnot(v_edge_dy_lt, v_w_blockMax_dy)
	);
	const vec8i v_trivialRejectOffset_0 = vec8i_fromInt(vec4i_getX(v_trivialRejectOffset));
	const vec8i v_trivialRejectOffset_1 = vec8i_fromInt(vec4i_getY(v_trivialRejectOffset));
	const vec8i v_trivialRejectOffset_2 = vec8i_fromInt(vec4i_getZ(v_trivialRejectOffset));

	const vec4i v_trivialAcceptOffset = vec4i_sub(vec4i_add(v_w_blockMax_dx, v_w_blockMax_dy), v_trivialRejectOffset);
	const vec8i v_trivialAcceptOffset_0 = vec8i_fromInt(vec4i_getX(v_trivialAcceptOffset));
	const vec8i v_trivialAcceptOffset_1 = vec8i_fromInt(vec4i_getY(v_trivialAcceptOffset));
	const vec8i v_trivialAcceptOffset_2 = vec8i_fromInt(vec4i_getZ(v_trivialAcceptOffset));

	// Rasterize
	const vec8i v_w0_bboxMin = vec8i_fromInt(swr_edgeEval(edge0, bboxMinX_aligned, bboxMinY_aligned));
	const vec8i v_w1_bboxMin = vec8i_fromInt(swr_edgeEval(edge1, bboxMinX_aligned, bboxMinY_aligned));
	const vec8i v_w2_bboxMin = vec8i_fromInt(swr_edgeEval(edge2, bboxMinX_aligned, bboxMinY_aligned));

	const vec8i v_w0_nextBlock_dx = vec8i_fromInt(edge0.m_dx * 64);
	const vec8i v_w1_nextBlock_dx = vec8i_fromInt(edge1.m_dx * 64);
	const vec8i v_w2_nextBlock_dx = vec8i_fromInt(edge2.m_dx * 64);
	const vec8i v_w0_nextBlock_dy = vec8i_fromInt(edge0.m_dy * 8);
	const vec8i v_w1_nextBlock_dy = vec8i_fromInt(edge1.m_dy * 8);
	const vec8i v_w2_nextBlock_dy = vec8i_fromInt(edge2.m_dy * 8);

	const vec8i v_blockOffsets = vec8i_fromInt8(0, 8, 16, 24, 32, 40, 48, 56);
	vec8i v_w0_blockY = vec8i_add(v_w0_bboxMin, vec8i_mullo(vec8i_fromInt(edge0.m_dx), v_blockOffsets));
	vec8i v_w1_blockY = vec8i_add(v_w1_bboxMin, vec8i_mullo(vec8i_fromInt(edge1.m_dx), v_blockOffsets));
	vec8i v_w2_blockY = vec8i_add(v_w2_bboxMin, vec8i_mullo(vec8i_fromInt(edge2.m_dx), v_blockOffsets));

	for (int32_t blockMinY = bboxMinY_aligned; blockMinY < bboxMaxY_aligned; blockMinY += 8) {
		uint32_t* fb_blockY = &ctx->m_FrameBuffer[blockMinY * ctx->m_Width];
		vec8i v_w0_blockMin = v_w0_blockY;
		vec8i v_w1_blockMin = v_w1_blockY;
		vec8i v_w2_blockMin = v_w2_blockY;

		for (int32_t blockMinX = bboxMinX_aligned; blockMinX < bboxMaxX_aligned; blockMinX += 64) {
			// Evaluate each edge function at its trivial reject corner (the most positive block corner).
			// If the trivial rejct corner of any edge is negative (outside the edge) then the triangle 
			// does not touch the block.
			const vec8i v_w0_trivialReject = vec8i_add(v_w0_blockMin, v_trivialRejectOffset_0);
			const vec8i v_w1_trivialReject = vec8i_add(v_w1_blockMin, v_trivialRejectOffset_1);
			const vec8i v_w2_trivialReject = vec8i_add(v_w2_blockMin, v_trivialRejectOffset_2);
			const vec8i v_w_trivialReject = vec8i_or3(v_w0_trivialReject, v_w1_trivialReject, v_w2_trivialReject);
			uint32_t trivialRejectBlockMask = ~vec8i_getSignMask(v_w_trivialReject) & 0xFF;
			if (trivialRejectBlockMask == 0) {
				v_w0_blockMin = vec8i_add(v_w0_blockMin, v_w0_nextBlock_dx);
				v_w1_blockMin = vec8i_add(v_w1_blockMin, v_w1_nextBlock_dx);
				v_w2_blockMin = vec8i_add(v_w2_blockMin, v_w2_nextBlock_dx);
				continue;
			}

			// Evaluate each edge function at its trivial accept corner (the most negative block corner).
			// If the trivial accept corner of all edges is positive (inside the edge) then the triangle
			// fully covers the block.
			const vec8i v_w0_trivialAccept = vec8i_add(v_w0_blockMin, v_trivialAcceptOffset_0);
			const vec8i v_w1_trivialAccept = vec8i_add(v_w1_blockMin, v_trivialAcceptOffset_1);
			const vec8i v_w2_trivialAccept = vec8i_add(v_w2_blockMin, v_trivialAcceptOffset_2);
			const vec8i v_w_trivialAccept = vec8i_or3(v_w0_trivialAccept, v_w1_trivialAccept, v_w2_trivialAccept);
			uint32_t trivialAcceptBlockMask = vec8i_getSignMask(v_w_trivialAccept);

			int32_t w0_blockMin[8], w1_blockMin[8], w2_blockMin[8];
			vec8i_toInt8va(v_w0_blockMin, &w0_blockMin[0]);
			vec8i_toInt8va(v_w1_blockMin, &w1_blockMin[0]);
			vec8i_toInt8va(v_w2_blockMin, &w2_blockMin[0]);

			for (uint32_t iBlock = 0; trivialRejectBlockMask != 0;
				++iBlock, trivialRejectBlockMask >>= 1, trivialAcceptBlockMask >>= 1) {
				if ((trivialRejectBlockMask & 1) == 0) {
					continue;
				}

				tile.blockMin_w[0] = w0_blockMin[iBlock];
				tile.blockMin_w[1] = w1_blockMin[iBlock];
				tile.blockMin_w[2] = w2_blockMin[iBlock];
#if SWR_CONFIG_TREAT_ALL_TILES_AS_PARTIAL
				swr_drawTile8x8_partial_aligned(tile, va_r, va_g, va_b, va_a, &fb_blockY[blockMinX + iBlock * 8], ctx->m_Width);
#else
				if ((trivialAcceptBlockMask & 1) != 0) {
					swr_drawTile8x8_partial_aligned(tile, va_r, va_g, va_b, va_a, &fb_blockY[blockMinX + iBlock * 8], ctx->m_Width);
				} else {
					swr_drawTile8x8_full(tile, va_r, va_g, va_b, va_a, &fb_blockY[blockMinX + iBlock * 8], ctx->m_Width);
				}
#endif
			}

			v_w0_blockMin = vec8i_add(v_w0_blockMin, v_w0_nextBlock_dx);
			v_w1_blockMin = vec8i_add(v_w1_blockMin, v_w1_nextBlock_dx);
			v_w2_blockMin = vec8i_add(v_w2_blockMin, v_w2_nextBlock_dx);
		}

		v_w0_blockY = vec8i_add(v_w0_blockY, v_w0_nextBlock_dy);
		v_w1_blockY = vec8i_add(v_w1_blockY, v_w1_nextBlock_dy);
		v_w2_blockY = vec8i_add(v_w2_blockY, v_w2_nextBlock_dy);
	}
}
