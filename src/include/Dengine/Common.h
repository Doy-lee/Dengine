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

typedef size_t MemoryIndex;

#define TRUE 1
#define FALSE 0

#define GLOBAL_VAR static
#define INTERNAL static
#define LOCAL_PERSIST static

#define ARRAY_COUNT(array) (sizeof(array) / sizeof(array[0]))
#define CAST(type) (type)
#define ASSERT(expr) if (!(expr)) { *(int *)0 = 0; }
#define IS_EVEN(value) (((value) & 1) == 0)

#define KILOBYTES(val) (val * 1024)
#define MEGABYTES(val) ((KILOBYTES(val)) * 1024)
#define GIGABYTES(val) ((MEGABYTES(val)) * 1024)

#define DENGINE_DEBUG

i32 common_strlen(const char *const string);
i32 common_strcmp(const char *a, const char *b);
void common_strncat(char *dest, const char *src, i32 numChars);
char *common_strncpy(char *dest, const char *src, i32 numChars);
u8 *common_memset(u8 *const ptr, const i32 value, const i32 numBytes);

// Max buffer size should be 11 for 32 bit integers
void common_itoa(i32 value, char *buf, i32 bufSize);
i32 common_atoi(const char *string, const i32 len);


//-----------------------------------------------------------------------------
// MurmurHash2, by Austin Appleby

// Note - This code makes a few assumptions about how your machine behaves -

// 1. We can read a 4-byte value from any address without crashing
// 2. sizeof(int) == 4

// And it has a few limitations -

// 1. It will not work incrementally.
// 2. It will not produce the same results on little-endian and big-endian
//    machines.
u32 common_murmurHash2(const void *key, i32 len, u32 seed);

// TODO(doyle): Use a proper random seed
#define RANDOM_SEED 0xDEADBEEF
inline u32 common_getHashIndex(const char *const key, u32 tableSize)
{
	u32 result = common_murmurHash2(key, common_strlen(key), RANDOM_SEED);
	result     = result % tableSize;
	return result;
}


#endif
