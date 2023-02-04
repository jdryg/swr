#include "swr.h"
#include "swr_p.h"
#include "../core/math.h"
#include <assert.h>

typedef struct swr_edge
{
	int32_t m_x0;
	int32_t m_y0;
	int32_t m_dx;
	int32_t m_dy;
} swr_edge;

static inline swr_edge swr_edgeInit(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
	return (swr_edge){
		.m_x0 = x0,
		.m_y0 = y0,
		.m_dx = (y1 - y0),
		.m_dy = (x0 - x1),
	};
}

static inline int32_t swr_edgeEval(swr_edge edge, int32_t x, int32_t y)
{
	return 0
		+ (x - edge.m_x0) * edge.m_dx 
		+ (y - edge.m_y0) * edge.m_dy
		;
}

// Reference implementation
// https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
// NOTE: No fill rule used. All pixels lying ON an edge are drawn.
void swrDrawTriangleRef(swr_context* ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color0, uint32_t color1, uint32_t color2)
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
	const int32_t minX = core_maxi32(core_min3i32(x0, x1, x2), 0);
	const int32_t minY = core_maxi32(core_min3i32(y0, y1, y2), 0);
	const int32_t maxX = core_mini32(core_max3i32(x0, x1, x2), (int32_t)(ctx->m_Width - 1));
	const int32_t maxY = core_mini32(core_max3i32(y0, y1, y2), (int32_t)(ctx->m_Height - 1));
	const int32_t bboxWidth = maxX - minX;
	const int32_t bboxHeight = maxY - minY;
	if (bboxWidth <= 0 || bboxHeight <= 0) {
		return;
	}

	// Prepare interpolated attributes
#if !SWR_CONFIG_DISABLE_PIXEL_SHADERS
	const uint32_t c0r = (color0 & SWR_COLOR_RED_Msk) >> SWR_COLOR_RED_Pos;
	const uint32_t c0g = (color0 & SWR_COLOR_GREEN_Msk) >> SWR_COLOR_GREEN_Pos;
	const uint32_t c0b = (color0 & SWR_COLOR_BLUE_Msk) >> SWR_COLOR_BLUE_Pos;
	const uint32_t c0a = (color0 & SWR_COLOR_ALPHA_Msk) >> SWR_COLOR_ALPHA_Pos;
	const uint32_t c1r = (color1 & SWR_COLOR_RED_Msk) >> SWR_COLOR_RED_Pos;
	const uint32_t c1g = (color1 & SWR_COLOR_GREEN_Msk) >> SWR_COLOR_GREEN_Pos;
	const uint32_t c1b = (color1 & SWR_COLOR_BLUE_Msk) >> SWR_COLOR_BLUE_Pos;
	const uint32_t c1a = (color1 & SWR_COLOR_ALPHA_Msk) >> SWR_COLOR_ALPHA_Pos;
	const uint32_t c2r = (color2 & SWR_COLOR_RED_Msk) >> SWR_COLOR_RED_Pos;
	const uint32_t c2g = (color2 & SWR_COLOR_GREEN_Msk) >> SWR_COLOR_GREEN_Pos;
	const uint32_t c2b = (color2 & SWR_COLOR_BLUE_Msk) >> SWR_COLOR_BLUE_Pos;
	const uint32_t c2a = (color2 & SWR_COLOR_ALPHA_Msk) >> SWR_COLOR_ALPHA_Pos;
	const int32_t cr02 = (int32_t)c0r - (int32_t)c2r;
	const int32_t cg02 = (int32_t)c0g - (int32_t)c2g;
	const int32_t cb02 = (int32_t)c0b - (int32_t)c2b;
	const int32_t ca02 = (int32_t)c0a - (int32_t)c2a;
	const int32_t cr12 = (int32_t)c1r - (int32_t)c2r;
	const int32_t cg12 = (int32_t)c1g - (int32_t)c2g;
	const int32_t cb12 = (int32_t)c1b - (int32_t)c2b;
	const int32_t ca12 = (int32_t)c1a - (int32_t)c2a;
#endif

	// Triangle setup
	const swr_edge edge0 = swr_edgeInit(x2, y2, x1, y1);
	const swr_edge edge1 = swr_edgeInit(x0, y0, x2, y2);
	const swr_edge edge2 = swr_edgeInit(x1, y1, x0, y0);
	const int32_t w0_pmin = swr_edgeEval(edge0, minX, minY);
	const int32_t w1_pmin = swr_edgeEval(edge1, minX, minY);
	const int32_t w2_pmin = swr_edgeEval(edge2, minX, minY);

	// Barycentric coordinate normalization
#if !SWR_CONFIG_DISABLE_PIXEL_SHADERS
	const float inv_area = 1.0f / (float)iarea;
