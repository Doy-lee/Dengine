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

INTERNAL void addToRenderGroup(Renderer *renderer, Texture *texture,
                               Vertex *vertexList, i32 numVertexes)
{
	/* Find vacant/matching render group */
	RenderGroup *targetGroup = NULL;
	for (i32 i = 0; i < ARRAY_COUNT(renderer->groups); i++)
	{
		RenderGroup *group = &renderer->groups[i];
		if (group->tex == NULL || group->tex->id == texture->id)
		{
			i32 freeVertexSlots = renderer->groupCapacity - group->vertexIndex;
			if (numVertexes < freeVertexSlots)
			{
				if (!group->tex)
				{
					// NOTE(doyle): Mark first vertex as degenerate vertex
					group->vertexIndex++;

					group->tex = texture;
				}

				targetGroup = &renderer->groups[i];
			}
			break;
		}
	}

	/* Valid group, add to the render group for rendering */
	if (targetGroup)
	{
		for (i32 i = 0; i < numVertexes; i++)
		{
			targetGroup->vertexList[targetGroup->vertexIndex++] = vertexList[i];
		}
	}
	else
	{
		// TODO(doyle): Log no remaining render groups
		DEBUG_LOG(
		    "WARNING: All render groups used up, some items will not be "
		    "rendered!");

		printf(
		    "WARNING: All render groups used up, some items will not be "
		    "rendered!\n");
	}
}

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
                                 const Vertex *const vertexList,
                                 const i32 numVertex)
{
	// TODO(doyle): We assume that vbo and vao are assigned
	renderer->numVertexesInVbo = numVertex;

	glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
	glBufferData(GL_ARRAY_BUFFER, numVertex * sizeof(Vertex), vertexList,
	             GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

INTERNAL void bufferRenderGroupToGL(Renderer *renderer, RenderGroup *group)
{
	updateBufferObject(renderer, group->vertexList, group->vertexIndex);
}

INTERNAL RenderQuad_ createTexQuad(Renderer *renderer, v4 quadRect,
                                   RenderTex renderTex)
{
	/*
	 * Rendering order
	 *
	 * 0   2
	 * +---+
	 * +  /+
	 * + / +
	 * +/  +
	 * +---+
	 * 1   3
	 *
	 */

	// NOTE(doyle): Draws a series of triangles using vertices v0, v1, v2, then
	// v2, v1, v3 (note the order)
	RenderQuad_ result = {0};

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
	result.vertex[0].e = V4(quadRectNdc.x, quadRectNdc.w, texRectNdc.x,
	                        texRectNdc.w); // Top left
	result.vertex[1].e = V4(quadRectNdc.x, quadRectNdc.y, texRectNdc.x,
	                        texRectNdc.y); // Bottom left
	result.vertex[2].e = V4(quadRectNdc.z, quadRectNdc.w, texRectNdc.z,
	                        texRectNdc.w); // Top right
	result.vertex[3].e = V4(quadRectNdc.z, quadRectNdc.y, texRectNdc.z,
	                        texRectNdc.y); // Bottom right
	return result;
}

INTERNAL inline RenderQuad_
createDefaultTexQuad(Renderer *renderer, RenderTex renderTex)
{
	RenderQuad_ result = {0};
	v4 defaultQuad    = V4(0.0f, 0.0f, renderer->size.w, renderer->size.h);
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
	Texture *emptyTex = asset_getTex(assetManager, "nullTex");
	RenderTex result  = {emptyTex, V4(0, 1, 1, 0)};
	return result;
}

void renderer_rect(Renderer *const renderer, Rect camera, v2 pos, v2 size,
                   v2 pivotPoint, f32 rotate, RenderTex renderTex, v4 color)
{
	// TODO(doyle): Use render groups
	v2 posInCameraSpace = v2_sub(pos, camera.pos);
	RenderQuad_ quad = createDefaultTexQuad(renderer, renderTex);
	updateBufferObject(renderer, quad.vertex, ARRAY_COUNT(quad.vertex));

	renderObject(renderer, posInCameraSpace, size, pivotPoint, rotate,
	             color, renderTex.tex);
}

void renderer_string(Renderer *const renderer, MemoryArena *arena, Rect camera,
                     Font *const font, const char *const string, v2 pos,
                     v2 pivotPoint, f32 rotate, v4 color)
{
	i32 strLen = common_strlen(string);

	// TODO(doyle): Slightly incorrect string length in pixels calculation,
	// because we use the advance metric of each character for length not
	// maximum character size in rendering
	v2 rightAlignedP =
	    v2_add(pos, V2((CAST(f32) font->maxSize.w * CAST(f32) strLen),
	                   CAST(f32) font->maxSize.h));
	v2 leftAlignedP  = pos;
	if (math_pointInRect(camera, leftAlignedP) ||
	    math_pointInRect(camera, rightAlignedP))
	{

#define DISABLE_TEXT_RENDER_GROUPS TRUE
#if RENDERER_USE_RENDER_GROUPS && !DISABLE_TEXT_RENDER_GROUPS
		// NOTE(doyle): 2 degenerate vertexes, at start and end of string since-
		// chars are rendered side by side. Reserve first vertex as degenerate.
		i32 vertexIndex              = 1;
		const i32 numVertexPerQuad   = 4;
		const i32 numVertexesToAlloc = (strLen * numVertexPerQuad) + 2;
#else
		i32 vertexIndex              = 0;
		const i32 numVertexPerQuad   = 4;
		const i32 numVertexesToAlloc = (strLen * numVertexPerQuad);
#endif
		Vertex *vertexList =
		    PLATFORM_MEM_ALLOC(arena, numVertexesToAlloc, Vertex);

		v2 posInCameraSpace = v2_sub(pos, camera.pos);

		pos = posInCameraSpace;

		// TODO(doyle): Find why font is 1px off, might be arial font semantics
		Texture *tex = font->atlas->tex;
		f32 baseline = pos.y - font->verticalSpacing + 1;
		for (i32 i = 0; i < strLen; i++)
		{
			i32 codepoint     = string[i];
			i32 relativeIndex = CAST(i32)(codepoint - font->codepointRange.x);
			CharMetrics charMetric = font->charMetrics[relativeIndex];
			pos.y                  = baseline - (charMetric.offset.y);

			const v4 charRectOnScreen =
			    math_getRect(pos, font->maxSize);
			pos.x += charMetric.advance;

			/* Get texture out */
			SubTexture charTexRect =
			    asset_getAtlasSubTex(font->atlas, &CAST(char)codepoint);

			v4 deprecatedTexRect = {0};
			deprecatedTexRect.vec2[0] = charTexRect.rect.pos;
			deprecatedTexRect.vec2[1] =
			    v2_add(charTexRect.rect.pos, charTexRect.rect.size);

			flipTexCoord(&deprecatedTexRect, FALSE, TRUE);

			RenderTex renderTex = {tex, deprecatedTexRect};
			RenderQuad_ charQuad =
			    createTexQuad(renderer, charRectOnScreen, renderTex);

			for (i32 i = 0; i < ARRAY_COUNT(charQuad.vertex); i++)
			{
				vertexList[vertexIndex++] = charQuad.vertex[i];
			}
		}

        // NOTE(doyle): We render at the renderer's size because we create quads
		// relative to the window size, hence we also render at the origin since
		// we're rendering a window sized buffer

// TODO(doyle): Render group differentiate between null tex and colors
#if RENDERER_USE_RENDER_GROUPS && !DISABLE_TEXT_RENDER_GROUPS
		// NOTE(doyle): Degenerate vertex at end of string
		vertexList[vertexIndex++] = vertexList[vertexIndex - 1];
		addToRenderGroup(renderer, tex, vertexList, numVertexesToAlloc);
#else
		updateBufferObject(renderer, vertexList, vertexIndex);
		renderObject(renderer, V2(0.0f, 0.0f), renderer->size, pivotPoint,
		             rotate, color, tex);
#endif

		PLATFORM_MEM_FREE(arena, vertexList,
		                  sizeof(Vertex) * numVertexesToAlloc);
	}
}

void renderer_entity(Renderer *renderer, Rect camera, Entity *entity,
                     v2 pivotPoint, f32 rotate, v4 color)
{
	// TODO(doyle): Batch into render groups

	// NOTE(doyle): Pos + Size since the origin of an entity is it's bottom left
	// corner. Add the two together so that the clipping point is the far right
	// side of the entity
	v2 rightAlignedP = v2_add(entity->pos, entity->hitbox);
	v2 leftAlignedP = entity->pos;
	if (math_pointInRect(camera, leftAlignedP) ||
	    math_pointInRect(camera, rightAlignedP))
	{
		EntityAnim *entityAnim = &entity->animList[entity->currAnimId];
		Animation *anim        = entityAnim->anim;
		char *frameName        = anim->frameList[entityAnim->currFrame];
		SubTexture animRect    = asset_getAtlasSubTex(anim->atlas, frameName);

		v4 animTexRect = {0};
		animTexRect.vec2[0] = animRect.rect.pos;
		animTexRect.vec2[1] = v2_add(animRect.rect.pos, animRect.rect.size);

		flipTexCoord(&animTexRect, entity->flipX, entity->flipY);

		if (entity->direction == direction_east)
		{
			flipTexCoord(&animTexRect, TRUE, FALSE);
		}

		RenderTex renderTex = {entity->tex, animTexRect};

		// TODO(doyle): getRect needs a better name
		v2 posInCameraSpace     = v2_sub(entity->pos, camera.pos);

#if RENDERER_USE_RENDER_GROUPS
		v4 entityVertexOnScreen = math_getRect(posInCameraSpace, entity->size);
		RenderQuad_ entityQuad =
		    createTexQuad(renderer, entityVertexOnScreen, renderTex);

		/*
		   NOTE(doyle): Entity rendering is always done in two pairs of
		   triangles, i.e. quad. To batch render quads as a triangle strip, we
		   need to create zero-area triangles which OGL will omit from
		   rendering. Render groups are initialised with 1 degenerate vertex and
		   then the first two vertexes sent to the render group are the same to
		   form 1 zero-area triangle strip.

		   A degenerate vertex has to be copied from the last vertex in the
		   rendering quad, to repeat this process as more entities are
		   renderered.

		   Alternative implementation is recognising if the rendered
		   entity is the first in its render group, then we don't need to init
		   a degenerate vertex, and only at the end of its vertex list. But on
		   subsequent renders, we need a degenerate vertex at the front to
		   create the zero-area triangle strip.

		   The first has been chosen for simplicity of code, at the cost of
		   2 degenerate vertexes at the start of each render group.
	   */
		Vertex degenerateVertexes[2] = {entityQuad.vertex[0],
		                                entityQuad.vertex[3]};

		Vertex vertexList[6] = {degenerateVertexes[0], entityQuad.vertex[0],
		                        entityQuad.vertex[1],  entityQuad.vertex[2],
		                        entityQuad.vertex[3],  degenerateVertexes[1]};

		addToRenderGroup(renderer, entity->tex, vertexList,
		                 ARRAY_COUNT(vertexList));
#else
		RenderQuad_ entityQuad = createDefaultTexQuad(renderer, renderTex);
		// TODO(doyle): getRect needs a better name
		updateBufferObject(renderer, entityQuad.vertex,
		                   ARRAY_COUNT(entityQuad.vertex));
		renderObject(renderer, posInCameraSpace,
		             entity->size, pivotPoint,
		             entity->rotation + rotate, color, entity->tex);
#endif
	}
}

void renderer_renderGroups(Renderer *renderer)
{
	for (i32 i = 0; i < ARRAY_COUNT(renderer->groups); i++)
	{
		RenderGroup *currGroup = &renderer->groups[i];
		if (currGroup->tex)
		{
			bufferRenderGroupToGL(renderer, currGroup);

			v2 pivotPoint = V2(0, 0);
			f32 rotate    = 0;
			v4 color      = V4(1, 1, 1, 1);
			renderObject(renderer, V2(0.0f, 0.0f), renderer->size, pivotPoint,
			             rotate, color, currGroup->tex);

			RenderGroup cleanGroup = {0};
			cleanGroup.vertexList = currGroup->vertexList;
			*currGroup = cleanGroup;
		}
	}
}
