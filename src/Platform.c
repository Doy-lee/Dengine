#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "Dengine/Platform.h"
#include "Dengine/Debug.h"

void platform_memoryFree(void *data, i32 numBytes)
{
	if (data) free(data);

#ifdef DENGINE_DEBUG
	GLOBAL_debugState.totalMemoryAllocated -= numBytes;
#endif
}

void *platform_memoryAlloc(i32 numBytes)
{
	void *result = calloc(1, numBytes);

#ifdef DENGINE_DEBUG
	if (result)
		GLOBAL_debugState.totalMemoryAllocated += numBytes;
#endif
	return result;
}

void platform_closeFileRead(PlatformFileRead *file)
{
	PLATFORM_MEM_FREE(file->buffer, file->size);
}

i32 platform_readFileToBuffer(const char *const filePath,
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
	file->buffer = PLATFORM_MEM_ALLOC(fileSize.LowPart, char);
	file->size = fileSize.LowPart;

	DWORD numBytesRead = 0;

	status =
	    ReadFile(fileHandle, file->buffer, file->size, &numBytesRead, NULL);
	if (!status)
	{
		printf("ReadFile() failed: %d error number\n",
		       status);
		PLATFORM_MEM_FREE(file->buffer, file->size);
		return status;
	}
	else if (numBytesRead != file->size)
	{
		printf(
		    "ReadFile() failed: Number of bytes read doesn't match file "
		    "size\n");
		PLATFORM_MEM_FREE(file->buffer, file->size);
		return -1;
	}

	return 0;
}
