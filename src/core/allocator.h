#ifndef CORE_ALLOCATOR_H
#define CORE_ALLOCATOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct core_allocator_o core_allocator_o;
typedef struct core_allocator_i
{
	core_allocator_o* m_Inst;

	void* (*realloc)(core_allocator_o* allocator, void* ptr, uint64_t sz, uint64_t align, const char* file, uint32_t line);
} core_allocator_i;

#define CORE_ALLOC(allocator, sz)                    (allocator)->realloc(allocator->m_Inst, NULL, sz, 0, __FILE__, __LINE__)
#define CORE_FREE(allocator, ptr)                    (void)(allocator)->realloc(allocator->m_Inst, ptr, 0, 0, __FILE__, __LINE__)
#define CORE_REALLOC(allocator, ptr, sz)             (allocator)->realloc(allocator->m_Inst, ptr, sz, 0, __FILE__, __LINE__)
#define CORE_ALIGNED_ALLOC(allocator, sz, align)     (allocator)->realloc(allocator->m_Inst, NULL, sz, align, __FILE__, __LINE__)
#define CORE_ALIGNED_FREE(allocator, ptr, align)     (void)(allocator)->realloc(allocator->m_Inst, ptr, 0, align, __FILE__, __LINE__)
#define CORE_ALIGNED_REALLOC(allocator, ptr, align)  (allocator)->realloc(allocator->m_Inst, ptr, 0, align, __FILE__, __LINE__)

typedef struct core_allocator_api
{
	core_allocator_i* m_SystemAllocator;

	core_allocator_i* (*createAllocator)(const char* name);
	void              (*destroyAllocator)(core_allocator_i* allocator);

	// Initialize a linear allocator with an initial capacity of 'chunkSize'.
	// - If 'backingAllocator' is not NULL, the allocator will allocate a new 'chunkSize' buffer
	//   every time the last buffer is full.
	// - If 'backingAllocator' is NULL, all allocations beyond the initial capacity will fail.
	// 
	// 'backingAllocator' is used for allocating all required internal memory. If 'backingAllocator'
	// is NULL, the system allocator will be used instead.
	// 
	// Linear allocators do not support reallocations (will assert in debug mode and will always 
	// return a NULL pointer). Frees are silently ignored. You have to destroy the allocator in order
	// to free the allocated memory.
	//
	// WARNING: Linear allocators are not thread-safe. The caller is expected to limit access to one thread at a time.
	core_allocator_i* (*createLinearAllocator)(uint32_t chunkSize, const core_allocator_i* backingAllocator);

	// Same as 'createLinearAllocator' but uses 'buffer' as the initial chunk of memory.
	//
	// The initially available memory from this allocator will be less than the specified 'sz' because
	// part of the buffer is used for the internal representation of the allocator.
	core_allocator_i* (*createLinearAllocatorWithBuffer)(uint8_t* buffer, uint32_t sz, const core_allocator_i* backingAllocator);

	// Destroy a linear allocator by deallocating all its buffers.
	void              (*destroyLinearAllocator)(core_allocator_i* allocator);

	// Reset a linear allocator
	void              (*resetLinearAllocator)(core_allocator_i* allocator);
} core_allocator_api;

extern core_allocator_api* allocator_api;

static core_allocator_i* core_allocatorGetSystemAllocator(void);
static core_allocator_i* core_allocatorCreateAllocator(const char* name);
static void core_allocatorDestroyAllocator(core_allocator_i* allocator);
static core_allocator_i* core_allocatorCreateLinearAllocator(uint32_t chunkSize, const core_allocator_i* backingAllocator);
static core_allocator_i* core_allocatorCreateLinearAllocatorWithBuffer(uint8_t* buffer, uint32_t sz, const core_allocator_i* backingAllocator);
static void              core_allocatorDestroyLinearAllocator(core_allocator_i* allocator);
static void              core_allocatorResetLinearAllocator(core_allocator_i* allocator);

#ifdef __cplusplus
}
#endif // __cplusplus

#include "inline/allocator.inl"

#endif // CORE_ALLOCATOR_H
