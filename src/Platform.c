#include "Dengine/Platform.h"

i32 platform_readFileToBuffer(const char *const filePath,
                              PlatformFileReadResult *file)
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
	file->buffer = (void *)calloc(fileSize.LowPart, sizeof(char));
	file->size = fileSize.LowPart;

	DWORD numBytesRead = 0;

	status =
	    ReadFile(fileHandle, file->buffer, file->size, &numBytesRead, NULL);
	if (!status)
	{
		printf("ReadFile() failed: %d error number\n",
		       status);
		free(file->buffer);
		return status;
	}
	else if (numBytesRead != file->size)
	{
		printf(
		    "ReadFile() failed: Number of bytes read doesn't match file "
		    "size\n");
		free(file->buffer);
		return -1;
	}

	return 0;
}
