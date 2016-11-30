#include <stdlib.h>

#include "Dengine/Common.h"
#include "Dengine/MemoryArena.h"

void common_optimalArrayV2Create(OptimalArrayV2 *array)
{
	array->ptr  = array->fastStorage;
	array->size = ARRAY_COUNT(array->fastStorage);
}

i32 common_optimalArrayV2Push(OptimalArrayV2 *array, v2 data)
{
	if (array->index + 1 > array->size)
	{
		array->size += ARRAY_COUNT(array->fastStorage);
		i32 newSizeInBytes = array->size * sizeof(v2);

		/* If first time expanding, we need to manually malloc and copy */
		if (array->ptr == array->fastStorage)
		{
			array->ptr = malloc(newSizeInBytes);
			for (i32 i = 0; i < ARRAY_COUNT(array->fastStorage); i++)
			{
				array->ptr[i] = array->fastStorage[i];
			}
		}
		else
		{
			array->ptr = realloc(array->ptr, newSizeInBytes);
		}

		if (!array->ptr) return optimalarrayerror_out_of_memory;
	}

	array->ptr[array->index++] = data;

	return 0;
}

void common_optimalArrayV2Destroy(OptimalArrayV2 *array)
{
	if (array->ptr != array->fastStorage)
	{
		free(array->ptr);
	}
}

/*
 * +-------------------------------------+
 * | Header | C-String | Null Terminator |
 * +-------------------------------------+
 *          |
 *          +--> Functions return the c-string for compatibility with other
 *               string libraries
 *
 * Headers are retrieved using pointer arithmetric from the C string. These
 * strings are typechecked by their own typedef char String.
 */

typedef struct StringHeader
{
	i32 len;

	// NOTE(doyle): A string is stored as one contiguous chunk of memory. We
	// don't use a pointer for storing the string as this'd require an extra
	// 4 bytes to store the pointer, which we don't need if everything is
	// contiguous. The string follows on from the len, and we return the address
	// of the string to simulate a pointer.
	String string;
} StringHeader;

// TODO(doyle): string capacity- append if already enough space
INTERNAL StringHeader *stringGetHeader(String *const string)
{
	StringHeader *result = NULL;

	// NOTE(doyle): C-String must be located at end of struct type for offset to
	// be correct! We cannot just subtract the string-header since we start at
	// the string ptr position
	if (string)
	{
		i32 byteOffsetToHeader = sizeof(StringHeader) - sizeof(String *);
		result = CAST(StringHeader *)((CAST(u8 *) string) - byteOffsetToHeader);
	}

	return result;
}

i32 common_stringLen(String *const string)
{
	if (!string) return -1;

	StringHeader *header = stringGetHeader(string);
	i32 result           = header->len;
	return result;
}

String *const common_stringAppend(MemoryArena_ *const arena, String *oldString,
                                  char *appendString, i32 appendLen)

{
	if (!oldString || !appendString || !arena) return oldString;

	/* Calculate size of new string */
	StringHeader *oldHeader = stringGetHeader(oldString);
	i32 newLen              = oldHeader->len + appendLen;
	String *newString       = common_stringMakeLen(arena, newLen);

	/* Append strings together */
	String *insertPtr = newString;
	common_strncpy(insertPtr, oldString, oldHeader->len);
	insertPtr += oldHeader->len;
	common_strncpy(insertPtr, appendString, appendLen);

	/* Free old string */
	common_stringFree(arena, oldString);

	return newString;
}

void common_stringFree(MemoryArena_ *arena, String *string)
{
	if (!string || !arena) return;

	StringHeader *header = stringGetHeader(string);
	i32 bytesToFree      = sizeof(StringHeader) + header->len;

	common_memset((u8 *)header, 0, bytesToFree);

	// TODO(doyle): Mem free
	// PLATFORM_MEM_FREE(arena, header, bytesToFree);

	string = NULL;
}

String *const common_stringMake(MemoryArena_ *const arena, char *string)
{
	if (!arena) return NULL;

	i32 len        = common_strlen(string);
	String *result = common_stringMakeLen(arena, len);
	common_strncpy(result, string, len);

	return result;
}

