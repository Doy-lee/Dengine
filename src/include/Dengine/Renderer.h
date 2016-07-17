#ifndef DENGINE_RENDERER_H
#define DENGINE_RENDERER_H

#include "Dengine/AssetManager.h"
#include "Dengine/Common.h"
#include "Dengine/Entity.h"
#include "Dengine/Math.h"
#include "Dengine/Shader.h"

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
	v4 texRect;
} RenderTex;

// TODO(doyle): Clean up lines
// Renderer::~Renderer() { glDeleteVertexArrays(1, &this->quadVAO); }
void renderer_rect(Renderer *const renderer, v4 cameraBounds, v2 pos, v2 size,
                   f32 rotate, RenderTex renderTex, v4 color);

inline void renderer_staticRect(Renderer *const renderer, v2 pos, v2 size,
                                f32 rotate, RenderTex renderTex, v4 color)
{
	v4 staticCameraBounds = math_getRect(V2(0, 0), renderer->size);
	renderer_rect(renderer, staticCameraBounds, pos, size, rotate, renderTex,
	              color);
}


void renderer_string(Renderer *const renderer, v4 cameraBounds,
                     Font *const font, const char *const string, v2 pos,
                     f32 rotate, v4 color);

inline void renderer_staticString(Renderer *const renderer, Font *const font,
                                  const char *const string, v2 pos, f32 rotate,
                                  v4 color)
{
	v4 staticCameraBounds = math_getRect(V2(0, 0), renderer->size);
	renderer_string(renderer, staticCameraBounds, font, string, pos, rotate,
	                color);
}

void renderer_entity(Renderer *renderer, v4 cameraBounds, Entity *entity,
                     v2 entityRenderSize, f32 rotate, v4 color);

#endif
