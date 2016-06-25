#include <Dengine/OpenGL.h>
#include <Dengine/Renderer.h>

void renderer_entity(Renderer *renderer, Entity *entity, f32 rotate, v3 color)
{
	shader_use(renderer->shader);
	mat4 transMatrix  = mat4_translate(entity->pos.x, entity->pos.y, 0.0f);
	mat4 rotateMatrix = mat4_rotate(rotate, 0.0f, 0.0f, 1.0f);

	// NOTE(doyle): We draw everything as a unit square in OGL. Scale it to size
	// TODO(doyle): We should have a notion of hitbox size and texture size
	// we're going to render so we can draw textures that may be bigger than the
	// entity, (slightly) but keep a consistent bounding box
	mat4 scaleMatrix = mat4_scale(entity->size.x, entity->size.y, 1.0f);

	mat4 model = mat4_mul(transMatrix, mat4_mul(rotateMatrix, scaleMatrix));
	shader_uniformSetMat4fv(renderer->shader, "model", model);
	glCheckError();

	// TODO(doyle): Unimplemented
	// this->shader->uniformSetVec3f("spriteColor", color);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, entity->tex->id);
	shader_uniformSet1i(renderer->shader, "tex", 0);
	glCheckError();

	glBindVertexArray(renderer->vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glCheckError();
}
