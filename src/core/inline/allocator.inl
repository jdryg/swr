#ifndef CORE_ALLOCATOR_H
#error "Must be included from allocator.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline core_allocator_i* core_allocatorGetSystemAllocator(void)
{
	return allocator_api->m_SystemAllocator;
}

static inline core_allocator_i* core_allocatorCreateAllocator(const char* name)
{
	return allocator_api->createAllocator(name);
}

static inline void core_allocatorDestroyAllocator(core_allocator_i* allocator)
{
	allocator_api->destroyAllocator(allocator);
}

static inline core_allocator_i* core_allocatorCreateLinearAllocator(uint32_t chunkSize, const core_allocator_i* backingAllocator)
{
	return allocator_api->createLinearAllocator(chunkSize, backingAllocator);
}

static inline core_allocator_i* core_allocatorCreateLinearAllocatorWithBuffer(uint8_t* buffer, uint32_t sz, const core_allocator_i* backingAllocator)
{
	return allocator_api->createLinearAllocatorWithBuffer(buffer, sz, backingAllocator);
}

static inline void core_allocatorDestroyLinearAllocator(core_allocator_i* allocator)
{
	allocator_api->destroyLinearAllocator(allocator);
}

static inline void core_allocatorResetLinearAllocator(core_allocator_i* allocator)
{
	allocator_api->resetLinearAllocator(allocator);
}

#ifdef __cplusplus
}
#endif
