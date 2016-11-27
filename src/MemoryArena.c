#include "Dengine/MemoryArena.h"

void memory_arenaInit(MemoryArena_ *arena, void *base, size_t size)
{
	arena->size            = size;
	arena->used            = 0;
	arena->base            = CAST(u8 *) base;
	arena->tempMemoryCount = 0;
}

TempMemory memory_begin_temporary_region(MemoryArena_ *arena)
{
	TempMemory result = {0};
	result.arena      = arena;
	result.used       = arena->used;

	arena->tempMemoryCount++;

	return result;
}

void memory_end_temporary_region(TempMemory tempMemory)
{
	MemoryArena_ *arena = tempMemory.arena;
	ASSERT(arena->used > tempMemory.used)

	arena->used        = tempMemory.used;
	ASSERT(arena->tempMemoryCount > 0)

	arena->tempMemoryCount--;
}
