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

// NOTE(doyle): A vertex batch is the batch of vertexes comprised to make one
// shape
INTERNAL void beginVertexBatch(Renderer *renderer)
{
	ASSERT(renderer->vertexBatchState == vertexbatchstate_off);
	ASSERT(renderer->groupIndexForVertexBatch == -1);
	renderer->vertexBatchState = vertexbatchstate_initial_add;
}

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
   1 degenerate vertex at the start of each render group. The initial degenrate
   vertexes are added in the add to vertex group and the ending degenerates at
   on ending a vertex batch.
   */
INTERNAL void endVertexBatch(Renderer *renderer)
{
	ASSERT(renderer->vertexBatchState != vertexbatchstate_off);
	ASSERT(renderer->groupIndexForVertexBatch != -1);

	i32 numDegenerateVertexes = 2;
	RenderGroup *group = &renderer->groups[renderer->groupIndexForVertexBatch];

	i32 freeVertexSlots = renderer->groupCapacity - group->vertexIndex;
	if (numDegenerateVertexes < freeVertexSlots)
	{
		Vertex degenerateVertex = group->vertexList[group->vertexIndex-1];
		group->vertexList[group->vertexIndex++] = degenerateVertex;
		group->vertexList[group->vertexIndex++] = degenerateVertex;
	}

	renderer->vertexBatchState         = vertexbatchstate_off;
	renderer->groupIndexForVertexBatch = -1;
}

INTERNAL void addVertexToRenderGroup_(Renderer *renderer, Texture *tex,
                                      v4 color, Vertex *vertexList,
                                      i32 numVertexes,
                                      enum RenderMode targetRenderMode,
                                      RenderFlags flags)
{
	ASSERT(renderer->vertexBatchState != vertexbatchstate_off);
	ASSERT(numVertexes > 0);

#ifdef DENGINE_DEBUG
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

			// NOTE(doyle): Two at start, two at end
			i32 numDegenerateVertexes = 0;
			if (renderer->vertexBatchState == vertexbatchstate_initial_add)
			{
				numDegenerateVertexes = 2;
			}

			if ((numDegenerateVertexes + numVertexes) < freeVertexSlots)
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
		if (renderer->vertexBatchState == vertexbatchstate_initial_add)
		{
			targetGroup->vertexList[targetGroup->vertexIndex++] = vertexList[0];
			targetGroup->vertexList[targetGroup->vertexIndex++] = vertexList[0];
			renderer->vertexBatchState = vertexbatchstate_active;

			// NOTE(doyle): We swap groups to the front if it is valid, so
			// target group should always be 0
			ASSERT(renderer->groupIndexForVertexBatch == -1);
			renderer->groupIndexForVertexBatch = 0;
		}

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

	beginVertexBatch(renderer);
	addVertexToRenderGroup_(renderer, renderTex->tex, color, quad.vertex,
	                        ARRAY_COUNT(quad.vertex), rendermode_quad, flags);
	endVertexBatch(renderer);
}

void renderer_polygon(Renderer *const renderer, Rect camera,
                      v2 *polygonPoints, i32 numPoints, v2 pivotPoint,
                      Radians rotate, RenderTex *renderTex, v4 color,
                      RenderFlags flags)
{
	ASSERT(numPoints > 3);

	for (i32 i = 0; i < numPoints; i++)
		polygonPoints[i] = v2_sub(polygonPoints[i], camera.min);

	RenderTex emptyRenderTex  = {0};
	if (!renderTex) renderTex = &emptyRenderTex;

	i32 numTrisInTriangulation = numPoints - 2;
	v2 triangulationBaseP = polygonPoints[0];
	i32 triangulationIndex = 0;

	Vertex triangulationBaseVertex = {0};
	triangulationBaseVertex.pos = triangulationBaseP;


	beginVertexBatch(renderer);
	for (i32 i = 1; triangulationIndex < numTrisInTriangulation; i++)
	{
		ASSERT((i + 1) <= numPoints);

		RenderTriangle_ tri = {0};
		tri.vertex[0].pos  = triangulationBaseP;
		tri.vertex[1].pos  = polygonPoints[i + 1];
		tri.vertex[2].pos  = polygonPoints[i];

		addVertexToRenderGroup_(renderer, renderTex->tex, color, tri.vertex,
		                        ARRAY_COUNT(tri.vertex), rendermode_polygon,
		                        flags);
		triangulationIndex++;
	}
	endVertexBatch(renderer);
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

	beginVertexBatch(renderer);
	addVertexToRenderGroup_(
	    renderer, renderTex->tex, color, renderTriangle.vertex,
	    ARRAY_COUNT(renderTriangle.vertex), rendermode_triangle, flags);
	endVertexBatch(renderer);
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

			beginVertexBatch(renderer);
			addVertexToRenderGroup_(renderer, tex, color, quad.vertex,
			                        ARRAY_COUNT(quad.vertex), rendermode_quad,
			                        flags);
			endVertexBatch(renderer);
			pos.x += metric.advance;
		}
	}
}

void renderer_entity(Renderer *renderer, MemoryArena_ *transientArena,
                     Rect camera, Entity *entity, v2 pivotPoint, Degrees rotate,
                     v4 color, RenderFlags flags)
{
	// TODO(doyle): Add early exit on entities out of camera bounds
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
	else if (entity->renderMode == rendermode_polygon)
	{
		ASSERT(entity->numVertexPoints > 3);
		ASSERT(entity->vertexPoints);

		v2 *offsetVertexPoints = memory_pushBytes(
		    transientArena, entity->numVertexPoints * sizeof(v2));
		for (i32 i = 0; i < entity->numVertexPoints; i++)
		{
			offsetVertexPoints[i] =
			    v2_add(entity->vertexPoints[i], entity->pos);
		}

		renderer_polygon(renderer, camera, offsetVertexPoints,
		                 entity->numVertexPoints, pivotPoint, totalRotation,
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
