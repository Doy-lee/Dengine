#ifndef DENGINE_PLATFORM_H
#define DENGINE_PLATFORM_H

#include <Windows.h>
#include <stdio.h>

#include "Dengine/Common.h"

typedef struct
{
	void *buffer;
	i32 size;
} PlatformFileRead;

i32 platform_readFileToBuffer(const char *const filePath,
                              PlatformFileRead *file);

inline void platform_closeFileRead(PlatformFileRead *file)
{
	if (file->buffer)
	{
		free(file->buffer);
	}
}

#endif
