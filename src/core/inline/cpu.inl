#ifndef CORE_CPU_H
#error "Must be included from cpu.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline const char* core_cpuGetVendorID(void)
{
	return cpu_api->getVendorID();
}

static inline const char* core_cpuGetProcessorBrandString(void)
{
	return cpu_api->getProcessorBrandString();
}

static inline uint64_t core_cpuGetFeatures(void)
{
	return cpu_api->getFeatures();
}

static inline const core_cpu_version* core_cpuGetVersionInfo(void)
{
	return cpu_api->getVersionInfo();
}

#ifdef __cplusplus
}
#endif
