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

	void initVertexArrayObject(GLuint vao);
	void render(Shader *shader, GLuint shaderVao);
private:
	glm::vec2 mPos;
	Texture *mTex;
	GLuint mVbo;
};
}

#endif
