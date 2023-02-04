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
static void swrSetWorldToScreenTransform(swr_context* ctx, const swr_matrix2d* mtx);
static void swrBindVertexBuffer(swr_context* ctx, swr_vertex_attrib va, swr_format format, uint32_t stride, uint32_t n, const void* ptr);
static void swrUnbindVertexBuffer(swr_context* ctx, swr_vertex_attrib va);
static void swrBindIndexBuffer(swr_context* ctx, uint32_t n, const uint16_t* ptr);
static void swrUnbindIndexBuffer(swr_context* ctx);
static void swrDrawPrimitives(swr_context* ctx, swr_primitive_type primType, uint16_t startIndex, uint16_t endIndex, uint32_t numIndices, uint32_t baseIndex, uint32_t baseVertex);
static void swrDrawPixel(swr_context* ctx, int32_t x, int32_t y, uint32_t color);
static void swrDrawLine(swr_context* ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color);
static void swrDrawTriangleDispatch(swr_context* ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color0, uint32_t color1, uint32_t color2);
static void swrDrawText(swr_context* ctx, const swr_font* font, int32_t x0, int32_t y0, const char* str, const char* end, uint32_t color);

static void swrTransformPos2fTo2i(uint32_t n, const float* posf, int32_t* posi, const float* mtx);

swr_api* swr = &(swr_api){
	.createContext = swrCreateContext,
	.destroyContext = swrDestroyContext,
	.getFrameBufferPtr = swrGetFrameBufferPtr,
	.clear = swrClear,
	.setWorldToScreenTransform = swrSetWorldToScreenTransform,
	.bindVertexBuffer = swrBindVertexBuffer,
	.unbindVertexBuffer = swrUnbindVertexBuffer,
	.bindIndexBuffer = swrBindIndexBuffer,
	.unbindIndexBuffer = swrUnbindIndexBuffer,
	.drawPrimitives = swrDrawPrimitives,
	.drawPixel = swrDrawPixel,
	.drawLine = swrDrawLine,
	.drawTriangle = swrDrawTriangleDispatch,
	.drawText = swrDrawText,

	.transformPos2fTo2i = swrTransformPos2fTo2i
};

static swr_context* swrCreateContext(core_allocator_i* allocator, uint32_t w, uint32_t h)
{
	swr_context* ctx = (swr_context*)CORE_ALLOC(allocator, sizeof(swr_context));
	if (!ctx) {
		return NULL;
	}

	core_memSet(ctx, 0, sizeof(swr_context));
	ctx->m_BoundBuffers = 0;

	ctx->m_FrameBuffer = (uint32_t*)CORE_ALIGNED_ALLOC(allocator, sizeof(uint32_t) * (size_t)w * (size_t)h, 16);
	if (!ctx->m_FrameBuffer) {
		swrDestroyContext(allocator, ctx);
		return NULL;
	}

	core_memSet(ctx->m_FrameBuffer, 0, sizeof(uint32_t) * (size_t)w * (size_t)h);
	ctx->m_Width = w;
	ctx->m_Height = h;

	ctx->m_TempAllocator = core_allocatorCreateLinearAllocator(4 << 20, allocator);
	if (!ctx->m_TempAllocator) {
		swrDestroyContext(allocator, ctx);
		return NULL;
	}

	swrMatrix2DIdentity(&ctx->m_WorldToScreenTransform);

	return ctx;
}

