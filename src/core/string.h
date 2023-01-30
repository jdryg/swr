#ifndef CORE_STRING_H
#define CORE_STRING_H

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h> // va_list

#ifdef __cplusplus
extern "C" {
#endif

typedef struct core_allocator_i core_allocator_i;

typedef struct core_str_api
{
	int32_t  (*snprintf)(char* buf, int32_t max, const char* format, ...);
	int32_t  (*vsnprintf)(char* buf, int32_t max, const char* format, va_list argList);

	uint32_t (*strnlen)(const char* str, uint32_t max);
	char*    (*strndup)(const char* str, uint32_t n, core_allocator_i* allocator);
	uint32_t (*strcpy)(char* dst, uint32_t dstMax, const char* src, uint32_t srcLen);
	int32_t  (*strcmp)(const char* lhs, uint32_t lhsMax, const char* rhs, uint32_t rhsMax);
	char*    (*strnrchr)(char* str, uint32_t n, char ch);
	char*    (*strnchr)(char* str, uint32_t n, char ch);
	uint32_t (*strncat)(char* dst, uint32_t dstMax, const char* src, uint32_t srcLen);

	int32_t (*strTo_int)(const char* str, char** endPtr, int32_t base);
	double (*strTo_double)(const char* str, char** endPtr);

	uint32_t (*utf8to_utf16)(uint16_t* dst, uint32_t dstMax, const char* src, uint32_t srcLen);
	uint32_t (*utf8from_utf16)(char* dst, uint32_t dstMax, const uint16_t* src, uint32_t srcLen);
	uint32_t (*utf8from_utf32)(char* dstUtf8, uint32_t dstMaxChars, const uint32_t* srcUtf32, uint32_t srcLen);
	uint32_t (*utf8nlen)(const char* str, uint32_t max);
} core_str_api;

extern core_str_api* str_api;

static int32_t core_snprintf(char* buf, int32_t max, const char* fmt, ...);
static int32_t core_vsnprintf(char* buf, int32_t max, const char* fmt, va_list argList);
static uint32_t core_strlen(const char* str);
static uint32_t core_strnlen(const char* str, uint32_t max);
static char* core_strdup(const char* str, core_allocator_i* allocator);
static char* core_strndup(const char* str, uint32_t n, core_allocator_i* allocator);
static uint32_t core_strcpy(char* dst, uint32_t dstMax, const char* src, uint32_t srcLen);
static int32_t core_strcmp(const char* lhs, const char* rhs);
static int32_t core_strncmp(const char* lhs, const char* rhs, uint32_t n);
static char* core_strnrchr(char* str, uint32_t n, char ch);
static char* core_strrchr(char* str, char ch);
static char* core_strnchr(char* str, uint32_t n, char ch);
static char* core_strchr(char* str, char ch);
static uint32_t core_strcat(char* dst, const char* src);
static uint32_t core_strncat(char* dst, uint32_t dstMax, const char* src, uint32_t srcLen);

static int32_t core_strToInt(const char* str, char** endPtr, int32_t base);
static float core_strToFloat(const char* str, char** endPtr);
static double core_strToDouble(const char* str, char** endPtr);

static bool core_isupper(char ch);
static bool core_islower(char ch);
static bool core_isalpha(char ch);
static bool core_isdigit(char ch);
static bool core_isspace(char ch);
static char core_tolower(char ch);
static char core_toupper(char ch);

static uint32_t core_utf8to_utf16(uint16_t* dstUTF16, uint32_t dstMax, const char* srcUTF8, uint32_t srcSize);
static uint32_t core_utf8from_utf16(char* dstUTF8, uint32_t dstMax, const uint16_t* srcUTF16, uint32_t srcLen);
static uint32_t core_utf8from_utf32(char* dstUTF8, uint32_t dstMax, const uint32_t* srcUTF32, uint32_t srcLen);
static uint32_t core_utf8nlen(const char* str, uint32_t max);
static uint32_t core_utf8len(const char* str);

#ifdef __cplusplus
}
#endif

#include "inline/string.inl"

#endif // CORE_STRING_H
