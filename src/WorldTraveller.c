#include "Dengine/AssetManager.h"
#include "Dengine/Math.h"
#include "WorldTraveller/WorldTraveller.h"

//TODO(doyle): This is temporary! Maybe abstract into our platform layer, or
//choose to load assets outside of WorldTraveller!
#include <stdlib.h>

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

void worldTraveller_gameInit(GameState *state, v2i windowSize)
{
	AssetManager *assetManager = &state->assetManager;
	/* Initialise assets */
	asset_loadTextureImage(assetManager,
	                       "data/textures/WorldTraveller/TerraSprite1024.png",
	                       texlist_hero);

	asset_loadTextureImage(assetManager,
	                       "data/textures/WorldTraveller/Terrain.png",
	                       texlist_terrain);
	TexAtlas *terrainAtlas =
	    asset_getTextureAtlas(assetManager, texlist_terrain);
	f32 atlasTileSize = 128.0f;
	terrainAtlas->texRect[terraincoords_ground] =
	    V4(384.0f, 512.0f, 384.0f + atlasTileSize, 512.0f + atlasTileSize);

	asset_loadShaderFiles(assetManager, "data/shaders/sprite.vert.glsl",
	                      "data/shaders/sprite.frag.glsl", shaderlist_sprite);

	asset_loadTTFont(assetManager, "C:/Windows/Fonts/Arial.ttf");
	glCheckError();

	state->state          = state_active;
	state->tileSize       = 64;
	state->currWorldIndex = 0;

	/* Init world tiles */
	i32 highestSquaredValue = 1;
	while (squared(highestSquaredValue) < ARRAY_COUNT(state->world[0].tiles))
		highestSquaredValue++;

	const i32 worldSize = highestSquaredValue - 1;

	// NOTE(doyle): Origin is center of the world
	for (i32 i = 0; i < ARRAY_COUNT(state->world); i++)
	{
		for (i32 y = 0; y < worldSize; y++)
		{
			for (i32 x = 0; x < worldSize; x++)
			{
				i32 packedDimension = y * worldSize + x;
				World *world = state->world;

				world[i].texType = texlist_terrain;
				world[i].tiles[packedDimension].pos =
				    V2(CAST(f32) x, CAST(f32) y);
			}
		}
	}

	/* Init hero */
	Entity heroEnt = {V2(0.0f, 0.0f),
	                  V2(0.0f, 0.0f),
	                  V2(58.0f, 98.0f),
	                  direction_east,
	                  asset_getTexture(assetManager, texlist_hero),
	                  TRUE,
	                  0,
	                  0,
	                  0};

	SpriteAnim heroAnimIdle = {NULL, 1, 0, 1.0f, 1.0f};
	// TODO(doyle): Get rid of
	heroAnimIdle.rect = (v4 *)calloc(1, sizeof(v4));
	heroAnimIdle.rect[0] = V4(746.0f, 1018.0f, 804.0f, 920.0f);
	heroEnt.anim[heroEnt.freeAnimIndex++] = heroAnimIdle;

	SpriteAnim heroAnimWalk = {NULL, 3, 0, 0.10f, 0.10f};
	// TODO(doyle): Get rid of
	heroAnimWalk.rect = (v4 *)calloc(heroAnimWalk.numRects, sizeof(v4));
	heroAnimWalk.rect[0] = V4(641.0f, 1018.0f, 699.0f, 920.0f);
	heroAnimWalk.rect[1] = heroAnimIdle.rect[0];
	heroAnimWalk.rect[2] = V4(849.0f, 1018.0f, 904.0f, 920.0f);
	heroEnt.anim[heroEnt.freeAnimIndex++] = heroAnimWalk;
	heroEnt.currAnimIndex = 0;

	state->heroIndex = state->freeEntityIndex;
	state->entityList[state->freeEntityIndex++] = heroEnt;
	Entity *hero = &state->entityList[state->heroIndex];

	Texture *heroSheet = hero->tex;
	v2 sheetSize = V2(CAST(f32)heroSheet->width, CAST(f32)heroSheet->height);
	if (sheetSize.x != sheetSize.y)
	{
		printf(
		    "worldTraveller_gameInit() warning: Sprite sheet is not square: "
		    "%dx%dpx\n",
		    CAST(i32) sheetSize.w, CAST(i32) sheetSize.h);
	}

	/* Create a NPC */
	SpriteAnim npcAnim = {NULL, 2, 0, 0.3f, 0.3f};
	// TODO(doyle): Get rid of
	npcAnim.rect = (v4 *)calloc(2, sizeof(v4));
	npcAnim.rect[0] = V4(944.0f, 918.0f, 1010.0f, 816.0f);
	npcAnim.rect[1] = V4(944.0f, 812.0f, 1010.0f, 710.0f);

	Entity npcEnt = {V2(300.0f, 300.0f),
	                 V2(0.0f, 0.0f),
	                 hero->size,
	                 direction_null,
	                 hero->tex,
	                 TRUE,
	                 0,
	                 0,
	                 0};
	npcEnt.anim[npcEnt.freeAnimIndex++] = npcAnim;
	state->entityList[state->freeEntityIndex++] = npcEnt;

	/* Init renderer */
	Renderer *renderer = &state->renderer;
	renderer->size = V2(CAST(f32)windowSize.x, CAST(f32)windowSize.y);
	// NOTE(doyle): Value to map a screen coordinate to NDC coordinate
	renderer->vertexNdcFactor =
	    V2(1.0f / renderer->size.w, 1.0f / renderer->size.h);
	renderer->shader = asset_getShader(assetManager, shaderlist_sprite);
	shader_use(renderer->shader);

	const mat4 projection =
	    mat4_ortho(0.0f, renderer->size.w, 0.0f, renderer->size.h, 0.0f, 1.0f);
	shader_uniformSetMat4fv(renderer->shader, "projection", projection);
	glCheckError();


	/* Create buffers */
	glGenVertexArrays(1, &renderer->vao);
	glGenBuffers(1, &renderer->vbo);
	glCheckError();

	/* Bind buffers */
	glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
	glBindVertexArray(renderer->vao);

	/* Configure VAO */
	const GLuint numVertexElements = 4;
	const GLuint vertexSize        = sizeof(v4);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, numVertexElements, GL_FLOAT, GL_FALSE, vertexSize,
	                      (GLvoid *)0);
	glCheckError();

	/* Unbind */
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glCheckError();
}