static void swrDestroyContext(core_allocator_i* allocator, swr_context* ctx)
{
	if (ctx->m_TempAllocator) {
		core_allocatorDestroyLinearAllocator(ctx->m_TempAllocator);
		ctx->m_TempAllocator = NULL;
	}

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

static void swrSetWorldToScreenTransform(swr_context* ctx, const swr_matrix2d* mtx)
{
	core_memCopy(&ctx->m_WorldToScreenTransform, mtx, sizeof(swr_matrix2d));
}

static void swrBindVertexBuffer(swr_context* ctx, swr_vertex_attrib va, swr_format format, uint32_t stride, uint32_t n, const void* ptr)
{
	swr_vertex_buffer* vb = &ctx->m_VertexBuffers[va];
	vb->m_Ptr = ptr;
	vb->m_Count = n;
	vb->m_Format = format;
	vb->m_Stride = stride;

	ctx->m_BoundBuffers |= (1u << va);
}

static void swrUnbindVertexBuffer(swr_context* ctx, swr_vertex_attrib va)
{
	ctx->m_BoundBuffers &= ~(1u << va);
}

static void swrBindIndexBuffer(swr_context* ctx, uint32_t n, const uint16_t* ptr)
{
	swr_index_buffer* ib = &ctx->m_IndexBuffer;
	ib->m_Ptr = ptr;
	ib->m_Count = n;

	ctx->m_BoundBuffers |= (1u << 31);
}

static void swrUnbindIndexBuffer(swr_context* ctx)
{
	ctx->m_BoundBuffers &= ~(1u << 31);
}

static void swrDrawPrimitives(swr_context* ctx, swr_primitive_type primType, uint16_t startIndex, uint16_t endIndex, uint32_t numIndices, uint32_t baseIndex, uint32_t baseVertex)
{
	if (primType != SWR_PRIMITIVE_TYPE_TRIANGLE_LIST || numIndices == 0 || startIndex >= endIndex) {
		return;
	}

	const uint32_t boundBuffers = ctx->m_BoundBuffers;
	const bool hasPos = (boundBuffers & (1u << SWR_VERTEX_ATTRIB_POSITION)) != 0;
	const bool hasColor = (boundBuffers & (1u << SWR_VERTEX_ATTRIB_COLOR)) != 0;
	const bool hasIndex = (boundBuffers & (1u << 31)) != 0;
	if (!hasPos || !hasIndex) {
		return;
	}

	core_allocatorResetLinearAllocator(ctx->m_TempAllocator);

	// Transform vertex position to screen space
	const swr_vertex_buffer* posBuffer = &ctx->m_VertexBuffers[SWR_VERTEX_ATTRIB_POSITION];
	const uint32_t maxVertices = (uint32_t)(endIndex - startIndex) + 1;
	if ((baseVertex + maxVertices) > posBuffer->m_Count) {
		return;
	}

	int32_t* posBufferScreen = (int32_t*)CORE_ALLOC(ctx->m_TempAllocator, sizeof(int32_t) * 2 * maxVertices);
	if (posBuffer->m_Format == SWR_FORMAT_2F && posBuffer->m_Stride == 0) {
		const float* posBufferPtr = (float*)posBuffer->m_Ptr;
		const float* posBufferWorld = &posBufferPtr[baseVertex * 2];
		swr->transformPos2fTo2i(maxVertices, posBufferWorld, posBufferScreen, ctx->m_WorldToScreenTransform.m_Elem);
	} else {
		// TODO: Combination not implemented yet.
	}

	// Rasterize all primitives
	if (primType == SWR_PRIMITIVE_TYPE_TRIANGLE_LIST) {
		const uint32_t numTriangles = numIndices / 3;

		const swr_vertex_buffer* colorBuffer = &ctx->m_VertexBuffers[SWR_VERTEX_ATTRIB_COLOR];
		const bool colorBufferIsValid = colorBuffer->m_Stride == 0
			&& (colorBuffer->m_Format == SWR_FORMAT_1UI || colorBuffer->m_Format == SWR_FORMAT_4UB)
			;

		if (hasColor && colorBufferIsValid && colorBuffer->m_Count == posBuffer->m_Count) {
			// Per-vertex color triangle rasterization
			const uint32_t* colorBufferPtr = (uint32_t*)colorBuffer->m_Ptr;
			const uint32_t* colorPtr = &colorBufferPtr[baseVertex];
			const uint16_t* indexPtr = &ctx->m_IndexBuffer.m_Ptr[baseIndex];
			for (uint32_t iTri = 0; iTri < numTriangles; ++iTri) {
				const uint16_t id0 = indexPtr[0];
				const uint16_t id1 = indexPtr[1];
				const uint16_t id2 = indexPtr[2];

				// TODO: Check if indices are in bounds.

				swr->drawTriangle(ctx
					, posBufferScreen[id0 * 2 + 0], posBufferScreen[id0 * 2 + 1]
					, posBufferScreen[id1 * 2 + 0], posBufferScreen[id1 * 2 + 1]
					, posBufferScreen[id2 * 2 + 0], posBufferScreen[id2 * 2 + 1]
					, colorPtr[id0]
					, colorPtr[id1]
					, colorPtr[id2]
				);

				indexPtr += 3;
			}
		} else {
			// Constant color triangle rasterization.
			const uint32_t color = colorBufferIsValid
				? ((uint32_t*)colorBuffer->m_Ptr)[0]
				: SWR_COLOR_WHITE
				;

			const uint16_t* indexPtr = &ctx->m_IndexBuffer.m_Ptr[baseIndex];
			for (uint32_t iTri = 0; iTri < numTriangles; ++iTri) {
				const uint16_t id0 = indexPtr[0];
				const uint16_t id1 = indexPtr[1];
				const uint16_t id2 = indexPtr[2];

				// TODO: Check if indices are in bounds.

				// TODO: Optimized drawTriangle without color interpolation
				swr->drawTriangle(ctx
					, posBufferScreen[id0 * 2 + 0], posBufferScreen[id0 * 2 + 1]
					, posBufferScreen[id1 * 2 + 0], posBufferScreen[id1 * 2 + 1]
					, posBufferScreen[id2 * 2 + 0], posBufferScreen[id2 * 2 + 1]
					, color
					, color
					, color
				);

				indexPtr += 3;
			}
		}
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

static void swrTransformPos2fTo2i(uint32_t n, const float* posf, int32_t* posi, const float* mtx)
{
	const float* src = posf;
	int32_t* dst = posi;
	for (uint32_t i = 0; i < n; ++i) {
		const float px = src[0];
		const float py = src[1];

		dst[0] = (int32_t)(mtx[0] * px + mtx[2] * py + mtx[4]);
		dst[1] = (int32_t)(mtx[1] * px + mtx[3] * py + mtx[5]);

		src += 2;
		dst += 2;
	}
}
