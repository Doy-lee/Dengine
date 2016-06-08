#include <Dengine\Sprite.h>
#include <Dengine\OpenGL.h>

namespace Dengine
{

Sprite::Sprite()
: pos(0, 0),
tex(nullptr)
{
}

Sprite::~Sprite() {}

const b32 Sprite::loadSprite(const Texture *tex, const glm::vec2 pos)
{
	if (!tex) return -1;
	
	this->tex = tex;
	this->pos = pos;

	// NOTE(doyle): We encode in a vec4 (vec2)pos, (vec2)texCoords
	glm::vec4 spriteVertex[] = {
		//   x     y       s     t
		{+0.5f, +0.5f, 1.0f, 1.0f}, // Top right
		{+0.5f, -0.5f, 1.0f, 0.0f}, // Bottom right
		{-0.5f, +0.5f, 0.0f, 1.0f}, // Top left
		{-0.5f, -0.5f, 0.0f, 0.0f}, // Bottom left
	};

	/* Create buffers */
	glGenBuffers(1, &this->vbo);
	glGenVertexArrays(1, &this->vao);

	/* Bind buffers */
	glBindVertexArray(this->vao);
	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);

	/* Configure VBO */
	glBufferData(GL_ARRAY_BUFFER, sizeof(spriteVertex), spriteVertex,
	             GL_STATIC_DRAW);

	/* Configure VAO */
	i32 numElementsInVertex = 4;
	i32 vertexSize = sizeof(glm::vec4);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, numElementsInVertex, GL_FLOAT, GL_FALSE,
	                      vertexSize, (GLvoid *)(0));

	/* Unbind */
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return 0;
}

void Sprite::render(const Shader *shader) const
{
	// NOTE(doyle): Associate the VAO with this sprites VBO
	glBindVertexArray(this->vao);

	/* Set texture */
	glActiveTexture(GL_TEXTURE0);
	this->tex->bind();
	glUniform1i(glGetUniformLocation(shader->program, "tex"), 0);

	/* Render */
	i32 numVerticesToDraw = 4;
	glDrawArrays(GL_TRIANGLE_STRIP, 0, numVerticesToDraw);

	// Unbind
	glBindVertexArray(0);
}

}
