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
		GLuint mProgram;

		Shader(std::string vertexPath, std::string fragmentPath);
		~Shader();

		void use();

	};
}

#endif
