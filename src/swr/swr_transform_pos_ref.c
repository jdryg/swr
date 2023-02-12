#include "swr.h"
#include "swr_p.h"

void swrTransformPos2fTo2iRef(uint32_t n, const float* posf, int32_t* posi, const float* mtx)
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
