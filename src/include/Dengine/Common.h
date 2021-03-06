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
typedef char String;

typedef struct MemoryArena MemoryArena_;

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

#include "Dengine/Math.h"

/*
   NOTE(doyle): Small sized optimised dynamic array that grows as required. The
   array uses the stack first, only if it runs out of space does it rely on
   memory allocated from the machine.

   The array->ptr is initially set to fast storage. Once we are out of space
   we allocate space on the heap for the ptr and copy over the elements in
   fast storage.

   The default behaviour expands the array storage by the size of fastStorage.
 */

enum OptimalArrayError
{
	optimalarrayerror_out_of_memory = 1,
	optimalarrayerror_count,
};

typedef struct OptimalArrayV2
{
	v2 fastStorage[16];
	v2 *ptr;
	i32 index;
	i32 size;
} OptimalArrayV2;
void common_optimalArrayV2Create(OptimalArrayV2 *array);
i32 common_optimalArrayV2Push(OptimalArrayV2 *array, v2 data);
void common_optimalArrayV2Destroy(OptimalArrayV2 *array);

i32 common_stringLen(String *const string);
String *const common_stringAppend(MemoryArena_ *const arena, String *oldString,
                                  String *appendString, i32 appendLen);
void common_stringFree(MemoryArena_ *arena, String *string);
String *const common_stringMake(MemoryArena_ *const arena, char *string);
String *const common_stringMakeLen(MemoryArena_ *const arena, i32 len);

i32 common_strlen(const char *const string);
i32 common_strcmp(const char *a, const char *b);
void common_strncat(char *dest, const char *src, i32 numChars);
char *common_strncpy(char *dest, const char *src, i32 numChars);

u8 *common_memset(u8 *const ptr, const i32 value, const i32 numBytes);

// Max buffer size should be 11 for 32 bit integers
#define COMMON_ITOA_MAX_BUFFER_32BIT 11
void common_itoa(i32 value, char *buf, i32 bufSize);
i32 common_atoi(const char *string, const i32 len);

inline b32 common_isSet(u32 bitfield, u32 flags)
{
	b32 result = FALSE;
	if ((bitfield & flags) == flags) result = TRUE;

	return result;
}

inline void common_unitTest()
{
	{
		char itoa[COMMON_ITOA_MAX_BUFFER_32BIT] = {0};
		common_itoa(0, itoa, ARRAY_COUNT(itoa));
		ASSERT(itoa[0] == '0');
		ASSERT(itoa[1] == 0);

		common_memset(itoa, 1, ARRAY_COUNT(itoa));
		for (i32 i = 0; i < ARRAY_COUNT(itoa); i++) ASSERT(itoa[i] == 1);

		common_memset(itoa, 0, ARRAY_COUNT(itoa));
		for (i32 i = 0; i < ARRAY_COUNT(itoa); i++) ASSERT(itoa[i] == 0);

		common_itoa(-123, itoa, ARRAY_COUNT(itoa));
		ASSERT(itoa[0] == '-');
		ASSERT(itoa[1] == '1');
		ASSERT(itoa[2] == '2');
		ASSERT(itoa[3] == '3');
		ASSERT(itoa[4] == 0);

		common_memset(itoa, 0, ARRAY_COUNT(itoa));
		for (i32 i = 0; i < ARRAY_COUNT(itoa); i++) ASSERT(itoa[i] == 0);
		common_itoa(93, itoa, ARRAY_COUNT(itoa));
		ASSERT(itoa[0] == '9');
		ASSERT(itoa[1] == '3');
		ASSERT(itoa[2] == 0);

		common_memset(itoa, 0, ARRAY_COUNT(itoa));
		for (i32 i = 0; i < ARRAY_COUNT(itoa); i++) ASSERT(itoa[i] == 0);
		common_itoa(-0, itoa, ARRAY_COUNT(itoa));
		ASSERT(itoa[0] == '0');
		ASSERT(itoa[1] == 0);
	}
}

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
