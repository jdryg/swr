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
static uint32_t core_str_utf8ToUtf16(uint16_t* dst, uint32_t dstMax, const char* src, uint32_t srcLen);
static uint32_t core_str_utf8FromUtf16(char* dst, uint32_t dstMax, const uint16_t* src, uint32_t srcLen);
static uint32_t core_str_utf8FromUtf32(char* dst, uint32_t dstMax, const uint32_t* src, uint32_t srcLen);
static uint32_t core_str_utf8nlen(const char* str, uint32_t max);

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
	.utf8to_utf16 = core_str_utf8ToUtf16,
	.utf8from_utf16 = core_str_utf8FromUtf16,
	.utf8from_utf32 = core_str_utf8FromUtf32,
	.utf8nlen = core_str_utf8nlen
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

//////////////////////////////////////////////////////////////////////////
// UTF-8 functions
//
// https://www.rfc-editor.org/rfc/rfc3629.html
// 
#define UTF8_INVALID_CODEPOINT 0xFFFFFFFF
#define UTF8_INVALID_CHAR      '?'

static const char* utf8ToCodepoint(const char* str, uint32_t* cp);
static char* utf8FromCodepoint(uint32_t cp, char* dst, uint32_t* dstSize);
static const uint16_t* utf16ToCodepoint(const uint16_t* str, uint32_t* cp);
static uint16_t* utf16FromCodepoint(uint32_t cp, uint16_t* dst, uint32_t* dstSize);

static uint32_t core_str_utf8ToUtf16(uint16_t* dstUtf16, uint32_t dstMaxChars, const char* srcUtf8, uint32_t srcLen)
{
	if (dstMaxChars == 0) {
		return 0;
	}

	uint16_t* dst = dstUtf16;

	// Don't care if 'srcLen' is UINT32_MAX because the loop 
	// ends when the null character is encountered.
	const char* srcEnd = srcUtf8 + srcLen;

	// Make sure there's enough room for the null character
	uint32_t remaining = dstMaxChars - 1;
	while (remaining != 0 && srcUtf8 < srcEnd) {
		uint32_t cp = UTF8_INVALID_CODEPOINT;
		srcUtf8 = utf8ToCodepoint(srcUtf8, &cp);

		if (!cp) {
			break;
		}

		dst = utf16FromCodepoint(cp, dst, &remaining);
	}

	*dst = 0;
	return (uint32_t)(dst - dstUtf16);
}

static uint32_t core_str_utf8FromUtf16(char* dstUtf8, uint32_t dstMaxChars, const uint16_t* srcUtf16, uint32_t srcLen)
{
	if (dstMaxChars == 0) {
		return 0;
	}

	char* dst = dstUtf8;

	// Don't care if 'srcLen' is UINT32_MAX because the loop 
	// ends when the null character is encountered.
	const uint16_t* srcEnd = srcUtf16 + srcLen;

	// Make sure there's enough room for the null character
	uint32_t remaining = dstMaxChars - 1;
	while (remaining != 0 && srcUtf16 < srcEnd) {
		uint32_t cp = UTF8_INVALID_CODEPOINT;
		srcUtf16 = utf16ToCodepoint(srcUtf16, &cp);

		if (!cp) {
			break;
		}

		dst = utf8FromCodepoint(cp, dst, &remaining);
	}

	*dst = 0;
	return (uint32_t)(dst - dstUtf8);
}

static uint32_t core_str_utf8FromUtf32(char* dstUtf8, uint32_t dstMaxChars, const uint32_t* srcUtf32, uint32_t srcLen)
{
	if (dstMaxChars == 0) {
		return 0;
	}

	char* dst = dstUtf8;

	// Don't care if 'srcLen' is UINT32_MAX because the loop 
	// ends when the null character is encountered.
	const uint32_t* srcEnd = srcUtf32 + srcLen;

	// Make sure there's enough room for the null character
	uint32_t remaining = dstMaxChars - 1;
	while (remaining != 0 && srcUtf32 < srcEnd) {
		uint32_t cp = *srcUtf32;
		++srcUtf32;
		if (!cp) {
			break;
		}

		dst = utf8FromCodepoint(cp, dst, &remaining);
	}

	*dst = 0;
	return (uint32_t)(dst - dstUtf8);
}

static uint32_t core_str_utf8nlen(const char* str, uint32_t max)
{
	uint32_t n = 0;

	const char* strEnd = str + max;
	while (*str && str < strEnd) {
		uint32_t cp = UTF8_INVALID_CODEPOINT;
		str = utf8ToCodepoint(str, &cp);
		if (!cp) {
			break;
		}

		++n;
	}

	return n;
}

