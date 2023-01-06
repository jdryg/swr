#include "cpu.h"
#include <intrin.h>

#define CPUINFO_MAKE_ID(type, cpuid_eax_value, cpuid_ecx_value, cpuid_res_id, first_bit, num_bits) 0 \
	| (((cpuid_eax_value) & 0xFF) << 0) \
	| (((cpuid_ecx_value) & 0xFF) << 8) \
	| (((cpuid_res_id) & 0x03) << 16) \
	| (((first_bit) & 0x1F) << 18) \
	| (((num_bits) & 0x3F) << 23) \
	| (((type) & 0x01) << 30)

#define CPUINFO_BASIC                          0
#define CPUINFO_EXTENDED                       1

#define CPUINFO_EAX                            0
#define CPUINFO_EBX                            1
#define CPUINFO_ECX                            2
#define CPUINFO_EDX                            3

#define CPUINFO_BASIC_VENDOR_ID_0              CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x00, 0x00, CPUINFO_EBX, 0, 32)
#define CPUINFO_BASIC_VENDOR_ID_1              CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x00, 0x00, CPUINFO_EDX, 0, 32)
#define CPUINFO_BASIC_VENDOR_ID_2              CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x00, 0x00, CPUINFO_ECX, 0, 32)
#define CPUINFO_BASIC_PROCESSOR_SIGNATURE      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EAX, 0, 32)
#define CPUINFO_BASIC_STEPPING_ID              CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EAX, 0, 4)
#define CPUINFO_BASIC_MODEL_ID                 CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EAX, 4, 4)
#define CPUINFO_BASIC_FAMILY_ID                CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EAX, 8, 4)
#define CPUINFO_BASIC_PROCESSOR_TYPE           CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EAX, 12, 2)
#define CPUINFO_BASIC_EXTENDED_MODEL_ID        CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EAX, 16, 4) // NOTE: Needs further processing. See comments below Table 3-9: Processor Type Field
#define CPUINFO_BASIC_EXTENDED_FAMILY_ID       CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EAX, 20, 8) // NOTE: Needs further processing. See comments below Table 3-9: Processor Type Field
#define CPUINFO_BASIC_BRAND_INDEX              CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EBX, 0, 8)
#define CPUINFO_BASIC_CLFLUSH_LINE_SIZE        CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EBX, 8, 8)
#define CPUINFO_BASIC_UNIQUE_INITIAL_APICS     CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EBX, 16, 8)
#define CPUINFO_BASIC_INITIAL_APIC_ID          CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EBX, 24, 8)
#define CPUINFO_BASIC_SSE3                     CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 0, 1)
#define CPUINFO_BASIC_PCLMULQDQ                CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 1, 1)
#define CPUINFO_BASIC_DTES64                   CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 2, 1)
#define CPUINFO_BASIC_MONITOR                  CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 3, 1)
#define CPUINFO_BASIC_DS_CPL                   CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 4, 1)
#define CPUINFO_BASIC_VMX                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 5, 1)
#define CPUINFO_BASIC_SMX                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 6, 1)
#define CPUINFO_BASIC_EIST                     CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 7, 1)
#define CPUINFO_BASIC_TM2                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 8, 1)
#define CPUINFO_BASIC_SSSE3                    CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 9, 1)
#define CPUINFO_BASIC_CNXT_ID                  CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 10, 1)
#define CPUINFO_BASIC_SDBG                     CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 11, 1)
#define CPUINFO_BASIC_FMA                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 12, 1)
#define CPUINFO_BASIC_CMPXCHG16B               CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 13, 1)
#define CPUINFO_BASIC_XTPR_UPDATE_CONTROL      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 14, 1)
#define CPUINFO_BASIC_PDCM                     CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 15, 1)
#define CPUINFO_BASIC_PCID                     CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 17, 1)
#define CPUINFO_BASIC_DCA                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 18, 1)
#define CPUINFO_BASIC_SSE4_1                   CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 19, 1)
#define CPUINFO_BASIC_SSE4_2                   CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 20, 1)
#define CPUINFO_BASIC_X2APIC                   CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 21, 1)
#define CPUINFO_BASIC_MOVBE                    CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 22, 1)
#define CPUINFO_BASIC_POPCNT                   CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 23, 1)
#define CPUINFO_BASIC_TSC_DEADLINE             CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 24, 1)
#define CPUINFO_BASIC_AESNI                    CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 25, 1)
#define CPUINFO_BASIC_XSAVE                    CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 26, 1)
#define CPUINFO_BASIC_OSXSAVE                  CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 27, 1)
#define CPUINFO_BASIC_AVX                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 28, 1)
#define CPUINFO_BASIC_F16C                     CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 29, 1)
#define CPUINFO_BASIC_RDRAND                   CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_ECX, 30, 1)
#define CPUINFO_BASIC_FPU                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 0, 1)
#define CPUINFO_BASIC_VME                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 1, 1)
#define CPUINFO_BASIC_DE                       CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 2, 1)
#define CPUINFO_BASIC_PSE                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 3, 1)
#define CPUINFO_BASIC_TSC                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 4, 1)
#define CPUINFO_BASIC_MSR                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 5, 1)
#define CPUINFO_BASIC_PAE                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 6, 1)
#define CPUINFO_BASIC_MCE                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 7, 1)
#define CPUINFO_BASIC_CX8                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 8, 1)
#define CPUINFO_BASIC_APIC                     CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 9, 1)
#define CPUINFO_BASIC_SEP                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 11, 1)
#define CPUINFO_BASIC_MTRR                     CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 12, 1)
#define CPUINFO_BASIC_PGE                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 13, 1)
#define CPUINFO_BASIC_MCA                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 14, 1)
#define CPUINFO_BASIC_CMOV                     CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 15, 1)
#define CPUINFO_BASIC_PAT                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 16, 1)
#define CPUINFO_BASIC_PSE_36                   CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 17, 1)
#define CPUINFO_BASIC_PSN                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 18, 1)
#define CPUINFO_BASIC_CLFSH                    CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 19, 1)
#define CPUINFO_BASIC_DS                       CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 21, 1)
#define CPUINFO_BASIC_ACPI                     CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 22, 1)
#define CPUINFO_BASIC_MMX                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 23, 1)
#define CPUINFO_BASIC_FXSR                     CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 24, 1)
#define CPUINFO_BASIC_SSE                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 25, 1)
#define CPUINFO_BASIC_SSE2                     CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 26, 1)
#define CPUINFO_BASIC_SS                       CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 27, 1)
#define CPUINFO_BASIC_HTT                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 28, 1)
#define CPUINFO_BASIC_TM                       CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 29, 1)
#define CPUINFO_BASIC_PBE                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x01, 0x00, CPUINFO_EDX, 31, 1)
#define CPUINFO_BASIC_TLB_CACHE_INFO_0         CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x02, 0x00, CPUINFO_EAX, 0, 32)
#define CPUINFO_BASIC_TLB_CACHE_INFO_1         CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x02, 0x00, CPUINFO_EBX, 0, 32)
#define CPUINFO_BASIC_TLB_CACHE_INFO_2         CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x02, 0x00, CPUINFO_ECX, 0, 32)
#define CPUINFO_BASIC_TLB_CACHE_INFO_3         CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x02, 0x00, CPUINFO_EDX, 0, 32)