INTERNAL void parseInput(GameState *state, const f32 dt)
{
	/*
	   Equations of Motion
	   f(t)  = position     m
	   f'(t) = velocity     m/s
	   f"(t) = acceleration m/s^2

	   The user supplies an acceleration, a, and by integrating
	   f"(t) = a,                   where a is a constant, acceleration
	   f'(t) = a*t + v,             where v is a constant, old velocity
	   f (t) = (a/2)*t^2 + v*t + p, where p is a constant, old position
	 */

	Entity *hero = &state->entityList[state->heroIndex];
	v2 ddPos = V2(0, 0);

	if (state->keys[GLFW_KEY_SPACE])
	{
	}

	if (state->keys[GLFW_KEY_RIGHT])
	{
		ddPos.x = 1.0f;
		hero->direction = direction_east;
	}
	if (state->keys[GLFW_KEY_LEFT])
	{
		ddPos.x = -1.0f;
		hero->direction = direction_west;
	}

	if (state->keys[GLFW_KEY_UP])
	{
		ddPos.y = 1.0f;
	}
	if (state->keys[GLFW_KEY_DOWN])
	{
		ddPos.y = -1.0f;
	}

	if (ddPos.x != 0.0f && ddPos.y != 0.0f)
	{
		// NOTE(doyle): Cheese it and pre-compute the vector for diagonal using
		// pythagoras theorem on a unit triangle
		// 1^2 + 1^2 = c^2
		ddPos = v2_scale(ddPos, 0.70710678118f);
	}

	f32 epsilon    = 20.0f;
	v2 epsilonDpos = v2_sub(V2(epsilon, epsilon),
	                        V2(absolute(hero->dPos.x), absolute(hero->dPos.y)));
	if (epsilonDpos.x >= 0.0f && epsilonDpos.y >= 0.0f)
	{
		hero->dPos = V2(0.0f, 0.0f);
		// TODO(doyle): Change index to use some meaningful name like a string
		// or enum for referencing animations, in this case 0 is idle and 1 is
		// walking
		if (hero->currAnimIndex == 1)
		{
			SpriteAnim *currAnim    = &hero->anim[hero->currAnimIndex];
			currAnim->currDuration  = currAnim->duration;
			currAnim->currRectIndex = 0;
			hero->currAnimIndex     = 0;
		}
	}
	else if (hero->currAnimIndex == 0)
	{
		SpriteAnim *currAnim    = &hero->anim[hero->currAnimIndex];
		currAnim->currDuration  = currAnim->duration;
		currAnim->currRectIndex = 0;
		hero->currAnimIndex     = 1;
	}

	f32 heroSpeed = CAST(f32)(22.0f * METERS_TO_PIXEL); // m/s^2
	if (state->keys[GLFW_KEY_LEFT_SHIFT])
	{
		heroSpeed = CAST(f32)(22.0f * 5.0f * METERS_TO_PIXEL);
	}
	ddPos = v2_scale(ddPos, heroSpeed);

	// TODO(doyle): Counteracting force on player's acceleration is arbitrary
    ddPos = v2_sub(ddPos, v2_scale(hero->dPos, 5.5f));

	/*
	   NOTE(doyle): Calculate new position from acceleration with old velocity
	   new Position     = (a/2) * (t^2) + (v*t) + p,
	   acceleration     = (a/2) * (t^2)
	   old velocity     =                 (v*t)
	 */
	v2 ddPosNew = v2_scale(v2_scale(ddPos, 0.5f), squared(dt));
	v2 dPos     = v2_scale(hero->dPos, dt);
	v2 newHeroP = v2_add(v2_add(ddPosNew, dPos), hero->pos);

	b32 heroCollided = FALSE;
	if (hero->collides == TRUE)
	{
		for (i32 i = 0; i < ARRAY_COUNT(state->entityList); i++)
		{
			if (i == state->heroIndex) continue;
			Entity entity = state->entityList[i];
			if (entity.collides)
			{
				v4 heroRect =
				    V4(newHeroP.x, newHeroP.y, (newHeroP.x + hero->size.x),
				       (newHeroP.y + hero->size.y));
				v4 entityRect = getEntityScreenRect(entity);

				if (((heroRect.z >= entityRect.x && heroRect.z <= entityRect.z) ||
				     (heroRect.x >= entityRect.x && heroRect.x <= entityRect.z)) &&
				    ((heroRect.w <= entityRect.y && heroRect.w >= entityRect.w) ||
				     (heroRect.y <= entityRect.y && heroRect.y >= entityRect.w)))
				{
					heroCollided = TRUE;
					break;
				}
			}
		}
	}

	if (heroCollided)
	{
		hero->dPos = V2(0.0f, 0.0f);
	}
	else
	{
		// f'(t) = curr velocity = a*t + v, where v is old velocity
		hero->dPos = v2_add(hero->dPos, v2_scale(ddPos, dt));
		hero->pos  = newHeroP;
	}
}

