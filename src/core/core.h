#ifndef CORE_CORE_H
#define CORE_CORE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

bool coreInit(uint64_t cpuFeatureMask);
void coreShutdown(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CORE_CORE_H
