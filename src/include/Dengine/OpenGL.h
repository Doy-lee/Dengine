#ifndef DENGINE_OPENGL_H
#define DENGINE_OPENGL_H

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stdio.h>

#define GL_CHECK_ERROR() glCheckError_(__FILE__, __LINE__)
inline GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		switch (errorCode)
		{
		case GL_INVALID_ENUM:
			printf("OPENGL INVALID_ENUM | ");
			break;
		case GL_INVALID_VALUE:
			printf("OPENGL INVALID_VALUE | ");
			break;
		case GL_INVALID_OPERATION:
			printf("OPENGL INVALID_OPERATION | ");
			break;
		case GL_STACK_OVERFLOW:
			printf("OPENGL STACK_OVERFLOW | ");
			break;
		case GL_STACK_UNDERFLOW:
			printf("OPENGL STACK_UNDERFLOW | ");
			break;
		case GL_OUT_OF_MEMORY:
			printf("OPENGL OUT_OF_MEMORY | ");
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			printf("OPENGL INVALID_FRAMEBUFFER_OPERATION | ");
			break;
		}
		printf("%s (%d)\n", file, line);
	}
	return errorCode;
}

#endif
