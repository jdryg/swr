#ifndef SWR_SWR_H
#define SWR_SWR_H

#include <stdint.h>
#include "../core/math.h"

typedef struct core_allocator_i core_allocator_i;

#ifndef SWR_CONFIG_DISABLE_PIXEL_SHADERS
#define SWR_CONFIG_DISABLE_PIXEL_SHADERS 0
#endif

#define SWR_COLOR_FORMAT_RGBA 0
#define SWR_COLOR_FORMAT_BGRA 1

#ifndef SWR_FRAMEBUFFER_FORMAT
#define SWR_FRAMEBUFFER_FORMAT SWR_COLOR_FORMAT_BGRA
#endif

#if SWR_FRAMEBUFFER_FORMAT == SWR_COLOR_FORMAT_BGRA
#define SWR_COLOR_BLUE_Pos     0
#define SWR_COLOR_GREEN_Pos    8
#define SWR_COLOR_RED_Pos      16
#define SWR_COLOR_ALPHA_Pos    24
#else
#define SWR_COLOR_RED_Pos      0
#define SWR_COLOR_GREEN_Pos    8
#define SWR_COLOR_BLUE_Pos     16
#define SWR_COLOR_ALPHA_Pos    24
#endif

#define SWR_COLOR_RED_Msk      (0xFF << SWR_COLOR_RED_Pos)
#define SWR_COLOR_GREEN_Msk    (0xFF << SWR_COLOR_GREEN_Pos)
#define SWR_COLOR_BLUE_Msk     (0xFF << SWR_COLOR_BLUE_Pos)
#define SWR_COLOR_ALPHA_Msk    (0xFF << SWR_COLOR_ALPHA_Pos)
#define SWR_COLOR(r, g, b, a)  (0 \
	| (((r) << SWR_COLOR_RED_Pos) & SWR_COLOR_RED_Msk) \
	| (((g) << SWR_COLOR_GREEN_Pos) & SWR_COLOR_GREEN_Msk) \
	| (((b) << SWR_COLOR_BLUE_Pos) & SWR_COLOR_BLUE_Msk) \
	| (((a) << SWR_COLOR_ALPHA_Pos) & SWR_COLOR_ALPHA_Msk))

#define SWR_COLOR_BLACK        SWR_COLOR(0, 0, 0, 255)
#define SWR_COLOR_RED          SWR_COLOR(255, 0, 0, 255)
#define SWR_COLOR_GREEN        SWR_COLOR(0, 255, 0, 255)
#define SWR_COLOR_BLUE         SWR_COLOR(0, 0, 255, 255)
#define SWR_COLOR_YELLOW       SWR_COLOR(255, 255, 0, 255)
#define SWR_COLOR_WHITE        SWR_COLOR(255, 255, 255, 255)
#define SWR_COLOR_GRAY         SWR_COLOR(128, 128, 128, 255)

typedef enum swr_vertex_attrib
{
	SWR_VERTEX_ATTRIB_POSITION = 0,
	SWR_VERTEX_ATTRIB_COLOR
} swr_vertex_attrib;

typedef enum swr_data_type
{
	SWR_DATA_TYPE_FLOAT,
	SWR_DATA_TYPE_UBYTE,
	SWR_DATA_TYPE_UINT,
} swr_data_type;

typedef enum swr_format
{
	SWR_FORMAT_2F,
	SWR_FORMAT_4UB,
	SWR_FORMAT_1UI
} swr_format;

typedef enum swr_primitive_type
{
	SWR_PRIMITIVE_TYPE_TRIANGLE_LIST
} swr_primitive_type;

typedef struct swr_font
{
	const uint8_t* m_CharData;
	uint32_t m_CharWidth;
	uint32_t m_CharHeight;
	uint8_t m_CharMin;
	uint8_t m_CharMax;
	uint8_t m_MissingCharFallbackID;
} swr_font;

typedef struct swr_matrix2d
{
	float m_Elem[6];
} swr_matrix2d;

typedef struct swr_context swr_context;

typedef struct swr_api
{
	swr_context* (*createContext)(core_allocator_i* allocator, uint32_t w, uint32_t h);
	void (*destroyContext)(core_allocator_i* allocator, swr_context* ctx);

	const void* (*getFrameBufferPtr)(swr_context* ctx);
	void (*clear)(swr_context* ctx, uint32_t color);
	void (*setWorldToScreenTransform)(swr_context* ctx, const swr_matrix2d* mtx);

	void (*bindVertexBuffer)(swr_context* ctx, swr_vertex_attrib va, swr_format format, uint32_t stride, uint32_t n, const void* ptr);
	void (*unbindVertexBuffer)(swr_context* ctx, swr_vertex_attrib va);
	void (*bindIndexBuffer)(swr_context* ctx, uint32_t n, const uint16_t* ptr);
	void (*unbindIndexBuffer)(swr_context* ctx);
	void (*drawPrimitives)(swr_context* ctx, swr_primitive_type primType, uint16_t startIndex, uint16_t endIndex, uint32_t numIndices, uint32_t baseIndex, uint32_t baseVertex);

	void (*drawPixel)(swr_context* ctx, int32_t x, int32_t y, uint32_t color);
	void (*drawLine)(swr_context* ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color);
	void (*drawTriangle)(swr_context* ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color0, uint32_t color1, uint32_t color2);
	void (*drawText)(swr_context* ctx, const swr_font* font, int32_t x, int32_t y, const char* str, const char* end, uint32_t color);

	void (*transformPos2fTo2i)(uint32_t n, const float* posf, int32_t* posi, const float* mtx);
} swr_api;

extern swr_api* swr;

static void swrMatrix2DIdentity(swr_matrix2d* mtx);
static void swrMatrix2DTranslate(swr_matrix2d* mtx, float x, float y);
static void swrMatrix2DScale(swr_matrix2d* mtx, float x, float y);
static void swrMatrix2DRotate(swr_matrix2d* mtx, float angle_deg);

#endif // SWR_SWR_H

#include "inline/swr.inl"
