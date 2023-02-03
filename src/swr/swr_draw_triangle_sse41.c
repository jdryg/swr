#include "swr.h"
#include "swr_p.h"
#include "../core/math.h"

#if 1
#define SWR_VEC_MATH_SSE41
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
	vec4f m_Val2;
	vec4f m_dVal02;
	vec4f m_dVal12;
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

static __forceinline swr_vertex_attrib_data swr_vertexAttribInit(vec4f v2, vec4f dv02, vec4f dv12)
{
	return (swr_vertex_attrib_data){
		.m_Val2 = v2,
		.m_dVal02 = dv02,
		.m_dVal12 = dv12
	};
}

static __forceinline vec4f swr_vertexAttribEval(swr_vertex_attrib_data va, vec4f w0, vec4f w1)
{
	return vec4f_madd(va.m_dVal02, w0, vec4f_madd(va.m_dVal12, w1, va.m_Val2));
}

typedef struct swr_tile
{
	vec4i v_edge_dx0123[3];
	vec4i v_edge_dy[3];
	vec4f v_inv_area;
	int32_t blockMin_w[3];
} swr_tile;

static __forceinline void swr_drawTile4x4_partial_aligned(swr_tile tile, swr_vertex_attrib_data va_r, swr_vertex_attrib_data va_g, swr_vertex_attrib_data va_b, swr_vertex_attrib_data va_a, uint32_t* tileFB, uint32_t rowStride)
{
	const vec4i v_w0_row0 = vec4i_add(vec4i_fromInt(tile.blockMin_w[0]), tile.v_edge_dx0123[0]);
	const vec4i v_w1_row0 = vec4i_add(vec4i_fromInt(tile.blockMin_w[1]), tile.v_edge_dx0123[1]);
	const vec4i v_w2_row0 = vec4i_add(vec4i_fromInt(tile.blockMin_w[2]), tile.v_edge_dx0123[2]);

	const vec4i v_w0_row1 = vec4i_add(v_w0_row0, tile.v_edge_dy[0]);
	const vec4i v_w1_row1 = vec4i_add(v_w1_row0, tile.v_edge_dy[1]);
	const vec4i v_w2_row1 = vec4i_add(v_w2_row0, tile.v_edge_dy[2]);
	const vec4i v_w0_row2 = vec4i_add(v_w0_row1, tile.v_edge_dy[0]);
	const vec4i v_w1_row2 = vec4i_add(v_w1_row1, tile.v_edge_dy[1]);
	const vec4i v_w2_row2 = vec4i_add(v_w2_row1, tile.v_edge_dy[2]);
	const vec4i v_w0_row3 = vec4i_add(v_w0_row2, tile.v_edge_dy[0]);
	const vec4i v_w1_row3 = vec4i_add(v_w1_row2, tile.v_edge_dy[1]);
	const vec4i v_w2_row3 = vec4i_add(v_w2_row2, tile.v_edge_dy[2]);

	// Calculate the (inverse) pixel mask.
	// If any of the barycentric coordinates is negative, the pixel mask will 
	// be equal to 0xFFFFFFFF for that pixel. This mask is used to replace
	// the existing framebuffer values and the new values.
	const vec4i v_w_row0_or = vec4i_or3(v_w0_row0, v_w1_row0, v_w2_row0);
	const vec4i v_w_row1_or = vec4i_or3(v_w0_row1, v_w1_row1, v_w2_row1);
	const vec4i v_w_row2_or = vec4i_or3(v_w0_row2, v_w1_row2, v_w2_row2);
	const vec4i v_w_row3_or = vec4i_or3(v_w0_row3, v_w1_row3, v_w2_row3);

#if SWR_CONFIG_DISABLE_PIXEL_SHADERS
	const vec4i v_rgba8 = vec4i_fromInt(-1);

	// Row 0
	if (!vec4i_allNegative(v_w_row0_or)) {
		const vec4i v_notPixelMask = vec4i_sar(v_w_row0_or, 31);
		vec4i_toInt4va_maskedInv(v_rgba8, v_notPixelMask, &tileFB[0]);
	}

	// Row 1
	if (!vec4i_allNegative(v_w_row1_or)) {
		const vec4i v_notPixelMask = vec4i_sar(v_w_row1_or, 31);
		vec4i_toInt4va_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride]);
	}

	// Row 2
	if (!vec4i_allNegative(v_w_row2_or)) {
		const vec4i v_notPixelMask = vec4i_sar(v_w_row2_or, 31);
		vec4i_toInt4va_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 2]);
	}

	// Row 3
	if (!vec4i_allNegative(v_w_row3_or)) {
		const vec4i v_notPixelMask = vec4i_sar(v_w_row3_or, 31);
		vec4i_toInt4va_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 3]);
	}
