#include "memory.h"

static void mem_copy_ref(void* __restrict dst, const void* __restrict src, size_t n);
static void mem_move_ref(void* __restrict dst, const void* __restrict src, size_t n);
static void mem_set_ref(void* dst, uint8_t ch, size_t n);
static int32_t mem_cmp_ref(const void* __restrict lhs, const void* __restrict rhs, size_t n);

core_mem_api* mem_api = &(core_mem_api){
	.copy = mem_copy_ref,
	.move = mem_move_ref,
	.set = mem_set_ref,
	.cmp = mem_cmp_ref
};

bool core_mem_initAPI(void)
{
	return true;
}

void core_mem_shutdownAPI(void)
{

}

static void mem_copy_ref(void* __restrict dstPtr, const void* __restrict srcPtr, size_t n)
{
	uint8_t* dst = (uint8_t*)dstPtr;
	const uint8_t* src = (uint8_t*)srcPtr;
	const uint8_t* end = dst + n;
	while (dst != end) {
		*dst++ = *src++;
	}
}

static void mem_move_ref(void* __restrict dstPtr, const void* __restrict srcPtr, size_t n)
{
	uint8_t* dst = (uint8_t*)dstPtr;
	const uint8_t* src = (const uint8_t*)srcPtr;

	if (n == 0 || dst == src) {
		return;
	}

	if (dst < src) {
		core_memCopy(dstPtr, srcPtr, n);
		return;
	}

	for (intptr_t ii = n - 1; ii >= 0; --ii) {
		dst[ii] = src[ii];
	}
}

static void mem_set_ref(void* dstPtr, uint8_t ch, size_t n)
{
	uint8_t* dst = (uint8_t*)dstPtr;
	const uint8_t* end = dst + n;
	while (dst != end) {
		*dst++ = ch;
	}
}

static int32_t mem_cmp_ref(const void* __restrict lhsPtr, const void* __restrict rhsPtr, size_t n)
{
	if (lhsPtr == rhsPtr) {
		return 0;
	}

	const char* lhs = (const char*)lhsPtr;
	const char* rhs = (const char*)rhsPtr;
	for (; n > 0 && *lhs == *rhs; ++lhs, ++rhs, --n) {
	}

	return n == 0 
		? 0 
		: *lhs - *rhs
		;
}
