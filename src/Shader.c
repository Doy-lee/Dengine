#include "Dengine/Shader.h"

void shader_uniformSet1i(Shader *const shader, const GLchar *name,
                         const GLuint data)
{
	GLint uniformLoc = glGetUniformLocation(shader->id, name);
	glUniform1i(uniformLoc, data);
}

void shader_uniformSetMat4fv(Shader *const shader, const GLchar *name,
                             mat4 data)
{
	GLint uniformLoc = glGetUniformLocation(shader->id, name);
	glUniformMatrix4fv(uniformLoc, 1, GL_FALSE, data.e[0]);
}

void shader_uniformSetVec4f(Shader *const shader, const GLchar *name,
                            v4 data)
{
	GLint uniformLoc = glGetUniformLocation(shader->id, name);
	glUniform4f(uniformLoc, data.e[0], data.e[1], data.e[2], data.e[3]);
}


void shader_use(const Shader *const shader) { glUseProgram(shader->id); }
