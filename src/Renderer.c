#include <Dengine/OpenGL.h>
#include <Dengine/Renderer.h>

void renderer_entity(Renderer *renderer, Entity *entity, f32 rotate, v3 color)
{
	renderer_object(renderer, entity->pos, entity->size, rotate, color,
	                entity->tex);
}

void renderer_object(Renderer *renderer, v2 pos, v2 size, f32 rotate, v3 color,
                     Texture *tex)
{
	shader_use(renderer->shader);
	mat4 transMatrix  = mat4_translate(pos.x, pos.y, 0.0f);
	mat4 rotateMatrix = mat4_rotate(rotate, 0.0f, 0.0f, 1.0f);

	// NOTE(doyle): We draw everything as a unit square in OGL. Scale it to size
	// TODO(doyle): We should have a notion of hitbox size and texture size
	// we're going to render so we can draw textures that may be bigger than the
	// entity, (slightly) but keep a consistent bounding box
	mat4 scaleMatrix = mat4_scale(size.x, size.y, 1.0f);

	mat4 model = mat4_mul(transMatrix, mat4_mul(rotateMatrix, scaleMatrix));
	//mat4 model = mat4_mul(transMatrix, rotateMatrix);
	shader_uniformSetMat4fv(renderer->shader, "model", model);
	glCheckError();

	// TODO(doyle): Unimplemented
	// this->shader->uniformSetVec3f("spriteColor", color);

	glActiveTexture(GL_TEXTURE0);

	if (tex)
	{
		glBindTexture(GL_TEXTURE_2D, tex->id);
		shader_uniformSet1i(renderer->shader, "tex", 0);
	}

	glBindVertexArray(renderer->vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, renderer->numVertexesInVbo);
	glBindVertexArray(0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glCheckError();
}
