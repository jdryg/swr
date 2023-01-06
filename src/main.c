#include <stdint.h>
#include <memory.h>
#include <MiniFB.h>
#include "core/core.h"
#include "core/cpu.h"
#include "core/allocator.h"

int32_t main(void)
{
    coreInit(CORE_CPU_FEATURE_MASK_ALL);

    core_allocator_i* allocator = allocator_api->createAllocator("app");

	struct mfb_window* window = mfb_open_ex("swr", 800, 600, 0);
	if (!window) {
		return -1;
	}

    uint32_t* buffer = (uint32_t*)CORE_ALLOC(allocator, sizeof(uint32_t) * 800 * 600);
    if (!buffer) {
        mfb_close(window);
        return -2;
    }

    memset(buffer, 0, sizeof(uint32_t) * 800 * 600);

    do {
        // TODO: add some fancy rendering to the buffer of size 800 * 600

        const int32_t winState = mfb_update_ex(window, buffer, 800, 600);

        if (winState < 0) {
            window = NULL;
            break;
        }
    } while (mfb_wait_sync(window));

    CORE_FREE(allocator, buffer);
    allocator_api->destroyAllocator(allocator);
    coreShutdown();

	return 0;
}
