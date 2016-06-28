#ifndef DENGINE_PLATFORM_H
#define DENGINE_PLATFORM_H

#include <Windows.h>
#include <stdio.h>

#include "Dengine/Common.h"

typedef struct
{
	void *buffer;
	i32 size;
} PlatformFileReadResult;

i32 platform_readFileToBuffer(const char *const filePath,
                              PlatformFileReadResult *file);

inline void platform_closeFileReadResult(PlatformFileReadResult *file)
{
	if (file->buffer)
	{
		free(file->buffer);
	}
}

#endif
