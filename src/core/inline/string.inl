#ifndef CORE_STRING_H
#error "Must be included from string.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

static inline int32_t core_snprintf(char* buf, int32_t max, const char* fmt, ...)
{
	va_list argList;
	va_start(argList, fmt);
	const int32_t retVal = str_api->vsnprintf(buf, max, fmt, argList);
	va_end(argList);
	return retVal;
}

static inline int32_t core_vsnprintf(char* buf, int32_t max, const char* fmt, va_list argList) 
{
	return str_api->vsnprintf(buf, max, fmt, argList);
}

static inline uint32_t core_strlen(const char* str)
{
	return str_api->strnlen(str, UINT32_MAX);
}

static inline uint32_t core_strnlen(const char* str, uint32_t max)
{
	return str_api->strnlen(str, max);
}

static inline char* core_strdup(const char* str, core_allocator_i* allocator)
{
	return str_api->strndup(str, UINT32_MAX, allocator);
}

static inline char* core_strndup(const char* str, uint32_t n, core_allocator_i* allocator)
{
	return str_api->strndup(str, n, allocator);
}

static inline uint32_t core_strcpy(char* dst, uint32_t dstSize, const char* src, uint32_t n)
{
	return str_api->strcpy(dst, dstSize, src, n);
}

static inline int32_t core_strcmp(const char* lhs, const char* rhs)
{
	return str_api->strcmp(lhs, UINT32_MAX, rhs, UINT32_MAX);
}

static inline int32_t core_strncmp(const char* lhs, const char* rhs, uint32_t n)
{
	return str_api->strcmp(lhs, n, rhs, n);
}

static inline char* core_strnrchr(char* str, uint32_t n, char ch)
{
	return str_api->strnrchr(str, n, ch);
}

static inline char* core_strrchr(char* str, char ch)
{
	return str_api->strnrchr(str, UINT32_MAX, ch);
}

static inline char* core_strnchr(char* str, uint32_t n, char ch)
{
	return str_api->strnchr(str, n, ch);
}

static inline char* core_strchr(char* str, char ch)
{
	return str_api->strnchr(str, UINT32_MAX, ch);
}

static inline uint32_t core_strcat(char* dst, const char* src)
{
	return str_api->strncat(dst, UINT32_MAX, src, UINT32_MAX);
}

static inline uint32_t core_strncat(char* dst, uint32_t dstMax, const char* src, uint32_t srcLen)
{
	return str_api->strncat(dst, dstMax, src, srcLen);
}

static inline bool core_isupper(char ch)
{
	return ((uint32_t)ch - 'A') < 26;
}

static inline bool core_islower(char ch)
{
	return ((uint32_t)ch - 'a') < 26;
}

static inline bool core_isalpha(char ch)
{
	return (((uint32_t)ch | 0x20) - 'a') < 26;
}

static inline bool core_isdigit(char ch)
{
	return ((uint32_t)ch - '0') < 10;
}

static inline bool core_isspace(char ch)
{
	return ch == ' ' || ((uint32_t)ch - '\t') < 5;
}

static inline char core_tolower(char ch)
{
	return ch + (core_isupper(ch) ? 0x20 : 0);
}

static inline char core_toupper(char ch)
{
	return ch - (core_islower(ch) ? 0x20 : 0);
}

#ifdef __cplusplus
}
#endif
