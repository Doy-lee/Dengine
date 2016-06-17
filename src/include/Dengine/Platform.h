#ifndef DENGINE_PLATFORM_H
#define DENGINE_PLATFORM_H

#include <Dengine/Common.h>

#include <stdio.h>
#include <Windows.h>

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
