#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "Dengine/Platform.h"
#include "Dengine/Debug.h"
#include "Dengine/MemoryArena.h"

void platform_memoryFree(MemoryArena_ *arena, void *data, i32 numBytes)
{
	if (data) free(data);

#ifdef DENGINE_DEBUG
	debug_countIncrement(debugcount_platformMemFree);
	arena->used -= numBytes;
#endif
}

void *platform_memoryAlloc(MemoryArena_ *arena, i32 numBytes)
{
	void *result = calloc(1, numBytes);

#ifdef DENGINE_DEBUG
	debug_countIncrement(debugcount_platformMemAlloc);
	if (result && arena)
		arena->used += numBytes;
#endif
	return result;
}

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