#define CPUINFO_BASIC_MAX_SUBLEAVES_07         CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EAX, 0, 32)
#define CPUINFO_BASIC_FSGSBASE                 CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 0, 1)
#define CPUINFO_BASIC_IA32_TSC_ADJUST          CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 1, 1)
#define CPUINFO_BASIC_SGX                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 2, 1)
#define CPUINFO_BASIC_BMI1                     CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 3, 1)
#define CPUINFO_BASIC_HLE                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 4, 1)
#define CPUINFO_BASIC_AVX2                     CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 5, 1)
#define CPUINFO_BASIC_FDP_EXCPTN_ONLY          CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 6, 1)
#define CPUINFO_BASIC_SMEP                     CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 7, 1)
#define CPUINFO_BASIC_BMI2                     CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 8, 1)
#define CPUINFO_BASIC_ENHANCED_REPMOVSB        CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 9, 1)
#define CPUINFO_BASIC_INVPCID                  CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 10, 1)
#define CPUINFO_BASIC_RTM                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 11, 1)
#define CPUINFO_BASIC_RDT_M                    CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 12, 1)
#define CPUINFO_BASIC_DEPRECATED_FPU_CS_DS     CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 13, 1)
#define CPUINFO_BASIC_MPX                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 14, 1)
#define CPUINFO_BASIC_RDT_A                    CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 15, 1)
#define CPUINFO_BASIC_AVX512F                  CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 16, 1)
#define CPUINFO_BASIC_AVX512DQ                 CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 17, 1)
#define CPUINFO_BASIC_RDSEED                   CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 18, 1)
#define CPUINFO_BASIC_ADX                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 19, 1)
#define CPUINFO_BASIC_SMAP                     CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 20, 1)
#define CPUINFO_BASIC_AVX512_IFMA              CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 21, 1)
#define CPUINFO_BASIC_CLFLUSHOPT               CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 23, 1)
#define CPUINFO_BASIC_CLWB                     CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 24, 1)
#define CPUINFO_BASIC_INTEL_PROCESSOR_TRACE    CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 25, 1)
#define CPUINFO_BASIC_AVX512PF                 CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 26, 1)
#define CPUINFO_BASIC_AVX512ER                 CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 27, 1)
#define CPUINFO_BASIC_AVX512CD                 CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 28, 1)
#define CPUINFO_BASIC_SHA                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 29, 1)
#define CPUINFO_BASIC_AVX512BW                 CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 30, 1)
#define CPUINFO_BASIC_AVX512VL                 CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_EBX, 31, 1)
#define CPUINFO_BASIC_PREFETCHWT1              CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_ECX, 0, 1)
#define CPUINFO_BASIC_AVX512_VBMI              CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_ECX, 1, 1)
#define CPUINFO_BASIC_UMIP                     CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_ECX, 2, 1)
#define CPUINFO_BASIC_PKU                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_ECX, 3, 1)
#define CPUINFO_BASIC_OSPKE                    CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_ECX, 4, 1)
#define CPUINFO_BASIC_WAITPKG                  CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_ECX, 5, 1)
#define CPUINFO_BASIC_AVX512_VBMI2             CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_ECX, 6, 1)
#define CPUINFO_BASIC_CET_SS                   CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_ECX, 7, 1)
#define CPUINFO_BASIC_GFNI                     CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_ECX, 8, 1)
#define CPUINFO_BASIC_VAES                     CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_ECX, 9, 1)
#define CPUINFO_BASIC_VPCLMULQDQ               CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_ECX, 10, 1)
#define CPUINFO_BASIC_AVX512_VNNI              CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_ECX, 11, 1)
#define CPUINFO_BASIC_AVX512_BITALG            CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_ECX, 12, 1)
#define CPUINFO_BASIC_TME_EN                   CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_ECX, 13, 1)
#define CPUINFO_BASIC_AVX512_VPOPCNTDQ         CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_ECX, 14, 1)
#define CPUINFO_BASIC_LA57                     CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_ECX, 16, 1)
#define CPUINFO_BASIC_MAWAU                    CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_ECX, 17, 5)
#define CPUINFO_BASIC_RDPID                    CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_ECX, 22, 1)
#define CPUINFO_BASIC_KL                       CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_ECX, 23, 1)
#define CPUINFO_BASIC_CLDEMOTE                 CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_ECX, 25, 1)
#define CPUINFO_BASIC_MOVDIRI                  CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_ECX, 27, 1)
#define CPUINFO_BASIC_MOVDIR64B                CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_ECX, 28, 1)
#define CPUINFO_BASIC_SGX_LC                   CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_ECX, 30, 1)
#define CPUINFO_BASIC_PKS                      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x07, 0x00, CPUINFO_ECX, 31, 1)

