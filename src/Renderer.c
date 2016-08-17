#include "Dengine/Renderer.h"
#include "Dengine/AssetManager.h"
#include "Dengine/Assets.h"
#include "Dengine/Debug.h"
#include "Dengine/Entity.h"
#include "Dengine/OpenGL.h"
#include "Dengine/Platform.h"
#include "Dengine/Shader.h"
#include "Dengine/Texture.h"

#define RENDER_BOUNDING_BOX FALSE

typedef struct RenderQuad
{
	// Vertex composition
	// x, y: Coordinates
	// z, w: Texture Coords
	v4 vertex[4];
} RenderQuad;

INTERNAL inline void flipTexCoord(v4 *texCoords, b32 flipX, b32 flipY)
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

INTERNAL void updateBufferObject(Renderer *const renderer,
                                 RenderQuad *const quads, const i32 numQuads)
{
	// TODO(doyle): We assume that vbo and vao are assigned
	const i32 numVertexesInQuad = 4;
	renderer->numVertexesInVbo = numQuads * numVertexesInQuad;

	glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
	glBufferData(GL_ARRAY_BUFFER, numQuads * sizeof(RenderQuad), quads,
	             GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

INTERNAL RenderQuad createTexQuad(Renderer *renderer, v4 quadRect,
                                  RenderTex renderTex)
{
	// NOTE(doyle): Draws a series of triangles using vertices v0, v1, v2, then
	// v2, v1, v3 (note the order)
	RenderQuad result = {0};

	/* Convert screen coordinates to normalised device coordinates */
	v4 quadRectNdc = quadRect;
	quadRectNdc.e[0] *= renderer->vertexNdcFactor.w;
	quadRectNdc.e[1] *= renderer->vertexNdcFactor.h;
	quadRectNdc.e[2] *= renderer->vertexNdcFactor.w;
	quadRectNdc.e[3] *= renderer->vertexNdcFactor.h;

	/* Convert texture coordinates to normalised texture coordinates */
	v4 texRectNdc = renderTex.texRect;
	if (renderTex.tex)
	{
		v2 texNdcFactor =
		    V2(1.0f / renderTex.tex->width, 1.0f / renderTex.tex->height);
		texRectNdc.e[0] *= texNdcFactor.w;
		texRectNdc.e[1] *= texNdcFactor.h;
		texRectNdc.e[2] *= texNdcFactor.w;
		texRectNdc.e[3] *= texNdcFactor.h;
	}

	/* Form the quad */
#if 0
	result.vertex[0] = V4(quadRectNdc.x, quadRectNdc.y, texRectNdc.x,
	                      texRectNdc.y); // Top left
	result.vertex[1] = V4(quadRectNdc.x, quadRectNdc.w, texRectNdc.x,
	                      texRectNdc.w); // Bottom left
	result.vertex[2] = V4(quadRectNdc.z, quadRectNdc.y, texRectNdc.z,
	                      texRectNdc.y); // Top right
	result.vertex[3] = V4(quadRectNdc.z, quadRectNdc.w, texRectNdc.z,
	                      texRectNdc.w); // Bottom right
#else
	result.vertex[0] = V4(quadRectNdc.x, quadRectNdc.w, texRectNdc.x,
	                      texRectNdc.w); // Top left
	result.vertex[1] = V4(quadRectNdc.x, quadRectNdc.y, texRectNdc.x,
	                      texRectNdc.y); // Bottom left
	result.vertex[2] = V4(quadRectNdc.z, quadRectNdc.w, texRectNdc.z,
	                      texRectNdc.w); // Top right
	result.vertex[3] = V4(quadRectNdc.z, quadRectNdc.y, texRectNdc.z,
	                      texRectNdc.y); // Bottom right
#endif
	return result;
}

INTERNAL inline RenderQuad
createDefaultTexQuad(Renderer *renderer, RenderTex renderTex)
{
	RenderQuad result = {0};
	// TODO(doyle): We need to switch this so its xy bottom left, zw top right!!
#if 0
	v4 defaultQuad    = V4(0.0f, renderer->size.h, renderer->size.w, 0.0f);
#else
	v4 defaultQuad = V4(0.0f, 0.0f, renderer->size.w, renderer->size.h);
#endif
	result            = createTexQuad(renderer, defaultQuad, renderTex);
	return result;
}

INTERNAL void renderObject(Renderer *renderer, v2 pos, v2 size, v2 pivotPoint,
                           f32 rotate, v4 color, Texture *tex)
{
	mat4 transMatrix  = mat4_translate(pos.x, pos.y, 0.0f);
	// NOTE(doyle): Rotate from pivot point of the object, (0, 0) is bottom left
	mat4 rotateMatrix = mat4_translate(pivotPoint.x, pivotPoint.y, 0.0f);
	rotateMatrix = mat4_mul(rotateMatrix, mat4_rotate(rotate, 0.0f, 0.0f, 1.0f));
	rotateMatrix = mat4_mul(rotateMatrix,
	                        mat4_translate(-pivotPoint.x, -pivotPoint.y, 0.0f));

	// NOTE(doyle): We draw everything as a unit square in OGL. Scale it to size
	mat4 scaleMatrix = mat4_scale(size.x, size.y, 1.0f);
	mat4 model = mat4_mul(transMatrix, mat4_mul(rotateMatrix, scaleMatrix));

	/* Load transformation matrix */
	shader_use(renderer->shader);
	shader_uniformSetMat4fv(renderer->shader, "model", model);
	GL_CHECK_ERROR();

	/* Set color modulation value */
	shader_uniformSetVec4f(renderer->shader, "spriteColor", color);

	/* Send draw calls */
#if RENDER_BOUNDING_BOX
	glBindVertexArray(renderer->vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, renderer->numVertexesInVbo);
	glBindVertexArray(0);
#endif

	if (tex)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex->id);
		shader_uniformSet1i(renderer->shader, "tex", 0);
	}

	glBindVertexArray(renderer->vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, renderer->numVertexesInVbo);

#ifdef DENGINE_DEBUG
	debug_callCountIncrement(debugcallcount_drawArrays);
#endif

	/* Unbind */
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	GL_CHECK_ERROR();
}

