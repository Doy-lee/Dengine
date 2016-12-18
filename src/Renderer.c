#include "Dengine/Renderer.h"
#include "Dengine/AssetManager.h"
#include "Dengine/Assets.h"
#include "Dengine/Debug.h"
#include "Dengine/Entity.h"
#include "Dengine/MemoryArena.h"
#include "Dengine/OpenGL.h"

INTERNAL void shaderUniformSet1i(u32 shaderId, const GLchar *name,
                         const GLuint data)
{
	GLint uniformLoc = glGetUniformLocation(shaderId, name);
	glUniform1i(uniformLoc, data);
}

INTERNAL void shaderUniformSetMat4fv(u32 shaderId, const GLchar *name,
                             mat4 data)
{
	GLint uniformLoc = glGetUniformLocation(shaderId, name);
	GL_CHECK_ERROR();
	glUniformMatrix4fv(uniformLoc, 1, GL_FALSE, data.e[0]);
	GL_CHECK_ERROR();
}

INTERNAL void shaderUniformSetVec4f(u32 shaderId, const GLchar *name,
                            v4 data)
{
	GLint uniformLoc = glGetUniformLocation(shaderId, name);
	glUniform4f(uniformLoc, data.e[0], data.e[1], data.e[2], data.e[3]);
}


INTERNAL void shaderUse(u32 shaderId) { glUseProgram(shaderId); }

void renderer_updateSize(Renderer *renderer, AssetManager *assetManager, v2 windowSize)
{
	renderer->size = windowSize;
	// renderer->displayScale =
	//     V2(windowSize.x * 1.0f / renderer->referenceScale.x,
	//        windowSize.y * 1.0f / renderer->referenceScale.y);

	// NOTE(doyle): Value to map a screen coordinate to NDC coordinate
	renderer->vertexNdcFactor =
	    V2(1.0f / renderer->size.w, 1.0f / renderer->size.h);
	renderer->groupIndexForVertexBatch = -1;

	const mat4 projection =
	    mat4_ortho(0.0f, renderer->size.w, 0.0f, renderer->size.h, 0.0f, 1.0f);

	for (i32 i = 0; i < shaderlist_count; i++)
	{
		renderer->shaderList[i] = asset_shaderGet(assetManager, i);
		shaderUse(renderer->shaderList[i]);
		shaderUniformSetMat4fv(renderer->shaderList[i], "projection",
		                       projection);
		GL_CHECK_ERROR();
	}

	renderer->activeShaderId = renderer->shaderList[shaderlist_default];
	GL_CHECK_ERROR();
}

