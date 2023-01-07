#ifndef SWR_SWR_H
#define SWR_SWR_H

#include <stdint.h>

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

typedef struct swr_font
{
	uint8_t* m_CharData;
	uint32_t m_CharWidth;
	uint32_t m_CharHeight;
	uint8_t m_CharMin;
	uint8_t m_CharMax;
	uint8_t m_MissingCharFallbackID;
} swr_font;

typedef struct swr_context
{
	uint32_t* m_FrameBuffer;
	uint32_t m_Width;
	uint32_t m_Height;
} swr_context;

typedef struct swr_api
{
	swr_context* (*createContext)(core_allocator_i* allocator, uint32_t w, uint32_t h);
	void (*destroyContext)(core_allocator_i* allocator, swr_context* ctx);

	void (*clear)(swr_context* ctx, uint32_t color);
	void (*drawPixel)(swr_context* ctx, int32_t x, int32_t y, uint32_t color);
	void (*drawLine)(swr_context* ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t color);
	void (*drawTriangle)(swr_context* ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color0, uint32_t color1, uint32_t color2);
	void (*drawText)(swr_context* ctx, const swr_font* font, int32_t x, int32_t y, const char* str, const char* end, uint32_t color);
} swr_api;

extern swr_api* swr;

#endif // CORE_SWR_H