#else // SWR_CONFIG_DISABLE_PIXEL_SHADERS
	const vec4f v_inv_area = tile.v_inv_area;

	// Row 0
	if (!vec4i_allNegative(v_w_row0_or)) {
		const vec4i v_notPixelMask = vec4i_sar(v_w_row0_or, 31);
		const vec4f v_l0 = vec4f_mul(vec4f_fromVec4i(v_w0_row0), v_inv_area);
		const vec4f v_l1 = vec4f_mul(vec4f_fromVec4i(v_w1_row0), v_inv_area);
		const vec4f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec4f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec4f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec4f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec4i v_rgba8 = vec4i_packR32G32B32A32_to_RGBA8(vec4i_fromVec4f(v_cr), vec4i_fromVec4f(v_cg), vec4i_fromVec4f(v_cb), vec4i_fromVec4f(v_ca));
		vec4i_toInt4va_maskedInv(v_rgba8, v_notPixelMask, &tileFB[0]);
	}

	// Row 1
	if (!vec4i_allNegative(v_w_row1_or)) {
		const vec4i v_notPixelMask = vec4i_sar(v_w_row1_or, 31);
		const vec4f v_l0 = vec4f_mul(vec4f_fromVec4i(v_w0_row1), v_inv_area);
		const vec4f v_l1 = vec4f_mul(vec4f_fromVec4i(v_w1_row1), v_inv_area);
		const vec4f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec4f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec4f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec4f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec4i v_rgba8 = vec4i_packR32G32B32A32_to_RGBA8(vec4i_fromVec4f(v_cr), vec4i_fromVec4f(v_cg), vec4i_fromVec4f(v_cb), vec4i_fromVec4f(v_ca));
		vec4i_toInt4va_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride]);
	}

	// Row 2
	if (!vec4i_allNegative(v_w_row2_or)) {
		const vec4i v_notPixelMask = vec4i_sar(v_w_row2_or, 31);
		const vec4f v_l0 = vec4f_mul(vec4f_fromVec4i(v_w0_row2), v_inv_area);
		const vec4f v_l1 = vec4f_mul(vec4f_fromVec4i(v_w1_row2), v_inv_area);
		const vec4f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec4f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec4f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec4f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec4i v_rgba8 = vec4i_packR32G32B32A32_to_RGBA8(vec4i_fromVec4f(v_cr), vec4i_fromVec4f(v_cg), vec4i_fromVec4f(v_cb), vec4i_fromVec4f(v_ca));
		vec4i_toInt4va_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 2]);
}

	// Row 3
	if (!vec4i_allNegative(v_w_row3_or)) {
		const vec4i v_notPixelMask = vec4i_sar(v_w_row3_or, 31);
		const vec4f v_l0 = vec4f_mul(vec4f_fromVec4i(v_w0_row3), v_inv_area);
		const vec4f v_l1 = vec4f_mul(vec4f_fromVec4i(v_w1_row3), v_inv_area);
		const vec4f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec4f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec4f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec4f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec4i v_rgba8 = vec4i_packR32G32B32A32_to_RGBA8(vec4i_fromVec4f(v_cr), vec4i_fromVec4f(v_cg), vec4i_fromVec4f(v_cb), vec4i_fromVec4f(v_ca));
		vec4i_toInt4va_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 3]);
	}
#endif // SWR_CONFIG_DISABLE_PIXEL_SHADERS
}

static __forceinline void swr_drawTile4x4_partial_unaligned(swr_tile tile, swr_vertex_attrib_data va_r, swr_vertex_attrib_data va_g, swr_vertex_attrib_data va_b, swr_vertex_attrib_data va_a, uint32_t* tileFB, uint32_t rowStride)
{
	const vec4i v_w0_row0 = vec4i_add(vec4i_fromInt(tile.blockMin_w[0]), tile.v_edge_dx0123[0]);
	const vec4i v_w1_row0 = vec4i_add(vec4i_fromInt(tile.blockMin_w[1]), tile.v_edge_dx0123[1]);
	const vec4i v_w2_row0 = vec4i_add(vec4i_fromInt(tile.blockMin_w[2]), tile.v_edge_dx0123[2]);

	const vec4i v_w0_row1 = vec4i_add(v_w0_row0, tile.v_edge_dy[0]);
	const vec4i v_w1_row1 = vec4i_add(v_w1_row0, tile.v_edge_dy[1]);
	const vec4i v_w2_row1 = vec4i_add(v_w2_row0, tile.v_edge_dy[2]);
	const vec4i v_w0_row2 = vec4i_add(v_w0_row1, tile.v_edge_dy[0]);
	const vec4i v_w1_row2 = vec4i_add(v_w1_row1, tile.v_edge_dy[1]);
	const vec4i v_w2_row2 = vec4i_add(v_w2_row1, tile.v_edge_dy[2]);
	const vec4i v_w0_row3 = vec4i_add(v_w0_row2, tile.v_edge_dy[0]);
	const vec4i v_w1_row3 = vec4i_add(v_w1_row2, tile.v_edge_dy[1]);
	const vec4i v_w2_row3 = vec4i_add(v_w2_row2, tile.v_edge_dy[2]);

	// Calculate the (inverse) pixel mask.
	// If any of the barycentric coordinates is negative, the pixel mask will 
	// be equal to 0xFFFFFFFF for that pixel. This mask is used to replace
	// the existing framebuffer values and the new values.
	const vec4i v_w_row0_or = vec4i_or3(v_w0_row0, v_w1_row0, v_w2_row0);
	const vec4i v_w_row1_or = vec4i_or3(v_w0_row1, v_w1_row1, v_w2_row1);
	const vec4i v_w_row2_or = vec4i_or3(v_w0_row2, v_w1_row2, v_w2_row2);
	const vec4i v_w_row3_or = vec4i_or3(v_w0_row3, v_w1_row3, v_w2_row3);

#if SWR_CONFIG_DISABLE_PIXEL_SHADERS
	const vec4i v_rgba8 = vec4i_fromInt(-1);

	// Row 0
	if (!vec4i_allNegative(v_w_row0_or)) {
		const vec4i v_notPixelMask = vec4i_sar(v_w_row0_or, 31);
		vec4i_toInt4vu_maskedInv(v_rgba8, v_notPixelMask, &tileFB[0]);
	}

	// Row 1
	if (!vec4i_allNegative(v_w_row1_or)) {
		const vec4i v_notPixelMask = vec4i_sar(v_w_row1_or, 31);
		vec4i_toInt4vu_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride]);
	}

	// Row 2
	if (!vec4i_allNegative(v_w_row2_or)) {
		const vec4i v_notPixelMask = vec4i_sar(v_w_row2_or, 31);
		vec4i_toInt4vu_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 2]);
	}

	// Row 3
	if (!vec4i_allNegative(v_w_row3_or)) {
		const vec4i v_notPixelMask = vec4i_sar(v_w_row3_or, 31);
		vec4i_toInt4vu_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 3]);
	}
