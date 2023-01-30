#include "allocator.h"
#include "memory.h"
#include <stdbool.h>
#include <malloc.h>

static void* allocator_sysRealloc(core_allocator_o* a, void* ptr, uint64_t sz, uint64_t align, const char* file, uint32_t line);
static core_allocator_i* allocator_createAllocator(const char* name);
static void allocator_destroyAllocator(core_allocator_i* alloc);
static core_allocator_i* allocator_createLinearAllocator(uint32_t initialSize, const core_allocator_i* backingAllocator);
static core_allocator_i* allocator_createLinearAllocatorWithBuffer(uint8_t* buffer, uint32_t sz, const core_allocator_i* backingAllocator);
static void allocator_destroyLinearAllocator(core_allocator_i* allocator);
static void allocator_resetLinearAllocator(core_allocator_i* allocator);

static const uint64_t kSystemAllocatorNaturalAlignment = 8;

static core_allocator_i g_SystemAllocator = {
	.m_Inst = NULL,
	.realloc = allocator_sysRealloc
};

core_allocator_api* allocator_api = &(core_allocator_api){
	.m_SystemAllocator = NULL,
	.createAllocator = allocator_createAllocator,
	.destroyAllocator = allocator_destroyAllocator,
	.createLinearAllocator = allocator_createLinearAllocator,
	.createLinearAllocatorWithBuffer = allocator_createLinearAllocatorWithBuffer,
	.destroyLinearAllocator = allocator_destroyLinearAllocator,
	.resetLinearAllocator = allocator_resetLinearAllocator,
};

bool core_allocator_initAPI(void)
{
	allocator_api->m_SystemAllocator = allocator_createAllocator("system");
	if (!allocator_api->m_SystemAllocator) {
		return false;
	}

	return true;
}