void renderer_init(Renderer *renderer, AssetManager *assetManager,
                   MemoryArena_ *persistentArena, v2 windowSize)
{
	renderer->referenceScale = V2(1280, 720);
	renderer_updateSize(renderer, assetManager, windowSize);

	/* Create buffers */
	glGenVertexArrays(ARRAY_COUNT(renderer->vao), renderer->vao);
	glGenBuffers(ARRAY_COUNT(renderer->vbo), renderer->vbo);
	GL_CHECK_ERROR();

	// Bind buffers and configure vao, vao automatically intercepts
	// glBindCalls and associates the state with that buffer for us
	for (enum RenderMode mode = 0; mode < rendermode_count; mode++)
	{
		glBindVertexArray(renderer->vao[mode]);
		glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo[mode]);

		glEnableVertexAttribArray(0);
		u32 numVertexElements = 4;
		u32 stride            = sizeof(RenderVertex);

		glVertexAttribPointer(0, numVertexElements, GL_FLOAT,
		                      GL_FALSE, stride, (GLvoid *)0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	/* Unbind */
	GL_CHECK_ERROR();

	// TODO(doyle): Lazy allocate render group capacity
	renderer->groupCapacity = 4096;
	for (i32 i = 0; i < ARRAY_COUNT(renderer->groups); i++)
	{
		renderer->groups[i].vertexList = memory_pushBytes(
		    persistentArena, renderer->groupCapacity * sizeof(RenderVertex));
	}
}


typedef struct RenderQuad
{
	RenderVertex vertexList[4];
} RenderQuad;

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
   rendering.

   The implementation is recognising if the rendered
   entity is the first in its render group, then we don't need to init
   a degenerate vertex, and only at the end of its vertex list. But on
   subsequent renders, we need a degenerate vertex at the front to
   create the zero-area triangle strip.
   */
INTERNAL void endVertexBatch(Renderer *renderer)
{
	ASSERT(renderer->vertexBatchState != vertexbatchstate_off);
	ASSERT(renderer->groupIndexForVertexBatch != -1);

	i32 numDegenerateVertexes = 1;
	RenderGroup *group = &renderer->groups[renderer->groupIndexForVertexBatch];

	i32 freeVertexSlots = renderer->groupCapacity - group->vertexIndex;
	if (numDegenerateVertexes < freeVertexSlots)
	{
		RenderVertex degenerateVertex =
		    group->vertexList[group->vertexIndex - 1];
		group->vertexList[group->vertexIndex++] = degenerateVertex;
	}

	renderer->vertexBatchState         = vertexbatchstate_off;
	renderer->groupIndexForVertexBatch = -1;
}

INTERNAL void applyRotationToVertexes(v2 pos, v2 pivotPoint, Radians rotate,
                                      RenderVertex *vertexList,
                                      i32 vertexListSize)
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

INTERNAL void addVertexToRenderGroup_(Renderer *renderer, Texture *tex,
                                      v4 color, i32 zDepth,
                                      RenderVertex *vertexList, i32 numVertexes,
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

			if (!(group->mode == targetRenderMode)) continue;
			if (!(v4_equals(group->color, color))) continue;
			if (!(group->flags == flags)) continue;
			if (!(group->zDepth == zDepth)) continue;
			if (!tex && group->tex) continue;

			if (tex && group->tex)
			{
				if (!(group->tex->id == tex->id)) continue;
			}

			groupIsValid = TRUE;
		}
		else
		{
			/* New group, unused so initialise it */
			groupIsValid = TRUE;

			group->init   = TRUE;
			group->tex    = tex;
			group->color  = color;
			group->mode   = targetRenderMode;
			group->flags  = flags;
			group->zDepth = zDepth;

			renderer->groupsInUse++;
		}

		if (groupIsValid)
		{
			i32 freeVertexSlots = renderer->groupCapacity - group->vertexIndex;

			// NOTE(doyle): Two at start, two at end
			i32 numDegenerateVertexes = 0;

			if (renderer->vertexBatchState == vertexbatchstate_initial_add)
				numDegenerateVertexes = 1;

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
		// NOTE(doyle): If we are adding 3 vertexes, then we are adding a
		// triangle to the triangle strip. If so, then depending on which "n-th"
		// triangle it is we're adding, the winding order in a t-strip
		// alternates with each triangle (including degenerates). Hence we track
		// so we know the last winding order in the group.

		// For this to work, we must ensure all incoming vertexes are winding in
		// ccw order initially. There is also the presumption that, other
		// rendering methods, such as the quad, consists of an even number of
		// triangles such that the winding order gets alternated back to the
		// same order it started with.
		if (numVertexes == 3)
		{
			if (targetGroup->clockwiseWinding)
			{
				RenderVertex tmp = vertexList[0];
				vertexList[0]    = vertexList[2];
				vertexList[2]    = tmp;
			}

			targetGroup->clockwiseWinding =
			    (targetGroup->clockwiseWinding) ? FALSE : TRUE;
		}

		if (renderer->vertexBatchState == vertexbatchstate_initial_add)
		{
			if (targetGroup->vertexIndex != 0)
			{
				targetGroup->vertexList[targetGroup->vertexIndex++] =
				    vertexList[0];
			}
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

INTERNAL RenderQuad createRenderQuad(Renderer *renderer, v2 pos, v2 size,
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

	v4 vertexPair      = {0};
	vertexPair.vec2[0] = pos;
	vertexPair.vec2[1] = v2_add(pos, size);
	v4 texRectNdc      = getTexRectNormaliseDeviceCoords(renderTex);

	// NOTE(doyle): Create a quad composed of 4 vertexes to be rendered as
	// a triangle strip using vertices v0, v1, v2, then v2, v1, v3 (note the
	// order)
	v2 vertexList[4] = {0};
	v2 texCoordList[4] = {0};

	// Top left
	vertexList[0]   = V2(vertexPair.x, vertexPair.w);
	texCoordList[0] = V2(texRectNdc.x, texRectNdc.w);

	// Bottom left
	vertexList[1]   = V2(vertexPair.x, vertexPair.y);
	texCoordList[1] = V2(texRectNdc.x, texRectNdc.y);

	// Top right
	vertexList[2]   = V2(vertexPair.z, vertexPair.w);
	texCoordList[2] = V2(texRectNdc.z, texRectNdc.w);

	// Bottom right
	vertexList[3]   = V2(vertexPair.z, vertexPair.y);
	texCoordList[3] = V2(texRectNdc.z, texRectNdc.y);

	// NOTE(doyle): Precalculate rotation on vertex positions
	// NOTE(doyle): No translation/scale matrix as we pre-calculate it from
	// entity data and work in world space until GLSL uses the projection matrix

	math_applyRotationToVertexes(pos, pivotPoint, rotate, vertexList,
	                             ARRAY_COUNT(vertexList));

	RenderQuad result = {0};

	ASSERT(ARRAY_COUNT(vertexList) == ARRAY_COUNT(result.vertexList));
	for (i32 i = 0; i < ARRAY_COUNT(vertexList); i++)
	{
		result.vertexList[i].pos = vertexList[i];
		result.vertexList[i].texCoord = texCoordList[i];
	}

	return result;
}

INTERNAL inline RenderQuad
createDefaultTexQuad(Renderer *renderer, RenderTex *renderTex)
{
	RenderQuad result = {0};
	result = createRenderQuad(renderer, V2(0, 0), V2(0, 0), V2(0, 0), 0.0f,
	                          *renderTex);
	return result;
}

INTERNAL void renderGLBufferedData(Renderer *renderer, RenderGroup *group)
{
}

RenderTex renderer_createNullRenderTex(AssetManager *const assetManager)
{
	Texture *emptyTex = asset_texGet(assetManager, "nullTex");
	RenderTex result  = {emptyTex, V4(0, 1, 1, 0)};
	return result;
}

void renderer_rect(Renderer *const renderer, Rect camera, v2 pos, v2 size,
                   v2 pivotPoint, Radians rotate, RenderTex *renderTex,
                   v4 color, i32 zDepth, RenderFlags flags)
{
	// NOTE(doyle): Bottom left and top right position of quad in world space
	v2 posInCameraSpace = v2_sub(pos, camera.min);

	RenderTex emptyRenderTex = {0};
	if (!renderTex)
	{
		renderTex = &emptyRenderTex;
		ASSERT(common_isSet(flags, renderflag_no_texture));
	}

	RenderQuad quad = createRenderQuad(renderer, posInCameraSpace, size,
	                                   pivotPoint, rotate, *renderTex);

	beginVertexBatch(renderer);
	addVertexToRenderGroup_(renderer, renderTex->tex, color, zDepth,
	                        quad.vertexList, ARRAY_COUNT(quad.vertexList),
	                        rendermode_quad, flags);
	endVertexBatch(renderer);
}

void renderer_polygon(Renderer *const renderer, Rect camera,
                      v2 *polygonPoints, i32 numPoints, v2 pivotPoint,
                      Radians rotate, RenderTex *renderTex, v4 color, i32 zDepth,
                      RenderFlags flags)
{
	ASSERT(numPoints >= 3);

	{ // Validate polygon is CCW
		/*
		   NOTE(doyle): Polygon vertexes must be specified in a CCW order!
		   This check utilises the equation for calculating the bounding area
		   of a polygon by decomposing the shapes into line segments and
		   calculating the area under the segment. On cartesian plane, if the
		   polygon is CCW, then by creating these "area" calculatings in
		   sequential order, we'll produce negative valued areas since we
		   determine line segment length by subtracting x2-x1.

		   Better explanation over here
		   http://blog.element84.com/polygon-winding.html
		*/

		f32 areaSum = 0.0f;
		for (i32 i = 0; i < numPoints - 1; i++)
		{
			f32 lengthX = polygonPoints[i + 1].x - polygonPoints[i].x;

			// NOTE(doyle): The height of a line segment is actually (y1 + y2)/2
			// But since the (1/2) is a constant factor we can get rid of for
			// checking the winding order..
			// i.e. a negative number halved is still always negative.
			f32 lengthY = polygonPoints[i + 1].y + polygonPoints[i].y;

			areaSum += (lengthX * lengthY);
		}

		f32 lengthX = polygonPoints[0].x - polygonPoints[numPoints - 1].x;
		f32 lengthY = polygonPoints[0].y + polygonPoints[numPoints - 1].y;
		areaSum += (lengthX * lengthY);

		if (areaSum < 0)
		{
			// NOTE(doyle): Is counter clockwise
		}
		else if (areaSum > 0)
		{
			// NOTE(doyle): Is clockwise
			ASSERT(INVALID_CODE_PATH);
		}
		else
		{
			// NOTE(doyle): CW + CCW combination, i.e. figure 8 shape
			ASSERT(INVALID_CODE_PATH);
		}
	}

	for (i32 i           = 0; i < numPoints; i++)
		polygonPoints[i] = v2_sub(polygonPoints[i], camera.min);

	// TODO(doyle): Do something with render texture
	RenderTex emptyRenderTex  = {0};
	if (!renderTex)
	{
		renderTex = &emptyRenderTex;
		ASSERT(common_isSet(flags, renderflag_no_texture));
	}

	v2 triangulationBaseP                = polygonPoints[0];
	RenderVertex triangulationBaseVertex = {0};
	triangulationBaseVertex.pos          = triangulationBaseP;

	i32 numTrisInTriangulation = numPoints - 2;
	i32 triangulationIndex     = 0;
	for (i32 i = 1; triangulationIndex < numTrisInTriangulation; i++)
	{
		ASSERT((i + 1) < numPoints);

		v2 vertexList[3] = {triangulationBaseP, polygonPoints[i],
		                    polygonPoints[i + 1]};

		beginVertexBatch(renderer);

		RenderVertex triangle[3] = {0};
		triangle[0].pos          = vertexList[0];
		triangle[1].pos          = vertexList[1];
		triangle[2].pos          = vertexList[2];

		addVertexToRenderGroup_(renderer, renderTex->tex, color, zDepth,
		                        triangle, ARRAY_COUNT(triangle),
		                        rendermode_polygon, flags);
		endVertexBatch(renderer);
		triangulationIndex++;
	}
}

void renderer_string(Renderer *const renderer, MemoryArena_ *arena, Rect camera,
                     Font *const font, const char *const string, v2 pos,
                     v2 pivotPoint, Radians rotate, v4 color, i32 zDepth,
                     RenderFlags flags)
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
	if (math_rectContainsP(camera, leftAlignedP) ||
	    math_rectContainsP(camera, rightAlignedP))
	{
		i32 vertexIndex        = 0;
		i32 numVertexPerQuad   = 4;
		i32 numVertexesToAlloc = (strLen * (numVertexPerQuad + 2));
		RenderVertex *vertexList =
		    memory_pushBytes(arena, numVertexesToAlloc * sizeof(RenderVertex));

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
			    asset_atlasGetSubTex(font->atlas, &CAST(char)codepoint);

			v4 charTexRect      = {0};
			charTexRect.vec2[0] = subTexture.rect.min;
			charTexRect.vec2[1] =
			    v2_add(subTexture.rect.min, subTexture.rect.max);
			flipTexCoord(&charTexRect, FALSE, TRUE);

			RenderTex renderTex = {tex, charTexRect};
			RenderQuad quad     = createRenderQuad(renderer, pos, font->maxSize,
			                                   pivotPoint, rotate, renderTex);

			beginVertexBatch(renderer);
			addVertexToRenderGroup_(renderer, tex, color, zDepth, quad.vertexList,
			                        ARRAY_COUNT(quad.vertexList),
			                        rendermode_quad, flags);
			endVertexBatch(renderer);
			pos.x += metric.advance;
		}
	}
}

void renderer_entity(Renderer *renderer, MemoryArena_ *transientArena,
                     Rect camera, Entity *entity, v2 pivotPoint, Degrees rotate,
                     v4 color, i32 zDepth,  RenderFlags flags)
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
			SubTexture subTex = asset_atlasGetSubTex(anim->atlas, frameName);

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

	// TODO(doyle): Proper blending
	v4 renderColor = color;
	if (v4_equals(color, V4(0, 0, 0, 0))) renderColor = entity->color;

	if (entity->renderMode == rendermode_quad)
	{
		renderer_rect(renderer, camera, entity->pos, entity->size,
		              v2_add(entity->offset, pivotPoint), totalRotation,
		              &renderTex, entity->color, zDepth, flags);
	}
	else if (entity->renderMode == rendermode_polygon)
	{
		ASSERT(entity->numVertexPoints >= 3);
		ASSERT(entity->vertexPoints);

		v2 *offsetVertexPoints =
		    entity_generateUpdatedVertexList(transientArena, entity);

		renderer_polygon(renderer, camera, offsetVertexPoints,
		                 entity->numVertexPoints,
		                 v2_add(entity->offset, pivotPoint), totalRotation,
		                 &renderTex, renderColor, zDepth, flags);
	}
	else
	{
		ASSERT(INVALID_CODE_PATH);
	}
}