#else // SWR_CONFIG_DISABLE_PIXEL_SHADERS
	const vec4f v_inv_area = tile.v_inv_area;

	// Row 0
	if (!vec4i_allNegative(v_w_row0_or)) {
		const vec4i v_notPixelMask = vec4i_sar(v_w_row0_or, 31);
		const vec4f v_l0 = vec4f_mul(vec4f_fromVec4i(v_w0_row0), v_inv_area);
		const vec4f v_l1 = vec4f_mul(vec4f_fromVec4i(v_w1_row0), v_inv_area);
		const vec4f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec4f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec4f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec4f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec4i v_rgba8 = vec4i_packR32G32B32A32_to_RGBA8(vec4i_fromVec4f(v_cr), vec4i_fromVec4f(v_cg), vec4i_fromVec4f(v_cb), vec4i_fromVec4f(v_ca));
		vec4i_toInt4vu_maskedInv(v_rgba8, v_notPixelMask, &tileFB[0]);
	}

	// Row 1
	if (!vec4i_allNegative(v_w_row1_or)) {
		const vec4i v_notPixelMask = vec4i_sar(v_w_row1_or, 31);
		const vec4f v_l0 = vec4f_mul(vec4f_fromVec4i(v_w0_row1), v_inv_area);
		const vec4f v_l1 = vec4f_mul(vec4f_fromVec4i(v_w1_row1), v_inv_area);
		const vec4f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec4f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec4f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec4f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec4i v_rgba8 = vec4i_packR32G32B32A32_to_RGBA8(vec4i_fromVec4f(v_cr), vec4i_fromVec4f(v_cg), vec4i_fromVec4f(v_cb), vec4i_fromVec4f(v_ca));
		vec4i_toInt4vu_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride]);
	}

	// Row 2
	if (!vec4i_allNegative(v_w_row2_or)) {
		const vec4i v_notPixelMask = vec4i_sar(v_w_row2_or, 31);
		const vec4f v_l0 = vec4f_mul(vec4f_fromVec4i(v_w0_row2), v_inv_area);
		const vec4f v_l1 = vec4f_mul(vec4f_fromVec4i(v_w1_row2), v_inv_area);
		const vec4f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec4f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec4f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec4f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec4i v_rgba8 = vec4i_packR32G32B32A32_to_RGBA8(vec4i_fromVec4f(v_cr), vec4i_fromVec4f(v_cg), vec4i_fromVec4f(v_cb), vec4i_fromVec4f(v_ca));
		vec4i_toInt4vu_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 2]);
	}

	// Row 3
	if (!vec4i_allNegative(v_w_row3_or)) {
		const vec4i v_notPixelMask = vec4i_sar(v_w_row3_or, 31);
		const vec4f v_l0 = vec4f_mul(vec4f_fromVec4i(v_w0_row3), v_inv_area);
		const vec4f v_l1 = vec4f_mul(vec4f_fromVec4i(v_w1_row3), v_inv_area);
		const vec4f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec4f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec4f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec4f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec4i v_rgba8 = vec4i_packR32G32B32A32_to_RGBA8(vec4i_fromVec4f(v_cr), vec4i_fromVec4f(v_cg), vec4i_fromVec4f(v_cb), vec4i_fromVec4f(v_ca));
		vec4i_toInt4vu_maskedInv(v_rgba8, v_notPixelMask, &tileFB[rowStride * 3]);
	}
#endif // SWR_CONFIG_DISABLE_PIXEL_SHADERS
}

static __forceinline void swr_drawTile4x4_full(swr_tile tile, swr_vertex_attrib_data va_r, swr_vertex_attrib_data va_g, swr_vertex_attrib_data va_b, swr_vertex_attrib_data va_a, uint32_t* tileFB, uint32_t rowStride)
{
#if SWR_CONFIG_DISABLE_PIXEL_SHADERS
	const vec4i v_rgba8 = vec4i_fromInt(-1);
	vec4i_toInt4va(v_rgba8, &tileFB[0]);
	vec4i_toInt4va(v_rgba8, &tileFB[rowStride]);
	vec4i_toInt4va(v_rgba8, &tileFB[rowStride * 2]);
	vec4i_toInt4va(v_rgba8, &tileFB[rowStride * 3]);
#else // SWR_CONFIG_DISABLE_PIXEL_SHADERS
	const vec4i v_w0_row0 = vec4i_add(vec4i_fromInt(tile.blockMin_w[0]), tile.v_edge_dx0123[0]);
	const vec4i v_w1_row0 = vec4i_add(vec4i_fromInt(tile.blockMin_w[1]), tile.v_edge_dx0123[1]);
	const vec4i v_edge0_dy = tile.v_edge_dy[0];
	const vec4i v_edge1_dy = tile.v_edge_dy[1];
	const vec4f v_inv_area = tile.v_inv_area;

	const vec4i v_w0_row1 = vec4i_add(v_w0_row0, v_edge0_dy);
	const vec4i v_w1_row1 = vec4i_add(v_w1_row0, v_edge1_dy);
	const vec4i v_w0_row2 = vec4i_add(v_w0_row1, v_edge0_dy);
	const vec4i v_w1_row2 = vec4i_add(v_w1_row1, v_edge1_dy);
	const vec4i v_w0_row3 = vec4i_add(v_w0_row2, v_edge0_dy);
	const vec4i v_w1_row3 = vec4i_add(v_w1_row2, v_edge1_dy);

	// Row 0
	{
		const vec4f v_l0 = vec4f_mul(vec4f_fromVec4i(v_w0_row0), v_inv_area);
		const vec4f v_l1 = vec4f_mul(vec4f_fromVec4i(v_w1_row0), v_inv_area);
		const vec4f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec4f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec4f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec4f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec4i v_rgba8 = vec4i_packR32G32B32A32_to_RGBA8(vec4i_fromVec4f(v_cr), vec4i_fromVec4f(v_cg), vec4i_fromVec4f(v_cb), vec4i_fromVec4f(v_ca));
		vec4i_toInt4va(v_rgba8, &tileFB[0]);
	}

	// Row 1
	{
		const vec4f v_l0 = vec4f_mul(vec4f_fromVec4i(v_w0_row1), v_inv_area);
		const vec4f v_l1 = vec4f_mul(vec4f_fromVec4i(v_w1_row1), v_inv_area);
		const vec4f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec4f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec4f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec4f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec4i v_rgba8 = vec4i_packR32G32B32A32_to_RGBA8(vec4i_fromVec4f(v_cr), vec4i_fromVec4f(v_cg), vec4i_fromVec4f(v_cb), vec4i_fromVec4f(v_ca));
		vec4i_toInt4va(v_rgba8, &tileFB[rowStride]);
	}

	// Row 2
	{
		const vec4f v_l0 = vec4f_mul(vec4f_fromVec4i(v_w0_row2), v_inv_area);
		const vec4f v_l1 = vec4f_mul(vec4f_fromVec4i(v_w1_row2), v_inv_area);
		const vec4f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec4f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec4f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec4f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec4i v_rgba8 = vec4i_packR32G32B32A32_to_RGBA8(vec4i_fromVec4f(v_cr), vec4i_fromVec4f(v_cg), vec4i_fromVec4f(v_cb), vec4i_fromVec4f(v_ca));
		vec4i_toInt4va(v_rgba8, &tileFB[rowStride * 2]);
	}

	// Row 3
	{
		const vec4f v_l0 = vec4f_mul(vec4f_fromVec4i(v_w0_row3), v_inv_area);
		const vec4f v_l1 = vec4f_mul(vec4f_fromVec4i(v_w1_row3), v_inv_area);
		const vec4f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
		const vec4f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
		const vec4f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
		const vec4f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);
		const vec4i v_rgba8 = vec4i_packR32G32B32A32_to_RGBA8(vec4i_fromVec4f(v_cr), vec4i_fromVec4f(v_cg), vec4i_fromVec4f(v_cb), vec4i_fromVec4f(v_ca));
		vec4i_toInt4va(v_rgba8, &tileFB[rowStride * 3]);
	}
