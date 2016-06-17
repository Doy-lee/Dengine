#include <Dengine/Shader.h>

#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>

const i32 shader_loadProgram(Shader *const shader, const GLuint vertexShader,
                             const GLuint fragmentShader)
{
	shader->id = glCreateProgram();
	glAttachShader(shader->id, vertexShader);
	glAttachShader(shader->id, fragmentShader);
	glLinkProgram(shader->id);

	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);

	GLint success;
	GLchar infoLog[512];
	glGetProgramiv(shader->id, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shader->id, 512, NULL, infoLog);
		printf("glLinkProgram failed: %s\n", infoLog);
		return -1;
	}

	return 0;
}

void shader_uniformSet1i(Shader *const shader, const GLchar *name,
                         const GLuint data)
{
	GLint uniformLoc = glGetUniformLocation(shader->id, name);
	glUniform1i(uniformLoc, data);
}

void shader_uniformSetMat4fv(Shader *const shader, const GLchar *name,
                             const glm::mat4 data)
{
	GLint uniformLoc = glGetUniformLocation(shader->id, name);
	glUniformMatrix4fv(uniformLoc, 1, GL_FALSE, glm::value_ptr(data));
}

void shader_use(const Shader *const shader) { glUseProgram(shader->id); }
