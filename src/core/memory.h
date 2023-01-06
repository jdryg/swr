#ifndef CORE_MEMORY_H
#define CORE_MEMORY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct core_mem_api
{
	void (*copy)(void* dst, const void* src, size_t n);
	void (*move)(void* dst, const void* src, size_t n);
	void (*set)(void* dst, uint8_t ch, size_t n);
	int32_t(*cmp)(const void* lhs, const void* rhs, size_t n);
} core_mem_api;

extern core_mem_api* mem_api;

static void core_memcpy(void* dst, const void* src, size_t n);
static void core_memmove(void* dst, const void* src, size_t n);
static void core_memset(void* dst, uint8_t ch, size_t n);
static int32_t core_memcmp(const void* lhs, const void* rhs, size_t n);

#ifdef __cplusplus
}
#endif

#endif // CORE_MEMORY_H

#include "inline/memory.inl"
