#ifndef DENGINE_RENDERER_H
#define DENGINE_RENDERER_H

#include "Dengine/Math.h"
#include "Dengine/Entity.h"
#include "Dengine/Shader.h"

typedef struct Renderer
{
	Shader *shader;
	GLuint vao;
	GLuint vbo;
	i32 numVertexesInVbo;
	v2 vertexNdcFactor;
	v2 size;

} Renderer;

typedef struct RenderQuad
{
	v4 vertex[4];
} RenderQuad;

void renderer_entity(Renderer *renderer, Entity *entity, f32 rotate,
                     v3 color);

void renderer_object(Renderer *renderer, v2 pos, v2 size, f32 rotate, v3 color,
                     Texture *tex);

RenderQuad renderer_createQuad(Renderer *renderer, v4 quadRect, v4 texRect,
                                    Texture *tex);

INTERNAL inline void renderer_flipTexCoord(v4 *texCoords, b32 flipX, b32 flipY)
{
	if (flipX)
	{
		v4 tmp       = *texCoords;
		texCoords->x = tmp.z;
		texCoords->z = tmp.x;
	}

	if (flipY)
	{
		v4 tmp       = *texCoords;
		texCoords->y = tmp.w;
		texCoords->w = tmp.y;
	}
}

INTERNAL inline RenderQuad renderer_createDefaultQuad(Renderer *renderer,
                                                      v4 texRect, Texture *tex)
{
	RenderQuad result = {0};
	v4 defaultQuad = V4(0.0f, renderer->size.h, renderer->size.w, 0.0f);
	result = renderer_createQuad(renderer, defaultQuad, texRect, tex);
	return result;
}

#endif