#endif

	// Rasterize
	int32_t w0_row = w0_pmin;
	int32_t w1_row = w1_pmin;
	int32_t w2_row = w2_pmin;
	uint32_t* fb_row = &ctx->m_FrameBuffer[minX + minY * ctx->m_Width];

	for (int32_t py = 0; py <= bboxHeight; ++py) {
		int32_t pxmin = 0;
		int32_t pxmax = bboxWidth;

		// Calculate the range of x values for which the barycentric coordinates
		// will always be greater than or equal to 0.
		{
			// The barycentric coordinates are linear functions: w_pmin + i * w_px
			// 
			// The inequality w_pmin + i * w_px >= 0 holds for all i's in the range:
			// 1. w_pmin >= 0 && w_px >= 0 : [0, bboxWidth]
			// 2. w_pmin >= 0 && w_px < 0  : [0, imax]         where imax = -(w_pmin / w_px)
			// 3. w_pmin < 0  && w_px > 0  : [imin, bboxWidth] where imin = -(w_pmin / w_px) + 1
			// 4. w_pmin < 0  && w_px <= 0 : never
			// 
			// From the 3 barycentric coordinates we have 3 equations. All of them
			// should be greater than or equal to 0 to draw a pixel.

			// Make sure we aren't in an invalid state.
			assert(!(w0_row < 0 && edge0.m_dx <= 0));
			assert(!(w1_row < 0 && edge1.m_dx <= 0));
			assert(!(w2_row < 0 && edge2.m_dx <= 0));

			// Calculate x range based on w0...
			if (w0_row >= 0 && edge0.m_dx < 0) {
				pxmax = core_mini32(pxmax, -(w0_row / edge0.m_dx));
			} else if (w0_row < 0 && edge0.m_dx > 0) {
				pxmin = core_maxi32(pxmin, (-w0_row / edge0.m_dx) + ((-w0_row % edge0.m_dx) != 0 ? 1 : 0));
			}

			// Calculate x range based on w1...
			if (w1_row >= 0 && edge1.m_dx < 0) {
				pxmax = core_mini32(pxmax, -(w1_row / edge1.m_dx));
			} else if (w1_row < 0 && edge1.m_dx > 0) {
				pxmin = core_maxi32(pxmin, (-w1_row / edge1.m_dx) + ((-w1_row % edge1.m_dx) != 0 ? 1 : 0));
			}

			// Calculate x range based on w2...
			if (w2_row >= 0 && edge2.m_dx < 0) {
				pxmax = core_mini32(pxmax, -(w2_row / edge2.m_dx));
			} else if (w2_row < 0 && edge2.m_dx > 0) {
				pxmin = core_maxi32(pxmin, (-w2_row / edge2.m_dx) + ((-w2_row % edge2.m_dx) != 0 ? 1 : 0));
			}
		}

		// Calculate barycentric coords at pxmin
		int32_t w0 = w0_row + pxmin * edge0.m_dx;
		int32_t w1 = w1_row + pxmin * edge1.m_dx;
		int32_t w2 = w2_row + pxmin * edge2.m_dx;

		for (int32_t px = pxmin; px <= pxmax; ++px) {
			// (px, py) is guaranteed to be inside the triangle (or on one of the edges)
			// Render the pixel
			{
				assert(w0 >= 0 && w1 >= 0 && w2 >= 0);

#if SWR_CONFIG_DISABLE_PIXEL_SHADERS
				const uint32_t rgba = 0xFFFFFFFF;
#else
				const float l0 = (float)w0 * inv_area;
				const float l1 = (float)w1 * inv_area;

				// l2 = 1.0f - (l0 + l1)
				//
				// attr = attr0 * l0 + attr1 * l1 + attr2 * l2 <=>
				// attr = attr0 * l0 + attr1 * l1 + attr2 * (1.0 - l0 - l1) <=>
				// attr = (attr0 - attr2) * l0 + (attr1 - attr2) * l1 + attr2 <=>
				// attr = dattr02 * l0 + dattr12 * l1 + attr2 <=>
				//
				// attr = fmad(dattr02, l0, fmad(dattr12, l1, attr2));
				const uint32_t cr = (uint32_t)(cr02 * l0 + cr12 * l1 + c2r);
				const uint32_t cg = (uint32_t)(cg02 * l0 + cg12 * l1 + c2g);
				const uint32_t cb = (uint32_t)(cb02 * l0 + cb12 * l1 + c2b);
				const uint32_t ca = (uint32_t)(ca02 * l0 + ca12 * l1 + c2a);
				const uint32_t rgba = SWR_COLOR(cr, cg, cb, ca);
#endif

				fb_row[px] = rgba;
			}

			w0 += edge0.m_dx;
			w1 += edge1.m_dx;
			w2 += edge2.m_dx;
		}

		w0_row += edge0.m_dy;
		w1_row += edge1.m_dy;
		w2_row += edge2.m_dy;
		fb_row += ctx->m_Width;
	}
}
