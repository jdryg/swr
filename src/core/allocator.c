#include "allocator.h"
#include <stdbool.h>
#include <malloc.h>

static void* core_allocator_sysRealloc(core_allocator_o* a, void* ptr, uint64_t sz, uint64_t align, const char* file, uint32_t line);
static core_allocator_i* core_allocator_createAllocator(const char* name);
static void core_allocator_destroyAllocator(core_allocator_i* alloc);

static const uint64_t kSystemAllocatorNaturalAlignment = 8;

static core_allocator_i g_SystemAllocator = {
	.m_Inst = NULL,
	.realloc = core_allocator_sysRealloc
};

core_allocator_api* allocator_api = &(core_allocator_api){
	.m_SystemAllocator = NULL,
	.createAllocator = core_allocator_createAllocator,
	.destroyAllocator = core_allocator_destroyAllocator
};

bool core_allocator_initAPI(void)
{
	allocator_api->m_SystemAllocator = core_allocator_createAllocator("system");
	if (!allocator_api->m_SystemAllocator) {
		return false;
	}

	return true;
}

void core_allocator_shutdownAPI(void)
{
	if (allocator_api->m_SystemAllocator) {
		core_allocator_destroyAllocator(allocator_api->m_SystemAllocator);
		allocator_api->m_SystemAllocator = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////
// Internal API
//
// bx::DefaultAllocator::realloc
static void* core_allocator_sysRealloc(core_allocator_o* allocator, void* ptr, uint64_t sz, uint64_t align, const char* file, uint32_t line)
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

static core_allocator_i* core_allocator_createAllocator(const char* name)
{
	core_allocator_i* allocator = NULL;

#if JX_CONFIG_TRACE_ALLOCATIONS
	// TODO: Create tracing allocator
#else
	allocator = &g_SystemAllocator;
#endif

	return allocator;
}

static void core_allocator_destroyAllocator(core_allocator_i* allocator)
{
#if JX_CONFIG_TRACE_ALLOCATIONS
	// TODO: Destroy tracing allocator
#endif
}