#endif // SWR_CONFIG_DISABLE_PIXEL_SHADERS
}

void swrDrawTriangleSSE41(swr_context* ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color0, uint32_t color1, uint32_t color2)
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

	const swr_vertex_attrib_data va_r = swr_vertexAttribInit(vec4f_getXXXX(v_c2), vec4f_getXXXX(v_c02), vec4f_getXXXX(v_c12));
	const swr_vertex_attrib_data va_g = swr_vertexAttribInit(vec4f_getYYYY(v_c2), vec4f_getYYYY(v_c02), vec4f_getYYYY(v_c12));
	const swr_vertex_attrib_data va_b = swr_vertexAttribInit(vec4f_getZZZZ(v_c2), vec4f_getZZZZ(v_c02), vec4f_getZZZZ(v_c12));
	const swr_vertex_attrib_data va_a = swr_vertexAttribInit(vec4f_getWWWW(v_c2), vec4f_getWWWW(v_c02), vec4f_getWWWW(v_c12));

	// Barycentric coordinate normalization
	const vec4f v_inv_area = vec4f_fromFloat(1.0f / (float)iarea);

	// Triangle setup
	const swr_edge edge0 = swr_edgeInit(x2, y2, x1, y1);
	const swr_edge edge1 = swr_edgeInit(x0, y0, x2, y2);
	const swr_edge edge2 = swr_edgeInit(x1, y1, x0, y0);

	const vec4i v_edge_dx = vec4i_fromInt4(edge0.m_dx, edge1.m_dx, edge2.m_dx, 0);
	const vec4i v_edge_dy = vec4i_fromInt4(edge0.m_dy, edge1.m_dy, edge2.m_dy, 0);

	const vec4i v_pixelOffsets = vec4i_fromInt4(0, 1, 2, 3);
	const vec4i v_edge0_dx0123 = vec4i_mullo(vec4i_getXXXX(v_edge_dx), v_pixelOffsets);
	const vec4i v_edge1_dx0123 = vec4i_mullo(vec4i_getYYYY(v_edge_dx), v_pixelOffsets);
	const vec4i v_edge2_dx0123 = vec4i_mullo(vec4i_getZZZZ(v_edge_dx), v_pixelOffsets);
	const vec4i v_edge0_dy = vec4i_getXXXX(v_edge_dy);
	const vec4i v_edge1_dy = vec4i_getYYYY(v_edge_dy);
	const vec4i v_edge2_dy = vec4i_getZZZZ(v_edge_dy);

	swr_tile tile = {
		.blockMin_w = { 0, 0, 0 },
		.v_edge_dx0123 = { v_edge0_dx0123, v_edge1_dx0123, v_edge2_dx0123 },
		.v_edge_dy = { v_edge0_dy, v_edge1_dy, v_edge2_dy },
		.v_inv_area = v_inv_area
	};

#if SWR_CONFIG_SMALL_TRIANGLE_OPTIMIZATION
	if (bboxWidth <= 4 && bboxHeight <= 4) {
		const int32_t tileX = (bboxMinX + 4 >= (int32_t)ctx->m_Width)
			? ctx->m_Width - 4
			: bboxMinX
			;

		const int32_t tileY = (bboxMinY + 4 >= (int32_t)ctx->m_Height)
			? ctx->m_Height - 4
			: bboxMinY
			;
		tile.blockMin_w[0] = swr_edgeEval(edge0, tileX, tileY);
		tile.blockMin_w[1] = swr_edgeEval(edge1, tileX, tileY);
		tile.blockMin_w[2] = swr_edgeEval(edge2, tileX, tileY);
		swr_drawTile4x4_partial_unaligned(tile, va_r, va_g, va_b, va_a, &ctx->m_FrameBuffer[tileX + tileY * ctx->m_Width], ctx->m_Width);
		return;
	}
