#ifndef CORE_ALLOCATOR_H
#define CORE_ALLOCATOR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct core_allocator_o core_allocator_o;
typedef struct core_allocator_i
{
	core_allocator_o* m_Inst;

	void* (*realloc)(core_allocator_o* allocator, void* ptr, uint64_t sz, uint64_t align, const char* file, uint32_t line);
} core_allocator_i;

#define CORE_ALLOC(allocator, sz)                        (allocator)->realloc(allocator->m_Inst, NULL, sz, 0, __FILE__, __LINE__)
#define CORE_FREE(allocator, ptr)                        (void)(allocator)->realloc(allocator->m_Inst, ptr, 0, 0, __FILE__, __LINE__)
#define CORE_REALLOC(allocator, ptr, sz)                 (allocator)->realloc(allocator->m_Inst, ptr, sz, 0, __FILE__, __LINE__)
#define CORE_ALIGNED_ALLOC(allocator, sz, align)         (allocator)->realloc(allocator->m_Inst, NULL, sz, align, __FILE__, __LINE__)
#define CORE_ALINGED_FREE(allocator, ptr, sz, align)     (void)(allocator)->realloc(allocator->m_Inst, ptr, sz, align, __FILE__, __LINE__)
#define CORE_ALIGNED_REALLOC(allocator, ptr, sz, align)  (allocator)->realloc(allocator->m_Inst, ptr, sz, align, __FILE__, __LINE__)

typedef struct core_allocator_api
{
	core_allocator_i* m_SystemAllocator;

	core_allocator_i* (*createAllocator)(const char* name);
	void              (*destroyAllocator)(core_allocator_i* allocator);
} core_allocator_api;

extern core_allocator_api* allocator_api;

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CORE_ALLOCATOR_H