void core_allocator_shutdownAPI(void)
{
	if (allocator_api->m_SystemAllocator) {
		allocator_destroyAllocator(allocator_api->m_SystemAllocator);
		allocator_api->m_SystemAllocator = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////
// Internal API
//
// bx::DefaultAllocator::realloc
static void* allocator_sysRealloc(core_allocator_o* allocator, void* ptr, uint64_t sz, uint64_t align, const char* file, uint32_t line)
{
	if (sz == 0) {
		if (ptr != NULL) {
			if (align <= kSystemAllocatorNaturalAlignment) {
				free(ptr);
			} else {
				_aligned_free(ptr);
			}
		}

		return NULL;
	} else if (ptr == NULL) {
		return (align <= kSystemAllocatorNaturalAlignment)
			? malloc(sz)
			: _aligned_malloc(sz, align)
			;
	}

	return align <= kSystemAllocatorNaturalAlignment
		? realloc(ptr, sz)
		: _aligned_realloc(ptr, sz, align)
		;
}

static core_allocator_i* allocator_createAllocator(const char* name)
{
	core_allocator_i* allocator = NULL;

#if JX_CONFIG_TRACE_ALLOCATIONS
	// TODO: Create tracing allocator
#else
	allocator = &g_SystemAllocator;
#endif

	return allocator;
}

static void allocator_destroyAllocator(core_allocator_i* allocator)
{
#if JX_CONFIG_TRACE_ALLOCATIONS
	// TODO: Destroy tracing allocator
#endif
}

//////////////////////////////////////////////////////////////////////////
// Linear allocator
//
#define LINEAR_ALLOCATOR_FLAGS_ALLOW_RESIZE    (1u << 0)
#define LINEAR_ALLOCATOR_FLAGS_FREE_ALLOCATOR  (1u << 1)

typedef struct linear_allocator_chunk
{
	struct linear_allocator_chunk* m_Next;
	uint8_t* m_Buffer;
	uint32_t m_Pos;
	uint32_t m_Capacity;
} linear_allocator_chunk;

typedef struct linear_allocator_o
{
	const core_allocator_i* m_ParentAllocator;
	linear_allocator_chunk* m_FirstChunk;
	linear_allocator_chunk* m_LastChunk;
	uint32_t m_ChunkSize;
	uint32_t m_Flags;
} linear_allocator_o;

static void* allocator_linearRealloc(core_allocator_o* allocator, void* ptr, uint64_t sz, uint64_t align, const char* file, uint32_t line);

static core_allocator_i* allocator_createLinearAllocator(uint32_t chunkSize, const core_allocator_i* backingAllocator)
{
	const core_allocator_i* allocator = backingAllocator == NULL
		? allocator_api->m_SystemAllocator
		: backingAllocator
		;

	const size_t totalMem = 0
		+ sizeof(core_allocator_i)
		+ sizeof(linear_allocator_o)
		+ sizeof(linear_allocator_chunk)
		+ (size_t)chunkSize
		;
	uint8_t* buffer = (uint8_t*)CORE_ALLOC(allocator, totalMem);
	if (!buffer) {
		return NULL;
	}

	core_allocator_i* linearAllocatorInterface = allocator_createLinearAllocatorWithBuffer(buffer, (uint32_t)totalMem, backingAllocator);
	if (linearAllocatorInterface) {
		linear_allocator_o* linearAllocator = (linear_allocator_o*)linearAllocatorInterface->m_Inst;
		linearAllocator->m_Flags |= LINEAR_ALLOCATOR_FLAGS_FREE_ALLOCATOR;
		linearAllocator->m_ParentAllocator = allocator;
	}

	return linearAllocatorInterface;
}

static core_allocator_i* allocator_createLinearAllocatorWithBuffer(uint8_t* buffer, uint32_t sz, const core_allocator_i* backingAllocator)
{
	const size_t requiredMemory = 0
		+ sizeof(core_allocator_i)
		+ sizeof(linear_allocator_o)
		+ sizeof(linear_allocator_chunk)
		+ 8 // Make sure there is at least 8 bytes of free memory for allocations
		;
	if (sz < requiredMemory) {
//		CORE_CHECK(false, "Linear allocator buffer too small.", 0);
		return NULL;
	}

	uint8_t* ptr = buffer;
	core_allocator_i* linearAllocatorInterface = (core_allocator_i*)ptr;
	ptr += sizeof(core_allocator_i);

	linear_allocator_o* linearAllocator = (linear_allocator_o*)ptr;
	ptr += sizeof(linear_allocator_o);

	linear_allocator_chunk* linearAllocatorChunk = (linear_allocator_chunk*)ptr;
	ptr += sizeof(linear_allocator_chunk);

	linearAllocatorChunk->m_Buffer = ptr;
	linearAllocatorChunk->m_Next = NULL;
	linearAllocatorChunk->m_Pos = 0;
	linearAllocatorChunk->m_Capacity = sz - (uint32_t)(ptr - buffer);

	linearAllocator->m_ParentAllocator = backingAllocator;
	linearAllocator->m_FirstChunk = linearAllocatorChunk;
	linearAllocator->m_LastChunk = linearAllocatorChunk;
	linearAllocator->m_ChunkSize = sz;
	linearAllocator->m_Flags = 0
		| (backingAllocator != NULL ? LINEAR_ALLOCATOR_FLAGS_ALLOW_RESIZE : 0)
		;

	*linearAllocatorInterface = (core_allocator_i){
		.m_Inst = (core_allocator_o*)linearAllocator,
		.realloc = allocator_linearRealloc
	};

	return linearAllocatorInterface;
}

static void allocator_destroyLinearAllocator(core_allocator_i* allocator)
{
	linear_allocator_o* linearAllocator = (linear_allocator_o*)allocator->m_Inst;

	// NOTE: First chunk is always allocated with the allocator itself.
	// If there are any other chunks it means that they have been allocated with 'm_ParentAllocator'.
	linear_allocator_chunk* chunk = linearAllocator->m_FirstChunk->m_Next;
	while (chunk) {
		linear_allocator_chunk* nextChunk = chunk->m_Next;
		CORE_FREE(linearAllocator->m_ParentAllocator, chunk);
		chunk = nextChunk;
	}

	if ((linearAllocator->m_Flags & LINEAR_ALLOCATOR_FLAGS_FREE_ALLOCATOR) != 0) {
		CORE_FREE(linearAllocator->m_ParentAllocator, allocator);
	}
}

static void allocator_resetLinearAllocator(core_allocator_i* allocator)
{
	linear_allocator_o* linearAllocator = (linear_allocator_o*)allocator->m_Inst;

	linear_allocator_chunk* chunk = linearAllocator->m_FirstChunk;
	while (chunk) {
		chunk->m_Pos = 0;
		chunk = chunk->m_Next;
	}

	linearAllocator->m_LastChunk = linearAllocator->m_FirstChunk;
}

static void* allocator_linearRealloc(core_allocator_o* inst, void* ptr, uint64_t sz, uint64_t align, const char* file, uint32_t line)
{
//	CORE_CHECK(sz < UINT32_MAX, "Linear allocator cannot allocate more than 4GB per allocation.", 0);

	if (ptr != NULL) {
		// Realloc or free.
		// - Reallocs are not supported.
		// - Frees are ignored.
		// TODO: Allow resizing/freeing the last allocated block? (see https://www.gingerbill.org/article/2019/02/08/memory-allocation-strategies-002/)
//		CORE_CHECK(sz == 0, "Linear allocators do not support reallocations.", 0);
		return NULL;
	}

	linear_allocator_o* allocator = (linear_allocator_o*)inst;
	linear_allocator_chunk* lastChunk = allocator->m_LastChunk;

	if (align < kSystemAllocatorNaturalAlignment) {
		align = kSystemAllocatorNaturalAlignment;
	}

	// Align buffer pointer to 'align'
	uint8_t* bufferPtr = core_alignPtr(lastChunk->m_Buffer + lastChunk->m_Pos, align);

	const uint8_t* bufferEnd = lastChunk->m_Buffer + lastChunk->m_Capacity;
	if (bufferPtr + sz > bufferEnd) {
		if ((allocator->m_Flags & LINEAR_ALLOCATOR_FLAGS_ALLOW_RESIZE) == 0) {
			// Resize not allowed.
			return NULL;
		}

		// TODO: Make sure the new 'bufferPtr' is properly aligned.
		if (lastChunk->m_Next == NULL) {
			// Allocate new chunk, insest into list, update 'lastChunk' pointer
			// and let the code below handle the actual allocation.
			const uint32_t chunkSize = allocator->m_ChunkSize < (uint32_t)sz ? (uint32_t)sz : allocator->m_ChunkSize;
			const size_t totalMem = 0
				+ sizeof(linear_allocator_chunk)
				+ (size_t)chunkSize
				;
			uint8_t* newBuffer = (uint8_t*)allocator->m_ParentAllocator->realloc(allocator->m_ParentAllocator->m_Inst, NULL, totalMem, 0, file, line);
			if (!newBuffer) {
				// Couldn't allocate new chunk. Allocation failed.
				return NULL;
			}

			uint8_t* newBufferPtr = newBuffer;
			linear_allocator_chunk* linearAllocatorChunk = (linear_allocator_chunk*)newBufferPtr;
			newBufferPtr += sizeof(linear_allocator_chunk);

			linearAllocatorChunk->m_Buffer = newBufferPtr;
			linearAllocatorChunk->m_Next = NULL;
			linearAllocatorChunk->m_Pos = 0;
			linearAllocatorChunk->m_Capacity = chunkSize;

			lastChunk->m_Next = linearAllocatorChunk;

			allocator->m_LastChunk = linearAllocatorChunk;
		} else {
			allocator->m_LastChunk = lastChunk->m_Next;
		}

		lastChunk = allocator->m_LastChunk;
		bufferPtr = lastChunk->m_Buffer;
	}

//	CORE_CHECK(jx_isAlignedPtr(bufferPtr, align), "Buffer is not aligned properly.", 0);
	lastChunk->m_Pos = (uint32_t)((bufferPtr + sz) - lastChunk->m_Buffer);
	return bufferPtr;
}
