#ifndef DENGINE_RENDERER_H
#define DENGINE_RENDERER_H

#include "Dengine/Entity.h"
#include "Dengine/AssetManager.h"

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

// TODO(doyle): Clean up lines
// Renderer::~Renderer() { glDeleteVertexArrays(1, &this->quadVAO); }

void renderer_rect(Renderer *const renderer, v4 cameraBounds, v2 pos, v2 size,
                   f32 rotate, Texture *tex, v4 texRect, v4 color);

void renderer_string(Renderer *const renderer, v4 cameraBounds,
                     Font *const font, const char *const string, v2 pos,
                     f32 rotate, v4 color);

inline void renderer_staticString(Renderer *const renderer, Font *const font,
                                  const char *const string, v2 pos, f32 rotate,
                                  v4 color)
{
	renderer_string(renderer, V4(0, renderer->size.h, renderer->size.w, 0),
	                font, string, pos, rotate, color);
}

void renderer_entity(Renderer *renderer, v4 cameraBounds, Entity *entity,
                     f32 dt, f32 rotate, v4 color);

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
#endif
