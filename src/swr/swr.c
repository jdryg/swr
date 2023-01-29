#include "swr.h"
#include "swr_p.h"
#include "../core/allocator.h"
#include "../core/memory.h"
#include "../core/string.h"
#include "../core/math.h"
#include "../core/cpu.h"
#include <stdbool.h>

static swr_context* swrCreateContext(core_allocator_i* allocator, uint32_t w, uint32_t h);
static void swrDestroyContext(core_allocator_i* allocator, swr_context* ctx);
static const void* swrGetFrameBufferPtr(swr_context* ctx);
static void swrClear(swr_context* ctx, uint32_t color);
static void swrDrawPixel(swr_context* ctx, int32_t x, int32_t y, uint32_t color);
static void swrDrawLine(swr_context* ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color);
static void swrDrawTriangleDispatch(swr_context* ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color0, uint32_t color1, uint32_t color2);
static void swrDrawText(swr_context* ctx, const swr_font* font, int32_t x0, int32_t y0, const char* str, const char* end, uint32_t color);

swr_api* swr = &(swr_api){
	.createContext = swrCreateContext,
	.destroyContext = swrDestroyContext,
	.getFrameBufferPtr = swrGetFrameBufferPtr,
	.clear = swrClear,
	.drawPixel = swrDrawPixel,
	.drawLine = swrDrawLine,
	.drawTriangle = swrDrawTriangleDispatch,
	.drawText = swrDrawText
};

static swr_context* swrCreateContext(core_allocator_i* allocator, uint32_t w, uint32_t h)
{
	swr_context* ctx = (swr_context*)CORE_ALLOC(allocator, sizeof(swr_context));
	if (!ctx) {
		return NULL;
	}

	core_memSet(ctx, 0, sizeof(swr_context));
	ctx->m_FrameBuffer = (uint32_t*)CORE_ALIGNED_ALLOC(allocator, sizeof(uint32_t) * (size_t)w * (size_t)h, 16);
	if (!ctx->m_FrameBuffer) {
		swrDestroyContext(allocator, ctx);
		return NULL;
	}

	core_memSet(ctx->m_FrameBuffer, 0, sizeof(uint32_t) * (size_t)w * (size_t)h);
	ctx->m_Width = w;
	ctx->m_Height = h;

	return ctx;
}

static void swrDestroyContext(core_allocator_i* allocator, swr_context* ctx)
{
	CORE_ALIGNED_FREE(allocator, ctx->m_FrameBuffer, 16);
	CORE_FREE(allocator, ctx);
}

static const void* swrGetFrameBufferPtr(swr_context* ctx)
{
	return ctx->m_FrameBuffer;
}

static void swrClear(swr_context* ctx, uint32_t color)
{
	uint32_t* buffer = ctx->m_FrameBuffer;
	const uint32_t numPixels = ctx->m_Width * ctx->m_Height;
	for (uint32_t i = 0; i < numPixels; ++i) {
		*buffer++ = color;
	}
}

static void swrDrawPixel(swr_context* ctx, int32_t x, int32_t y, uint32_t color)
{
	if (x < 0 || x >= (int32_t)ctx->m_Width || y < 0 || y >= (int32_t)ctx->m_Height) {
		return;
	}

	ctx->m_FrameBuffer[x + y * ctx->m_Width] = color;
}

static void swrDrawLine(swr_context* ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color)
{
	bool steep = false;
	if (core_absi32(x0 - x1) < core_absi32(y0 - y1)) {
		{ int32_t tmp = x0; x0 = y0; y0 = tmp; }
		{ int32_t tmp = x1; x1 = y1; y1 = tmp; }
		steep = true;
	}

	if (x0 > x1) {
		{ int32_t tmp = x0; x0 = x1; x1 = tmp; }
		{ int32_t tmp = y0; y0 = y1; y1 = tmp; }
	}

	const int32_t dx = x1 - x0;
	const int32_t derror2 = core_absi32(y1 - y0) * 2;
	const int32_t yinc = y1 > y0 ? 1 : -1;

	int32_t error2 = 0;
	int32_t y = y0;

	if (steep) {
		for (int32_t x = x0; x <= x1; x++) {
			swrDrawPixel(ctx, y, x, color);

			error2 += derror2;
			if (error2 > dx) {
				y += yinc;
				error2 -= dx * 2;
			}
		}
	} else {
		for (int32_t x = x0; x <= x1; x++) {
			swrDrawPixel(ctx, x, y, color);

			error2 += derror2;
			if (error2 > dx) {
				y += yinc;
				error2 -= dx * 2;
			}
		}
	}
}

static void swrDrawText(swr_context* ctx, const swr_font* font, int32_t x0, int32_t y0, const char* str, const char* end, uint32_t color)
{
	end = end != NULL
		? end
		: str + core_strlen(str)
		;

	const int32_t chw = (int32_t)font->m_CharWidth;
	const int32_t chh = (int32_t)font->m_CharHeight;
	const uint8_t* chdata = font->m_CharData;

	int32_t x = x0;
	int32_t y = y0;
	while (str != end) {
		char ch = *str;
		if (ch < font->m_CharMin || ch > font->m_CharMax) {
			ch = font->m_MissingCharFallbackID;
		}

		const uint8_t chID = (uint8_t)ch - font->m_CharMin;
		const uint8_t* charData = &chdata[chID * chh];
		for (int32_t chy = 0; chy < chh; ++chy) {
			const uint8_t chrow = charData[chy];
			for (int32_t chx = 0; chx < chw; ++chx) {
				if ((chrow & (1u << chx)) != 0) {
					swrDrawPixel(ctx, x + chx, y + chy, color);
				}
			}
		}

		x += chw;

		++str;
	}
}

extern void swrDrawTriangleRef(swr_context* ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color0, uint32_t color1, uint32_t color2);
extern void swrDrawTriangleSSE2(swr_context* ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color0, uint32_t color1, uint32_t color2);
extern void swrDrawTriangleSSSE3(swr_context* ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color0, uint32_t color1, uint32_t color2);
extern void swrDrawTriangleSSE41(swr_context* ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color0, uint32_t color1, uint32_t color2);

static void swrDrawTriangleDispatch(swr_context* ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color0, uint32_t color1, uint32_t color2)
{
#if 1
	const uint64_t cpuFeatures = core_cpuGetFeatures();
	if ((cpuFeatures & CORE_CPU_FEATURE_SSE4_1) != 0) {
		swr->drawTriangle = swrDrawTriangleSSE41;
	} else if ((cpuFeatures & CORE_CPU_FEATURE_SSSE3) != 0) {
		swr->drawTriangle = swrDrawTriangleSSSE3;
	} else if ((cpuFeatures & CORE_CPU_FEATURE_SSE2) != 0) {
		swr->drawTriangle = swrDrawTriangleSSE2;
	} else {
		swr->drawTriangle = swrDrawTriangleRef;
	}
#else
	swr->drawTriangle = swrDrawTriangleRef;
#endif

	// Call the new function
	swr->drawTriangle(ctx, x0, y0, x1, y1, x2, y2, color0, color1, color2);
}
