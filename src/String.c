#include "Dengine/String.h"
#include "Dengine/Platform.h"

/*
 * +-------------------------------------+
 * | Header | C-String | Null Terminator |
 * +-------------------------------------+
 *          |
 *          +--> Functions return the c-string for compatibility with other
 *               string libraries
 *
 * Headers are retrieved using pointer arithmetric from the C string. These
 * strings are typechecked by their own typedef char String.
 */

typedef struct StringHeader
{
	i32 len;

	// NOTE(doyle): A string is stored as one contiguous chunk of memory. We
	// don't use a pointer for storing the string as this'd require an extra
	// 4 bytes to store the pointer, which we don't need if everything is
	// contiguous. The string follows on from the len, and we return the address
	// of the string to simulate a pointer.
	String string;
} StringHeader;

// TODO(doyle): string capacity- append if already enough space
INTERNAL StringHeader *string_getHeader(String *const string)
{
	StringHeader *result = NULL;

	// NOTE(doyle): C-String must be located at end of struct type for offset to
	// be correct! We cannot just subtract the string-header since we start at
	// the string ptr position
	if (string)
	{
		i32 byteOffsetToHeader = sizeof(StringHeader) - sizeof(String *);
		result = CAST(StringHeader *)((CAST(u8 *) string) - byteOffsetToHeader);
	}

	return result;
}

i32 string_len(String *const string)
{
	if (!string) return -1;

	StringHeader *header = string_getHeader(string);
	i32 result           = header->len;
	return result;
}

String *const string_append(MemoryArena *const arena, String *oldString,
                            char *appendString, i32 appendLen)

{
	if (!oldString || !appendString || !arena) return oldString;

	/* Calculate size of new string */
	StringHeader *oldHeader = string_getHeader(oldString);
	i32 newLen              = oldHeader->len + appendLen;
	String *newString       = string_makeLen(arena, newLen);

	/* Append strings together */
	String *insertPtr = newString;
	common_strncpy(insertPtr, oldString, oldHeader->len);
	insertPtr += oldHeader->len;
	common_strncpy(insertPtr, appendString, appendLen);

	/* Free old string */
	string_free(arena, oldString);

	return newString;
}

void string_free(MemoryArena *arena, String *string)
{
	if (!string || !arena) return;

	StringHeader *header = string_getHeader(string);
	i32 bytesToFree = sizeof(StringHeader) + header->len;

	common_memset((u8 *)header, 0, bytesToFree);
	PLATFORM_MEM_FREE(arena, header, bytesToFree);
	string = NULL;
}

String *const string_make(MemoryArena *const arena, char *string)
{
	if (!arena) return NULL;

	i32 len        = common_strlen(string);
	String *result = string_makeLen(arena, len);
	common_strncpy(result, string, len);

	return result;
}

String *const string_makeLen(MemoryArena *const arena, i32 len)
{
	if (!arena) return NULL;

	// NOTE(doyle): Allocate the string header size plus the len. But _note_
	// that StringHeader contains a single String character. This has
	// a side-effect of already preallocating a byte for the null-terminating
	// character. Whilst the len of a string counts up to the last character
	// _not_ including null-terminator.
	i32 bytesToAllocate = sizeof(StringHeader) + len;
	void *chunk         = PLATFORM_MEM_ALLOC(arena, bytesToAllocate, u8);
	if (!chunk) return NULL;

	StringHeader *header   = CAST(StringHeader *) chunk;
	header->len            = len;
	return &header->string;
}

