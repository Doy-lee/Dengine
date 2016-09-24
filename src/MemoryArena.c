#include "Dengine/MemoryArena.h"

void memory_arenaInit(MemoryArena_ *arena, void *base, size_t size)
{
	arena->size = size;
	arena->used = 0;
	arena->base = CAST(u8 *)base;
}
