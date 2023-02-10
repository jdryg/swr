#include "swr.h"
#include "swr_p.h"
#include "../core/math.h"

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
	// BUG: There is something wrong with this code path. Visual comparison with reference
	// implementation does not give correct results.
	if (bboxWidth < 4 && bboxHeight < 4) {
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

	// 4x1 4x4 block scan
	int32_t bboxMinX_aligned = core_roundDown(bboxMinX, 16);
	int32_t bboxMaxX_aligned = core_roundUp(bboxMaxX + 1, 16);
	if (bboxMaxX_aligned >= (int32_t)ctx->m_Width) {
		const uint32_t n = (bboxMaxX_aligned - bboxMinX_aligned) / 16;
		bboxMaxX_aligned = ctx->m_Width;
		bboxMinX_aligned = ctx->m_Width - n * 16;
	}

	int32_t bboxMinY_aligned = core_roundDown(bboxMinY, 4);
	int32_t bboxMaxY_aligned = core_roundUp(bboxMaxY + 1, 4);
	if (bboxMaxY_aligned >= (int32_t)ctx->m_Height) {
		const uint32_t n = (bboxMaxY_aligned - bboxMinY_aligned) / 4;
		bboxMaxY_aligned = ctx->m_Height;
		bboxMinY_aligned = ctx->m_Height - n * 4;
	}

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
