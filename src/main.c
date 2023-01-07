#include <stdint.h>
#include <memory.h>
#include <MiniFB.h>
#include "core/core.h"
#include "core/cpu.h"
#include "core/allocator.h"
#include "swr/swr.h"

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

    do {
        swr->clear(swrCtx, SWR_COLOR_RED);

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
