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

#ifdef __cplusplus
}
#endif
