#ifndef DENGINE_RENDERER_H
#define DENGINE_RENDERER_H

#include <Dengine/Common.h>
#include <Dengine/Texture.h>
#include <Dengine/Shader.h>
#include <GLM/glm.hpp>

namespace Dengine
{
class Renderer
{
public:
	Renderer(Shader *shader);
	~Renderer();

	void drawSprite(const Texture *texture, glm::vec2 position,
	                glm::vec2 size = glm::vec2(10, 10), GLfloat rotate = 0.0f,
	                glm::vec3 color = glm::vec3(1.0f));

private:
	Shader *shader;
	GLuint quadVAO;

	void initRenderData();
};
}

#endif
