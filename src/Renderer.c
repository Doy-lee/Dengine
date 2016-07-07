#include "Dengine/Platform.h"
#include "Dengine/OpenGL.h"
#include "Dengine/Renderer.h"

#define RENDER_BOUNDING_BOX FALSE

DebugRenderer debugRenderer = {0};

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

#if 0
void renderer_backgroundTiles(Renderer *const renderer, const v2 tileSize,
                              World *const world, TexAtlas *const atlas,
                              Texture *const tex)
{
	RenderQuad worldQuads[ARRAY_COUNT(world->tiles)] = {0};
	i32 quadIndex = 0;

	for (i32 i = 0; i < ARRAY_COUNT(world->tiles); i++)
	{
		Tile tile = world->tiles[i];
		v2 tilePosInPixel = v2_scale(tile.pos, tileSize.x);

		if ((tilePosInPixel.x < renderer->size.w && tilePosInPixel.x >= 0) &&
		    (tilePosInPixel.y < renderer->size.h && tilePosInPixel.y >= 0))
		{
			const v4 texRect  = atlas->texRect[terraincoords_ground];
			const v4 tileRect = getRect(tilePosInPixel, tileSize);

			RenderQuad tileQuad =
			    renderer_createQuad(renderer, tileRect, texRect, tex);
			worldQuads[quadIndex++] = tileQuad;
		}
	}

	updateBufferObject(renderer, worldQuads, quadIndex);
	renderer_object(renderer, V2(0.0f, 0.0f), renderer->size, 0.0f,
	                V3(0, 0, 0), tex);
}
#endif

void renderer_string(Renderer *const renderer, Font *const font,
                     const char *const string, v2 pos, f32 rotate,
                     v3 color)
{
	i32 quadIndex = 0;
	i32 strLen = common_strlen(string);
	RenderQuad *stringQuads = CAST(RenderQuad *)calloc(strLen, sizeof(RenderQuad));

	f32 baseline = pos.y;
	for (i32 i = 0; i < strLen; i++)
	{
		// NOTE(doyle): Atlas packs fonts tightly, so offset the codepoint to
		// its actual atlas index, i.e. we skip the first 31 glyphs
		i32 codepoint = string[i];
		i32 relativeIndex = codepoint - font->codepointRange.x;
		CharMetrics charMetric = font->charMetrics[relativeIndex];
		pos.y = baseline - charMetric.offset.y;

		const v4 charRectOnScreen = getRect(
		    pos, V2(CAST(f32) font->maxSize.w, CAST(f32) font->maxSize.h));

		pos.x += charMetric.advance;
		
		/* Get texture out */
		v4 charTexRect = font->atlas->texRect[relativeIndex];
		renderer_flipTexCoord(&charTexRect, FALSE, TRUE);

		RenderQuad charQuad = renderer_createQuad(renderer, charRectOnScreen,
		                                          charTexRect, font->tex);
		stringQuads[quadIndex++] = charQuad;
	}

	// NOTE(doyle): We render at the renderer's size because we create quads
	// relative to the window size, hence we also render at the origin since
	// we're rendering a window sized buffer
	updateBufferObject(renderer, stringQuads, quadIndex);
	renderer_object(renderer, V2(0.0f, 0.0f), renderer->size, rotate, color,
	                font->tex);
	free(stringQuads);

}

void renderer_debugString(Renderer *const renderer, Font *const font,
                          const char *const string)
{
	/* Intialise debug object */
	if (!debugRenderer.init)
	{
		debugRenderer.stringPos =
		    V2(0.0f, renderer->size.y -
		                 (1.8f * asset_getVFontSpacing(font->metrics)));
		debugRenderer.init = TRUE;
	}

	f32 rotate = 0;
	v3 color = V3(0, 0, 0);
	renderer_string(renderer, font, string, debugRenderer.stringPos, rotate,
	                color);
	debugRenderer.stringPos.y -= (0.9f * asset_getVFontSpacing(font->metrics));
}

