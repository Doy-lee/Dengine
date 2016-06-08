#ifndef DENGINE_SHADER_H
#define DENGINE_SHADER_H

#include <Dengine/Common.h>
#include <Dengine/OpenGL.h>
#include <string>

namespace Dengine
{
class Shader
{
public:
	GLuint program;

	Shader();
	~Shader();

	i32 loadProgram(GLuint vertexShader, GLuint fragmentShader);

	void use() const;
};
}

#endif
