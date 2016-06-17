#ifndef DENGINE_OPENGL_H
#define DENGINE_OPENGL_H

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdio.h>

inline GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		switch (errorCode)
		{
		case GL_INVALID_ENUM:
			printf("INVALID_ENUM | ");
			break;
		case GL_INVALID_VALUE:
			printf("INVALID_VALUE | ");
			break;
		case GL_INVALID_OPERATION:
			printf("INVALID_OPERATION | ");
			break;
		case GL_STACK_OVERFLOW:
			printf("STACK_OVERFLOW | ");
			break;
		case GL_STACK_UNDERFLOW:
			printf("STACK_UNDERFLOW | ");
			break;
		case GL_OUT_OF_MEMORY:
			printf("OUT_OF_MEMORY | ");
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			printf("INVALID_FRAMEBUFFER_OPERATION | ");
			break;
		}
		printf("%s (%d)\n", file, line);
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

#endif
