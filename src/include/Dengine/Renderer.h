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
} Renderer;

typedef struct RenderQuad
{
	v4 vertex[4];
} RenderQuad;

void renderer_entity(Renderer *renderer, Entity *entity, f32 rotate,
                     v3 color);

void renderer_object(Renderer *renderer, v2 pos, v2 size, f32 rotate, v3 color,
                     Texture *tex);

INTERNAL inline RenderQuad renderer_createQuad(v4 quadRectNdc, v4 texRectNdc)
{
	// NOTE(doyle): Draws a series of triangles (three-sided polygons) using
	// vertices v0, v1, v2, then v2, v1, v3 (note the order)
	RenderQuad result = {0};
	result.vertex[0] = V4(quadRectNdc.x, quadRectNdc.y, texRectNdc.x, texRectNdc.y); // Top left
	result.vertex[1] = V4(quadRectNdc.x, quadRectNdc.w, texRectNdc.x, texRectNdc.w); // Bottom left
	result.vertex[2] = V4(quadRectNdc.z, quadRectNdc.y, texRectNdc.z, texRectNdc.y); // Top right
	result.vertex[3] = V4(quadRectNdc.z, quadRectNdc.w, texRectNdc.z, texRectNdc.w); // Bottom right
	return result;
}

INTERNAL inline RenderQuad renderer_createDefaultQuad(v4 texRectNdc)
{
	RenderQuad result = {0};
	v4 defaultVertices = V4(0.0f, 1.0f, 1.0f, 0.0f);
	result = renderer_createQuad(defaultVertices, texRectNdc);
	return result;
}

#endif
