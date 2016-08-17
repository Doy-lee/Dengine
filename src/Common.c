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
