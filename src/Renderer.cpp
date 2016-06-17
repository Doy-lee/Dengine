#include <Dengine/OpenGL.h>
#include <Dengine/Renderer.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

void renderer_entity(Renderer *renderer, Entity *entity, GLfloat rotate,
                     glm::vec3 color)
{
	shader_use(renderer->shader);
	glm::mat4 transMatrix  = glm::translate(glm::vec3(entity->pos, 0.0f));
	glm::mat4 rotateMatrix = glm::rotate(rotate, glm::vec3(0.0f, 0.0f, 1.0f));
	glCheckError();

	// NOTE(doyle): We draw everything as a unit square in OGL. Scale it to size
	glm::mat4 scaleMatrix = glm::scale(glm::vec3(entity->size, 1.0f));

	glm::mat4 model = transMatrix * rotateMatrix * scaleMatrix;
	shader_uniformSetMat4fv(renderer->shader, "model", model);
	glCheckError();

	// TODO(doyle): Unimplemented
	// this->shader->uniformSetVec3f("spriteColor", color);

	glActiveTexture(GL_TEXTURE0);
	glCheckError();
	glBindTexture(GL_TEXTURE_2D, entity->tex->id);
	glCheckError();
	shader_uniformSet1i(renderer->shader, "tex", 0);
	glCheckError();

	glBindVertexArray(renderer->quadVAO);
	glCheckError();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glCheckError();
	glBindVertexArray(0);
	glCheckError();
}
