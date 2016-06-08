#include <Dengine\Shader.h>

#include <fstream>
#include <iostream>

namespace Dengine
{
Shader::Shader()
: program(0)
{
}

Shader::~Shader() {}

i32 Shader::loadProgram(GLuint vertexShader, GLuint fragmentShader)
{
	this->program = glCreateProgram();
	glAttachShader(this->program, vertexShader);
	glAttachShader(this->program, fragmentShader);
	glLinkProgram(this->program);

	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);

	GLint success;
	GLchar infoLog[512];
	glGetProgramiv(this->program, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(this->program, 512, NULL, infoLog);
		std::cout << "glLinkProgram failed: " << infoLog << std::endl;
		return -1;
	}

	return 0;

}

void Shader::use() const { glUseProgram(this->program); }
}
