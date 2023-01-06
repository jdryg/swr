#include "core.h"
#include "allocator.h"

extern bool core_cpu_initAPI(uint64_t featureMask);
extern void core_cpu_shutdownAPI(void);
extern bool core_allocator_initAPI(void);
extern void core_allocator_shutdownAPI(void);

bool coreInit(uint64_t cpuFeaturesMask)
{
	if (!core_cpu_initAPI(cpuFeaturesMask)) {
		return false;
	}

	if (!core_allocator_initAPI()) {
		return false;
	}

	return true;
}

void coreShutdown(void)
{
	core_allocator_shutdownAPI();
	core_cpu_shutdownAPI();
}
