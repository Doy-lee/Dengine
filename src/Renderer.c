#include "Dengine/OpenGL.h"
#include "Dengine/Renderer.h"

#define RENDER_BOUNDING_BOX FALSE

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

#if RENDER_BOUNDING_BOX
	glBindVertexArray(renderer->vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, renderer->numVertexesInVbo);
	glBindVertexArray(0);
#endif

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

RenderQuad renderer_createQuad(Renderer *renderer, v4 quadRect, v4 texRect,
                               Texture *tex)
{
	// NOTE(doyle): Draws a series of triangles (three-sided polygons) using
	// vertices v0, v1, v2, then v2, v1, v3 (note the order)
	RenderQuad result = {0};

	v4 quadRectNdc = quadRect;
	quadRectNdc.e[0] *= renderer->vertexNdcFactor.w;
	quadRectNdc.e[1] *= renderer->vertexNdcFactor.h;
	quadRectNdc.e[2] *= renderer->vertexNdcFactor.w;
	quadRectNdc.e[3] *= renderer->vertexNdcFactor.h;

	v2 texNdcFactor = V2(1.0f / tex->width, 1.0f / tex->height);
	v4 texRectNdc = texRect;
	texRectNdc.e[0] *= texNdcFactor.w;
	texRectNdc.e[1] *= texNdcFactor.h;
	texRectNdc.e[2] *= texNdcFactor.w;
	texRectNdc.e[3] *= texNdcFactor.h;

	result.vertex[0] = V4(quadRectNdc.x, quadRectNdc.y, texRectNdc.x, texRectNdc.y); // Top left
	result.vertex[1] = V4(quadRectNdc.x, quadRectNdc.w, texRectNdc.x, texRectNdc.w); // Bottom left
	result.vertex[2] = V4(quadRectNdc.z, quadRectNdc.y, texRectNdc.z, texRectNdc.y); // Top right
	result.vertex[3] = V4(quadRectNdc.z, quadRectNdc.w, texRectNdc.z, texRectNdc.w); // Bottom right
	return result;
}
