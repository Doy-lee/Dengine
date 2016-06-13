#include <Dengine/Renderer.h>
#include <Dengine/OpenGL.h>

#include <glm/gtx/transform.hpp>
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
	glm::mat4 transMatrix  = glm::translate(glm::vec3(position, 0.0f));
	glm::mat4 rotateMatrix = glm::rotate(rotate, glm::vec3(0.0f, 0.0f, 1.0f));


	// NOTE(doyle): We draw everything as a unit square in OGL. Scale it to size
	glm::mat4 scaleMatrix  = glm::scale(glm::vec3(size, 1.0f));

	glm::mat4 model = transMatrix * rotateMatrix * scaleMatrix;

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
	// NOTE(doyle): Draws a series of triangles (three-sided polygons) using
	// vertices v0, v1, v2, then v2, v1, v3 (note the order)
	glm::vec4 vertices[] = {
	    //  x     y       s     t
	    {0.0f, 1.0f, 0.0f, 1.0f}, // Top left
	    {0.0f, 0.0f, 0.0f, 0.0f}, // Bottom left
	    {1.0f, 1.0f, 1.0f, 1.0f}, // Top right
	    {1.0f, 0.0f, 1.0f, 0.0f}, // Bottom right
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
