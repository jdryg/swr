#ifndef SWR_SWR_P_H
#define SWR_SWR_P_H

#include <stdint.h>

typedef struct swr_context
{
	uint32_t* m_FrameBuffer;
	uint32_t m_Width;
	uint32_t m_Height;
} swr_context;

#endif // SWR_SWR_P_H
