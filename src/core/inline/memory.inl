#ifndef CORE_MEMORY_H
#error "Must be included from memory.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline void core_memCopy(void* dst, const void* src, size_t n)
{
	mem_api->copy(dst, src, n);
}

static inline void core_memMove(void* dst, const void* src, size_t n)
{
	mem_api->move(dst, src, n);
}

static inline void core_memSet(void* dst, uint8_t ch, size_t n)
{
	mem_api->set(dst, ch, n);
}

static inline int32_t core_memCmp(const void* lhs, const void* rhs, size_t n)
{
	return mem_api->cmp(lhs, rhs, n);
}

#ifdef __cplusplus
}
#endif
