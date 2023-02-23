#include "swr.h"
#include "swr_p.h"
#include "../core/math.h"

#define SWR_VEC_MATH_AVX2
#define SWR_VEC_MATH_FMA
#include "swr_vec_math.h"

#define SWR_CONFIG_CHECK_CONST_COLOR 0 // Test if all 3 vertex colors are equal and avoid interpolating over the triangle.
#define SWR_CONFIG_TILE_HEIGHT       4 // Either 4 or 8; width is always 8

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

typedef struct swr_tile_desc
{
	uint32_t m_CoverageMask03;
#if SWR_CONFIG_TILE_HEIGHT == 8
	uint32_t m_CoverageMask47;
#endif
	uint32_t m_FrameBufferOffset;
	int32_t m_BarycentricCoords[2];
} swr_tile_desc;

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

static __forceinline void rasterizeTile8x8_constColor(uint32_t color, uint32_t coverageMask03, uint32_t coverageMask47, uint32_t* tileFB, uint32_t rowStride)
{
	const vec8i rgba = vec8i_fromInt(color);

	const vec8i v_coverageMask03 = vec8i_fromInt(coverageMask03);

	// Row #0
	{
		const vec8i pixelMask = vec8i_sllv(v_coverageMask03, vec8i_fromInt8(31, 27, 23, 19, 15, 11, 7, 3));
		vec8i_toInt8va_masked(rgba, pixelMask, tileFB);

		tileFB += rowStride;
	}

	// Row #1
	{
		const vec8i pixelMask = vec8i_sllv(v_coverageMask03, vec8i_fromInt8(30, 26, 22, 18, 14, 10, 6, 2));
		vec8i_toInt8va_masked(rgba, pixelMask, tileFB);

		tileFB += rowStride;
	}

	// Row #2
	{
		const vec8i pixelMask = vec8i_sllv(v_coverageMask03, vec8i_fromInt8(29, 25, 21, 17, 13, 9, 5, 1));
		vec8i_toInt8va_masked(rgba, pixelMask, tileFB);

		tileFB += rowStride;
	}

	// Row #3
	{
		const vec8i pixelMask = vec8i_sllv(v_coverageMask03, vec8i_fromInt8(28, 24, 20, 16, 12, 8, 4, 0));
		vec8i_toInt8va_masked(rgba, pixelMask, tileFB);

		tileFB += rowStride;
	}

#if SWR_CONFIG_TILE_HEIGHT == 8
	const vec8i v_coverageMask47 = vec8i_fromInt(coverageMask47);

	// Row #4
	{
		const vec8i pixelMask = vec8i_sllv(v_coverageMask47, vec8i_fromInt8(31, 27, 23, 19, 15, 11, 7, 3));
		vec8i_toInt8va_masked(rgba, pixelMask, tileFB);

		tileFB += rowStride;
	}

	// Row #5
	{
		const vec8i pixelMask = vec8i_sllv(v_coverageMask47, vec8i_fromInt8(30, 26, 22, 18, 14, 10, 6, 2));
		vec8i_toInt8va_masked(rgba, pixelMask, tileFB);

		tileFB += rowStride;
	}

	// Row #6
	{
		const vec8i pixelMask = vec8i_sllv(v_coverageMask47, vec8i_fromInt8(29, 25, 21, 17, 13, 9, 5, 1));
		vec8i_toInt8va_masked(rgba, pixelMask, tileFB);

		tileFB += rowStride;
	}

	// Row #7
	{
		const vec8i pixelMask = vec8i_sllv(v_coverageMask47, vec8i_fromInt8(28, 24, 20, 16, 12, 8, 4, 0));
		vec8i_toInt8va_masked(rgba, pixelMask, tileFB);
	}
#endif
}

