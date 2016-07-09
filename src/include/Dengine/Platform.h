#ifndef DENGINE_PLATFORM_H
#define DENGINE_PLATFORM_H

#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "Dengine/Common.h"
#include "Dengine/Debug.h"

// TODO(doyle): Create own custom memory allocator
#define PLATFORM_MEM_ALLOC(num, type)                                          \
	CAST(type *) platform_memoryAlloc(num * sizeof(type))
#define PLATFORM_MEM_FREE(ptr, bytes)                                          \
	platform_memoryFree(CAST(void *) ptr, bytes)

typedef struct
{
	void *buffer;
	i32 size;
} PlatformFileRead;

// TODO(doyle): numBytes in mem free is temporary until we create custom
// allocator since we haven't put in a system to track memory usage per
// allocation
inline void platform_memoryFree(void *data, i32 numBytes)
{
	if (data) free(data);

#ifdef DENGINE_DEBUG
	GLOBAL_debugState.totalMemoryAllocated -= numBytes;
#endif

}

inline void *platform_memoryAlloc(i32 numBytes)
{
	void *result = calloc(1, numBytes);

#ifdef DENGINE_DEBUG
	if (result)
		GLOBAL_debugState.totalMemoryAllocated += numBytes;
#endif

	return result;
}

inline void platform_closeFileRead(PlatformFileRead *file)
{
	PLATFORM_MEM_FREE(file->buffer, file->size);
}


i32 platform_readFileToBuffer(const char *const filePath,
                              PlatformFileRead *file);

#endif
