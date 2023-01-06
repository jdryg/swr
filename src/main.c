#include <stdint.h>
#include <malloc.h>
#include <MiniFB.h>

int32_t main(void)
{
	struct mfb_window* window = mfb_open_ex("swr", 800, 600, WF_RESIZABLE);
	if (!window) {
		return -1;
	}

    uint32_t* buffer = (uint32_t*)malloc(800 * 600 * 4);

    do {
        // TODO: add some fancy rendering to the buffer of size 800 * 600

        const int32_t winState = mfb_update_ex(window, buffer, 800, 600);

        if (winState < 0) {
            window = NULL;
            break;
        }
    } while (mfb_wait_sync(window));

	return 0;
}