void worldTraveller_gameUpdateAndRender(GameState *state, const f32 dt)
{
	/* Update */
	parseInput(state, dt);
	glCheckError();

	AssetManager *assetManager = &state->assetManager;
	Renderer *renderer         = &state->renderer;

	World *const world = &state->world[state->currWorldIndex];
	TexAtlas *const worldAtlas =
	    asset_getTextureAtlas(assetManager, world->texType);
	Texture *const worldTex = asset_getTexture(assetManager, world->texType);

	f32 texNdcFactor = 1.0f / MAX_TEXTURE_SIZE;

	RenderQuad worldQuads[ARRAY_COUNT(world->tiles)] = {0};
	i32 quadIndex = 0;


	/* Render background tiles */
	const v2 tileSize = V2(CAST(f32) state->tileSize, CAST(f32) state->tileSize);
	for (i32 i = 0; i < ARRAY_COUNT(world->tiles); i++)
	{
		Tile tile = world->tiles[i];
		v2 tilePosInPixel = v2_scale(tile.pos, tileSize.x);

		if ((tilePosInPixel.x < renderer->size.w && tilePosInPixel.x >= 0) &&
		    (tilePosInPixel.y < renderer->size.h && tilePosInPixel.y >= 0))
		{
			const v4 texRect  = worldAtlas->texRect[terraincoords_ground];
			const v4 tileRect = getRect(tilePosInPixel, tileSize);

			RenderQuad tileQuad = renderer_createQuad(
			    &state->renderer, tileRect, texRect, worldTex);
			worldQuads[quadIndex++] = tileQuad;
		}
	}

	updateBufferObject(renderer, worldQuads, quadIndex);
	renderer_object(renderer, V2(0.0f, 0.0f), renderer->size, 0.0f,
	                V3(0, 0, 0), worldTex);

	Font *font = &assetManager->font;
	char *string = "hello world";
	i32 strLen = 11;
	quadIndex = 0;
	RenderQuad *stringQuads = CAST(RenderQuad *)calloc(strLen, sizeof(RenderQuad));

	f32 xPosOnScreen = 20.0f;
	for (i32 i = 0; i < strLen; i++)
	{
		// NOTE(doyle): Atlas packs fonts tightly, so offset the codepoint to
		// its actual atlas index, i.e. we skip the first 31 glyphs
		i32 atlasIndex = string[i] - font->codepointRange.x;

		v4 charTexRect = font->atlas->texRect[atlasIndex];
		renderer_flipTexCoord(&charTexRect, FALSE, TRUE);

		const v4 charRectOnScreen =
		    getRect(V2(xPosOnScreen, 100.0f),
		            V2(CAST(f32) font->charSize.w, CAST(f32) font->charSize.w));
		xPosOnScreen += font->charSize.w;

		RenderQuad charQuad = renderer_createQuad(
		    &state->renderer, charRectOnScreen, charTexRect, font->tex);
		stringQuads[quadIndex++] = charQuad;
	}

	updateBufferObject(&state->renderer, stringQuads, quadIndex);
	renderer_object(&state->renderer, V2(0.0f, 100.0f), renderer->size, 0.0f,
	                V3(0, 0, 0), font->tex);
	free(stringQuads);

	/* Render entities */
	ASSERT(state->freeEntityIndex < ARRAY_COUNT(state->entityList));
	for (i32 i = 0; i < state->freeEntityIndex; i++)
	{
		Entity *const entity   = &state->entityList[i];
		SpriteAnim *anim = &entity->anim[entity->currAnimIndex];

		v4 texRect = anim->rect[anim->currRectIndex];
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
		    renderer_createDefaultQuad(&state->renderer, texRect, entity->tex);
		updateBufferObject(&state->renderer, &entityQuad, 1);
		renderer_entity(&state->renderer, entity, 0.0f, V3(0, 0, 0));
	}

	// TODO(doyle): Clean up lines
	// Renderer::~Renderer() { glDeleteVertexArrays(1, &this->quadVAO); }
}
