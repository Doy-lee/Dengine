#include <Dengine/Renderer.h>
#include <Dengine/OpenGL.h>

#include <glm/gtc/matrix_transform.hpp>

namespace Dengine
{
Renderer::Renderer(Shader *shader)
{
	this->shader = shader;
	this->initRenderData();
}

Renderer::~Renderer()
{
	glDeleteVertexArrays(1, &this->quadVAO);
}
void Renderer::drawSprite(const Texture *texture, glm::vec2 position,
                          glm::vec2 size, GLfloat rotate, glm::vec3 color)
{
	this->shader->use();
	glm::mat4 model;
	// First translate (transformations are: scale happens first, then rotation
	// and then finall translation happens; reversed order)
	model = glm::translate(model, glm::vec3(position, 0.0f));

	// Move origin of rotation to center of quad
	model = glm::translate(model, glm::vec3(0.5f * size.x, 0.5f * size.y, 0.0f));

	// Then rotate
	model = glm::rotate(model, rotate, glm::vec3(0.0f, 0.0f, 1.0f));

	// Move origin back
	model = glm::translate(model, glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.0f));

	model = glm::scale(model, glm::vec3(size, 1.0f)); // Last scale

	this->shader->uniformSetMat4fv("model", model);

	// TODO(doyle): Unimplemented
	// this->shader->uniformSetVec3f("spriteColor", color);

	glActiveTexture(GL_TEXTURE0);
	texture->bind();
	this->shader->uniformSet1i("tex", 0);

	glBindVertexArray(this->quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void Renderer::initRenderData()
{
	glm::vec4 vertices[] = {
	    //   x     y       s     t
	    {+1.0f, +1.0f, 1.0f, 1.0f}, // Top right
	    {+1.0f, -1.0f, 1.0f, 0.0f}, // Bottom right
	    {-1.0f, +1.0f, 0.0f, 1.0f}, // Top left
	    {-1.0f, -1.0f, 0.0f, 0.0f}, // Bottom left
	};

	GLuint VBO;

	/* Create buffers */
	glGenVertexArrays(1, &this->quadVAO);
	glGenBuffers(1, &VBO);

	/* Bind buffers */
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBindVertexArray(this->quadVAO);

	/* Configure VBO */
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	/* Configure VAO */
	const GLuint numVertexElements = 4;
	const GLuint vertexSize = sizeof(glm::vec4);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, numVertexElements, GL_FLOAT, GL_FALSE, vertexSize,
	                      (GLvoid *)0);

	/* Unbind */
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
}