void renderer_entity(Renderer *renderer, Entity *entity, f32 dt, f32 rotate,
                     v3 color)
{
	SpriteAnim *anim = &entity->anim[entity->currAnimIndex];
	v4 texRect       = anim->rect[anim->currRectIndex];

	anim->currDuration -= dt;
	if (anim->currDuration <= 0.0f)
	{
		anim->currRectIndex++;
		anim->currRectIndex = anim->currRectIndex % anim->numRects;
		texRect             = anim->rect[anim->currRectIndex];
		anim->currDuration  = anim->duration;
	}

	if (entity->direction == direction_east)
	{
		// NOTE(doyle): Flip the x coordinates to flip the tex
		renderer_flipTexCoord(&texRect, TRUE, FALSE);
	}

	RenderQuad entityQuad =
	    renderer_createDefaultQuad(renderer, texRect, entity->tex);
	updateBufferObject(renderer, &entityQuad, 1);
	renderer_object(renderer, entity->pos, entity->size, rotate, color,
	                entity->tex);
}

void renderer_object(Renderer *renderer, v2 pos, v2 size, f32 rotate, v3 color,
                     Texture *tex)
{
	shader_use(renderer->shader);
	mat4 transMatrix  = mat4_translate(pos.x, pos.y, 0.0f);
	mat4 rotateMatrix = mat4_rotate(rotate, 0.0f, 0.0f, 1.0f);

	// NOTE(doyle): We draw everything as a unit square in OGL. Scale it to size
	// TODO(doyle): We should have a notion of hitbox size and texture size
	// we're going to render so we can draw textures that may be bigger than the
	// entity, (slightly) but keep a consistent bounding box
	mat4 scaleMatrix = mat4_scale(size.x, size.y, 1.0f);

	mat4 model = mat4_mul(transMatrix, mat4_mul(rotateMatrix, scaleMatrix));
	//mat4 model = mat4_mul(transMatrix, rotateMatrix);
	shader_uniformSetMat4fv(renderer->shader, "model", model);
	glCheckError();

	// TODO(doyle): Unimplemented
	// this->shader->uniformSetVec3f("spriteColor", color);

#if RENDER_BOUNDING_BOX
	glBindVertexArray(renderer->vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, renderer->numVertexesInVbo);
	glBindVertexArray(0);
#endif

	glActiveTexture(GL_TEXTURE0);

	if (tex)
	{
		glBindTexture(GL_TEXTURE_2D, tex->id);
		shader_uniformSet1i(renderer->shader, "tex", 0);
	}

	glBindVertexArray(renderer->vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, renderer->numVertexesInVbo);
	glBindVertexArray(0);


	glBindTexture(GL_TEXTURE_2D, 0);
	glCheckError();
}

RenderQuad renderer_createQuad(Renderer *renderer, v4 quadRect, v4 texRect,
                               Texture *tex)
{
	// NOTE(doyle): Draws a series of triangles (three-sided polygons) using
	// vertices v0, v1, v2, then v2, v1, v3 (note the order)
	RenderQuad result = {0};

	v4 quadRectNdc = quadRect;
	quadRectNdc.e[0] *= renderer->vertexNdcFactor.w;
	quadRectNdc.e[1] *= renderer->vertexNdcFactor.h;
	quadRectNdc.e[2] *= renderer->vertexNdcFactor.w;
	quadRectNdc.e[3] *= renderer->vertexNdcFactor.h;

	v2 texNdcFactor = V2(1.0f / tex->width, 1.0f / tex->height);
	v4 texRectNdc   = texRect;
	texRectNdc.e[0] *= texNdcFactor.w;
	texRectNdc.e[1] *= texNdcFactor.h;
	texRectNdc.e[2] *= texNdcFactor.w;
	texRectNdc.e[3] *= texNdcFactor.h;

	result.vertex[0] = V4(quadRectNdc.x, quadRectNdc.y, texRectNdc.x,
	                      texRectNdc.y); // Top left
	result.vertex[1] = V4(quadRectNdc.x, quadRectNdc.w, texRectNdc.x,
	                      texRectNdc.w); // Bottom left
	result.vertex[2] = V4(quadRectNdc.z, quadRectNdc.y, texRectNdc.z,
	                      texRectNdc.y); // Top right
	result.vertex[3] = V4(quadRectNdc.z, quadRectNdc.w, texRectNdc.z,
	                      texRectNdc.w); // Bottom right
	return result;
}
