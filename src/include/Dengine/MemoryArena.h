#ifndef DENGINE_MEMORY_ARENA_H
#define DENGINE_MEMORY_ARENA_H

#include "Dengine/Common.h"

typedef struct MemoryArena
{
	MemoryIndex size;
	MemoryIndex used;
	u8 *base;

	i32 tempMemoryCount;
} MemoryArena_;

typedef struct Memory
{
	void *persistent;
	MemoryIndex persistentSize;

	void *transient;
	MemoryIndex transientSize;

	b32 init;
} Memory;

typedef struct TempMemory
{
	MemoryArena_ *arena;
	MemoryIndex used;
} TempMemory;

TempMemory memory_begin_temporary_region(MemoryArena_ *arena);
void memory_end_temporary_region(TempMemory tempMemory);

#define MEMORY_PUSH_STRUCT(arena, type) (type *)memory_pushBytes(arena, sizeof(type))
#define MEMORY_PUSH_ARRAY(arena, count, type) (type *)memory_pushBytes(arena, (count)*sizeof(type))
inline void *memory_pushBytes(MemoryArena_ *arena, MemoryIndex size)
{
	ASSERT((arena->used + size) <= arena->size);
	void *result = arena->base + arena->used;
	arena->used += size;

	return result;
}

void memory_arenaInit(MemoryArena_ *arena, void *base, MemoryIndex size);

#endif
