#ifndef DENGINE_SHADER_H
#define DENGINE_SHADER_H

#include <Dengine/Common.h>
#include <Dengine/OpenGL.h>

#include <glm/glm.hpp>

#include <string>

namespace Dengine
{
class Shader
{
public:
	GLuint id;

	Shader();
	~Shader();

	const i32 loadProgram(const GLuint vertexShader,
	                      const GLuint fragmentShader);

	void uniformSet1i(const GLchar *name, const GLuint data);
	void uniformSetMat4fv(const GLchar *name, const glm::mat4 data);

	void use() const;
};
}

#endif
