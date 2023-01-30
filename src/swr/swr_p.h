#ifndef SWR_SWR_P_H
#define SWR_SWR_P_H

#include <stdint.h>

typedef struct core_allocator_i core_allocator_i;

typedef struct swr_vertex_buffer
{
	const void* m_Ptr;
	uint32_t m_Count;
	uint16_t m_Format;
	uint16_t m_Stride;
} swr_vertex_buffer;

typedef struct swr_index_buffer
{
	const uint16_t* m_Ptr;
	uint32_t m_Count;
	uint32_t _padding;
} swr_index_buffer;

typedef struct swr_context
{
	core_allocator_i* m_TempAllocator;
	uint32_t* m_FrameBuffer;
	uint32_t m_Width;
	uint32_t m_Height;
	swr_index_buffer m_IndexBuffer;
	swr_vertex_buffer m_VertexBuffers[2]; // { Position, Color }
	uint32_t m_BoundBuffers;
	swr_matrix2d m_WorldToScreenTransform;
} swr_context;

#endif // SWR_SWR_P_H