#define CPUINFO_BASIC_PROCESSOR_BASE_FREQ      CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x16, 0x00, CPUINFO_EAX, 0, 16)
#define CPUINFO_BASIC_PROCESSOR_MAX_FREQ       CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x16, 0x00, CPUINFO_EBX, 0, 16)
#define CPUINFO_BASIC_BUS_FREQ                 CPUINFO_MAKE_ID(CPUINFO_BASIC, 0x16, 0x00, CPUINFO_ECX, 0, 16)

#define CPUINFO_EXT_MAX_EXTENDED_FUNC_ID       CPUINFO_MAKE_ID(CPUINFO_EXTENDED, 0x00, 0x00, CPUINFO_EAX, 0, 32)
#define CPUINFO_EXT_PROCESSOR_BRAND_STRING_0   CPUINFO_MAKE_ID(CPUINFO_EXTENDED, 0x02, 0x00, CPUINFO_EAX, 0, 32)
#define CPUINFO_EXT_PROCESSOR_BRAND_STRING_1   CPUINFO_MAKE_ID(CPUINFO_EXTENDED, 0x02, 0x00, CPUINFO_EBX, 0, 32)
#define CPUINFO_EXT_PROCESSOR_BRAND_STRING_2   CPUINFO_MAKE_ID(CPUINFO_EXTENDED, 0x02, 0x00, CPUINFO_ECX, 0, 32)
#define CPUINFO_EXT_PROCESSOR_BRAND_STRING_3   CPUINFO_MAKE_ID(CPUINFO_EXTENDED, 0x02, 0x00, CPUINFO_EDX, 0, 32)
#define CPUINFO_EXT_PROCESSOR_BRAND_STRING_4   CPUINFO_MAKE_ID(CPUINFO_EXTENDED, 0x03, 0x00, CPUINFO_EAX, 0, 32)
#define CPUINFO_EXT_PROCESSOR_BRAND_STRING_5   CPUINFO_MAKE_ID(CPUINFO_EXTENDED, 0x03, 0x00, CPUINFO_EBX, 0, 32)
#define CPUINFO_EXT_PROCESSOR_BRAND_STRING_6   CPUINFO_MAKE_ID(CPUINFO_EXTENDED, 0x03, 0x00, CPUINFO_ECX, 0, 32)
#define CPUINFO_EXT_PROCESSOR_BRAND_STRING_7   CPUINFO_MAKE_ID(CPUINFO_EXTENDED, 0x03, 0x00, CPUINFO_EDX, 0, 32)
#define CPUINFO_EXT_PROCESSOR_BRAND_STRING_8   CPUINFO_MAKE_ID(CPUINFO_EXTENDED, 0x04, 0x00, CPUINFO_EAX, 0, 32)
#define CPUINFO_EXT_PROCESSOR_BRAND_STRING_9   CPUINFO_MAKE_ID(CPUINFO_EXTENDED, 0x04, 0x00, CPUINFO_EBX, 0, 32)
#define CPUINFO_EXT_PROCESSOR_BRAND_STRING_10  CPUINFO_MAKE_ID(CPUINFO_EXTENDED, 0x04, 0x00, CPUINFO_ECX, 0, 32)
#define CPUINFO_EXT_PROCESSOR_BRAND_STRING_11  CPUINFO_MAKE_ID(CPUINFO_EXTENDED, 0x04, 0x00, CPUINFO_EDX, 0, 32)

