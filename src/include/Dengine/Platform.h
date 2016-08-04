#ifndef DENGINE_PLATFORM_H
#define DENGINE_PLATFORM_H

#include "Dengine/Common.h"
#include "Dengine/Math.h"

/* Forward Declaration */
typedef struct MemoryArena MemoryArena;

enum KeyCodes
{
	keycode_up,
	keycode_down,
	keycode_left,
	keycode_right,
	keycode_space,
	keycode_mouseLeft,
	keycode_count,
};

typedef struct KeyInput
{
	v2 mouseP;
	union
	{
		b32 keys[keycode_count - 1];
		struct
		{
			b32 up;
			b32 down;
			b32 left;
			b32 right;
			b32 space;
			b32 leftShift;
			b32 mouseLeft;
		};
	};
} KeyInput;

typedef struct PlatformFileRead
{
	void *buffer;
	i32 size;
} PlatformFileRead;

// TODO(doyle): Create own custom memory allocator
#define PLATFORM_MEM_FREE(arena, ptr, bytes) platform_memoryFree(arena, CAST(void *) ptr, bytes)
// TODO(doyle): numBytes in mem free is temporary until we create custom
// allocator since we haven't put in a system to track memory usage per
// allocation
void platform_memoryFree(MemoryArena *arena, void *data, i32 numBytes);

#define PLATFORM_MEM_ALLOC(arena, num, type) CAST(type *) platform_memoryAlloc(arena, num * sizeof(type))
void *platform_memoryAlloc(MemoryArena *arena, i32 numBytes);

void platform_closeFileRead(MemoryArena *arena, PlatformFileRead *file);
i32 platform_readFileToBuffer(MemoryArena *arena, const char *const filePath,
                              PlatformFileRead *file);

#endif
