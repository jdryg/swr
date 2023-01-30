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

static void core_memCopy(void* dst, const void* src, size_t n);
static void core_memMove(void* dst, const void* src, size_t n);
static void core_memSet(void* dst, uint8_t ch, size_t n);
static int32_t core_memCmp(const void* lhs, const void* rhs, size_t n);

static bool core_isAlignedPtr(const void* ptr, uint64_t alignment);
static void* core_alignPtr(void* ptr, uint64_t alignment);

#ifdef __cplusplus
}
#endif

#include "inline/memory.inl"

#endif // CORE_MEMORY_H
