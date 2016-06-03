#include <Dengine\Shader.h>

#include <fstream>
#include <iostream>

namespace Dengine
{
INTERNAL std::string stringFromFile(const std::string &filename)
{
	std::ifstream file;
	file.open(filename.c_str(), std::ios::in | std::ios::binary);

	std::string output;
	std::string line;

	if (!file.is_open())
	{
		std::runtime_error(std::string("Failed to open file: ") + filename);
	}
	else
	{
		while (file.good())
		{
			std::getline(file, line);
			output.append(line + "\n");
		}
	}
	file.close();
	return output;
}

INTERNAL GLuint createShaderFromPath(std::string path, GLuint shadertype)
{
	std::string shaderSource = stringFromFile(path);
	const GLchar *source = shaderSource.c_str();

	GLuint result = glCreateShader(shadertype);
	glShaderSource(result, 1, &source, NULL);
	glCompileShader(result);

	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(result, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(result, 512, NULL, infoLog);
		std::cout << "glCompileShader failed: " << infoLog << std::endl;
	}

	return result;
}

Shader::Shader(std::string vertexPath, std::string fragmentPath)
{

	GLuint vertexShader   = createShaderFromPath(vertexPath, GL_VERTEX_SHADER);
	GLuint fragmentShader =
	    createShaderFromPath(fragmentPath, GL_FRAGMENT_SHADER);

	mProgram = glCreateProgram();
	glAttachShader(mProgram, vertexShader);
	glAttachShader(mProgram, fragmentShader);
	glLinkProgram(mProgram);

	GLint success;
	GLchar infoLog[512];
	glGetProgramiv(mProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(mProgram, 512, NULL, infoLog);
		std::cout << "glLinkProgram failed: " << infoLog << std::endl;
	}

	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);
}

Shader::~Shader() {}

void Shader::use() { glUseProgram(mProgram); }
}
