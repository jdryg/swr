#include "string.h"
#include "allocator.h"
#include "memory.h"
#define STB_SPRINTF_IMPLEMENTATION
#include <stb_sprintf.h>

static uint32_t core_str_strnlen(const char* str, uint32_t max);
static char* core_str_strndup(const char* str, uint32_t n, core_allocator_i* allocator);
static uint32_t core_str_strcpy(char* dst, uint32_t dstSize, const char* src, uint32_t n);
static int32_t core_str_strcmp(const char* lhs, uint32_t lhsMax, const char* rhs, uint32_t rhsMax);
static char* core_str_strnrchr(char* str, uint32_t n, char ch);
static char* core_str_strnchr(char* str, uint32_t n, char ch);
static uint32_t core_str_strncat(char* dst, uint32_t dstSize, const char* src, uint32_t n);

core_str_api* str_api = &(core_str_api){
	.snprintf = stbsp_snprintf,
	.vsnprintf = stbsp_vsnprintf,
	.strnlen = core_str_strnlen,
	.strndup = core_str_strndup,
	.strcpy = core_str_strcpy,
	.strcmp = core_str_strcmp,
	.strnrchr = core_str_strnrchr,
	.strnchr = core_str_strnchr,
	.strncat = core_str_strncat,
};

static uint32_t core_str_strnlen(const char* str, uint32_t max)
{
	const char* ptr = str;
	if (ptr != NULL) {
		for (; max > 0 && *ptr != '\0'; ++ptr, --max);
	}
	return (uint32_t)(ptr - str);
}

static char* core_str_strndup(const char* str, uint32_t n, core_allocator_i* allocator)
{
	const uint32_t len = core_strnlen(str, n);
	char* dst = (char*)CORE_ALLOC(allocator, len + 1);
	core_memCopy(dst, str, len);
	dst[len] = '\0';

	return dst;
}

static uint32_t core_str_strcpy(char* dst, uint32_t dstSize, const char* src, uint32_t n)
{
	const uint32_t len = core_strnlen(src, n);
	const uint32_t max = dstSize - 1;
	const uint32_t num = len < max ? len : max;
	core_memCopy(dst, src, num);
	dst[num] = '\0';

	return num;
}

static int32_t core_str_strcmp(const char* lhs, uint32_t lhsMax, const char* rhs, uint32_t rhsMax)
{
	uint32_t max = lhsMax < rhsMax ? lhsMax : rhsMax;
	for (; max > 0 && *lhs == *rhs && *lhs != '\0'; ++lhs, ++rhs, --max);

	if (max == 0) {
		return lhsMax == rhsMax
			? 0
			: (lhsMax > rhsMax ? 1 : -1)
			;
	}

	return *lhs - *rhs;
}

static char* core_str_strnrchr(char* str, uint32_t n, char ch)
{
	if (n == UINT32_MAX) {
		char* lastOccurence = NULL;
		for (; *str != '\0'; ++str) {
			if (*str == ch) {
				lastOccurence = str;
			}
		}

		return lastOccurence;
	}

	while (n--) {
		if (str[n] == ch) {
			return &str[n];
		}
	}

	return NULL;
}

static char* core_str_strnchr(char* str, uint32_t n, char ch)
{
	for (; n > 0 && *str != '\0'; ++str, --n) {
		if (*str == ch) {
			return str;
		}
	}

	return NULL;
}

static uint32_t core_str_strncat(char* dst, uint32_t dstSize, const char* src, uint32_t n)
{
	const uint32_t max = dstSize;
	const uint32_t len = core_strnlen(dst, dstSize);
	return core_strcpy(&dst[len], max - len, src, n);
}
