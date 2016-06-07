#ifndef DENGINE_SPRITE_H
#define DENGINE_SPRITE_H

#include <Dengine\Common.h>
#include <Dengine\Texture.h>
#include <Dengine\Shader.h>
#include <GLM\glm.hpp>

namespace Dengine {
class Sprite
{
public:
	Sprite();
	~Sprite();

	b32 loadSprite(Texture *tex, glm::vec2 pos);

	void render(Shader *shader);
private:
	glm::vec2 pos;
	Texture *tex;
	GLuint vbo;
	GLuint vao;
};
}

#endif
