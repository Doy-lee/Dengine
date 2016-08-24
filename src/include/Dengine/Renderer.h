#ifndef DENGINE_RENDERER_H
#define DENGINE_RENDERER_H

#include "Dengine/Common.h"
#include "Dengine/Math.h"

/* Forward Declaration */
typedef struct AssetManager AssetManager;
typedef struct Entity Entity;
typedef struct Font Font;
typedef struct MemoryArena MemoryArena;
typedef struct Shader Shader;
typedef struct Texture Texture;

typedef struct Renderer
{
	Shader *shader;
	u32 vao;
	u32 vbo;
	i32 numVertexesInVbo;
	v2 vertexNdcFactor;
	v2 size;
} Renderer;

typedef struct RenderTex
{
	Texture *tex;

	// TODO(doyle): Switch to rect
	v4 texRect;
} RenderTex;

// TODO(doyle): Use z-index occluding for rendering
RenderTex renderer_createNullRenderTex(AssetManager *const assetManager);

// TODO(doyle): Clean up lines
// Renderer::~Renderer() { glDeleteVertexArrays(1, &this->quadVAO); }
void renderer_rect(Renderer *const renderer, Rect camera, v2 pos, v2 size,
                   v2 pivotPoint, f32 rotate, RenderTex renderTex, v4 color);

inline void renderer_staticRect(Renderer *const renderer, v2 pos, v2 size,
                                v2 pivotPoint, f32 rotate, RenderTex renderTex,
                                v4 color)
{
	Rect staticCamera = {V2(0, 0), renderer->size};
	renderer_rect(renderer, staticCamera, pos, size, pivotPoint, rotate,
	              renderTex, color);
}

void renderer_string(Renderer *const renderer, MemoryArena *arena,
                     Rect camera, Font *const font,
                     const char *const string, v2 pos, v2 pivotPoint,
                     f32 rotate, v4 color);

inline void renderer_staticString(Renderer *const renderer, MemoryArena *arena,
                                  Font *const font, const char *const string,
                                  v2 pos, v2 pivotPoint, f32 rotate, v4 color)
{
	Rect staticCamera = {V2(0, 0), renderer->size};
	renderer_string(renderer, arena, staticCamera, font, string, pos,
	                pivotPoint, rotate, color);
}

void renderer_entity(Renderer *renderer, Rect camera, Entity *entity,
                     v2 pivotPoint, f32 rotate, v4 color);

#endif
