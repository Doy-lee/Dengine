#ifndef DENGINE_PLATFORM_H
#define DENGINE_PLATFORM_H

#include "Dengine/Common.h"

typedef struct PlatformFileRead
{
	void *buffer;
	i32 size;
} PlatformFileRead;

// TODO(doyle): Create own custom memory allocator
#define PLATFORM_MEM_FREE(ptr, bytes) platform_memoryFree(CAST(void *) ptr, bytes)
// TODO(doyle): numBytes in mem free is temporary until we create custom
// allocator since we haven't put in a system to track memory usage per
// allocation
void platform_memoryFree(void *data, i32 numBytes);

#define PLATFORM_MEM_ALLOC(num, type) CAST(type *) platform_memoryAlloc(num * sizeof(type))
void *platform_memoryAlloc(i32 numBytes);

void platform_closeFileRead(PlatformFileRead *file);
i32 platform_readFileToBuffer(const char *const filePath,
                              PlatformFileRead *file);

#endif
