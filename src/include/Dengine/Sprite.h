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

	const b32 loadSprite(const Texture *tex, const glm::vec2 pos);

	void render(const Shader *shader) const;
private:
	glm::vec2 pos;
	const Texture *tex;
	GLuint vbo;
	GLuint vao;
};
}

#endif