static const char* utf8ToCodepoint(const char* str, uint32_t* cp)
{
	*cp = UTF8_INVALID_CODEPOINT;

	const uint32_t octet0 = (uint32_t)str[0];
	if (octet0 == 0) {
		*cp = 0;
		return str;
	} else if ((octet0 & 0x80) == 0) {
		// 1-octet "sequence"
		// 0xxxxxxx
		*cp = (uint32_t)(octet0 & 0x7F);
	} else if ((octet0 & 0xC0) == 0x80) {
		// Invalid start
		// Let it fall through
	} else if ((octet0 & 0xE0) == 0xC0) {
		// 2-octet sequence
		// 110xxxxx 10xxxxxx
		const uint32_t octet1 = (uint32_t)str[1];
		if ((octet1 & 0xC0) == 0x80) {
			const uint32_t val = 0
				| ((octet0 & 0x0F) << 6)
				| ((octet1 & 0x3F) << 0)
				;
			if (val >= 0x00000080 && val <= 0x000007FF) {
				*cp = val;
				return str + 2;
			}
		}
	} else if ((octet0 & 0xF0) == 0xE0) {
		// 3-octet sequence
		// 1110xxxx 10xxxxxx 10xxxxxx
		const uint32_t octet12 = (uint32_t)(*(uint16_t*)&str[1]);
		if ((octet12 & 0xC0C0) == 0x8080) {
			const uint32_t val = 0
				| ((octet0 & 0x0F) << 12)
				| ((octet12 & 0x003F) << 6)
				| ((octet12 & 0x3F00) >> 8)
				;
			if ((val >= 0x00000800 && val < 0x0000D800) || (val > 0x0000DFFF && val <= 0x0000FFFD)) {
				*cp = val;
				return str + 3;
			}
		}
	} else if ((octet0 & 0xF8) == 0xF0) {
		// 4-octet sequence
		// 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
		const uint32_t octet0123 = *(uint32_t*)&str[0];
		if ((octet0123 & 0xC0C0C000) == 0x80808000) {
			const uint32_t val = 0
				| ((octet0123 & 0x00000007) << 18)
				| ((octet0123 & 0x00003F00) << 4)
				| ((octet0123 & 0x003F0000) >> 10)
				| ((octet0123 & 0x3F000000) >> 24)
				;
			if (val >= 0x00010000 && val <= 0x0010FFFF) {
				*cp = val;
				return str + 4;
			}
		}
	}

	return str + 1;
}

static char* utf8FromCodepoint(uint32_t cp, char* dst, uint32_t* dstSize)
{
	if (*dstSize == 0) {
		return dst;
	}

	if (cp < 0x80) {
		// 1-octet sequence
		// 0xxxxxxx
		dst[0] = (char)(cp & 0x7F);
		return dst + 1;
	} else if (cp < 0x800) {
		// 2-octet sequence
		// 110xxxxx 10xxxxxx
		if (*dstSize >= 2) {
			dst[0] = (char)((uint8_t)(0xC0 | ((cp >> 6) & 0x1F)));
			dst[1] = (char)((uint8_t)(0x80 | ((cp >> 0) & 0x3F)));
			*dstSize -= 2;
			return dst + 2;
		}

		// Is there's not enough room to decode this character, inform the caller
		// that the whole buffer has been filled in order to avoid calling back
		// with the same buffer.
		*dstSize = 0;
		return dst;
	} else if (cp < 0x00010000) {
		// 3-octet sequence
		// 1110xxxx 10xxxxxx 10xxxxxx
		if (*dstSize >= 3) {
			dst[0] = (char)((uint8_t)(0xE0 | ((cp >> 12) & 0x0F)));
			dst[1] = (char)((uint8_t)(0x80 | ((cp >> 6) & 0x3F)));
			dst[2] = (char)((uint8_t)(0x80 | ((cp >> 0) & 0x3F)));
			*dstSize -= 3;
			return dst + 3;
		}

		*dstSize = 0;
		return dst;
	} else if (cp < 0x00110000) {
		// 4-octet sequence
		// 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
		if (*dstSize >= 4) {
			dst[0] = (char)((uint8_t)(0xF0 | ((cp >> 18) & 0x07)));
			dst[1] = (char)((uint8_t)(0x80 | ((cp >> 12) & 0x3F)));
			dst[2] = (char)((uint8_t)(0x80 | ((cp >> 6) & 0x3F)));
			dst[3] = (char)((uint8_t)(0x80 | ((cp >> 0) & 0x3F)));
			*dstSize -= 4;
			return dst + 4;
		}

		*dstSize = 0;
		return dst;
	}

	dst[0] = UTF8_INVALID_CHAR;
	*dstSize -= 1;
	return dst + 1;
}

static const uint16_t* utf16ToCodepoint(const uint16_t* str, uint32_t* cp)
{
	*cp = UTF8_INVALID_CODEPOINT;

	uint32_t word = (uint32_t)str[0];

	if (word == 0) {
		// null terminator, end of string.
		*cp = 0;
		return str;
	} else if (word >= 0xD800 && word <= 0xDBFF) {
		// Start surrogate pair
		const uint32_t pair = (uint32_t)str[1];
		if (pair >= 0xDC00 && pair <= 0xDFFF) {
			*cp = 0x10000
				| ((word & 0x03FF) << 10)
				| ((pair & 0x03FF) << 0)
				;

			return str + 2;
		}
	} else if (word < 0xDC00 || word > 0xDFFF) {
		*cp = word;
	} else {
		// 'word' is the second half of a surrogate pair? 
		// Let it fall through.
	}

	return str + 1;
}

static uint16_t* utf16FromCodepoint(uint32_t cp, uint16_t* dst, uint32_t* dstSize)
{
	if (*dstSize == 0) {
		return dst;
	}

	if (cp < 0x00010000) {
		dst[0] = (uint16_t)cp;
		*dstSize -= 1;
		return dst + 1;
	} else if (cp <= 0x0010FFFF) {
		if (*dstSize >= 2) {
			cp -= 0x10000;
			dst[0] = (uint16_t)(0xD800 | ((cp >> 10) & 0x3FF));
			dst[1] = (uint16_t)(0xDC00 | ((cp >> 0) & 0x3FF));
			*dstSize -= 2;
			return dst + 2;
		}

		*dstSize = 0;
		return dst;
	}

	dst[0] = (uint16_t)UTF8_INVALID_CHAR;
	*dstSize -= 1;
	return dst + 1;
}
