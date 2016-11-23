#ifndef DENGINE_RENDERER_H
#define DENGINE_RENDERER_H

#include "Dengine/Common.h"
#include "Dengine/Math.h"
#include "Dengine/Assets.h"

/* Forward Declaration */
typedef struct AssetManager AssetManager;
typedef struct Font Font;
typedef struct Entity Entity;
typedef struct MemoryArena MemoryArena_;
typedef struct Shader Shader;
typedef struct Texture Texture;

typedef struct RenderVertex
{
	v2 pos;
	v2 texCoord;
} RenderVertex;

typedef struct RenderTex
{
	Texture *tex;
	v4 texRect;
} RenderTex;

typedef u32 RenderFlags;
enum RenderFlag {
	renderflag_wireframe = 0x1,
	renderflag_no_texture = 0x2,
};

// TODO(doyle): Since all vertexes are built with degenerate vertices and
// in a triangle strip format, we should not split render groups by render mode
// nor either generate buffers vao/vbo based on rendermode count
enum RenderMode
{
	rendermode_quad,
	rendermode_polygon,
	rendermode_count,
	rendermode_invalid,
};

typedef struct RenderGroup
{
	b32 init;
	RenderFlags flags;
	enum RenderMode mode;

	// NOTE(doyle): Only for when adding singular triangles in triangle strip
	// mode
	b32 clockwiseWinding;

	Texture *tex;
	v4 color;

	RenderVertex *vertexList;
	i32 vertexIndex;

} RenderGroup;

enum VertexBatchState {
	vertexbatchstate_off,
	vertexbatchstate_initial_add,
	vertexbatchstate_active,
};

typedef struct Renderer
{
	u32 shaderList[shaderlist_count];
	u32 activeShaderId;

	u32 vao[rendermode_count];
	u32 vbo[rendermode_count];

	enum VertexBatchState vertexBatchState;
	i32 groupIndexForVertexBatch;

	i32 numVertexesInVbo;
	v2 vertexNdcFactor;
	v2 size;

	RenderGroup groups[128];
	i32 groupCapacity;
} Renderer;


RenderTex renderer_createNullRenderTex(AssetManager *const assetManager);

// TODO(doyle): Clean up lines
// Renderer::~Renderer() { glDeleteVertexArrays(1, &this->quadVAO); }
void renderer_rect(Renderer *const renderer, Rect camera, v2 pos, v2 size,
                   v2 pivotPoint, Radians rotate, RenderTex *renderTex,
                   v4 color, RenderFlags flags);

void renderer_polygon(Renderer *const renderer, Rect camera, v2 *polygonPoints,
                      i32 numPoints, v2 pivotPoint, Radians rotate,
                      RenderTex *renderTex, v4 color, RenderFlags flags);

inline void renderer_staticRect(Renderer *const renderer, v2 pos, v2 size,
                                v2 pivotPoint, Radians rotate,
                                RenderTex *renderTex, v4 color,
                                RenderFlags flags)
{
	Rect staticCamera = {V2(0, 0), renderer->size};
	renderer_rect(renderer, staticCamera, pos, size, pivotPoint, rotate,
	              renderTex, color, flags);
}

void renderer_string(Renderer *const renderer, MemoryArena_ *arena, Rect camera,
                     Font *const font, const char *const string, v2 pos,
                     v2 pivotPoint, Radians rotate, v4 color,
                     RenderFlags flags);

inline void renderer_staticString(Renderer *const renderer, MemoryArena_ *arena,
                                  Font *const font, const char *const string,
                                  v2 pos, v2 pivotPoint, Radians rotate,
                                  v4 color, RenderFlags flags)
{
	Rect staticCamera = {V2(0, 0), renderer->size};
	renderer_string(renderer, arena, staticCamera, font, string, pos,
	                pivotPoint, rotate, color, flags);
}

void renderer_entity(Renderer *renderer, MemoryArena_ *transientArena,
                     Rect camera, Entity *entity, v2 pivotPoint, Degrees rotate,
                     v4 color, RenderFlags flags);

void renderer_renderGroups(Renderer *renderer);

#endif
