#ifndef DENGINE_RENDERER_H
#define DENGINE_RENDERER_H

#include "Dengine/Common.h"
#include "Dengine/Math.h"
#include "Dengine/AssetManager.h"

/* Forward Declaration */
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
	i32 zDepth;

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
	i32 groupsInUse;
	i32 groupCapacity;
} Renderer;

void renderer_updateSize(Renderer *renderer, AssetManager *assetManager, v2 windowSize);
void renderer_init(Renderer *renderer, AssetManager *assetManager,
                   MemoryArena_ *persistentArena, v2 windowSize);

RenderTex renderer_createNullRenderTex(AssetManager *const assetManager);

// TODO(doyle): Rectangles with gradient alphas/gradient colours
void renderer_rect(Renderer *const renderer, Rect camera, v2 pos, v2 size,
                   v2 pivotPoint, Radians rotate, RenderTex *renderTex,
                   v4 color, i32 zDepth, RenderFlags flags);

inline void renderer_rectOutline(Renderer *const renderer, Rect camera, v2 pos,
                                 v2 size, f32 thickness, v2 pivotPoint,
                                 Radians rotate, RenderTex *renderTex, v4 color,
                                 i32 zDepth, RenderFlags flags)
{
	// TODO(doyle): Pivot point is probably not correct!
	// TODO(doyle): Rotation doesn't work!
	ASSERT(rotate == 0);

	/* Bottom line */
	renderer_rect(renderer, camera, pos, V2(size.w, thickness), pivotPoint,
	              rotate, renderTex, color, zDepth, flags);

	/* Top line */
	v2 topP = v2_add(pos, V2(0, size.h - thickness));
	renderer_rect(renderer, camera, topP, V2(size.w, thickness), pivotPoint,
	              rotate, renderTex, color, zDepth, flags);

	/* Left line */
	renderer_rect(renderer, camera, pos, V2(thickness, size.h), pivotPoint,
	              rotate, renderTex, color, zDepth, flags);

	/* Right line */
	v2 rightP = v2_add(pos, V2(size.w - thickness, 0));
	renderer_rect(renderer, camera, rightP, V2(thickness, size.h), pivotPoint,
	              rotate, renderTex, color, zDepth, flags);
}

inline void renderer_rectFixed(Renderer *const renderer, v2 pos, v2 size,
                               v2 pivotPoint, Radians rotate,
                               RenderTex *renderTex, v4 color, i32 zDepth,
                               RenderFlags flags)
{
	Rect staticCamera = {V2(0, 0), renderer->size};
	renderer_rect(renderer, staticCamera, pos, size, pivotPoint, rotate,
	              renderTex, color, zDepth, flags);
}

inline void renderer_rectFixedOutline(Renderer *const renderer, v2 pos, v2 size,
                                      v2 pivotPoint, f32 thickness,
                                      Radians rotate, RenderTex *renderTex,
                                      v4 color, i32 zDepth, RenderFlags flags)
{
	Rect staticCamera = {V2(0, 0), renderer->size};
	renderer_rectOutline(renderer, staticCamera, pos, size, thickness,
	                     pivotPoint, rotate, renderTex, color, zDepth, flags);
}


void renderer_polygon(Renderer *const renderer, Rect camera, v2 *polygonPoints,
                      i32 numPoints, v2 pivotPoint, Radians rotate,
                      RenderTex *renderTex, v4 color, i32 zDepth,
                      RenderFlags flags);


void renderer_string(Renderer *const renderer, MemoryArena_ *arena, Rect camera,
                     Font *const font, const char *const string, v2 pos,
                     v2 pivotPoint, Radians rotate, v4 color, i32 zDepth,
                     RenderFlags flags);

inline void renderer_stringFixed(Renderer *const renderer, MemoryArena_ *arena,
                                 Font *const font, const char *const string,
                                 v2 pos, v2 pivotPoint, Radians rotate,
                                 v4 color, i32 zDepth, RenderFlags flags)
{
	Rect staticCamera = {V2(0, 0), renderer->size};
	renderer_string(renderer, arena, staticCamera, font, string, pos,
	                pivotPoint, rotate, color, zDepth, flags);
}

inline void renderer_stringFixedCentered(Renderer *const renderer,
                                         MemoryArena_ *arena, Font *const font,
                                         const char *const string, v2 pos,
                                         v2 pivotPoint, Radians rotate,
                                         v4 color, i32 zDepth,
                                         enum RenderFlags flags)
{
	Rect staticCamera = {V2(0, 0), renderer->size};

	v2 dim     = asset_fontStringDimInPixels(font, string);
	v2 halfDim = v2_scale(dim, 0.5f);
	pos        = v2_sub(pos, halfDim);

	renderer_string(renderer, arena, staticCamera, font, string, pos,
	                pivotPoint, rotate, color, zDepth, flags);
}

void renderer_entity(Renderer *renderer, MemoryArena_ *transientArena,
                     Rect camera, Entity *entity, v2 pivotPoint, Degrees rotate,
                     v4 color, i32 zDepth, RenderFlags flags);

void renderer_renderGroups(Renderer *renderer);

#endif
