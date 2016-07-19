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
}