// TODO(doyle): We have no notion of sort order!!
void renderer_renderGroups(Renderer *renderer)
{

	/* Sort the group by zdepth */
	b32 groupHasSwapped = TRUE;
	i32 numGroupsToCheck = renderer->groupsInUse - 1;
	while (groupHasSwapped)
	{
		groupHasSwapped = FALSE;
		for (i32 i = 0; i < numGroupsToCheck; i++)
		{
			RenderGroup *group      = &renderer->groups[i];
			RenderGroup *checkGroup = &renderer->groups[i + 1];

			if (checkGroup->zDepth < group->zDepth)
			{
				RenderGroup tmp = *group;
				*group          = *checkGroup;
				*checkGroup     = tmp;

				groupHasSwapped = TRUE;
			}
		}

		numGroupsToCheck--;
	}

	/* Render groups */
	for (i32 i = 0; i < renderer->groupsInUse; i++)
	{
		RenderGroup *group = &renderer->groups[i];
		{ // Buffer render group to OpenGL
			RenderVertex *vertexList = group->vertexList;
			i32 numVertex            = group->vertexIndex;

			// TODO(doyle): We assume that vbo and vao are assigned
			renderer->numVertexesInVbo = numVertex;
			glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo[group->mode]);
			glBufferData(GL_ARRAY_BUFFER, numVertex * sizeof(RenderVertex),
			             vertexList, GL_STREAM_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}

		{ // Render buffered data in OpenGl

			ASSERT(group->mode < rendermode_invalid);

			if (group->flags & renderflag_wireframe)
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			}
			else
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
			GL_CHECK_ERROR();

			if (group->flags & renderflag_no_texture)
			{
				renderer->activeShaderId =
				    renderer->shaderList[shaderlist_default_no_tex];
				shaderUse(renderer->activeShaderId);
			}
			else
			{
				renderer->activeShaderId =
				    renderer->shaderList[shaderlist_default];
				shaderUse(renderer->activeShaderId);
				Texture *tex = group->tex;
				if (tex)
				{
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, tex->id);
					shaderUniformSet1i(renderer->activeShaderId, "tex", 0);
					GL_CHECK_ERROR();
				}
			}

#if 0
				glDisable(GL_CULL_FACE);
#endif

			/* Set color modulation value */
			shaderUniformSetVec4f(renderer->activeShaderId, "spriteColor",
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

		RenderGroup cleanGroup = {0};
		cleanGroup.vertexList  = group->vertexList;
		*group                 = cleanGroup;
	}

	renderer->groupsInUse = 0;
}
