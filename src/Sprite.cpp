#include <Dengine\Sprite.h>
#include <Dengine\OpenGL.h>

namespace Dengine
{

Sprite::Sprite()
: mPos(0, 0),
mTex(nullptr)
{
}

Sprite::~Sprite() {}

b32 Sprite::loadSprite(Texture *tex, glm::vec2 pos)
{
	if (!tex) return -1;
	
	mTex = tex;
	mPos = pos;

	// NOTE(doyle): We encode in a vec4 (vec2)pos, (vec2)texCoords
	glm::vec4 spriteVertex[] = {
		//   x     y       s     t
		{+0.5f, +0.5f, 1.0f, 1.0f}, // Top right
		{+0.5f, -0.5f, 1.0f, 0.0f}, // Bottom right
		{-0.5f, +0.5f, 0.0f, 1.0f}, // Top left
		{-0.5f, -0.5f, 0.0f, 0.0f}, // Bottom left
	};

	glGenBuffers(1, &mVbo);
	glBindBuffer(GL_ARRAY_BUFFER, mVbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(spriteVertex), spriteVertex,
	             GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return 0;
}

void Sprite::initVertexArrayObject(GLuint vao)
{
	// NOTE(doyle): Set the VAO object attributes to match a sprite's
    // vertex composition
	glBindVertexArray(vao);

	i32 numElementsInVertex = 4;
	i32 vertexSize = sizeof(glm::vec4);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, numElementsInVertex, GL_FLOAT, GL_FALSE,
	                      vertexSize, (GLvoid *)(0));

	/* Unbind */
	glBindVertexArray(0);

}

void Sprite::render(Shader *shader, GLuint spriteVAO)
{
	// NOTE(doyle): Associate the VAO with this sprites VBO
	glBindVertexArray(spriteVAO);
	glBindBuffer(GL_ARRAY_BUFFER, mVbo);

	/* Set texture */
	glActiveTexture(GL_TEXTURE0);
	mTex->bind();
	glUniform1i(glGetUniformLocation(shader->mProgram, "texture"), 0);

	/* Render */
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// Unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

}