String *const common_stringMakeLen(MemoryArena_ *const arena, i32 len)
{
	if (!arena) return NULL;

	// NOTE(doyle): Allocate the string header size plus the len. But _note_
	// that StringHeader contains a single String character. This has
	// a side-effect of already preallocating a byte for the null-terminating
	// character. Whilst the len of a string counts up to the last character
	// _not_ including null-terminator.
	i32 bytesToAllocate = sizeof(StringHeader) + len;
	void *chunk         = memory_pushBytes(arena, bytesToAllocate * sizeof(u8));
	if (!chunk) return NULL;

	StringHeader *header = CAST(StringHeader *) chunk;
	header->len          = len;
	return &header->string;
}

i32 common_strlen(const char *const string)
{
	i32 result = 0;
	while (string[result])
		result++;
	return result;
}

i32 common_strcmp(const char *a, const char *b)
{
	while (*a == *b)
	{
		if (!*a) return 0;
		a++;
		b++;
	}

	return ((*a < *b) ? -1 : 1);
}

void common_strncat(char *dest, const char *src, i32 numChars)
{
	char *stringPtr = dest;
	while (*stringPtr)
		stringPtr++;

	for (i32 i         = 0; i < numChars; i++)
		*(stringPtr++) = src[i];
}

char *common_strncpy(char *dest, const char *src, i32 numChars)
{
	for (i32 i  = 0; i < numChars; i++)
		dest[i] = src[i];

	return dest;
}

u8 *common_memset(u8 *const ptr, const i32 value, const i32 numBytes)
{
	for (i32 i = 0; i < numBytes; i++)
		ptr[i] = value;

	return ptr;
}

INTERNAL void reverseString(char *const buf, const i32 bufSize)
{
	if (!buf || bufSize == 0 || bufSize == 1) return;
	i32 mid = bufSize / 2;

	for (i32 i = 0; i < mid; i++)
	{
		char tmp               = buf[i];
		buf[i]                 = buf[(bufSize - 1) - i];
		buf[(bufSize - 1) - i] = tmp;
	}
}

void common_itoa(i32 value, char *buf, i32 bufSize)
{
	if (!buf || bufSize == 0) return;

	if (value == 0)
	{
		buf[0] = '0';
		return;
	}
	
	// NOTE(doyle): Max 32bit integer (+-)2147483647
	i32 charIndex = 0;
	b32 negative            = FALSE;
	if (value < 0) negative = TRUE;

	if (negative) buf[charIndex++] = '-';

	i32 val = ABS(value);
	while (val != 0 && charIndex < bufSize)
	{
		i32 rem          = val % 10;
		buf[charIndex++] = rem + '0';
		val /= 10;
	}

	// NOTE(doyle): If string is negative, we only want to reverse starting
	// from the second character, so we don't put the negative sign at the end
	if (negative)
	{
		reverseString(buf + 1, charIndex - 1);
	}
	else
	{
		reverseString(buf, charIndex);
	}
}

// TODO(doyle): Consider if we should trash ptrs in string operations in general
i32 common_atoi(const char *string, const i32 len)
{
	if (len == 0) return -1;

	// TODO(doyle): Implement ATOI
	i32 index = 0;
	if (string[index] != '-' && string[index] != '+' &&
	    (string[index] < '0' || string[index] > '9'))
	{
		return -1;
	}

	b32 isNegative = FALSE;
	if (string[index] == '-' || string[index] == '+')
	{
		if (string[index] == '-') isNegative = TRUE;
		index++;
	}

	i32 result = 0;
	for (i32 i = index; i < len; i++)
	{
		if (string[i] >= '0' && string[i] <= '9')
		{
			result *= 10;
			result += string[i] - '0';
		}
		else
		{
			break;
		}
	}

	if (isNegative) result *= -1;

	return result;
}

u32 common_murmurHash2(const void *key, i32 len, u32 seed)
{
	// 'm' and 'r' are mixing constants generated offline.
	// They're not really 'magic', they just happen to work well.

	const u32 m = 0x5bd1e995;
	const i32 r = 24;

	// Initialize the hash to a 'random' value

	u32 h = seed ^ len;

	// Mix 4 bytes at a time into the hash

	const unsigned char *data = (const unsigned char *)key;

	while (len >= 4)
	{
		u32 k = *(u32 *)data;

		k *= m;
		k ^= k >> r;
		k *= m;

		h *= m;
		h ^= k;

		data += 4;
		len -= 4;
	}

	// Handle the last few bytes of the input array

	switch (len)
	{
	case 3:
		h ^= data[2] << 16;
	case 2:
		h ^= data[1] << 8;
	case 1:
		h ^= data[0];
		h *= m;
	};

	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
}
