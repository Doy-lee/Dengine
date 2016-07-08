#include "Dengine/AssetManager.h"
#include "Dengine/Math.h"
#include "WorldTraveller/WorldTraveller.h"

//TODO(doyle): This is temporary! Maybe abstract into our platform layer, or
//choose to load assets outside of WorldTraveller!
#include <stdlib.h>

INTERNAL Entity *addEntity(World *world, v2 pos, v2 size,
                           enum EntityType type, enum Direction direction,
                           Texture *tex, b32 collides)
{

#ifdef WT_DEBUG
	ASSERT(tex && world);
	ASSERT(world->freeEntityIndex < world->maxEntities);
	ASSERT(type < entitytype_count);
#endif

	Entity entity    = {0};
	entity.pos       = pos;
	entity.size      = size;
	entity.type      = type;
	entity.direction = direction;
	entity.tex       = tex;
	entity.collides  = collides;

	world->entities[world->freeEntityIndex++] = entity;
	Entity *result = &world->entities[world->freeEntityIndex-1];

	return result;
}

INTERNAL void addAnim(Entity *entity, v4 *rects, i32 numRects, f32 duration)
{

#ifdef WT_DEBUG
	ASSERT(rects && numRects >= 0)
	ASSERT(entity->freeAnimIndex < ARRAY_COUNT(entity->anim));
#endif

	EntityAnim result   = {0};
	result.rect         = rects;
	result.numRects     = numRects;
	result.duration     = duration;
	result.currDuration = duration;

	entity->anim[entity->freeAnimIndex++] = result;
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

	asset_loadTTFont(assetManager, "C:/Windows/Fonts/Arialbd.ttf");
	glCheckError();

	state->state          = state_active;
	state->currWorldIndex = 0;
	state->tileSize       = 64;

	/* Init renderer */
	Renderer *renderer = &state->renderer;
	renderer->size     = V2(CAST(f32) windowSize.x, CAST(f32) windowSize.y);
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

	/* Init world */
	const i32 targetWorldWidth  = 500 * METERS_TO_PIXEL;
	const i32 targetWorldHeight = 15 * METERS_TO_PIXEL;
	v2i worldDimensionInTiles   = V2i(targetWorldWidth / state->tileSize,
	                                targetWorldHeight / state->tileSize);

	for (i32 i = 0; i < ARRAY_COUNT(state->world); i++)
	{
		World *const world = &state->world[i];
		world->maxEntities = 8192;
		world->entities =
		    CAST(Entity *) calloc(world->maxEntities, sizeof(Entity));
		world->texType = texlist_terrain;

		v2 worldDimensionInTilesf = V2(CAST(f32) worldDimensionInTiles.x,
		                               CAST(f32) worldDimensionInTiles.y);
		world->bounds = getRect(V2(0, 0), v2_scale(worldDimensionInTilesf,
		                                           CAST(f32) state->tileSize));

		TexAtlas *const atlas =
		    asset_getTextureAtlas(assetManager, world->texType);

		for (i32 y = 0; y < worldDimensionInTiles.y; y++)
		{
			for (i32 x = 0; x < worldDimensionInTiles.x; x++)
			{
#ifdef WT_DEBUG
				ASSERT(worldDimensionInTiles.x * worldDimensionInTiles.y <
				       world->maxEntities);
#endif
				v2 pos = V2(CAST(f32) x * state->tileSize,
				            CAST(f32) y * state->tileSize);
				v2 size =
				    V2(CAST(f32) state->tileSize, CAST(f32) state->tileSize);
				enum EntityType type = entitytype_tile;
				enum Direction dir = direction_null;
				Texture *tex = asset_getTexture(assetManager, world->texType);
				b32 collides = FALSE;
				Entity *tile =
				    addEntity(world, pos, size, type, dir, tex, collides);

				f32 duration = 1.0f;
				i32 numRects = 1;
				v4 *animRects = CAST(v4 *)calloc(numRects, sizeof(v4));
				animRects[0] = atlas->texRect[terraincoords_ground];
				addAnim(tile, animRects, numRects, duration);
			}
		}
	}

	World *const world = &state->world[state->currWorldIndex];
	world->cameraPos = V2(0.0f, 0.0f);

	/* Init hero entity */
	world->heroIndex   = world->freeEntityIndex;

	v2 size              = V2(58.0f, 98.0f);
	v2 pos               = V2(((renderer->size.w * 0.5f) - (size.w * 0.5f)),
	                          CAST(f32) state->tileSize);
	enum EntityType type = entitytype_hero;
	enum Direction dir   = direction_east;
	Texture *tex         = asset_getTexture(assetManager, texlist_hero);
	b32 collides         = TRUE;
	Entity *hero = addEntity(world, pos, size, type, dir, tex, collides);

	/* Add idle animation */
	f32 duration      = 1.0f;
	i32 numRects      = 1;
	v4 *heroIdleRects = CAST(v4 *) calloc(numRects, sizeof(v4));
	heroIdleRects[0]  = V4(746.0f, 1018.0f, 804.0f, 920.0f);
	addAnim(hero, heroIdleRects, numRects, duration);

	/* Add walking animation */
	duration          = 0.10f;
	numRects          = 3;
	v4 *heroWalkRects = CAST(v4 *) calloc(numRects, sizeof(v4));
	heroWalkRects[0]  = V4(641.0f, 1018.0f, 699.0f, 920.0f);
	heroWalkRects[1]  = V4(746.0f, 1018.0f, 804.0f, 920.0f);
	heroWalkRects[2]  = V4(849.0f, 1018.0f, 904.0f, 920.0f);
	addAnim(hero, heroWalkRects, numRects, duration);

	Texture *heroSheet = hero->tex;
	v2 sheetSize = V2(CAST(f32) heroSheet->width, CAST(f32) heroSheet->height);
	if (sheetSize.x != sheetSize.y)
	{
		printf(
		    "worldTraveller_gameInit() warning: Sprite sheet is not square: "
		    "%dx%dpx\n",
		    CAST(i32) sheetSize.w, CAST(i32) sheetSize.h);
	}

	/* Create a NPC */
	pos         = V2(300.0f, 300.0f);
	size        = hero->size;
	type        = entitytype_hero;
	dir         = direction_null;
	tex         = hero->tex;
	collides    = TRUE;
	Entity *npc = addEntity(world, pos, size, type, dir, tex, collides);

	/* Add npc waving animation */
	duration  = 0.30f;
	numRects  = 2;
	v4 *npcWavingRects = CAST(v4 *) calloc(numRects, sizeof(v4));
	npcWavingRects[0]  = V4(944.0f, 918.0f, 1010.0f, 816.0f);
	npcWavingRects[1]  = V4(944.0f, 812.0f, 1010.0f, 710.0f);
	addAnim(npc, npcWavingRects, numRects, duration);

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

	World *const world = &state->world[state->currWorldIndex];
	Entity *hero = &world->entities[world->heroIndex];
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

	// NOTE(doyle): Clipping threshold for snapping velocity to 0
	f32 epsilon    = 15.0f;
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
			EntityAnim *currAnim    = &hero->anim[hero->currAnimIndex];
			currAnim->currDuration  = currAnim->duration;
			currAnim->currRectIndex = 0;
			hero->currAnimIndex     = 0;
		}
	}
	else if (hero->currAnimIndex == 0)
	{
		EntityAnim *currAnim    = &hero->anim[hero->currAnimIndex];
		currAnim->currDuration  = currAnim->duration;
		currAnim->currRectIndex = 0;
		hero->currAnimIndex     = 1;
	}

	f32 heroSpeed = CAST(f32)(22.0f * METERS_TO_PIXEL); // m/s^2
	if (state->keys[GLFW_KEY_LEFT_SHIFT])
	{
		heroSpeed = CAST(f32)(22.0f * 10.0f * METERS_TO_PIXEL);
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

	// TODO(doyle): Only check collision for entities within small bounding box
	// of the hero
	b32 heroCollided = FALSE;
	if (hero->collides == TRUE)
	{
		for (i32 i = 0; i < world->maxEntities; i++)
		{
			if (i == world->heroIndex) continue;
			Entity entity = world->entities[i];
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

		v2 offsetFromHeroToOrigin =
		    V2((hero->pos.x - (0.5f * state->renderer.size.w)), (0.0f));

		// NOTE(doyle): Hero position is offset to the center so -recenter it
		offsetFromHeroToOrigin.x += (hero->size.x * 0.5f);
		world->cameraPos = offsetFromHeroToOrigin;
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

	/* Render entities */
	ASSERT(world->freeEntityIndex < world->maxEntities);
	v4 cameraBounds = getRect(world->cameraPos, renderer->size);

	// NOTE(doyle): Lock camera if it passes the bounds of the world
	if (cameraBounds.x <= world->bounds.x)
	{
		cameraBounds.x = world->bounds.x;
		cameraBounds.z = cameraBounds.x + renderer->size.w;
	}

	// TODO(doyle): Do the Y component when we need it
	if (cameraBounds.y >= world->bounds.y) cameraBounds.y = world->bounds.y;

	if (cameraBounds.z >= world->bounds.z)
	{
		cameraBounds.z = world->bounds.z;
		cameraBounds.x = cameraBounds.z - renderer->size.w;
	}

	if (cameraBounds.w <= world->bounds.w) cameraBounds.w = world->bounds.w;

	for (i32 i = 0; i < world->freeEntityIndex; i++)
	{
		Entity *const entity = &world->entities[i];
		renderer_entity(&state->renderer, cameraBounds, entity, dt, 0.0f,
		                V3(0, 0, 0));
	}

	// TODO(doyle): Clean up lines
	// Renderer::~Renderer() { glDeleteVertexArrays(1, &this->quadVAO); }

#ifdef WT_DEBUG
	LOCAL_PERSIST f32 debugUpdateCounter     = 0.0f;
	LOCAL_PERSIST char debugStrings[256][64] = {0};
	LOCAL_PERSIST i32 numDebugStrings        = 0;

	Font *font = &assetManager->font;
	if (debugUpdateCounter <= 0)
	{
		numDebugStrings    = 0;
		Entity *const hero = &world->entities[world->heroIndex];
		snprintf(debugStrings[0], ARRAY_COUNT(debugStrings[0]),
		         "Hero Pos: %06.2f,%06.2f", hero->pos.x, hero->pos.y);
		numDebugStrings++;

		snprintf(debugStrings[1], ARRAY_COUNT(debugStrings[1]),
		         "Hero dPos: %06.2f,%06.2f", hero->dPos.x, hero->dPos.y);
		numDebugStrings++;

		snprintf(debugStrings[2], ARRAY_COUNT(debugStrings[2]),
		         "FreeEntityIndex: %d", world->freeEntityIndex);
		numDebugStrings++;

		const f32 debugUpdateRate = 0.15f;
		debugUpdateCounter        = debugUpdateRate;
	}

	for (i32 i = 0; i < numDebugStrings; i++)
		renderer_debugString(&state->renderer, font, debugStrings[i]);

	debugUpdateCounter -= dt;
	debugRenderer.init = FALSE;
#endif
}