#define CPUINFO_PROCESSOR_BRAND_STRING_MAX_LEN 64
#define CPUINFO_VENDOR_ID_MAX_LEN              32

typedef struct core_cpu_info
{
	uint64_t m_Features;
	core_cpu_version m_Version;
	uint16_t m_BusFreq;
	char m_VendorID[CPUINFO_VENDOR_ID_MAX_LEN];
	char m_ProcessorBrandString[CPUINFO_PROCESSOR_BRAND_STRING_MAX_LEN];
} core_cpu_info;

static core_cpu_info s_CPUInfo = { 0 };

static const char* core_cpu_getVendorID(void);
static const char* core_cpu_getProcessorBrandString(void);
static uint64_t core_cpu_getFeatures(void);
static const core_cpu_version* core_cpu_getVersion(void);
static uint32_t core_cpu_readInfo(uint32_t id);

core_cpu_api* cpu_api = &(core_cpu_api)
{
	.getVendorID = core_cpu_getVendorID,
	.getProcessorBrandString = core_cpu_getProcessorBrandString,
	.getFeatures = core_cpu_getFeatures,
	.getVersionInfo = core_cpu_getVersion
};

bool core_cpu_initAPI(uint64_t featureMask)
{
	core_cpu_info* info = &s_CPUInfo;

	// Check for CPUID support.
	// NOTE: This shouldn't be needed. I'm just bored and decided to go "by the book"
	// The Book: https://www.scss.tcd.ie/~jones/CS4021/processor-identification-cpuid-instruction-note.pdf
	{
		// Read EFLAGS register and toggle ID bit
		const uint64_t eflagsInitial = __readeflags();
		const uint64_t eflagsNew = eflagsInitial ^ (1ull << 21);

		// Write EFLAGS register
		__writeeflags(eflagsNew);

		// Check if write succeeded
		if (__readeflags() != eflagsNew) {
			// CPUID instruction not supported.
			return false;
		}

		// Reset EFLAGS register to initial value
		__writeeflags(eflagsInitial);
	}

	// Vendor ID
	{
		char* str = &info->m_VendorID[0];
		*(uint32_t*)&str[0] = core_cpu_readInfo(CPUINFO_BASIC_VENDOR_ID_0);
		*(uint32_t*)&str[4] = core_cpu_readInfo(CPUINFO_BASIC_VENDOR_ID_1);
		*(uint32_t*)&str[8] = core_cpu_readInfo(CPUINFO_BASIC_VENDOR_ID_2);
		str[12] = '\0';
	}

	// Processor Brand String
	{
		char* str = &info->m_ProcessorBrandString[0];

		const uint32_t maxExtendedFuncID = core_cpu_readInfo(CPUINFO_EXT_MAX_EXTENDED_FUNC_ID);
		if (maxExtendedFuncID >= 0x80000004) {
			// If the CPUID.80000000h.EAX is greater then or equal to 0x80000004 the 
			// Brand String feature is supported and software should use CPUID functions
			// 0x80000002 through 0x80000004 to identify the processor.
			*(uint32_t*)&str[0] = core_cpu_readInfo(CPUINFO_EXT_PROCESSOR_BRAND_STRING_0);
			*(uint32_t*)&str[4] = core_cpu_readInfo(CPUINFO_EXT_PROCESSOR_BRAND_STRING_1);
			*(uint32_t*)&str[8] = core_cpu_readInfo(CPUINFO_EXT_PROCESSOR_BRAND_STRING_2);
			*(uint32_t*)&str[12] = core_cpu_readInfo(CPUINFO_EXT_PROCESSOR_BRAND_STRING_3);
			*(uint32_t*)&str[16] = core_cpu_readInfo(CPUINFO_EXT_PROCESSOR_BRAND_STRING_4);
			*(uint32_t*)&str[20] = core_cpu_readInfo(CPUINFO_EXT_PROCESSOR_BRAND_STRING_5);
			*(uint32_t*)&str[24] = core_cpu_readInfo(CPUINFO_EXT_PROCESSOR_BRAND_STRING_6);
			*(uint32_t*)&str[28] = core_cpu_readInfo(CPUINFO_EXT_PROCESSOR_BRAND_STRING_7);
			*(uint32_t*)&str[32] = core_cpu_readInfo(CPUINFO_EXT_PROCESSOR_BRAND_STRING_8);
			*(uint32_t*)&str[36] = core_cpu_readInfo(CPUINFO_EXT_PROCESSOR_BRAND_STRING_9);
			*(uint32_t*)&str[40] = core_cpu_readInfo(CPUINFO_EXT_PROCESSOR_BRAND_STRING_10);
			*(uint32_t*)&str[44] = core_cpu_readInfo(CPUINFO_EXT_PROCESSOR_BRAND_STRING_11);
			str[48] = '\0';
		} else {
			// If the Brand String feature is not supported execute CPUID.1.EBX to get
			// the Brand ID. If the Brand ID is not zero the brand id feature is supported.
			const uint32_t brandID = core_cpu_readInfo(CPUINFO_BASIC_BRAND_INDEX);
			if (brandID != 0) {
				// NOTE: Since Brand ID is going to be different between vendors, don't try
				// to decode it. Just write the Brand ID to the processor brand string.
				// TODO: 
			} else {
				// If the Brand ID feature is not supported software should use the processor
				// signature (NOTE: version info) in conjuction with cache descriptors to 
				// identify the processor.
#if 0
				// TODO:
				const uint32_t processorSignature = core_cpu_readInfo(CPUINFO_BASIC_PROCESSOR_SIGNATURE);
#endif
			}
		}
	}

	// Version
	{
		core_cpu_version* ver = &info->m_Version;
		ver->m_SteppingID = (uint8_t)core_cpu_readInfo(CPUINFO_BASIC_STEPPING_ID);

		const uint8_t familyID = (uint8_t)core_cpu_readInfo(CPUINFO_BASIC_FAMILY_ID);
		if (familyID == 0x0F) {
			ver->m_FamilyID = familyID + (uint8_t)core_cpu_readInfo(CPUINFO_BASIC_EXTENDED_FAMILY_ID);
		} else {
			ver->m_FamilyID = familyID;
		}

		ver->m_ModelID = (uint8_t)core_cpu_readInfo(CPUINFO_BASIC_MODEL_ID);
		if (familyID == 0x06 || familyID == 0x0F) {
			ver->m_ModelID += (uint8_t)core_cpu_readInfo(CPUINFO_BASIC_EXTENDED_MODEL_ID) << 4;
		}
	}

	// Features
	{
		uint64_t features = 0ull;
		features |= (core_cpu_readInfo(CPUINFO_BASIC_MMX) != 0) ? CORE_CPU_FEATURE_MMX : 0ull;
		features |= (core_cpu_readInfo(CPUINFO_BASIC_SSE) != 0) ? CORE_CPU_FEATURE_SSE : 0ull;
		features |= (core_cpu_readInfo(CPUINFO_BASIC_SSE2) != 0) ? CORE_CPU_FEATURE_SSE2 : 0ull;
		features |= (core_cpu_readInfo(CPUINFO_BASIC_SSE3) != 0) ? CORE_CPU_FEATURE_SSE3 : 0ull;
		features |= (core_cpu_readInfo(CPUINFO_BASIC_SSSE3) != 0) ? CORE_CPU_FEATURE_SSSE3 : 0ull;
		features |= (core_cpu_readInfo(CPUINFO_BASIC_SSE4_1) != 0) ? CORE_CPU_FEATURE_SSE4_1 : 0ull;
		features |= (core_cpu_readInfo(CPUINFO_BASIC_SSE4_2) != 0) ? CORE_CPU_FEATURE_SSE4_2 : 0ull;
		features |= (core_cpu_readInfo(CPUINFO_BASIC_AVX) != 0) ? CORE_CPU_FEATURE_AVX : 0ull;
		features |= (core_cpu_readInfo(CPUINFO_BASIC_AVX2) != 0) ? CORE_CPU_FEATURE_AVX2 : 0ull;
		features |= (core_cpu_readInfo(CPUINFO_BASIC_AVX512F) != 0) ? CORE_CPU_FEATURE_AVX512F : 0ull;
		features |= (core_cpu_readInfo(CPUINFO_BASIC_BMI1) != 0) ? CORE_CPU_FEATURE_BMI1 : 0ull;
		features |= (core_cpu_readInfo(CPUINFO_BASIC_BMI2) != 0) ? CORE_CPU_FEATURE_BMI2 : 0ull;
		features |= (core_cpu_readInfo(CPUINFO_BASIC_POPCNT) != 0) ? CORE_CPU_FEATURE_POPCNT : 0ull;
		features |= (core_cpu_readInfo(CPUINFO_BASIC_F16C) != 0) ? CORE_CPU_FEATURE_F16C : 0ull;
		features |= (core_cpu_readInfo(CPUINFO_BASIC_FMA) != 0) ? CORE_CPU_FEATURE_FMA : 0ull;
		features |= (core_cpu_readInfo(CPUINFO_BASIC_ENHANCED_REPMOVSB) != 0) ? CORE_CPU_FEATURE_ERMSB : 0ull;
		info->m_Features = (features & featureMask);
	}

	return true;
}

