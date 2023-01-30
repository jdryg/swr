#ifndef CORE_CPU_H
#define CORE_CPU_H

#include <stdint.h>
#include <stdbool.h>
#include "macros.h"

#define CORE_CPU_FEATURE_MMX      (1ull << 0)
#define CORE_CPU_FEATURE_SSE      (1ull << 1)
#define CORE_CPU_FEATURE_SSE2     (1ull << 2)
#define CORE_CPU_FEATURE_SSE3     (1ull << 3)
#define CORE_CPU_FEATURE_SSSE3    (1ull << 4)
#define CORE_CPU_FEATURE_SSE4_1   (1ull << 5)
#define CORE_CPU_FEATURE_SSE4_2   (1ull << 6)
#define CORE_CPU_FEATURE_AVX      (1ull << 7)
#define CORE_CPU_FEATURE_AVX2     (1ull << 8)
#define CORE_CPU_FEATURE_AVX512F  (1ull << 9)
#define CORE_CPU_FEATURE_BMI1     (1ull << 10)
#define CORE_CPU_FEATURE_BMI2     (1ull << 11)
#define CORE_CPU_FEATURE_POPCNT   (1ull << 12)
#define CORE_CPU_FEATURE_F16C     (1ull << 13) // CPU supports 16-bit floating point conversion instructions
#define CORE_CPU_FEATURE_FMA      (1ull << 14)
#define CORE_CPU_FEATURE_ERMSB    (1ull << 15) // Enhanced REP MOVSB

#define CORE_CPU_NUM_FEATURES     16

#define CORE_CPU_FEATURE_MASK_ALL        ((1ull << CORE_CPU_NUM_FEATURES) - 1)
#define CORE_CPU_FEATURE_MASK_NONE       0ull
#define CORE_CPU_FEATURE_MASK_SSE2       (CORE_CPU_FEATURE_MMX | CORE_CPU_FEATURE_SSE | CORE_CPU_FEATURE_SSE2 | CORE_CPU_FEATURE_BMI1 | CORE_CPU_FEATURE_BMI2 | CORE_CPU_FEATURE_POPCNT | CORE_CPU_FEATURE_F16C | CORE_CPU_FEATURE_FMA | CORE_CPU_FEATURE_ERMSB)
#define CORE_CPU_FEATURE_MASK_SSE3       (CORE_CPU_FEATURE_MASK_SSE2 | CORE_CPU_FEATURE_SSE3)
#define CORE_CPU_FEATURE_MASK_SSSE3      (CORE_CPU_FEATURE_MASK_SSE3 | CORE_CPU_FEATURE_SSSE3)
#define CORE_CPU_FEATURE_MASK_SSE4_1     (CORE_CPU_FEATURE_MASK_SSSE3 | CORE_CPU_FEATURE_SSE4_1)
#define CORE_CPU_FEATURE_MASK_SSE4_2     (CORE_CPU_FEATURE_MASK_SSE4_1 | CORE_CPU_FEATURE_SSE4_2)
#define CORE_CPU_FEATURE_MASK_AVX        (CORE_CPU_FEATURE_MASK_SSE4_2 | CORE_CPU_FEATURE_AVX)
#define CORE_CPU_FEATURE_MASK_AVX2       (CORE_CPU_FEATURE_MASK_AVX | CORE_CPU_FEATURE_AVX2)
#define CORE_CPU_FEATURE_MASK_AVX512F    (CORE_CPU_FEATURE_MASK_AVX2 | CORE_CPU_FEATURE_AVX512F)

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

static const char* core_cpu_kFeatureName[] = {
	"MMX",
	"SSE",
	"SSE2",
	"SSE3",
	"SSSE3",
	"SSE4.1",
	"SSE4.2",
	"AVX",
	"AVX2",
	"AVX512F",
	"BMI1",
	"BMI2",
	"POPCNT",
	"F16C",
	"FMA",
	"ERMSB"
};
static_assert(CORE_COUNTOF(core_cpu_kFeatureName) == CORE_CPU_NUM_FEATURES, "Missing CPU feature name");

typedef struct core_cpu_version
{
	uint8_t m_FamilyID;
	uint8_t m_ModelID;
	uint8_t m_SteppingID;
} core_cpu_version;

typedef struct core_cpu_api
{
	const char*             (*getVendorID)(void);
	const char*             (*getProcessorBrandString)(void);
	uint64_t                (*getFeatures)(void);
	const core_cpu_version* (*getVersionInfo)(void);
} core_cpu_api;

extern core_cpu_api* cpu_api;

static const char* core_cpuGetVendorID(void);
static const char* core_cpuGetProcessorBrandString(void);
static uint64_t core_cpuGetFeatures(void);
static const core_cpu_version* core_cpuGetVersionInfo(void);

#ifdef __cplusplus
}
#endif

#include "inline/cpu.inl"

#endif // CORE_CPU_H
