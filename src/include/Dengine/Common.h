#ifndef DENGINE_COMMON_H
#define DENGINE_COMMON_H

#include <stdint.h>

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int16_t i16;
typedef int32_t i32;
typedef i32 b32;

typedef float f32;
typedef double f64;

#define TRUE 1
#define FALSE 0

#define GLOBAL_VAR static
#define INTERNAL static
#define LOCAL_PERSIST static

#define ARRAY_COUNT(array) (sizeof(array) / sizeof(array[0]))
#define CAST(type) (type)
#define ASSERT(expr) if(!(expr)) { *(int *)0 = 0; }

#define DENGINE_DEBUG

i32 common_strlen(const char *const string);
i32 common_strcmp(const char *a, const char *b);
void common_itoa(i32 value, char *buf, i32 bufSize);

#endif