#endif

	const int32_t bboxMinX_aligned = core_roundDown(bboxMinX, 16);
	const int32_t bboxMinY_aligned = core_roundDown(bboxMinY, 4);
	const int32_t bboxMaxX_aligned = core_roundUp(bboxMaxX, 16);
	const int32_t bboxMaxY_aligned = core_roundUp(bboxMaxY, 4);

	// Trivial reject/accept corner offsets relative to block min/max.
	const vec4i v_blockSize_m1 = vec4i_fromInt(4 - 1);
	const vec4i v_w_blockMax_dx = vec4i_mullo(v_edge_dx, v_blockSize_m1);
	const vec4i v_w_blockMax_dy = vec4i_mullo(v_edge_dy, v_blockSize_m1);

	const vec4i v_zero = vec4i_zero();
	const vec4i v_edge_dx_lt = vec4i_cmplt(v_edge_dx, v_zero);
	const vec4i v_edge_dy_lt = vec4i_cmplt(v_edge_dy, v_zero);

	const vec4i v_trivialRejectOffset = vec4i_add(
		vec4i_andnot(v_edge_dx_lt, v_w_blockMax_dx),
		vec4i_andnot(v_edge_dy_lt, v_w_blockMax_dy)
	);
	const vec4i v_trivialRejectOffset_0 = vec4i_getXXXX(v_trivialRejectOffset);
	const vec4i v_trivialRejectOffset_1 = vec4i_getYYYY(v_trivialRejectOffset);
	const vec4i v_trivialRejectOffset_2 = vec4i_getZZZZ(v_trivialRejectOffset);

	const vec4i v_trivialAcceptOffset = vec4i_sub(vec4i_add(v_w_blockMax_dx, v_w_blockMax_dy), v_trivialRejectOffset);
	const vec4i v_trivialAcceptOffset_0 = vec4i_getXXXX(v_trivialAcceptOffset);
	const vec4i v_trivialAcceptOffset_1 = vec4i_getYYYY(v_trivialAcceptOffset);
	const vec4i v_trivialAcceptOffset_2 = vec4i_getZZZZ(v_trivialAcceptOffset);

	// Rasterize
	const vec4i v_w0_bboxMin = vec4i_fromInt(swr_edgeEval(edge0, bboxMinX_aligned, bboxMinY_aligned));
	const vec4i v_w1_bboxMin = vec4i_fromInt(swr_edgeEval(edge1, bboxMinX_aligned, bboxMinY_aligned));
	const vec4i v_w2_bboxMin = vec4i_fromInt(swr_edgeEval(edge2, bboxMinX_aligned, bboxMinY_aligned));

	const vec4i v_w0_nextBlock_dx = vec4i_fromInt(edge0.m_dx * 16);
	const vec4i v_w1_nextBlock_dx = vec4i_fromInt(edge1.m_dx * 16);
	const vec4i v_w2_nextBlock_dx = vec4i_fromInt(edge2.m_dx * 16);
	const vec4i v_w0_nextBlock_dy = vec4i_fromInt(edge0.m_dy * 4);
	const vec4i v_w1_nextBlock_dy = vec4i_fromInt(edge1.m_dy * 4);
	const vec4i v_w2_nextBlock_dy = vec4i_fromInt(edge2.m_dy * 4);

	const vec4i v_blockOffsets = vec4i_fromInt4(0, 4, 8, 12);
	vec4i v_w0_blockY = vec4i_add(v_w0_bboxMin, vec4i_mullo(vec4i_getXXXX(v_edge_dx), v_blockOffsets));
	vec4i v_w1_blockY = vec4i_add(v_w1_bboxMin, vec4i_mullo(vec4i_getYYYY(v_edge_dx), v_blockOffsets));
	vec4i v_w2_blockY = vec4i_add(v_w2_bboxMin, vec4i_mullo(vec4i_getZZZZ(v_edge_dx), v_blockOffsets));

	for (int32_t blockMinY = bboxMinY_aligned; blockMinY < bboxMaxY_aligned; blockMinY += 4) {
		uint32_t* fb_blockY = &ctx->m_FrameBuffer[blockMinY * ctx->m_Width];
		vec4i v_w0_blockMin = v_w0_blockY;
		vec4i v_w1_blockMin = v_w1_blockY;
		vec4i v_w2_blockMin = v_w2_blockY;

		for (int32_t blockMinX = bboxMinX_aligned; blockMinX < bboxMaxX_aligned; blockMinX += 16) {
			// Evaluate each edge function at its trivial reject corner (the most positive block corner).
			// If the trivial rejct corner of any edge is negative (outside the edge) then the triangle 
			// does not touch the block.
			const vec4i v_w0_trivialReject = vec4i_add(v_w0_blockMin, v_trivialRejectOffset_0);
			const vec4i v_w1_trivialReject = vec4i_add(v_w1_blockMin, v_trivialRejectOffset_1);
			const vec4i v_w2_trivialReject = vec4i_add(v_w2_blockMin, v_trivialRejectOffset_2);
			const vec4i v_w_trivialReject = vec4i_or3(v_w0_trivialReject, v_w1_trivialReject, v_w2_trivialReject);
			uint32_t trivialRejectBlockMask = ~vec4i_getSignMask(v_w_trivialReject) & 0x0F;
			if (trivialRejectBlockMask == 0) {
				v_w0_blockMin = vec4i_add(v_w0_blockMin, v_w0_nextBlock_dx);
				v_w1_blockMin = vec4i_add(v_w1_blockMin, v_w1_nextBlock_dx);
				v_w2_blockMin = vec4i_add(v_w2_blockMin, v_w2_nextBlock_dx);
				continue;
			}

			// Evaluate each edge function at its trivial accept corner (the most negative block corner).
			// If the trivial accept corner of all edges is positive (inside the edge) then the triangle
			// fully covers the block.
			const vec4i v_w0_trivialAccept = vec4i_add(v_w0_blockMin, v_trivialAcceptOffset_0);
			const vec4i v_w1_trivialAccept = vec4i_add(v_w1_blockMin, v_trivialAcceptOffset_1);
			const vec4i v_w2_trivialAccept = vec4i_add(v_w2_blockMin, v_trivialAcceptOffset_2);
			const vec4i v_w_trivialAccept = vec4i_or3(v_w0_trivialAccept, v_w1_trivialAccept, v_w2_trivialAccept);
			uint32_t trivialAcceptBlockMask = vec4i_getSignMask(v_w_trivialAccept);

			int32_t w0_blockMin[4], w1_blockMin[4], w2_blockMin[4];
			vec4i_toInt4va(v_w0_blockMin, &w0_blockMin[0]);
			vec4i_toInt4va(v_w1_blockMin, &w1_blockMin[0]);
			vec4i_toInt4va(v_w2_blockMin, &w2_blockMin[0]);

			for (uint32_t iBlock = 0; trivialRejectBlockMask != 0;
				++iBlock, trivialRejectBlockMask >>= 1, trivialAcceptBlockMask >>= 1) {
				if ((trivialRejectBlockMask & 1) == 0) {
					continue;
				}

				tile.blockMin_w[0] = w0_blockMin[iBlock];
				tile.blockMin_w[1] = w1_blockMin[iBlock];
				tile.blockMin_w[2] = w2_blockMin[iBlock];
#if SWR_CONFIG_TREAT_ALL_TILES_AS_PARTIAL
				swr_drawTile4x4_partial_aligned(tile, va_r, va_g, va_b, va_a, &fb_blockY[blockMinX + iBlock * 4], ctx->m_Width);
#else
				if ((trivialAcceptBlockMask & 1) != 0) {
					swr_drawTile4x4_partial_aligned(tile, va_r, va_g, va_b, va_a, &fb_blockY[blockMinX + iBlock * 4], ctx->m_Width);
				} else {
					swr_drawTile4x4_full(tile, va_r, va_g, va_b, va_a, &fb_blockY[blockMinX + iBlock * 4], ctx->m_Width);
				}
#endif
			}

			v_w0_blockMin = vec4i_add(v_w0_blockMin, v_w0_nextBlock_dx);
			v_w1_blockMin = vec4i_add(v_w1_blockMin, v_w1_nextBlock_dx);
			v_w2_blockMin = vec4i_add(v_w2_blockMin, v_w2_nextBlock_dx);
		}

		v_w0_blockY = vec4i_add(v_w0_blockY, v_w0_nextBlock_dy);
		v_w1_blockY = vec4i_add(v_w1_blockY, v_w1_nextBlock_dy);
		v_w2_blockY = vec4i_add(v_w2_blockY, v_w2_nextBlock_dy);
	}
}
#else
#include <immintrin.h>