INTERNAL v2 mapWorldToCameraSpace(v2 worldPos, v4 cameraBounds)
{
	// Convert the world position to the camera coordinate system
	v2 cameraBottomLeftBound  = V2(cameraBounds.x, cameraBounds.w);
	v2 posInCameraSpace       = v2_sub(worldPos, cameraBottomLeftBound);
	return posInCameraSpace;
}

RenderTex renderer_createNullRenderTex(AssetManager *const assetManager)
{
	Texture *emptyTex = asset_getTexture(assetManager, texlist_empty);
	RenderTex result  = {emptyTex, V4(0, 1, 1, 0)};
	return result;
}

void renderer_rect(Renderer *const renderer, Rect camera, v2 pos, v2 size,
                   v2 pivotPoint, f32 rotate, RenderTex renderTex, v4 color)
{
	RenderQuad quad = createDefaultTexQuad(renderer, renderTex);
	updateBufferObject(renderer, &quad, 1);

	v2 posInCameraSpace = v2_sub(pos, camera.pos);
	renderObject(renderer, posInCameraSpace, size, pivotPoint, rotate,
	             color, renderTex.tex);
}

void renderer_string(Renderer *const renderer, MemoryArena *arena, Rect camera,
                     Font *const font, const char *const string, v2 pos,
                     v2 pivotPoint, f32 rotate, v4 color)
{
	i32 strLen = common_strlen(string);
	// TODO(doyle): Scale, not too important .. but rudimentary infrastructure
	// laid out here
	f32 scale = 1.0f;

	// TODO(doyle): Slightly incorrect string length in pixels calculation,
	// because we use the advance metric of each character for length not
	// maximum character size in rendering
	v2 rightAlignedP =
	    v2_add(pos, V2(scale *(CAST(f32) font->maxSize.w * CAST(f32) strLen),
	                   scale * CAST(f32) font->maxSize.h));
	v2 leftAlignedP  = pos;
	if (math_pointInRect(camera, leftAlignedP) ||
	    math_pointInRect(camera, rightAlignedP))
	{
		i32 quadIndex           = 0;
		RenderQuad *stringQuads = PLATFORM_MEM_ALLOC(arena, strLen, RenderQuad);

		v2 posInCameraSpace = v2_sub(pos, camera.pos);

		pos          = posInCameraSpace;
		f32 baseline = pos.y;
		for (i32 i = 0; i < strLen; i++)
		{
			// NOTE(doyle): Atlas packs fonts tightly, so offset the codepoint
			// to its actual atlas index, i.e. we skip the first 31 glyphs
			i32 codepoint     = string[i];
			i32 relativeIndex = CAST(i32)(codepoint - font->codepointRange.x);
			CharMetrics charMetric = font->charMetrics[relativeIndex];
			pos.y                  = baseline - (scale * charMetric.offset.y);

			const v4 charRectOnScreen =
			    math_getRect(pos, V2(scale * CAST(f32) font->maxSize.w,
			                         scale * CAST(f32) font->maxSize.h));

			pos.x += scale * charMetric.advance;

			/* Get texture out */
			v4 charTexRect = font->atlas->texRect[relativeIndex];
			flipTexCoord(&charTexRect, FALSE, TRUE);
			RenderTex renderTex = {font->tex, charTexRect};

			RenderQuad charQuad =
			    createTexQuad(renderer, charRectOnScreen, renderTex);
			stringQuads[quadIndex++] = charQuad;
		}

		// NOTE(doyle): We render at the renderer's size because we create quads
		// relative to the window size, hence we also render at the origin since
		// we're rendering a window sized buffer
		updateBufferObject(renderer, stringQuads, quadIndex);
		renderObject(renderer, V2(0.0f, 0.0f), renderer->size, pivotPoint,
		             rotate, color, font->tex);
		PLATFORM_MEM_FREE(arena, stringQuads, strLen * sizeof(RenderQuad));
	}
}

void renderer_entity(Renderer *renderer, Rect camera, Entity *entity,
                     v2 pivotPoint, f32 rotate, v4 color)
{
	// TODO(doyle): Batch into render groups

	// NOTE(doyle): Pos + Size since the origin of an entity is it's bottom left
	// corner. Add the two together so that the clipping point is the far right
	// side of the entity
	v2 rightAlignedP = v2_add(entity->pos, entity->hitboxSize);
	v2 leftAlignedP = entity->pos;
	if (math_pointInRect(camera, leftAlignedP) ||
	    math_pointInRect(camera, rightAlignedP))
	{
		EntityAnim_ *entityAnim = &entity->anim[entity->currAnimId];
		Animation *anim         = entityAnim->anim;
		i32 frameIndex          = anim->frameIndex[entityAnim->currFrame];
		v4 animTexRect          = anim->atlas->texRect[frameIndex];

		if (entity->direction == direction_east)
		{
			flipTexCoord(&animTexRect, TRUE, FALSE);
		}

		RenderTex renderTex = {entity->tex, animTexRect};
		RenderQuad entityQuad =
		    createDefaultTexQuad(renderer, renderTex);
		updateBufferObject(renderer, &entityQuad, 1);

		v2 posInCameraSpace = v2_sub(entity->pos, camera.pos);
		renderObject(renderer, posInCameraSpace, entity->renderSize,
		             pivotPoint, rotate, color, entity->tex);
	}
}
