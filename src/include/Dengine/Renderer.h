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

typedef struct DebugRenderer
{
	b32 init;
	v2 stringPos;

} DebugRenderer;

typedef struct RenderQuad
{
	v4 vertex[4];
} RenderQuad;

extern DebugRenderer debugRenderer;

#if 0
void renderer_backgroundTiles(Renderer *const renderer, const v2 tileSize,
                              World *const world, TexAtlas *const atlasTexture,
                              Texture *const tex);
#endif

void renderer_string(Renderer *const renderer, Font *const font,
                     const char *const string, v2 pos, f32 rotate, v3 color);

void renderer_debugString(Renderer *const renderer, Font *const font,
                          const char *const string);

void renderer_entity(Renderer *renderer, v4 cameraBounds, Entity *entity,
                     f32 dt, f32 rotate, v3 color);

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
