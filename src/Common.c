#include "Dengine\Common.h"

i32 common_strlen(const char *const string)
{
	i32 result = 0;
	while (string[result]) result++;
	return result;
}