void core_cpu_shutdownAPI(void)
{

}

static const char* core_cpu_getVendorID(void)
{
	return &s_CPUInfo.m_VendorID[0];
}

static const char* core_cpu_getProcessorBrandString(void)
{
	return &s_CPUInfo.m_ProcessorBrandString[0];
}

static uint64_t core_cpu_getFeatures(void)
{
	return s_CPUInfo.m_Features;
}

static const core_cpu_version* core_cpu_getVersion(void)
{
	return &s_CPUInfo.m_Version;
}

static uint32_t core_cpu_readInfo(uint32_t id)
{
	const uint32_t cpuidEAX = (id >> 0) & 0xFF;
	const uint32_t cpuidECX = (id >> 8) & 0xFF;
	const uint32_t cpuidResID = (id >> 16) & 0x03;
	const uint32_t firstBit = (id >> 18) & 0x1F;
	const uint32_t numBits = (id >> 23) & 0x3F;
	const uint32_t type = (id >> 30) & 0x01;

	int32_t info[4];
	__cpuidex(&info[0], (type == CPUINFO_BASIC ? 0 : 0x80000000) + cpuidEAX, cpuidECX);

	return (((uint32_t)info[cpuidResID]) >> firstBit) & (uint32_t)((1ull << numBits) - 1);
}
