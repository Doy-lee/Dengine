#ifndef DENGINE_STRING_H
#define DENGINE_STRING_H

#include "Dengine/Common.h"

typedef struct MemoryArena MemoryArena;

typedef char String;

i32 string_len(String *const string);
String *const string_append(MemoryArena *const arena, String *oldString,
                            String *appendString, i32 appendLen);
void string_free(MemoryArena *arena, String *string);
String *const string_make(MemoryArena *const arena, char *string);
String *const string_makeLen(MemoryArena *const arena, i32 len);

#endif