static __forceinline void rasterizeTile8x8_varColor(vec8f v_l0, vec8f v_l1, vec8f v_dl0, vec8f v_dl1, swr_vertex_attrib_data va_r, swr_vertex_attrib_data va_g, swr_vertex_attrib_data va_b, swr_vertex_attrib_data va_a, uint32_t coverageMask03, uint32_t coverageMask47, uint32_t* tileFB, uint32_t rowStride)
{
	const vec8f v_dcr = vec8f_madd(va_r.m_dVal12, v_dl1, vec8f_mul(va_r.m_dVal02, v_dl0));
	const vec8f v_dcg = vec8f_madd(va_g.m_dVal12, v_dl1, vec8f_mul(va_g.m_dVal02, v_dl0));
	const vec8f v_dcb = vec8f_madd(va_b.m_dVal12, v_dl1, vec8f_mul(va_b.m_dVal02, v_dl0));
	const vec8f v_dca = vec8f_madd(va_a.m_dVal12, v_dl1, vec8f_mul(va_a.m_dVal02, v_dl0));

	vec8f v_cr = swr_vertexAttribEval(va_r, v_l0, v_l1);
	vec8f v_cg = swr_vertexAttribEval(va_g, v_l0, v_l1);
	vec8f v_cb = swr_vertexAttribEval(va_b, v_l0, v_l1);
	vec8f v_ca = swr_vertexAttribEval(va_a, v_l0, v_l1);

	const vec8i v_coverageMask03 = vec8i_fromInt(coverageMask03);
	
	// Row #0
	{
		const vec8i pixelMask = vec8i_sllv(v_coverageMask03, vec8i_fromInt8(31, 27, 23, 19, 15, 11, 7, 3));
		const vec8i rgba = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8va_masked(rgba, pixelMask, tileFB);

		tileFB += rowStride;
		v_cr = vec8f_add(v_cr, v_dcr);
		v_cg = vec8f_add(v_cg, v_dcg);
		v_cb = vec8f_add(v_cb, v_dcb);
		v_ca = vec8f_add(v_ca, v_dca);
	}

	// Row #1
	{
		const vec8i pixelMask = vec8i_sllv(v_coverageMask03, vec8i_fromInt8(30, 26, 22, 18, 14, 10, 6, 2));
		const vec8i rgba = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8va_masked(rgba, pixelMask, tileFB);

		tileFB += rowStride;
		v_cr = vec8f_add(v_cr, v_dcr);
		v_cg = vec8f_add(v_cg, v_dcg);
		v_cb = vec8f_add(v_cb, v_dcb);
		v_ca = vec8f_add(v_ca, v_dca);
	}

	// Row #2
	{
		const vec8i pixelMask = vec8i_sllv(v_coverageMask03, vec8i_fromInt8(29, 25, 21, 17, 13, 9, 5, 1));
		const vec8i rgba = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8va_masked(rgba, pixelMask, tileFB);

		tileFB += rowStride;
		v_cr = vec8f_add(v_cr, v_dcr);
		v_cg = vec8f_add(v_cg, v_dcg);
		v_cb = vec8f_add(v_cb, v_dcb);
		v_ca = vec8f_add(v_ca, v_dca);
	}

	// Row #3
	{
		const vec8i pixelMask = vec8i_sllv(v_coverageMask03, vec8i_fromInt8(28, 24, 20, 16, 12, 8, 4, 0));
		const vec8i rgba = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8va_masked(rgba, pixelMask, tileFB);

		tileFB += rowStride;
		v_cr = vec8f_add(v_cr, v_dcr);
		v_cg = vec8f_add(v_cg, v_dcg);
		v_cb = vec8f_add(v_cb, v_dcb);
		v_ca = vec8f_add(v_ca, v_dca);
	}

#if SWR_CONFIG_TILE_HEIGHT == 8
	const vec8i v_coverageMask47 = vec8i_fromInt(coverageMask47);

	// Row #4
	{
		const vec8i pixelMask = vec8i_sllv(v_coverageMask47, vec8i_fromInt8(31, 27, 23, 19, 15, 11, 7, 3));
		const vec8i rgba = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8va_masked(rgba, pixelMask, tileFB);

		tileFB += rowStride;
		v_cr = vec8f_add(v_cr, v_dcr);
		v_cg = vec8f_add(v_cg, v_dcg);
		v_cb = vec8f_add(v_cb, v_dcb);
		v_ca = vec8f_add(v_ca, v_dca);
	}

	// Row #5
	{
		const vec8i pixelMask = vec8i_sllv(v_coverageMask47, vec8i_fromInt8(30, 26, 22, 18, 14, 10, 6, 2));
		const vec8i rgba = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8va_masked(rgba, pixelMask, tileFB);

		tileFB += rowStride;
		v_cr = vec8f_add(v_cr, v_dcr);
		v_cg = vec8f_add(v_cg, v_dcg);
		v_cb = vec8f_add(v_cb, v_dcb);
		v_ca = vec8f_add(v_ca, v_dca);
	}

	// Row #6
	{
		const vec8i pixelMask = vec8i_sllv(v_coverageMask47, vec8i_fromInt8(29, 25, 21, 17, 13, 9, 5, 1));
		const vec8i rgba = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8va_masked(rgba, pixelMask, tileFB);

		tileFB += rowStride;
		v_cr = vec8f_add(v_cr, v_dcr);
		v_cg = vec8f_add(v_cg, v_dcg);
		v_cb = vec8f_add(v_cb, v_dcb);
		v_ca = vec8f_add(v_ca, v_dca);
	}

	// Row #7
	{
		const vec8i pixelMask = vec8i_sllv(v_coverageMask47, vec8i_fromInt8(28, 24, 20, 16, 12, 8, 4, 0));
		const vec8i rgba = vec8i_packR32G32B32A32_to_RGBA8(vec8i_fromVec8f(v_cr), vec8i_fromVec8f(v_cg), vec8i_fromVec8f(v_cb), vec8i_fromVec8f(v_ca));
		vec8i_toInt8va_masked(rgba, pixelMask, tileFB);
	}
#endif
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

	int32_t bboxMinX_aligned = core_roundDown(bboxMinX, 8);
	int32_t bboxMaxX_aligned = core_roundUp(bboxMaxX + 1, 8);
	if (bboxMaxX_aligned >= (int32_t)ctx->m_Width) {
		const uint32_t n = (bboxMaxX_aligned - bboxMinX_aligned) / 8;
		bboxMaxX_aligned = ctx->m_Width;
		bboxMinX_aligned = ctx->m_Width - n * 8;
	}

	int32_t bboxMinY_aligned = core_roundDown(bboxMinY, SWR_CONFIG_TILE_HEIGHT);
	int32_t bboxMaxY_aligned = core_roundUp(bboxMaxY + 1, SWR_CONFIG_TILE_HEIGHT);
	if (bboxMaxY_aligned >= (int32_t)ctx->m_Height) {
		const uint32_t n = (bboxMaxY_aligned - bboxMinY_aligned) / SWR_CONFIG_TILE_HEIGHT;
		bboxMaxY_aligned = ctx->m_Height;
		bboxMinY_aligned = ctx->m_Height - n * SWR_CONFIG_TILE_HEIGHT;
	}

	const swr_edge edge0 = swr_edgeInit(x2, y2, x1, y1);
	const swr_edge edge1 = swr_edgeInit(x0, y0, x2, y2);
	const swr_edge edge2 = swr_edgeInit(x1, y1, x0, y0);

	// The Trivial Reject Corner (TRC) is the most positive corner of a tile
	// for each edge function. If the edge function at the TRC is negative it 
	// means that it will never be positive inside that tile so it can be 
	// discarded immediately.
	//
	// The TRC depends only on the rate of change of the edge function (dx & dy) 
	// so it can be calculated once for each triangle.
	// 
	// The value of the edge function for each pixel, given the value at the tile's
	// min corner, is:
	// 
	// e(i,j) = e(xmin,ymin) + i * dx + j * dy, 
	//   i in [0, blockSize - 1]
	//   j in [0, blockSize - 1]
	// 
	// If dx >= 0, TRC_x = blockSize - 1 otherwise TRC_x = 0
	// If dy >= 0, TRC_y = blockSize - 1 otherwise TRC_y = 0
	// 
	// The value of the edge function at the TRC is:
	// 
	// e(TRC_x, TRC_y) = e(xmin, ymin) + TRC_x * dx + TRC_y * dy
	// 
	// So, for each tile we calculate the total TRC offset as:
	// 
	// TRC_offset = TRC_x * dx + TRC_y * dy
	// 
	// and later add it to each tile's edge function value at the tile's min corner.
	// 
	// NOTE #1: When dx and/or dy are 0, any of the 2 corners of the tile can be chosen.
	// The explanation above assumes TRC_x = xmax and TRC_y = ymax in these cases. It doesn't 
	// matter since the corresponding offset will be 0.
	// 
	// NOTE #2: The TRC is only a quick way to discard a tile. There are cases where the edge
	// functions at the respective TRCs are all positive but no pixel will be rasterized because
	// the pixel masks generated by each edge function do not intersect.
	// 
#if SWR_CONFIG_TILE_HEIGHT == 8
	const int32_t trivialRejectOffset0 = (core_maxi32(edge0.m_dx, 0) + core_maxi32(edge0.m_dy, 0)) * 7;
	const int32_t trivialRejectOffset1 = (core_maxi32(edge1.m_dx, 0) + core_maxi32(edge1.m_dy, 0)) * 7;
	const int32_t trivialRejectOffset2 = (core_maxi32(edge2.m_dx, 0) + core_maxi32(edge2.m_dy, 0)) * 7;
#else
	const int32_t trivialRejectOffset0 = (core_maxi32(edge0.m_dx, 0) * 7) + (core_maxi32(edge0.m_dy, 0) * 3);
	const int32_t trivialRejectOffset1 = (core_maxi32(edge1.m_dx, 0) * 7) + (core_maxi32(edge1.m_dy, 0) * 3);
	const int32_t trivialRejectOffset2 = (core_maxi32(edge2.m_dx, 0) * 7) + (core_maxi32(edge2.m_dy, 0) * 3);
#endif

	const vec8i v_pixelOffsets = vec8i_fromInt8(0, 1, 2, 3, 4, 5, 6, 7);
	const vec8i v_edge0_dx_off = vec8i_mullo(vec8i_fromInt(edge0.m_dx), v_pixelOffsets);
	const vec8i v_edge1_dx_off = vec8i_mullo(vec8i_fromInt(edge1.m_dx), v_pixelOffsets);
	const vec8i v_edge2_dx_off = vec8i_mullo(vec8i_fromInt(edge2.m_dx), v_pixelOffsets);
	const vec8i v_edge0_dy = vec8i_fromInt(edge0.m_dy);
	const vec8i v_edge1_dy = vec8i_fromInt(edge1.m_dy);
	const vec8i v_edge2_dy = vec8i_fromInt(edge2.m_dy);
	const vec8i v_edge0_dy2 = vec8i_sal(v_edge0_dy, 1);
	const vec8i v_edge1_dy2 = vec8i_sal(v_edge1_dy, 1);
	const vec8i v_edge2_dy2 = vec8i_sal(v_edge2_dy, 1);
	const vec8i v_edge0_dy3 = vec8i_add(v_edge0_dy, v_edge0_dy2);
	const vec8i v_edge1_dy3 = vec8i_add(v_edge1_dy, v_edge1_dy2);
	const vec8i v_edge2_dy3 = vec8i_add(v_edge2_dy, v_edge2_dy2);
#if SWR_CONFIG_TILE_HEIGHT == 8
	const vec8i v_edge0_dy4 = vec8i_add(v_edge0_dy2, v_edge0_dy2);
	const vec8i v_edge1_dy4 = vec8i_add(v_edge1_dy2, v_edge1_dy2);
	const vec8i v_edge2_dy4 = vec8i_add(v_edge2_dy2, v_edge2_dy2);
#endif

	const vec8i v_signMask = vec8i_fromInt(0x80000000);

	uint32_t numTiles = 0;
	swr_tile_desc* tiles = (swr_tile_desc*)ctx->m_TileBuffer[0];

	int32_t w0_y = swr_edgeEval(edge0, bboxMinX_aligned, bboxMinY_aligned);
	int32_t w1_y = swr_edgeEval(edge1, bboxMinX_aligned, bboxMinY_aligned);
	int32_t w2_y = swr_edgeEval(edge2, bboxMinX_aligned, bboxMinY_aligned);
	for (int32_t tileY = bboxMinY_aligned; tileY < bboxMaxY_aligned; tileY += SWR_CONFIG_TILE_HEIGHT) {
		int32_t w0_tileMin = w0_y;
		int32_t w1_tileMin = w1_y;
		int32_t w2_tileMin = w2_y;
		for (int32_t tileX = bboxMinX_aligned; tileX < bboxMaxX_aligned; tileX += 8) {
			const int32_t w0_trivialReject = w0_tileMin + trivialRejectOffset0;
			const int32_t w1_trivialReject = w1_tileMin + trivialRejectOffset1;
			const int32_t w2_trivialReject = w2_tileMin + trivialRejectOffset2;
			const int32_t trivialReject = w0_trivialReject | w1_trivialReject | w2_trivialReject;
			if (trivialReject < 0) {
				w0_tileMin += edge0.m_dx << 3;
				w1_tileMin += edge1.m_dx << 3;
				w2_tileMin += edge2.m_dx << 3;
				continue;
			}
			
			const vec8i v_w0_row0 = vec8i_add(vec8i_fromInt(w0_tileMin), v_edge0_dx_off);
			const vec8i v_w1_row0 = vec8i_add(vec8i_fromInt(w1_tileMin), v_edge1_dx_off);
			const vec8i v_w2_row0 = vec8i_add(vec8i_fromInt(w2_tileMin), v_edge2_dx_off);
			const vec8i v_w0_row1 = vec8i_add(v_w0_row0, v_edge0_dy);
			const vec8i v_w1_row1 = vec8i_add(v_w1_row0, v_edge1_dy);
			const vec8i v_w2_row1 = vec8i_add(v_w2_row0, v_edge2_dy);
			const vec8i v_w0_row2 = vec8i_add(v_w0_row0, v_edge0_dy2);
			const vec8i v_w1_row2 = vec8i_add(v_w1_row0, v_edge1_dy2);
			const vec8i v_w2_row2 = vec8i_add(v_w2_row0, v_edge2_dy2);
			const vec8i v_w0_row3 = vec8i_add(v_w0_row0, v_edge0_dy3);
			const vec8i v_w1_row3 = vec8i_add(v_w1_row0, v_edge1_dy3);
			const vec8i v_w2_row3 = vec8i_add(v_w2_row0, v_edge2_dy3);
#if SWR_CONFIG_TILE_HEIGHT == 8
			const vec8i v_w0_row4 = vec8i_add(v_w0_row0, v_edge0_dy4);
			const vec8i v_w1_row4 = vec8i_add(v_w1_row0, v_edge1_dy4);
			const vec8i v_w2_row4 = vec8i_add(v_w2_row0, v_edge2_dy4);
			const vec8i v_w0_row5 = vec8i_add(v_w0_row4, v_edge0_dy);
			const vec8i v_w1_row5 = vec8i_add(v_w1_row4, v_edge1_dy);
			const vec8i v_w2_row5 = vec8i_add(v_w2_row4, v_edge2_dy);
			const vec8i v_w0_row6 = vec8i_add(v_w0_row4, v_edge0_dy2);
			const vec8i v_w1_row6 = vec8i_add(v_w1_row4, v_edge1_dy2);
			const vec8i v_w2_row6 = vec8i_add(v_w2_row4, v_edge2_dy2);
			const vec8i v_w0_row7 = vec8i_add(v_w0_row4, v_edge0_dy3);
			const vec8i v_w1_row7 = vec8i_add(v_w1_row4, v_edge1_dy3);
			const vec8i v_w2_row7 = vec8i_add(v_w2_row4, v_edge2_dy3);
#endif

			const vec8i v_mask0 = vec8i_or3(v_w0_row0, v_w1_row0, v_w2_row0);
			const vec8i v_mask1 = vec8i_or3(v_w0_row1, v_w1_row1, v_w2_row1);
			const vec8i v_mask2 = vec8i_or3(v_w0_row2, v_w1_row2, v_w2_row2);
			const vec8i v_mask3 = vec8i_or3(v_w0_row3, v_w1_row3, v_w2_row3);
#if SWR_CONFIG_TILE_HEIGHT == 8
			const vec8i v_mask4 = vec8i_or3(v_w0_row4, v_w1_row4, v_w2_row4);
			const vec8i v_mask5 = vec8i_or3(v_w0_row5, v_w1_row5, v_w2_row5);
			const vec8i v_mask6 = vec8i_or3(v_w0_row6, v_w1_row6, v_w2_row6);
			const vec8i v_mask7 = vec8i_or3(v_w0_row7, v_w1_row7, v_w2_row7);
#endif

			const vec8i v_mask0_sign = vec8i_and(v_mask0, v_signMask);
			const vec8i v_mask1_sign = vec8i_and(v_mask1, v_signMask);
			const vec8i v_mask2_sign = vec8i_and(v_mask2, v_signMask);
			const vec8i v_mask3_sign = vec8i_and(v_mask3, v_signMask);
#if SWR_CONFIG_TILE_HEIGHT == 8
			const vec8i v_mask4_sign = vec8i_and(v_mask4, v_signMask);
			const vec8i v_mask5_sign = vec8i_and(v_mask5, v_signMask);
			const vec8i v_mask6_sign = vec8i_and(v_mask6, v_signMask);
			const vec8i v_mask7_sign = vec8i_and(v_mask7, v_signMask);
#endif

			const vec8i v_mask01 = vec8i_or(vec8i_slr(v_mask0_sign, 24), vec8i_slr(v_mask1_sign, 16));
			const vec8i v_mask23 = vec8i_or(vec8i_slr(v_mask2_sign, 8), v_mask3_sign);
			const vec8i v_mask0_3 = vec8i_or(v_mask01, v_mask23);
			const uint32_t mask0_3 = ~vec8i_getByteSignMask(v_mask0_3);

#if SWR_CONFIG_TILE_HEIGHT == 8
			const vec8i v_mask45 = vec8i_or(vec8i_slr(v_mask4_sign, 24), vec8i_slr(v_mask5_sign, 16));
			const vec8i v_mask67 = vec8i_or(vec8i_slr(v_mask6_sign, 8), v_mask7_sign);
			const vec8i v_mask4_7 = vec8i_or(v_mask45, v_mask67);
			const uint32_t mask4_7 = ~vec8i_getByteSignMask(v_mask4_7);
#else
			const uint32_t mask4_7 = 0;
#endif

			if ((mask0_3 | mask4_7) != 0) {
				swr_tile_desc* tile = &tiles[numTiles];
				tile->m_CoverageMask03 = mask0_3;
#if SWR_CONFIG_TILE_HEIGHT == 8
				tile->m_CoverageMask47 = mask4_7;
#endif
				tile->m_FrameBufferOffset = tileX + tileY * ctx->m_Width;
				tile->m_BarycentricCoords[0] = w0_tileMin;
				tile->m_BarycentricCoords[1] = w1_tileMin;
				++numTiles;
			}

			w0_tileMin += edge0.m_dx << 3;
			w1_tileMin += edge1.m_dx << 3;
			w2_tileMin += edge2.m_dx << 3;
		}

		w0_y += edge0.m_dy * SWR_CONFIG_TILE_HEIGHT;
		w1_y += edge1.m_dy * SWR_CONFIG_TILE_HEIGHT;
		w2_y += edge2.m_dy * SWR_CONFIG_TILE_HEIGHT;
	}

#if SWR_CONFIG_CHECK_CONST_COLOR
	// If all 3 vertex colors are equal there is no need for interpolation.
	// NOTE: I don't know if this actually helps in the general case but for the
	// 6502 mesh I'm currently testing with where each layer has a constant color
	// it does help a lot.
	if (color0 == color1 && color0 == color2) {
		for (uint32_t iTile = 0; iTile < numTiles; ++iTile) {
			const swr_tile_desc* tile = &tiles[iTile];
			const uint32_t mask03 = tile->m_CoverageMask03;
			const uint32_t mask47 =
#if SWR_CONFIG_TILE_HEIGHT == 8
			tile->m_CoverageMask47;
#else // SWR_CONFIG_TILE_HEIGHT
			0;
#endif
			rasterizeTile8x8_constColor(
				color0,
				mask03,
				mask47,
				&ctx->m_FrameBuffer[tile->m_FrameBufferOffset],
				ctx->m_Width
			);
		}
	} else {
#else // SWR_CONFIG_CHECK_CONST_COLOR
	{
#endif // SWR_CONFIG_CHECK_CONST_COLOR

#if !SWR_CONFIG_DISABLE_PIXEL_SHADERS
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

		const vec8f v_dl0 = vec8f_mul(vec8f_fromVec8i(v_edge0_dy), v_inv_area);
		const vec8f v_dl1 = vec8f_mul(vec8f_fromVec8i(v_edge1_dy), v_inv_area);
#endif

		for (uint32_t iTile = 0; iTile < numTiles; ++iTile) {
			const swr_tile_desc* tile = &tiles[iTile];
			const uint32_t mask03 = tile->m_CoverageMask03;
			const uint32_t mask47 =
#if SWR_CONFIG_TILE_HEIGHT == 8
				tile->m_CoverageMask47;
#else // SWR_CONFIG_TILE_HEIGHT
				0;
#endif

#if SWR_CONFIG_DISABLE_PIXEL_SHADERS
			rasterizeTile8x8_constColor(
				0xFFFFFFFFu,
				mask03,
				mask47,
				&ctx->m_FrameBuffer[tile->m_FrameBufferOffset],
				ctx->m_Width
			);
#else
			const vec8i v_w0_row0 = vec8i_add(vec8i_fromInt(tile->m_BarycentricCoords[0]), v_edge0_dx_off);
			const vec8i v_w1_row0 = vec8i_add(vec8i_fromInt(tile->m_BarycentricCoords[1]), v_edge1_dx_off);
			const vec8f v_l0 = vec8f_mul(vec8f_fromVec8i(v_w0_row0), v_inv_area);
			const vec8f v_l1 = vec8f_mul(vec8f_fromVec8i(v_w1_row0), v_inv_area);
			rasterizeTile8x8_varColor(
				v_l0, v_l1,
				v_dl0, v_dl1,
				va_r,
				va_g,
				va_b,
				va_a,
				mask03,
				mask47,
				&ctx->m_FrameBuffer[tile->m_FrameBufferOffset],
				ctx->m_Width
			);
#endif
		}
	}
}
