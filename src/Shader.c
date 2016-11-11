#include "Dengine/Shader.h"

void shader_uniformSet1i(u32 shaderId, const GLchar *name,
                         const GLuint data)
{
	GLint uniformLoc = glGetUniformLocation(shaderId, name);
	glUniform1i(uniformLoc, data);
}

void shader_uniformSetMat4fv(u32 shaderId, const GLchar *name,
                             mat4 data)
{
	GLint uniformLoc = glGetUniformLocation(shaderId, name);
	GL_CHECK_ERROR();
	glUniformMatrix4fv(uniformLoc, 1, GL_FALSE, data.e[0]);
	GL_CHECK_ERROR();
}

void shader_uniformSetVec4f(u32 shaderId, const GLchar *name,
                            v4 data)
{
	GLint uniformLoc = glGetUniformLocation(shaderId, name);
	glUniform4f(uniformLoc, data.e[0], data.e[1], data.e[2], data.e[3]);
}


void shader_use(u32 shaderId) { glUseProgram(shaderId); }
