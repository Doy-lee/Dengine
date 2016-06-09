#include <Dengine\Shader.h>

#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>

namespace Dengine
{
Shader::Shader()
: id(0)
{
}

Shader::~Shader() {}

const i32 Shader::loadProgram(const GLuint vertexShader,
                              const GLuint fragmentShader)
{
	this->id = glCreateProgram();
	glAttachShader(this->id, vertexShader);
	glAttachShader(this->id, fragmentShader);
	glLinkProgram(this->id);

	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);

	GLint success;
	GLchar infoLog[512];
	glGetProgramiv(this->id, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(this->id, 512, NULL, infoLog);
		std::cout << "glLinkProgram failed: " << infoLog << std::endl;
		return -1;
	}

	return 0;

}

void Shader::uniformSet1i(const GLchar *name, const GLuint data)
{
	GLint uniformLoc = glGetUniformLocation(this->id, name);
	glUniform1i(uniformLoc, data);
}

void Shader::uniformSetMat4fv(const GLchar *name, const glm::mat4 data)
{
	GLint uniformLoc = glGetUniformLocation(this->id, name);
	glCheckError();
	glUniformMatrix4fv(uniformLoc, 1, GL_FALSE, glm::value_ptr(data));
	glCheckError();
}

void Shader::use() const { glUseProgram(this->id); }
}