// Rasterizes a triangle by calculating the exact rows covered by the triangle and the exact 
// pixels touched on each row. This avoids conditionals inside the inner loop. It's faster
// compared to the "hierarchical" algorithm used above.
// 
// TODO: Rewrite using vec4 math.
void swrDrawTriangleSSE41(swr_context* ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color0, uint32_t color1, uint32_t color2)
{
	int32_t iarea = (x2 - x0) * (y1 - y0) - (x1 - x0) * (y2 - y0);
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

	const int32_t bboxMinX = core_maxi32(core_min3i32(x0, x1, x2), 0);
	const int32_t bboxMinY = core_maxi32(core_min3i32(y0, y1, y2), 0);
	const int32_t bboxMaxX = core_mini32(core_max3i32(x0, x1, x2), (int32_t)ctx->m_Width - 1);
	const int32_t bboxMaxY = core_mini32(core_max3i32(y0, y1, y2), (int32_t)ctx->m_Height - 1);
	const int32_t bboxWidth = bboxMaxX - bboxMinX;
	const int32_t bboxHeight = bboxMaxY - bboxMinY;

	const __m128i imm_zero = _mm_setzero_si128();
	const __m128 xmm_rgba0 = _mm_cvtepi32_ps(_mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_loadu_si32(&color0), imm_zero), imm_zero));
	const __m128 xmm_rgba1 = _mm_cvtepi32_ps(_mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_loadu_si32(&color1), imm_zero), imm_zero));
	const __m128 xmm_rgba2 = _mm_cvtepi32_ps(_mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_loadu_si32(&color2), imm_zero), imm_zero));
	const __m128 xmm_drgba20 = _mm_sub_ps(xmm_rgba2, xmm_rgba0);
	const __m128 xmm_drgba10 = _mm_sub_ps(xmm_rgba1, xmm_rgba0);

	const __m128 xmm_r0 = _mm_shuffle_ps(xmm_rgba0, xmm_rgba0, _MM_SHUFFLE(0, 0, 0, 0));
	const __m128 xmm_g0 = _mm_shuffle_ps(xmm_rgba0, xmm_rgba0, _MM_SHUFFLE(1, 1, 1, 1));
	const __m128 xmm_b0 = _mm_shuffle_ps(xmm_rgba0, xmm_rgba0, _MM_SHUFFLE(2, 2, 2, 2));
	const __m128 xmm_a0 = _mm_shuffle_ps(xmm_rgba0, xmm_rgba0, _MM_SHUFFLE(3, 3, 3, 3));
	const __m128 xmm_dr20 = _mm_shuffle_ps(xmm_drgba20, xmm_drgba20, _MM_SHUFFLE(0, 0, 0, 0));
	const __m128 xmm_dg20 = _mm_shuffle_ps(xmm_drgba20, xmm_drgba20, _MM_SHUFFLE(1, 1, 1, 1));
	const __m128 xmm_db20 = _mm_shuffle_ps(xmm_drgba20, xmm_drgba20, _MM_SHUFFLE(2, 2, 2, 2));
	const __m128 xmm_da20 = _mm_shuffle_ps(xmm_drgba20, xmm_drgba20, _MM_SHUFFLE(3, 3, 3, 3));
	const __m128 xmm_dr10 = _mm_shuffle_ps(xmm_drgba10, xmm_drgba10, _MM_SHUFFLE(0, 0, 0, 0));
	const __m128 xmm_dg10 = _mm_shuffle_ps(xmm_drgba10, xmm_drgba10, _MM_SHUFFLE(1, 1, 1, 1));
	const __m128 xmm_db10 = _mm_shuffle_ps(xmm_drgba10, xmm_drgba10, _MM_SHUFFLE(2, 2, 2, 2));
	const __m128 xmm_da10 = _mm_shuffle_ps(xmm_drgba10, xmm_drgba10, _MM_SHUFFLE(3, 3, 3, 3));

	const int32_t dy01 = y0 - y1;
	const int32_t dx01 = x0 - x1;
	const int32_t dx20 = x2 - x0;
	const int32_t dy20 = y2 - y0;
	const int32_t dy01_dy20 = dy01 + dy20;

	const __m128 xmm_zero = _mm_setzero_ps();
	const __m128 xmm_inv_area = _mm_set1_ps(1.0f / (float)iarea);

	// Barycentric coordinate deltas for the X direction
	const __m128i imm_x_duvw_ = _mm_set_epi32(0, dy01_dy20, -dy20, -dy01);
	const __m128 xmm_x_duvw_1 = _mm_mul_ps(_mm_cvtepi32_ps(imm_x_duvw_), xmm_inv_area);
	const __m128 xmm_x_duvw_2 = _mm_add_ps(xmm_x_duvw_1, xmm_x_duvw_1);
	const __m128 xmm_x_duvw_3 = _mm_add_ps(xmm_x_duvw_1, xmm_x_duvw_2);
	const __m128 xmm_x_duvw_4 = _mm_add_ps(xmm_x_duvw_2, xmm_x_duvw_2);

	// UV deltas for the 1st and 2nd pixel
	const __m128 xmm_x_duv0_duv1 = _mm_shuffle_ps(xmm_zero, xmm_x_duvw_1, _MM_SHUFFLE(1, 0, 1, 0));

	// UV deltas for the 3rd and 4th pixel
	const __m128 xmm_x_duv2_duv3 = _mm_shuffle_ps(xmm_x_duvw_2, xmm_x_duvw_3, _MM_SHUFFLE(1, 0, 1, 0));

	const __m128 xmm_x_du4 = _mm_shuffle_ps(xmm_x_duvw_4, xmm_x_duvw_4, _MM_SHUFFLE(0, 0, 0, 0));
	const __m128 xmm_x_dv4 = _mm_shuffle_ps(xmm_x_duvw_4, xmm_x_duvw_4, _MM_SHUFFLE(1, 1, 1, 1));

	// Barycentric coordinate deltas for the Y direction
	const __m128i imm_y_duvw_ = _mm_set_epi32(0, -(dx01 + dx20), dx20, dx01);

	// Calculate unnormalized barycentric coordinates of the bounding box min.
	const int32_t bboxMin_u = (x0 - bboxMinX) * dy01 - (y0 - bboxMinY) * dx01;
	const int32_t bboxMin_v = (x0 - bboxMinX) * dy20 - (y0 - bboxMinY) * dx20;
	const int32_t bboxMin_w = iarea - bboxMin_u - bboxMin_v;
	__m128i imm_row_uvw_ = _mm_set_epi32(0, bboxMin_w, bboxMin_v, bboxMin_u);

	// 
	const __m128 xmm_row_uvw_scale = _mm_set_ps(0.0f, 1.0f / (float)dy01_dy20, 1.0f / (float)dy20, 1.0f / (float)dy01);

	uint32_t* framebufferRow = &ctx->m_FrameBuffer[bboxMinX + bboxMinY * ctx->m_Width];
	for (int32_t iy = 0; iy <= bboxHeight; ++iy) {
		int32_t ixmin = 0;
		int32_t ixmax = (uint32_t)bboxWidth;

		// Calculate ixmin and ixmax
		{
			int32_t row_uvw_[4];
			_mm_storeu_si128((__m128i*) & row_uvw_[0], imm_row_uvw_);

			const __m128 xmm_row_uvw_ = _mm_mul_ps(_mm_cvtepi32_ps(imm_row_uvw_), xmm_row_uvw_scale);
			const __m128i imm_row_uvw_floor = _mm_cvtps_epi32(_mm_floor_ps(xmm_row_uvw_));
			const __m128i imm_row_uvw_ceil = _mm_cvtps_epi32(_mm_ceil_ps(xmm_row_uvw_));

			int32_t row_uvw_floor[4];
			_mm_storeu_si128((__m128i*) & row_uvw_floor[0], imm_row_uvw_floor);

			int32_t row_uvw_ceil[4];
			_mm_storeu_si128((__m128i*) & row_uvw_ceil[0], imm_row_uvw_ceil);

			if (dy01 > 0) {
				ixmax = core_mini32(ixmax, row_uvw_floor[0]);
			} else if (row_uvw_[0] != 0) {
				ixmin = core_maxi32(ixmin, row_uvw_ceil[0]);
			}

			if (dy20 > 0) {
				ixmax = core_mini32(ixmax, row_uvw_floor[1]);
			} else if (row_uvw_[1] != 0) {
				ixmin = core_maxi32(ixmin, row_uvw_ceil[1]);
			}

			if (dy01_dy20 < 0 && row_uvw_[2] >= 0) {
				ixmax = core_mini32(ixmax, -row_uvw_ceil[2]);
			} else if (dy01_dy20 > 0 && row_uvw_[2] < 0) {
				ixmin = core_maxi32(ixmin, -row_uvw_floor[2]);
			}
		}

		if (ixmin <= ixmax) {
			// Calculate normalized barycentric coordinates at ixmin of the current row of pixels.
			const __m128i imm_p0uvw_ = _mm_add_epi32(imm_row_uvw_, _mm_mullo_epi32(_mm_set1_epi32(ixmin), imm_x_duvw_));
			const __m128 xmm_p0uvw_ = _mm_mul_ps(_mm_cvtepi32_ps(imm_p0uvw_), xmm_inv_area);
			const __m128 xmm_p0uvuv = _mm_shuffle_ps(xmm_p0uvw_, xmm_p0uvw_, _MM_SHUFFLE(1, 0, 1, 0));

			// Calculate barycentric coordinates for the 4 pixels.
			const __m128 xmm_p0uv_p1uv = _mm_add_ps(xmm_p0uvuv, xmm_x_duv0_duv1); // Barycentric coordinates of 1st and 2nd pixels
			const __m128 xmm_p2uv_p3uv = _mm_add_ps(xmm_p0uvuv, xmm_x_duv2_duv3); // Barycentric coordinates of 3rd and 4th pixels

			// Extract barycentric coordinates for each pixel
			__m128 xmm_u0123 = _mm_shuffle_ps(xmm_p0uv_p1uv, xmm_p2uv_p3uv, _MM_SHUFFLE(2, 0, 2, 0));
			__m128 xmm_v0123 = _mm_shuffle_ps(xmm_p0uv_p1uv, xmm_p2uv_p3uv, _MM_SHUFFLE(3, 1, 3, 1));

			uint32_t* frameBuffer = &framebufferRow[ixmin];
			const uint32_t numPixels = (uint32_t)((ixmax - ixmin) + 1);
			const uint32_t numIter = numPixels >> 2; // 4 pixels per iteration
			for (uint32_t iIter = 0; iIter < numIter; ++iIter) {
				// Calculate the color of each pixel
				const __m128 xmm_r_p0123 = _mm_add_ps(xmm_r0, _mm_add_ps(_mm_mul_ps(xmm_dr20, xmm_u0123), _mm_mul_ps(xmm_dr10, xmm_v0123)));
				const __m128 xmm_g_p0123 = _mm_add_ps(xmm_g0, _mm_add_ps(_mm_mul_ps(xmm_dg20, xmm_u0123), _mm_mul_ps(xmm_dg10, xmm_v0123)));
				const __m128 xmm_b_p0123 = _mm_add_ps(xmm_b0, _mm_add_ps(_mm_mul_ps(xmm_db20, xmm_u0123), _mm_mul_ps(xmm_db10, xmm_v0123)));
				const __m128 xmm_a_p0123 = _mm_add_ps(xmm_a0, _mm_add_ps(_mm_mul_ps(xmm_da20, xmm_u0123), _mm_mul_ps(xmm_da10, xmm_v0123)));

				// Pack into uint8_t
				// (uint8_t){ r0, r1, r2, r3, g0, g1, g2, g3, b0, b1, b2, b3, a0, a1, a2, a3 }
				const __m128i imm_r0123_g0123_b0123_a0123_u8 = _mm_packus_epi16(
					_mm_packs_epi32(_mm_cvtps_epi32(xmm_r_p0123), _mm_cvtps_epi32(xmm_g_p0123)),
					_mm_packs_epi32(_mm_cvtps_epi32(xmm_b_p0123), _mm_cvtps_epi32(xmm_a_p0123))
				);

				// Shuffle into RGBA uint32_t
				const __m128i mask = _mm_set_epi8(15, 11, 7, 3, 14, 10, 6, 2, 13, 9, 5, 1, 12, 8, 4, 0);
				const __m128i imm_rgba_p0123_u8 = _mm_shuffle_epi8(imm_r0123_g0123_b0123_a0123_u8, mask);

				// Store
				_mm_storeu_si128((__m128i*)frameBuffer, imm_rgba_p0123_u8);

				// Move on to the next set of pixels
				xmm_u0123 = _mm_add_ps(xmm_u0123, xmm_x_du4);
				xmm_v0123 = _mm_add_ps(xmm_v0123, xmm_x_dv4);
				frameBuffer += 4;
			}

			// Calculate the colors of the 4 next pixels and selectively store only the number 
			// of remainder pixels for this row
			const uint32_t rem = numPixels & 3;
			{
				// Calculate the color of each pixel
				const __m128 xmm_r_p0123 = _mm_add_ps(xmm_r0, _mm_add_ps(_mm_mul_ps(xmm_dr20, xmm_u0123), _mm_mul_ps(xmm_dr10, xmm_v0123)));
				const __m128 xmm_g_p0123 = _mm_add_ps(xmm_g0, _mm_add_ps(_mm_mul_ps(xmm_dg20, xmm_u0123), _mm_mul_ps(xmm_dg10, xmm_v0123)));
				const __m128 xmm_b_p0123 = _mm_add_ps(xmm_b0, _mm_add_ps(_mm_mul_ps(xmm_db20, xmm_u0123), _mm_mul_ps(xmm_db10, xmm_v0123)));
				const __m128 xmm_a_p0123 = _mm_add_ps(xmm_a0, _mm_add_ps(_mm_mul_ps(xmm_da20, xmm_u0123), _mm_mul_ps(xmm_da10, xmm_v0123)));

				// Pack into uint8_t
				// (uint8_t){ r0, r1, r2, r3, g0, g1, g2, g3, b0, b1, b2, b3, a0, a1, a2, a3 }
				const __m128i imm_r0123_g0123_b0123_a0123_u8 = _mm_packus_epi16(
					_mm_packs_epi32(_mm_cvtps_epi32(xmm_r_p0123), _mm_cvtps_epi32(xmm_g_p0123)),
					_mm_packs_epi32(_mm_cvtps_epi32(xmm_b_p0123), _mm_cvtps_epi32(xmm_a_p0123))
				);

				// Shuffle into RGBA uint32_t
				const __m128i mask = _mm_set_epi8(15, 11, 7, 3, 14, 10, 6, 2, 13, 9, 5, 1, 12, 8, 4, 0);
				const __m128i imm_rgba_p0123_u8 = _mm_shuffle_epi8(imm_r0123_g0123_b0123_a0123_u8, mask);

				// Load existing frame buffer values.
				const __m128i imm_frameBuffer = _mm_lddqu_si128((const __m128i*)frameBuffer);

				// Replace only the number of remainder pixels
				const __m128 blendMask = _mm_castsi128_ps(_mm_cmpgt_epi32(_mm_set_epi32(rem, rem, rem, rem), _mm_set_epi32(3, 2, 1, 0)));
				const __m128i xmm_newFrameBuffer = _mm_castps_si128(_mm_blendv_ps(_mm_castsi128_ps(imm_frameBuffer), _mm_castsi128_ps(imm_rgba_p0123_u8), blendMask));

				// Store
				_mm_storeu_si128((__m128i*)frameBuffer, xmm_newFrameBuffer);
			}
		}

		// Move on to the next row of pixels.
		imm_row_uvw_ = _mm_add_epi32(imm_row_uvw_, imm_y_duvw_);
		framebufferRow += ctx->m_Width;
	}
}
#endif
