#include <Dengine\Shader.h>

#include <fstream>
#include <iostream>

namespace Dengine
{
Shader::Shader()
: mProgram(0)
{
}

Shader::~Shader() {}

i32 Shader::loadProgram(GLuint vertexShader, GLuint fragmentShader)
{
	mProgram = glCreateProgram();
	glAttachShader(mProgram, vertexShader);
	glAttachShader(mProgram, fragmentShader);
	glLinkProgram(mProgram);

	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);

	GLint success;
	GLchar infoLog[512];
	glGetProgramiv(mProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(mProgram, 512, NULL, infoLog);
		std::cout << "glLinkProgram failed: " << infoLog << std::endl;
		return -1;
	}

	return 0;

}

void Shader::use() { glUseProgram(mProgram); }
}
