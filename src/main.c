#include <stdint.h>
#include <memory.h>
#include <MiniFB.h>
#include "core/core.h"
#include "core/cpu.h"
#include "core/allocator.h"
#include "core/os.h"
#include "core/string.h"
#include "swr/swr.h"
#include "fonts/font8x8_basic.h"

static const uint32_t kWinWidth = 1280;
static const uint32_t kWinHeight = 720;

int32_t main(void)
{
	coreInit(CORE_CPU_FEATURE_MASK_ALL);

	core_allocator_i* allocator = core_allocatorCreateAllocator("app");

	struct mfb_window* window = mfb_open_ex("swr", kWinWidth, kWinHeight, 0);
	if (!window) {
		return -1;
	}

	swr_context* swrCtx = swr->createContext(allocator, kWinWidth, kWinHeight);
	if (!swrCtx) {
		mfb_close(window);
		return -1;
	}

	swr_font font = {
		.m_CharData = kFont8x8_basic,
		.m_CharWidth = 8,
		.m_CharHeight = 8,
		.m_CharMin = 0,
		.m_CharMax = 0x7F,
		.m_MissingCharFallbackID = 0
	};

	do {
		swr->clear(swrCtx, SWR_COLOR_BLACK);

		const uint64_t tStart = core_osTimeNow();
		{
			swr->drawTriangle(swrCtx, 100, 100, 100, 400, 500, 400, SWR_COLOR_RED, SWR_COLOR_GREEN, SWR_COLOR_BLUE);
		}
		const uint64_t tDelta = core_osTimeDiff(core_osTimeNow(), tStart);

		char str[256];
		core_snprintf(str, CORE_COUNTOF(str), "Frame Time [ms]: %.2f", core_osTimeConvertTo(tDelta, CORE_TIME_UNITS_MS));
		swr->drawText(swrCtx, &font, 5, 5, str, NULL, SWR_COLOR_WHITE);

		const int32_t winState = mfb_update_ex(window, swrCtx->m_FrameBuffer, swrCtx->m_Width, swrCtx->m_Height);
		if (winState < 0) {
			window = NULL;
			break;
		}
	} while (mfb_wait_sync(window));

	swr->destroyContext(allocator, swrCtx);
	core_allocatorDestroyAllocator(allocator);
	coreShutdown();

	return 0;
}
