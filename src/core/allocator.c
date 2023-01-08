#include "allocator.h"
#include <stdbool.h>
#include <malloc.h>

static void* allocator_sysRealloc(core_allocator_o* a, void* ptr, uint64_t sz, uint64_t align, const char* file, uint32_t line);
static core_allocator_i* allocator_createAllocator(const char* name);
static void allocator_destroyAllocator(core_allocator_i* alloc);

static const uint64_t kSystemAllocatorNaturalAlignment = 8;

static core_allocator_i g_SystemAllocator = {
	.m_Inst = NULL,
	.realloc = allocator_sysRealloc
};

core_allocator_api* allocator_api = &(core_allocator_api){
	.m_SystemAllocator = NULL,
	.createAllocator = allocator_createAllocator,
	.destroyAllocator = allocator_destroyAllocator
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
