#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "Dengine/Platform.h"
#include "Dengine/Debug.h"
#include "Dengine/MemoryArena.h"

void platform_memoryFree(MemoryArena_ *arena, void *data, size_t numBytes)
{
	if (data) free(data);

#ifdef DENGINE_DEBUG
	debug_countIncrement(debugcount_platformMemFree);
	arena->used -= numBytes;
#endif
}

void *platform_memoryAlloc(MemoryArena_ *arena, size_t numBytes)
{
	void *result = calloc(1, numBytes);

#ifdef DENGINE_DEBUG
	debug_countIncrement(debugcount_platformMemAlloc);
	if (result && arena)
		arena->used += numBytes;
#endif
	return result;
}

// TODO(doyle): If we use arena temporary memory this is not necessary
void platform_closeFileRead(MemoryArena_ *arena, PlatformFileRead *file)
{
	// TODO(doyle): Mem free
	// PLATFORM_MEM_FREE(arena, file->buffer, file->size);
}

i32 platform_readFileToBuffer(MemoryArena_ *arena, const char *const filePath,
                              PlatformFileRead *file)
{
	HANDLE fileHandle = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ,
	                               NULL, OPEN_ALWAYS, 0, NULL);
	if (fileHandle == INVALID_HANDLE_VALUE)
	{
		printf("CreateFile() failed: INVALID_HANDLE_VALUE\n");
		return -1;
	}

	LARGE_INTEGER fileSize;
	BOOL status = GetFileSizeEx(fileHandle, &fileSize);
	if (!status)
	{

		printf("GetFileSizeEx() failed: %d error number\n",
		       status);
		return status;
	}

	// TODO(doyle): Warning we assume files less than 4GB
	file->buffer = memory_pushBytes(arena, fileSize.LowPart * sizeof(char));
	file->size   = fileSize.LowPart;

	DWORD numBytesRead = 0;

	status =
	    ReadFile(fileHandle, file->buffer, file->size, &numBytesRead, NULL);
	if (!status)
	{
		printf("ReadFile() failed: %d error number\n",
		       status);

		char msgBuffer[512] = {0};
		DWORD dw = GetLastError();

		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		                  FORMAT_MESSAGE_FROM_SYSTEM |
		                  FORMAT_MESSAGE_IGNORE_INSERTS,
		              NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		              (LPTSTR)msgBuffer, 0, NULL);


		// TODO(doyle): Mem free
		// PLATFORM_MEM_FREE(arena, file->buffer, file->size);
		return status;
	}
	else if (numBytesRead != file->size)
	{
		printf(
		    "ReadFile() failed: Number of bytes read doesn't match file "
		    "size\n");
		// TODO(doyle): Mem free
		// PLATFORM_MEM_FREE(arena, file->buffer, file->size);
		return -1;
	}

	return 0;
}

void platform_inputBufferProcess(InputBuffer *inputBuffer, f32 dt)
{
	KeyState *keyBuffer = inputBuffer->keys;
	for (enum KeyCode code = 0; code < keycode_count; code++)
	{
		KeyState *key = &keyBuffer[code];

		u32 halfTransitionCount =
		    key->newHalfTransitionCount - key->oldHalfTransitionCount;

		if (halfTransitionCount > 0)
		{
			b32 transitionCountIsOdd = ((halfTransitionCount & 1) == 1);
			if (transitionCountIsOdd)
			{
				/* If it was not last ended down, then update interval if
				 * necessary */
				if (!common_isSet(key->flags, keystateflag_ended_down))
				{
					if (key->delayInterval > 0) key->delayInterval -= dt;

					key->flags |= keystateflag_pressed_on_curr_frame;
				}
				key->flags ^= keystateflag_ended_down;
			}
		}
		else
		{
			key->flags &= (~keystateflag_pressed_on_curr_frame);
		}

		key->newHalfTransitionCount = key->oldHalfTransitionCount;
	}
}

b32 platform_queryKey(KeyState *key, enum ReadKeyType readType,
                      f32 delayInterval)
{

	if (!common_isSet(key->flags, keystateflag_ended_down)) return FALSE;

	switch (readType)
	{
	case readkeytype_one_shot:
	{
		if (common_isSet(key->flags, keystateflag_pressed_on_curr_frame))
			return TRUE;
	}
	break;

	case readkeytype_repeat:
	case readkeytype_delay_repeat:
	{
		if (common_isSet(key->flags, keystateflag_pressed_on_curr_frame))
		{
			if (readType == readkeytype_delay_repeat)
			{
				// TODO(doyle): Let user set arbitrary delay after initial input
				key->delayInterval = 2 * delayInterval;
			}
			else
			{
				key->delayInterval = delayInterval;
			}
			return TRUE;
		}
		else if (key->delayInterval <= 0.0f)
		{
			key->delayInterval = delayInterval;
			return TRUE;
		}
	}
	break;

	default:
	{
		ASSERT(INVALID_CODE_PATH);
	}
	break;
	}

	return FALSE;
}
