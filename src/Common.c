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

char *common_strncpy(char *dest, const char *src, i32 numChars)
{
	for (i32 i = 0; i < numChars; i++)
		dest[i] = src[i];

	return dest;
}

char *common_memset(char *const ptr, const i32 value, const i32 numBytes)
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
