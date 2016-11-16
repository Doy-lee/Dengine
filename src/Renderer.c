#include "Dengine/Renderer.h"
#include "Dengine/AssetManager.h"
#include "Dengine/Assets.h"
#include "Dengine/Debug.h"
#include "Dengine/Entity.h"
#include "Dengine/MemoryArena.h"
#include "Dengine/OpenGL.h"
#include "Dengine/Shader.h"
#include "Dengine/Texture.h"

typedef struct RenderQuad
{
	Vertex vertex[4];
} RenderQuad_;

typedef struct RenderTriangle
{
	Vertex vertex[3];
} RenderTriangle_;

INTERNAL void addVertexToRenderGroup(Renderer *renderer, Texture *tex, v4 color,
                                     Vertex *vertexList, i32 numVertexes,
                                     enum RenderMode targetRenderMode,
                                     RenderFlags flags)
{

#ifdef DENGINE_DEBUG
	ASSERT(numVertexes > 0);

	for (i32 i = 0; i < numVertexes; i++)
		debug_countIncrement(debugcount_numVertex);
#endif

	/* Find vacant/matching render group */
	RenderGroup *targetGroup = NULL;
	for (i32 i = 0; i < ARRAY_COUNT(renderer->groups); i++)
	{
		RenderGroup *group = &renderer->groups[i];
		b32 groupIsValid = FALSE;
		if (group->init)
		{
			/* If the textures match and have the same color modulation, we can
			 * add these vertices to the current group */

			b32 renderModeMatches = FALSE;
			if (group->mode == targetRenderMode) renderModeMatches = TRUE;

			b32 colorMatches = FALSE;
			if (v4_equals(group->color, color)) colorMatches = TRUE;

			b32 flagsMatches = FALSE;
			if (group->flags == flags) flagsMatches = TRUE;

			b32 texMatches = TRUE;
			if (!tex && !group->tex)
			{
				texMatches = TRUE;
			}
			else if (tex && group->tex)
			{
				if (group->tex->id == tex->id)
				{
					texMatches = TRUE;
				}
			}

			if (texMatches && colorMatches && renderModeMatches && flagsMatches)
				groupIsValid = TRUE;
		}
		else
		{
			/* New group, unused so initialise it */
			groupIsValid = TRUE;

			// NOTE(doyle): Mark first vertex as degenerate vertex, but where we
			// request wireframe mode- we can't use degenerate vertexes for line
			// mode
			group->vertexIndex++;
			group->init  = TRUE;
			group->tex   = tex;
			group->color = color;
			group->mode  = targetRenderMode;
			group->flags = flags;

#ifdef DENGINE_DEBUG
			debug_countIncrement(debugcount_renderGroups);
#endif
		}

		if (groupIsValid)
		{
			i32 freeVertexSlots = renderer->groupCapacity - group->vertexIndex;
			if (numVertexes < freeVertexSlots)
			{
				if (i != 0)
				{
					RenderGroup tmp     = renderer->groups[0];
					renderer->groups[0] = renderer->groups[i];
					renderer->groups[i] = tmp;
				}
				targetGroup = &renderer->groups[0];
				break;
			}
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

INTERNAL void bufferRenderGroupToGL(Renderer *renderer, RenderGroup *group)
{
	Vertex *vertexList = group->vertexList;
	i32 numVertex = group->vertexIndex;

	// TODO(doyle): We assume that vbo and vao are assigned
	renderer->numVertexesInVbo = numVertex;
	glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo[group->mode]);
	glBufferData(GL_ARRAY_BUFFER, numVertex * sizeof(Vertex), vertexList,
	             GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

INTERNAL void applyRotationToVertexes(v2 pos, v2 pivotPoint, Radians rotate,
                                      Vertex *vertexList, i32 vertexListSize)
{
	if (rotate == 0) return;
	// NOTE(doyle): Move the world origin to the base position of the object.
	// Then move the origin to the pivot point (e.g. center of object) and
	// rotate from that point.
	v2 pointOfRotation = v2_add(pivotPoint, pos);

	mat4 rotateMat = mat4_translate(pointOfRotation.x, pointOfRotation.y, 0.0f);
	rotateMat      = mat4_mul(rotateMat, mat4_rotate(rotate, 0.0f, 0.0f, 1.0f));
	rotateMat      = mat4_mul(rotateMat, mat4_translate(-pointOfRotation.x,
	                                               -pointOfRotation.y, 0.0f));
	for (i32 i = 0; i < vertexListSize; i++)
	{
		// NOTE(doyle): Manual matrix multiplication since vertex pos is 2D and
		// matrix is 4D
		v2 oldP = vertexList[i].pos;
		v2 newP = {0};

		newP.x = (oldP.x * rotateMat.e[0][0]) + (oldP.y * rotateMat.e[1][0]) +
		         (rotateMat.e[3][0]);
		newP.y = (oldP.x * rotateMat.e[0][1]) + (oldP.y * rotateMat.e[1][1]) +
		         (rotateMat.e[3][1]);

		vertexList[i].pos = newP;
	}
}

INTERNAL v4 getTexRectNormaliseDeviceCoords(RenderTex renderTex)
{
	/* Convert texture coordinates to normalised texture coordinates */
	v4 result = renderTex.texRect;
	if (renderTex.tex)
	{
		v2 texNdcFactor =
		    V2(1.0f / renderTex.tex->width, 1.0f / renderTex.tex->height);
		result.e[0] *= texNdcFactor.w;
		result.e[1] *= texNdcFactor.h;
		result.e[2] *= texNdcFactor.w;
		result.e[3] *= texNdcFactor.h;
	}

	return result;

}

INTERNAL RenderQuad_ createRenderQuad(Renderer *renderer, v2 pos, v2 size,
                                      v2 pivotPoint, Radians rotate,
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

	v4 vertexPair       = {0};
	vertexPair.vec2[0]  = pos;
	vertexPair.vec2[1]  = v2_add(pos, size);
	v4 texRectNdc = getTexRectNormaliseDeviceCoords(renderTex);

	// NOTE(doyle): Create a quad composed of 4 vertexes to be rendered as
	// a triangle strip using vertices v0, v1, v2, then v2, v1, v3 (note the
	// order)
	RenderQuad_ result = {0};
	result.vertex[0].pos      = V2(vertexPair.x, vertexPair.w); // Top left
	result.vertex[0].texCoord = V2(texRectNdc.x, texRectNdc.w);

	result.vertex[1].pos      = V2(vertexPair.x, vertexPair.y); // Bottom left
	result.vertex[1].texCoord = V2(texRectNdc.x, texRectNdc.y);

	result.vertex[2].pos      = V2(vertexPair.z, vertexPair.w); // Top right
	result.vertex[2].texCoord = V2(texRectNdc.z, texRectNdc.w);

	result.vertex[3].pos      = V2(vertexPair.z, vertexPair.y); // Bottom right
	result.vertex[3].texCoord = V2(texRectNdc.z, texRectNdc.y);

	// NOTE(doyle): Precalculate rotation on vertex positions
	// NOTE(doyle): No translation/scale matrix as we pre-calculate it from
	// entity data and work in world space until GLSL uses the projection matrix
	applyRotationToVertexes(pos, pivotPoint, rotate, result.vertex,
	                        ARRAY_COUNT(result.vertex));
	return result;
}

INTERNAL RenderTriangle_ createRenderTriangle(Renderer *renderer,
                                              TrianglePoints triangle,
                                              v2 pivotPoint, Radians rotate,
                                              RenderTex renderTex)
{
	/* Convert texture coordinates to normalised texture coordinates */
	v4 texRectNdc = getTexRectNormaliseDeviceCoords(renderTex);

	RenderTriangle_ result = {0};
	result.vertex[0].pos       = triangle.points[0];
	result.vertex[0].texCoord  = V2(texRectNdc.x, texRectNdc.w);

	result.vertex[1].pos       = triangle.points[1];
	result.vertex[1].texCoord  = V2(texRectNdc.x, texRectNdc.y);

	result.vertex[2].pos       = triangle.points[2];
	result.vertex[2].texCoord  = V2(texRectNdc.z, texRectNdc.w);

	applyRotationToVertexes(triangle.points[0], pivotPoint, rotate,
	                        result.vertex, ARRAY_COUNT(result.vertex));

	return result;
}

INTERNAL inline RenderQuad_
createDefaultTexQuad(Renderer *renderer, RenderTex *renderTex)
{
	RenderQuad_ result = {0};
	result = createRenderQuad(renderer, V2(0, 0), V2(0, 0), V2(0, 0),
	                          0.0f, *renderTex);
	return result;
}

INTERNAL void renderGLBufferedData(Renderer *renderer, RenderGroup *group)
{
	ASSERT(group->mode < rendermode_invalid);

	if (group->flags & renderflag_wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	GL_CHECK_ERROR();

	if (group->flags & renderflag_no_texture)
	{
		renderer->activeShaderId =
		    renderer->shaderList[shaderlist_default_no_tex];
		shader_use(renderer->activeShaderId);
	}
	else
	{
		renderer->activeShaderId = renderer->shaderList[shaderlist_default];
		shader_use(renderer->activeShaderId);
		Texture *tex = group->tex;
		if (tex)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, tex->id);
			shader_uniformSet1i(renderer->activeShaderId, "tex", 0);
			GL_CHECK_ERROR();
		}
	}

	/* Set color modulation value */
	shader_uniformSetVec4f(renderer->activeShaderId, "spriteColor",
	                       group->color);

	glBindVertexArray(renderer->vao[group->mode]);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, renderer->numVertexesInVbo);
	GL_CHECK_ERROR();
	debug_countIncrement(debugcount_drawArrays);

	/* Unbind */
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	GL_CHECK_ERROR();

}

RenderTex renderer_createNullRenderTex(AssetManager *const assetManager)
{
	Texture *emptyTex = asset_getTex(assetManager, "nullTex");
	RenderTex result  = {emptyTex, V4(0, 1, 1, 0)};
	return result;
}

void renderer_rect(Renderer *const renderer, Rect camera, v2 pos, v2 size,
                   v2 pivotPoint, Radians rotate, RenderTex *renderTex, v4 color,
                   RenderFlags flags)
{
	// NOTE(doyle): Bottom left and top right position of quad in world space
	v2 posInCameraSpace = v2_sub(pos, camera.min);

	RenderTex emptyRenderTex = {0};
	if (!renderTex) renderTex = &emptyRenderTex;

	RenderQuad_ quad = createRenderQuad(renderer, posInCameraSpace, size,
	                                    pivotPoint, rotate, *renderTex);

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
	   1 degenerate vertex at the start of each render group.
	   */


		Vertex vertexList[6] = {quad.vertex[0], quad.vertex[0], quad.vertex[1],
		                        quad.vertex[2], quad.vertex[3], quad.vertex[3]};
		addVertexToRenderGroup(renderer, renderTex->tex, color, vertexList,
		                       ARRAY_COUNT(vertexList), rendermode_quad, flags);
}

void renderer_triangle(Renderer *const renderer, Rect camera,
                       TrianglePoints triangle, v2 pivotPoint, Radians rotate,
                       RenderTex *renderTex, v4 color, RenderFlags flags)
{
	TrianglePoints triangleInCamSpace = {0};
	ASSERT(ARRAY_COUNT(triangle.points) ==
	       ARRAY_COUNT(triangleInCamSpace.points));

	for (i32 i = 0; i < ARRAY_COUNT(triangleInCamSpace.points); i++)
		triangleInCamSpace.points[i] = v2_sub(triangle.points[i], camera.min);

	RenderTex emptyRenderTex = {0};
	if (!renderTex) renderTex = &emptyRenderTex;

	RenderTriangle_ renderTriangle = createRenderTriangle(
	    renderer, triangleInCamSpace, pivotPoint, rotate, *renderTex);

	// NOTE(doyle): Create degenerate vertex setup
	Vertex vertexList[5] = {renderTriangle.vertex[0], renderTriangle.vertex[0],
	                        renderTriangle.vertex[1], renderTriangle.vertex[2],
	                        renderTriangle.vertex[2]};

	addVertexToRenderGroup(renderer, renderTex->tex, color, vertexList,
	                       ARRAY_COUNT(vertexList), rendermode_triangle, flags);
}

void renderer_string(Renderer *const renderer, MemoryArena_ *arena, Rect camera,
                     Font *const font, const char *const string, v2 pos,
                     v2 pivotPoint, Radians rotate, v4 color, RenderFlags flags)
{
	i32 strLen = common_strlen(string);
	if (strLen <= 0) return;

	// TODO(doyle): Slightly incorrect string length in pixels calculation,
	// because we use the advance metric of each character for length not
	// maximum character size in rendering
	v2 rightAlignedP =
	    v2_add(pos, V2((CAST(f32) font->maxSize.w * CAST(f32) strLen),
	                   CAST(f32) font->maxSize.h));
	v2 leftAlignedP = pos;
	if (math_pointInRect(camera, leftAlignedP) ||
	    math_pointInRect(camera, rightAlignedP))
	{
		i32 vertexIndex        = 0;
		i32 numVertexPerQuad   = 4;
		i32 numVertexesToAlloc = (strLen * (numVertexPerQuad + 2));
		Vertex *vertexList =
		    memory_pushBytes(arena, numVertexesToAlloc * sizeof(Vertex));

		v2 posInCameraSpace = v2_sub(pos, camera.min);
		pos = posInCameraSpace;

		// TODO(doyle): Find why font is 1px off, might be arial font semantics
		Texture *tex = font->atlas->tex;
		f32 baseline = pos.y - font->verticalSpacing + 1;
		for (i32 i = 0; i < strLen; i++)
		{
			i32 codepoint     = string[i];
			i32 relativeIndex = CAST(i32)(codepoint - font->codepointRange.x);
			CharMetrics metric = font->charMetrics[relativeIndex];
			pos.y              = baseline - (metric.offset.y);

			/* Get texture out */
			SubTexture subTexture =
			    asset_getAtlasSubTex(font->atlas, &CAST(char)codepoint);

			v4 charTexRect      = {0};
			charTexRect.vec2[0] = subTexture.rect.min;
			charTexRect.vec2[1] =
			    v2_add(subTexture.rect.min, subTexture.rect.max);
			flipTexCoord(&charTexRect, FALSE, TRUE);

			RenderTex renderTex = {tex, charTexRect};
			RenderQuad_ quad    = createRenderQuad(renderer, pos, font->maxSize,
			                                    pivotPoint, rotate, renderTex);

			vertexList[vertexIndex++] = quad.vertex[0];
			for (i32 i = 0; i < ARRAY_COUNT(quad.vertex); i++)
			{
				vertexList[vertexIndex++] = quad.vertex[i];
			}
			vertexList[vertexIndex++] = quad.vertex[3];
			pos.x += metric.advance;
		}

		addVertexToRenderGroup(renderer, tex, color, vertexList,
		                       numVertexesToAlloc, rendermode_quad, flags);
		// TODO(doyle): Mem free
		// PLATFORM_MEM_FREE(arena, vertexList,
		//                  sizeof(Vertex) * numVertexesToAlloc);
	}
}

void renderer_entity(Renderer *renderer, Rect camera, Entity *entity,
                     v2 pivotPoint, Degrees rotate, v4 color, RenderFlags flags)
{
	Radians totalRotation = DEGREES_TO_RADIANS((entity->rotation + rotate));
	RenderTex renderTex   = {0};
	if (entity->tex)
	{
		EntityAnim *entityAnim = &entity->animList[entity->animListIndex];
		v4 texRect             = {0};
		if (entityAnim->anim)
		{
			Animation *anim   = entityAnim->anim;
			char *frameName   = anim->frameList[entityAnim->currFrame];
			SubTexture subTex = asset_getAtlasSubTex(anim->atlas, frameName);

			texRect.vec2[0] = subTex.rect.min;
			texRect.vec2[1] = v2_add(subTex.rect.min, subTex.rect.max);
			flipTexCoord(&texRect, entity->flipX, entity->flipY);
		}
		else
		{
			texRect = V4(0.0f, 0.0f, (f32)entity->tex->width,
			             (f32)entity->tex->height);
		}

		if (entity->direction == direction_east)
		{
			flipTexCoord(&texRect, TRUE, FALSE);
		}

		renderTex.tex     = entity->tex;
		renderTex.texRect = texRect;
	}

	if (entity->renderMode == rendermode_quad)
	{
		renderer_rect(renderer, camera, entity->pos, entity->size, pivotPoint,
		              totalRotation, &renderTex, color, flags);
	}
	else if (entity->renderMode == rendermode_triangle)
	{
		TrianglePoints triangle = {0};

		v2 entityPWithOffset = v2_sub(entity->pos, entity->offset);
		v2 triangleTopPoint  = V2(entityPWithOffset.x + (entity->size.w * 0.5f),
		                         entityPWithOffset.y + entity->size.h);

		v2 triangleRightSide =
		    V2(entityPWithOffset.x + entity->size.w, entityPWithOffset.y);

		triangle.points[0] = entityPWithOffset;
		triangle.points[1] = triangleRightSide;
		triangle.points[2] = triangleTopPoint;

		renderer_triangle(renderer, camera, triangle, pivotPoint, totalRotation,
		                  &renderTex, color, flags);
	}
	else
	{
		ASSERT(INVALID_CODE_PATH);
	}
}

void renderer_renderGroups(Renderer *renderer)
{
	for (i32 i = 0; i < ARRAY_COUNT(renderer->groups); i++)
	{
		RenderGroup *currGroup = &renderer->groups[i];
		if (currGroup->init)
		{
			bufferRenderGroupToGL(renderer, currGroup);
			renderGLBufferedData(renderer, currGroup);

			RenderGroup cleanGroup = {0};
			cleanGroup.vertexList = currGroup->vertexList;
			*currGroup = cleanGroup;
		}
		else
		{
			break;
		}
	}
}
