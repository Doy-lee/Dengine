#include "Dengine/Common.h"
#include "Dengine/Math.h"

i32 common_strlen(const char *const string)
{
	i32 result = 0;
	while (string[result]) result++;
	return result;
}

i32 common_strcmp(const char *a, const char *b)
{
	while (*a == *b)
	{
		if (!*a)
			return 0;
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

	for (i32 i = 0; i < numChars; i++)
		*(stringPtr++) = src[i];
}

char *common_strncpy(char *dest, const char *src, i32 numChars)
{
	for (i32 i = 0; i < numChars; i++)
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
		char tmp         = buf[i];
		buf[i]           = buf[(bufSize-1) - i];
		buf[(bufSize-1) - i] = tmp;
	}
}

void common_itoa(i32 value, char *buf, i32 bufSize)
{
	if (!buf || bufSize == 0) return;

	// NOTE(doyle): Max 32bit integer (+-)2147483647
	i32 charIndex   = 0;

	b32 negative = FALSE;
	if (value < 0) negative = TRUE;

	if (negative) buf[charIndex++] = '-';

	i32 val = ABS(value);
	while (val != 0 && charIndex < bufSize)
	{
		i32 rem = val % 10;
		buf[charIndex++] = rem + '0';
		val /= 10;
	}

	// NOTE(doyle): The actual string length may differ from the bufSize
	reverseString(buf, common_strlen(buf));
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

	const unsigned char * data = (const unsigned char *)key;

	while(len >= 4)
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

	switch(len)
	{
	case 3: h ^= data[2] << 16;
	case 2: h ^= data[1] << 8;
	case 1: h ^= data[0];
	        h *= m;
	};

	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
} 
