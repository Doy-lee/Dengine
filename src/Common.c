#include "Dengine\Common.h"

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
