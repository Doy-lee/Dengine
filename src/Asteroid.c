#include "Dengine/Asteroid.h"
#include "Dengine/Debug.h"

void initAssetManager(GameState *state)
{
	AssetManager *assetManager = &state->assetManager;
	MemoryArena_ *arena        = &state->persistentArena;

	i32 texAtlasEntries         = 8;
	assetManager->texAtlas.size = texAtlasEntries;
	assetManager->texAtlas.entries =
	    memory_pushBytes(arena, texAtlasEntries * sizeof(HashTableEntry));

	i32 animEntries          = 1024;
	assetManager->anims.size = animEntries;
	assetManager->anims.entries =
	    memory_pushBytes(arena, animEntries * sizeof(HashTableEntry));

	{ // Init texture assets
		i32 texEntries              = 32;
		assetManager->textures.size = texEntries;
		assetManager->textures.entries =
		    memory_pushBytes(arena, texEntries * sizeof(HashTableEntry));

		/* Create empty 1x1 4bpp black texture */
		u32 bitmap   = (0xFF << 24) | (0xFF << 16) | (0xFF << 8) | (0xFF << 0);
		Texture *tex = asset_getFreeTexSlot(assetManager, arena, "nullTex");
		*tex         = texture_gen(1, 1, 4, CAST(u8 *)(&bitmap));

		i32 result = asset_loadTTFont(assetManager, arena,
		                              "C:/Windows/Fonts/Arialbd.ttf");
	}

	{ // Init shaders assets
		asset_loadShaderFiles(
		    assetManager, arena, "data/shaders/default_tex.vert.glsl",
		    "data/shaders/default_tex.frag.glsl", shaderlist_default);

		asset_loadShaderFiles(
		    assetManager, arena, "data/shaders/default_no_tex.vert.glsl",
		    "data/shaders/default_no_tex.frag.glsl", shaderlist_default_no_tex);
	}

	{ // Init audio assets

		i32 audioEntries         = 32;
		assetManager->audio.size = audioEntries;
		assetManager->audio.entries =
		    memory_pushBytes(arena, audioEntries * sizeof(HashTableEntry));

		i32 result = asset_loadVorbis(assetManager, arena,
		                              "data/audio/Asteroids/bang_large.ogg",
		                              "bang_large");
		ASSERT(!result);
		result = asset_loadVorbis(assetManager, arena,
		                          "data/audio/Asteroids/bang_medium.ogg",
		                          "bang_medium");
		ASSERT(!result);
		result = asset_loadVorbis(assetManager, arena,
		                          "data/audio/Asteroids/bang_small.ogg",
		                          "bang_small");
		ASSERT(!result);
		result = asset_loadVorbis(assetManager, arena,
		                          "data/audio/Asteroids/beat1.ogg", "beat1");
		ASSERT(!result);
		result = asset_loadVorbis(assetManager, arena,
		                          "data/audio/Asteroids/beat2.ogg", "beat2");
		ASSERT(!result);
		result = asset_loadVorbis(assetManager, arena,
		                          "data/audio/Asteroids/extra_ship.ogg",
		                          "extra_ship");
		ASSERT(!result);
		result = asset_loadVorbis(assetManager, arena,
		                          "data/audio/Asteroids/fire.ogg", "fire");
		ASSERT(!result);
		result = asset_loadVorbis(assetManager, arena,
		                          "data/audio/Asteroids/saucer_big.ogg",
		                          "saucer_big");
		ASSERT(!result);
		result = asset_loadVorbis(assetManager, arena,
		                          "data/audio/Asteroids/saucer_small.ogg",
		                          "saucer_small");
		ASSERT(!result);
		result = asset_loadVorbis(assetManager, arena,
		                          "data/audio/Asteroids/thrust.ogg", "thrust");
		ASSERT(!result);
	}
}

void initRenderer(GameState *state, v2 windowSize)
{
	AssetManager *assetManager = &state->assetManager;
	Renderer *renderer         = &state->renderer;
	renderer->size             = windowSize;

	// NOTE(doyle): Value to map a screen coordinate to NDC coordinate
	renderer->vertexNdcFactor =
	    V2(1.0f / renderer->size.w, 1.0f / renderer->size.h);
	renderer->groupIndexForVertexBatch = -1;

	const mat4 projection =
	    mat4_ortho(0.0f, renderer->size.w, 0.0f, renderer->size.h, 0.0f, 1.0f);
	for (i32 i = 0; i < shaderlist_count; i++)
	{
		renderer->shaderList[i] = asset_getShader(assetManager, i);
		shader_use(renderer->shaderList[i]);
		shader_uniformSetMat4fv(renderer->shaderList[i], "projection",
		                        projection);
		GL_CHECK_ERROR();
	}

	renderer->activeShaderId = renderer->shaderList[shaderlist_default];
	GL_CHECK_ERROR();

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
		renderer->groups[i].vertexList =
		    memory_pushBytes(&state->persistentArena,
		                     renderer->groupCapacity * sizeof(RenderVertex));
	}
}

#include <stdlib.h>
#include <time.h>
v2 *createAsteroidVertexList(MemoryArena_ *arena, i32 iterations,
                             i32 asteroidRadius)
{
	f32 iterationAngle = 360.0f / iterations;
	iterationAngle     = DEGREES_TO_RADIANS(iterationAngle);
	v2 *result         = memory_pushBytes(arena, iterations * sizeof(v2));

	for (i32 i = 0; i < iterations; i++)
	{
		i32 randValue = rand();
		
		// NOTE(doyle): Sin/cos generate values from +-1, we want to create
		// vertices that start from 0, 0 (i.e. strictly positive)
		result[i] = V2(((math_cosf(iterationAngle * i) + 1) * asteroidRadius),
		               ((math_sinf(iterationAngle * i) + 1) * asteroidRadius));

		ASSERT(result[i].x >= 0 && result[i].y >= 0);

#if 1
		f32 displacementDist   = 0.50f * asteroidRadius;
		i32 vertexDisplacement =
		    randValue % (i32)displacementDist + (i32)(displacementDist * 0.25f);

		i32 quadrantSize = iterations / 4;

		i32 firstQuadrant  = quadrantSize;
		i32 secondQuadrant = quadrantSize * 2;
		i32 thirdQuadrant  = quadrantSize * 3;
		i32 fourthQuadrant = quadrantSize * 4;

		if (i < firstQuadrant)
		{
			result[i].x += vertexDisplacement;
			result[i].y += vertexDisplacement;
		}
		else if (i < secondQuadrant)
		{
			result[i].x -= vertexDisplacement;
			result[i].y += vertexDisplacement;
		}
		else if (i < thirdQuadrant)
		{
			result[i].x -= vertexDisplacement;
			result[i].y -= vertexDisplacement;
		}
		else
		{
			result[i].x += vertexDisplacement;
			result[i].y -= vertexDisplacement;
		}
#endif
	}

	return result;
}

v2 *createNormalEdgeList(MemoryArena_ *transientArena, v2 *vertexList,
                         i32 vertexListSize)
{
	v2 *result = memory_pushBytes(transientArena, sizeof(v2) * vertexListSize);
	for (i32 i = 0; i < vertexListSize - 1; i++)
	{
		ASSERT((i + 1) < vertexListSize);
		result[i] = v2_sub(vertexList[i + 1], vertexList[i]);
		result[i] = v2_perpendicular(result[i]);
	}

	// NOTE(doyle): Creating the last edge requires using the first
	// vertex point which is at index 0
	result[vertexListSize - 1] =
	    v2_sub(vertexList[0], vertexList[vertexListSize - 1]);
	result[vertexListSize - 1] = v2_perpendicular(result[vertexListSize - 1]);

	return result;
}

v2 calculateProjectionRangeForEdge(v2 *vertexList, i32 vertexListSize,
                                   v2 edgeNormal)
{
	v2 result = {0};
	result.min = v2_dot(vertexList[0], edgeNormal);
	result.max = result.min;

	for (i32 vertexIndex = 0; vertexIndex < vertexListSize; vertexIndex++)
	{
		f32 dist = v2_dot(vertexList[vertexIndex], edgeNormal);

		if (dist < result.min)
			result.min = dist;
		else if (dist > result.max)
			result.max = dist;
	}

	return result;
}

b32 checkEdgeProjectionOverlap(v2 *vertexList, i32 listSize,
                               v2 *checkVertexList, i32 checkListSize,
                               v2 *edgeList, i32 totalNumEdges)
{
	b32 result = TRUE;
	for (i32 edgeIndex = 0; edgeIndex < totalNumEdges && result; edgeIndex++)
	{
		v2 projectionRange = calculateProjectionRangeForEdge(
		    vertexList, listSize, edgeList[edgeIndex]);

		v2 checkProjectionRange = calculateProjectionRangeForEdge(
		    checkVertexList, checkListSize, edgeList[edgeIndex]);

		if (!v2_intervalsOverlap(projectionRange, checkProjectionRange))
		{
			result = FALSE;
			return result;
		}
	}

	return result;
}

INTERNAL u32 moveEntity(World *world, MemoryArena_ *transientArena,
                        Entity *entity, i32 entityIndex, v2 ddP, f32 dt,
                        f32 ddPSpeed)
{
	ASSERT(ABS(ddP.x) <= 1.0f && ABS(ddP.y) <= 1.0f);
	/*
	    Assuming acceleration A over t time, then integrate twice to get

	    newVelocity = a*t + oldVelocity
	    newPos = (a*t^2)/2 + oldVelocity*t + oldPos
	*/

	if (ddP.x > 0.0f && ddP.y > 0.0f)
	{
		// NOTE(doyle): Cheese it and pre-compute the vector for
		// diagonal using pythagoras theorem on a unit triangle 1^2
		// + 1^2 = c^2
		ddP = v2_scale(ddP, 0.70710678118f);
	}

	ddP           = v2_scale(ddP, world->pixelsPerMeter * ddPSpeed);
	v2 oldDp      = entity->dP;
	v2 resistance = v2_scale(oldDp, 2.0f);
	ddP           = v2_sub(ddP, resistance);

	v2 newDp = v2_add(v2_scale(ddP, dt), oldDp);

	v2 ddPHalf          = v2_scale(ddP, 0.5f);
	v2 ddPHalfDtSquared = v2_scale(ddPHalf, (SQUARED(dt)));
	v2 oldDpDt          = v2_scale(oldDp, dt);
	v2 oldPos           = entity->pos;

	v2 newPos = v2_add(v2_add(ddPHalfDtSquared, oldDpDt), oldPos);

	i32 collisionIndex = -1;
	// TODO(doyle): Collision for rects, (need to create vertex list for it)
	for (i32 i = 1; i < world->entityIndex; i++)
	{
		if (i == entityIndex) continue;

		Entity *checkEntity = &world->entityList[i];
		ASSERT(checkEntity->id != entity->id);

		if (world->collisionTable[entity->type][checkEntity->type])
		{
			ASSERT(entity->vertexPoints);
			ASSERT(checkEntity->vertexPoints);

			/* Create entity edge lists */
			v2 *entityVertexListOffsetToP = entity_generateUpdatedVertexList(
			    transientArena, entity);

			v2 *checkEntityVertexListOffsetToP =
			    entity_generateUpdatedVertexList(transientArena,
			                                     checkEntity);

			v2 *entityEdgeList = createNormalEdgeList(transientArena,
			                                          entityVertexListOffsetToP,
			                                          entity->numVertexPoints);

			v2 *checkEntityEdgeList = createNormalEdgeList(
			    transientArena, checkEntityVertexListOffsetToP,
			    checkEntity->numVertexPoints);

			/* Combine both edge lists into one */
			i32 totalNumEdges =
			    checkEntity->numVertexPoints + entity->numVertexPoints;
			v2 *edgeList =
			    memory_pushBytes(transientArena, totalNumEdges * sizeof(v2));
			for (i32 i = 0; i < entity->numVertexPoints; i++)
			{
				edgeList[i] = entityEdgeList[i];
			}

			for (i32 i = 0; i < checkEntity->numVertexPoints; i++)
			{
				edgeList[i + entity->numVertexPoints] = checkEntityEdgeList[i];
			}

			if (checkEdgeProjectionOverlap(
			        entityVertexListOffsetToP, entity->numVertexPoints,
			        checkEntityVertexListOffsetToP,
			        checkEntity->numVertexPoints, edgeList, totalNumEdges))
			{
				collisionIndex = i;
			}
		}

		if (collisionIndex != -1) break;
	}

	entity->dP  = newDp;
	entity->pos = newPos;

	return collisionIndex;
}

enum AsteroidSize
{
	asteroidsize_small,
	asteroidsize_medium,
	asteroidsize_large,
	asteroidsize_count,
};

typedef struct {
	v2 pos;
	v2 dP;
} AsteroidSpec;

INTERNAL void addAsteroidWithSpec(World *world, enum AsteroidSize asteroidSize,
                                  AsteroidSpec *spec)
{
	world->asteroidCounter++;

	enum EntityType type;
	v2 size;
	v2 **vertexCache = NULL;

	if (asteroidSize == asteroidsize_small)
	{
		size        = V2i(25, 25);
		type        = entitytype_asteroid_small;
		vertexCache = world->asteroidSmallVertexCache;
	}
	else if (asteroidSize == asteroidsize_medium)
	{
		size        = V2i(50, 50);
		type        = entitytype_asteroid_medium;
		vertexCache = world->asteroidMediumVertexCache;
	}
	else if (asteroidSize == asteroidsize_large)
	{
		type        = entitytype_asteroid_large;
		size        = V2i(100, 100);
		vertexCache = world->asteroidLargeVertexCache;
	}
	else
	{
		ASSERT(INVALID_CODE_PATH);
	}

	Entity *asteroid = &world->entityList[world->entityIndex++];
	asteroid->id     = world->entityIdCounter++;

	i32 randValue = rand();
	if (!spec)
	{
		i32 randX = (randValue % (i32)world->worldSize.w);
		i32 randY = (randValue % (i32)world->worldSize.h);

		v2 midpoint = v2_scale(world->worldSize, 0.5f);

		Rect topLeftQuadrant = {V2(0, midpoint.y),
		                        V2(midpoint.x, world->worldSize.y)};
		Rect botLeftQuadrant  = {V2(0, 0), midpoint};
		Rect topRightQuadrant = {midpoint, world->worldSize};
		Rect botRightQuadrant = {V2(midpoint.x, 0),
		                         V2(world->worldSize.x, midpoint.y)};

		// NOTE(doyle): Off-screen so asteroids "float" into view. There's no
		// particular order, just pushing things offscreen when they get
		// generated
		// to float back into game space
		v2 newP = V2i(randX, randY);
		if (math_pointInRect(topLeftQuadrant, newP))
		{
			newP.y += midpoint.y;
		}
		else if (math_pointInRect(botLeftQuadrant, newP))
		{
			newP.x -= midpoint.x;
		}
		else if (math_pointInRect(topRightQuadrant, newP))
		{
			newP.y -= midpoint.y;
		}
		else if (math_pointInRect(botRightQuadrant, newP))
		{
			newP.x += midpoint.x;
		}
		else
		{
			ASSERT(INVALID_CODE_PATH);
		}
		asteroid->pos       = newP;
	}
	else
	{
		asteroid->pos       = spec->pos;
		asteroid->dP        = spec->dP;
	}

	asteroid->size            = size;
	asteroid->hitbox          = asteroid->size;
	asteroid->offset          = v2_scale(asteroid->size, -0.5f);
	asteroid->type            = type;
	asteroid->renderMode      = rendermode_polygon;
	asteroid->numVertexPoints = 10;

	i32 cacheIndex = randValue % ARRAY_COUNT(world->asteroidSmallVertexCache);
	ASSERT(ARRAY_COUNT(world->asteroidSmallVertexCache) ==
	       ARRAY_COUNT(world->asteroidMediumVertexCache));
	ASSERT(ARRAY_COUNT(world->asteroidSmallVertexCache) ==
	       ARRAY_COUNT(world->asteroidLargeVertexCache));

	if (!vertexCache[cacheIndex])
	{
		vertexCache[cacheIndex] = createAsteroidVertexList(
		    &world->entityArena, asteroid->numVertexPoints,
		    (i32)(asteroid->size.w * 0.5f));
	}

	asteroid->vertexPoints = vertexCache[cacheIndex];
	asteroid->color = V4(0.0f, 0.5f, 0.5f, 1.0f);
}

INTERNAL void addAsteroid(World *world, enum AsteroidSize asteroidSize)
{
	addAsteroidWithSpec(world, asteroidSize, NULL);
}

INTERNAL void addBullet(World *world, Entity *shooter)
{
	Entity *bullet = &world->entityList[world->entityIndex++];
	bullet->id     = world->entityIdCounter++;

	bullet->pos        = shooter->pos;
	bullet->size       = V2(2.0f, 20.0f);
	bullet->offset     = v2_scale(bullet->size, -0.5f);
	bullet->hitbox     = bullet->size;
	bullet->rotation   = shooter->rotation;
	bullet->renderMode = rendermode_polygon;

	if (!world->bulletVertexCache)
	{
		world->bulletVertexCache =
		    MEMORY_PUSH_ARRAY(&world->entityArena, 4, v2);
		world->bulletVertexCache[0] = V2(0, bullet->size.h);
		world->bulletVertexCache[1] = V2(0, 0);
		world->bulletVertexCache[2] = V2(bullet->size.w, 0);
		world->bulletVertexCache[3] = bullet->size;
	}

	bullet->vertexPoints    = world->bulletVertexCache;
	bullet->numVertexPoints = 4;

	bullet->type  = entitytype_bullet;
	bullet->color = V4(1.0f, 1.0f, 0, 1.0f);
}

INTERNAL void setCollisionRule(World *world, enum EntityType a,
                               enum EntityType b, b32 rule)
{
	ASSERT(a <= entitytype_count);
	ASSERT(b <= entitytype_count);
	world->collisionTable[a][b] = rule;
	world->collisionTable[b][a] = rule;
}

INTERNAL AudioRenderer *getFreeAudioRenderer(World *world)
{
	for (i32 i = 0; i < world->numAudioRenderers; i++)
	{
		AudioRenderer *renderer = &world->audioRenderer[i];
		if (renderer->state == audiostate_stopped)
		{
			return renderer;
		}
	}

	return NULL;
}

INTERNAL void deleteEntity(World *world, i32 entityIndex)
{
	ASSERT(entityIndex > 0);
	ASSERT(entityIndex < ARRAY_COUNT(world->entityList));

	/* Last entity replaces the entity to delete */
	world->entityList[entityIndex] = world->entityList[world->entityIndex - 1];

	/* Make sure the replaced entity from end of list is cleared out */
	Entity emptyEntity = {0};
	world->entityList[--world->entityIndex] = emptyEntity;
}

void asteroid_gameUpdateAndRender(GameState *state, Memory *memory,
                                  v2 windowSize, f32 dt)
{
	MemoryIndex globalTransientArenaSize =
	    (MemoryIndex)((f32)memory->transientSize * 0.5f);
	memory_arenaInit(&state->transientArena, memory->transient,
	                 globalTransientArenaSize);

	World *world = &state->world;
	if (!state->init)
	{
		srand((u32)time(NULL));
		initAssetManager(state);
		initRenderer(state, windowSize);
		audio_init(&state->audioManager);

		world->pixelsPerMeter = 70.0f;

		MemoryIndex entityArenaSize =
		    (MemoryIndex)((f32)memory->transientSize * 0.5f);

		u8 *arenaBase = state->transientArena.base + state->transientArena.size;
		memory_arenaInit(&world->entityArena, arenaBase, entityArenaSize);

		{ // Init null entity
			Entity *nullEntity = &world->entityList[world->entityIndex++];
			nullEntity->id     = world->entityIdCounter++;
		}

		{ // Init asteroid entities
			world->numAsteroids = 15;
		}

		{ // Init audio renderer
			world->numAudioRenderers = 6;
			world->audioRenderer     = MEMORY_PUSH_ARRAY(
			    &world->entityArena, world->numAudioRenderers, AudioRenderer);
		}

		{ // Init ship entity
			Entity *ship     = &world->entityList[world->entityIndex++];
			ship->id         = world->entityIdCounter++;
			ship->pos        = V2(100, 100);
			ship->size       = V2(25.0f, 50.0f);
			ship->hitbox     = ship->size;
			ship->offset     = v2_scale(ship->size, -0.5f);

			ship->numVertexPoints = 3;
			ship->vertexPoints    = memory_pushBytes(
			    &world->entityArena, sizeof(v2) * ship->numVertexPoints);

			v2 triangleBaseP  = V2(0, 0);
			v2 triangleTopP   = V2(ship->size.w * 0.5f, ship->size.h);
			v2 triangleRightP = V2(ship->size.w, triangleBaseP.y);

			ship->vertexPoints[0] = triangleBaseP;
			ship->vertexPoints[1] = triangleRightP;
			ship->vertexPoints[2] = triangleTopP;

			ship->scale      = 1;
			ship->type       = entitytype_ship;
			ship->renderMode = rendermode_polygon;
			ship->color      = V4(1.0f, 0.5f, 0.5f, 1.0f);
		}

		{ // Global Collision Rules
			setCollisionRule(world, entitytype_ship, entitytype_asteroid_small,
			                 TRUE);
			setCollisionRule(world, entitytype_ship, entitytype_asteroid_medium,
			                 TRUE);
			setCollisionRule(world, entitytype_ship, entitytype_asteroid_large,
			                 TRUE);
			setCollisionRule(world, entitytype_bullet,
			                 entitytype_asteroid_small, TRUE);
			setCollisionRule(world, entitytype_bullet,
			                 entitytype_asteroid_medium, TRUE);
			setCollisionRule(world, entitytype_bullet,
			                 entitytype_asteroid_large, TRUE);
		}

		world->camera.min = V2(0, 0);
		world->camera.max = state->renderer.size;
		world->worldSize  = windowSize;

		state->init       = TRUE;

		debug_init(&state->persistentArena, windowSize,
		           state->assetManager.font);
	}

	for (u32 i = world->asteroidCounter; i < world->numAsteroids; i++)
		addAsteroid(world, (rand() % asteroidsize_count));

	platform_processInputBuffer(&state->input, dt);

	if (platform_queryKey(&state->input.keys[keycode_left_square_bracket],
	                 readkeytype_repeat, 0.2f))
	{
		addAsteroid(world, (rand() % asteroidsize_count));
	}

	ASSERT(world->entityList[0].id == NULL_ENTITY_ID);
	for (i32 i = 1; i < world->entityIndex; i++)
	{
		Entity *entity = &world->entityList[i];
		ASSERT(entity->type != entitytype_invalid);

		v2 pivotPoint    = {0};
		f32 ddPSpeedInMs = 0;
		v2 ddP           = {0};
		if (entity->type == entitytype_ship)
		{
			if (platform_queryKey(&state->input.keys[keycode_up],
			                      readkeytype_repeat, 0.0f))
			{
				// TODO(doyle): Renderer creates upfacing triangles by default,
				// but we need to offset rotation so that our base "0 degrees"
				// is right facing for trig to work
				Radians rotation =
				    DEGREES_TO_RADIANS((entity->rotation + 90.0f));
				v2 direction = V2(math_cosf(rotation), math_sinf(rotation));
				ddP          = direction;
			}

			if (platform_queryKey(&state->input.keys[keycode_space],
			                      readkeytype_one_shot, KEY_DELAY_NONE))
			{
				addBullet(world, entity);

				AudioRenderer *audioRenderer = getFreeAudioRenderer(world);
				if (audioRenderer)
				{
					AudioVorbis *fire =
					    asset_getVorbis(&state->assetManager, "fire");
					// TODO(doyle): Atm transient arena is not used, this is
					// just to fill out the arguments
					audio_playVorbis(&state->transientArena,
					                 &state->audioManager, audioRenderer, fire,
					                 1);
				}
			}

			Degrees rotationsPerSecond = 180.0f;
			if (platform_queryKey(&state->input.keys[keycode_left],
			                      readkeytype_repeat, 0.0f))
			{
				entity->rotation += (rotationsPerSecond)*dt;
			}

			if (platform_queryKey(&state->input.keys[keycode_right],
			                      readkeytype_repeat, 0.0f))
			{
				entity->rotation -= (rotationsPerSecond)*dt;
			}
			entity->rotation = (f32)((i32)entity->rotation);

			ddPSpeedInMs = 25;
			DEBUG_PUSH_VAR("Pos: %5.2f, %5.2f", entity->pos, "v2");
			DEBUG_PUSH_VAR("Velocity: %5.2f, %5.2f", entity->dP, "v2");
			DEBUG_PUSH_VAR("Rotation: %5.2f", entity->rotation, "f32");

			renderer_rect(&state->renderer, world->camera, entity->pos,
			              V2(5, 5), V2(0, 0),
			              DEGREES_TO_RADIANS(entity->rotation), NULL,
			              V4(1.0f, 1.0f, 1.0f, 1.0f), renderflag_no_texture);
		}
		else if (entity->type >= entitytype_asteroid_small &&
		         entity->type <= entitytype_asteroid_large)
		{

			i32 randValue = rand();

			// NOTE(doyle): If it is a new asteroid with no dp set, we need to
			// set a initial dp for it to move from.
			v2 localDp = {0};
			if ((i32)entity->dP.x == 0 && (i32)entity->dP.y == 0)
			{
				enum Direction direction = randValue % direction_count;
				switch (direction)
				{
				case direction_north:
				case direction_northwest:
				{
					localDp.x = 1.0f;
					localDp.y = 1.0f;
				}
				break;

				case direction_west:
				case direction_southwest:
				{
					localDp.x = -1.0f;
					localDp.y = -1.0f;
				}
				break;

				case direction_south:
				case direction_southeast:
				{
					localDp.x = 1.0f;
					localDp.y = -1.0f;
				}
				break;

				case direction_east:
				case direction_northeast:
				{
					localDp.x = 1.0f;
					localDp.y = 1.0f;
				}
				break;

				default:
				{
					ASSERT(INVALID_CODE_PATH);
				}
				break;
				}
			}
			// NOTE(doyle): Otherwise, if it has pre-existing dp, maintain our
			// direction by extrapolating from it's current dp
			else
			{
				if (entity->dP.x >= 0) localDp.x = 1.0f;
				else localDp.x = -1.0f;

				if (entity->dP.y >= 0) localDp.y = 1.0f;
				else localDp.y = -1.0f;
			}

			/*
			   NOTE(doyle): We compare current dP with the calculated dP. In the
			   event we want to artificially boost the asteroid, we set a higher
			   dP on creation, which will have a higher dP than the default dP
			   we calculate. So here we choose to keep it until it decays enough
			   that the default dP of the asteroid is accepted.
			 */
			v2 newDp     = v2_scale(localDp, world->pixelsPerMeter * 1.5f);
			f32 newDpSum = ABS(newDp.x) + ABS(newDp.y);
			f32 oldDpSum = ABS(entity->dP.x) + ABS(entity->dP.y);

			if (newDpSum > oldDpSum)
			{
				entity->dP = newDp;
			}
		}
		else if (entity->type == entitytype_bullet)
		{
			if (!math_pointInRect(world->camera, entity->pos))
			{
				deleteEntity(world, i--);
				continue;
			}

			Radians rotation = DEGREES_TO_RADIANS((entity->rotation + 90.0f));
			v2 localDp       = V2(math_cosf(rotation), math_sinf(rotation));
			entity->dP       = v2_scale(localDp, world->pixelsPerMeter * 5);
		}
		else if (entity->type == entitytype_particle)
		{
			f32 diff = entity->color.a - 0.1f;
			if (diff < 0.01f)
			{
				deleteEntity(world, i--);
				continue;
			}

			f32 divisor =
			    MAX(entity->particleInitDp.x, entity->particleInitDp.y);
			f32 maxDp   = MAX(entity->dP.x, entity->dP.y);

			entity->color.a = maxDp / divisor;

		}

		/* Loop entity around world */
		if (entity->pos.y >= world->worldSize.h)
			entity->pos.y = 0;
		else if (entity->pos.y < 0)
			entity->pos.y = world->worldSize.h;

		if (entity->pos.x >= world->worldSize.w)
			entity->pos.x = 0;
		else if (entity->pos.x < 0)
			entity->pos.x = world->worldSize.w;

		i32 collisionIndex = moveEntity(world, &state->transientArena, entity,
		                                i, ddP, dt, ddPSpeedInMs);

		v4 collideColor = {0};
		if (collisionIndex != -1)
		{
			ASSERT(collisionIndex < world->entityIndex);

			Entity *collideEntity = &world->entityList[collisionIndex];

			Entity *colliderA;
			Entity *colliderB;

			if (collideEntity->type < entity->type)
			{
				colliderA = collideEntity;
				colliderB = entity;
			}
			else
			{
				colliderA = entity;
				colliderB = collideEntity;
			}

			if (colliderA->type >= entitytype_asteroid_small &&
			    colliderA->type <= entitytype_asteroid_large)
			{

				f32 numParticles = 4;
				if (colliderA->type == entitytype_asteroid_medium)
				{
					AsteroidSpec spec = {0};
					spec.pos          = colliderA->pos;
					spec.dP           = v2_scale(colliderA->dP, -2.0f);
					addAsteroidWithSpec(world, asteroidsize_small, &spec);

					numParticles = 8;
				}
				else if (colliderA->type == entitytype_asteroid_large)
				{
					AsteroidSpec spec = {0};
					spec.pos          = colliderA->pos;
					spec.dP           = v2_scale(colliderA->dP, -4.0f);
					addAsteroidWithSpec(world, asteroidsize_medium, &spec);

					spec.dP        = v2_perpendicular(spec.dP);
					addAsteroidWithSpec(world, asteroidsize_small, &spec);

					spec.dP        = v2_perpendicular(colliderA->dP);
					addAsteroidWithSpec(world, asteroidsize_small, &spec);

					numParticles = 16;
				}

				for (i32 i = 0; i < numParticles; i++)
				{
					{ // Add particles
						Entity *particle =
						    &world->entityList[world->entityIndex++];
						particle->id = world->entityIdCounter++;

						particle->pos        = colliderA->pos;
						particle->size       = V2(4.0f, 4.0f);

						i32 randValue = rand();
						Radians rotation =
						    DEGREES_TO_RADIANS((randValue % 360));
						v2 randDirectionVec =
						    V2(math_cosf(rotation), math_sinf(rotation));

						i32 particleDpLimit = 8;
						f32 randDpMultiplier =
						    (f32)(randValue % particleDpLimit) + 1;

						v2 newDp = v2_scale(colliderA->dP, randDpMultiplier);
						newDp    = v2_hadamard(newDp, randDirectionVec);

						particle->dP             = newDp;
						particle->particleInitDp = newDp;

						particle->offset     = v2_scale(particle->size, -0.5f);
						particle->hitbox     = particle->size;
						particle->rotation   = 0;
						particle->renderMode = rendermode_polygon;

						if (!world->particleVertexCache)
						{
							world->particleVertexCache =
							    MEMORY_PUSH_ARRAY(&world->entityArena, 4, v2);
							world->particleVertexCache[0] =
							    V2(0, particle->size.h);
							world->particleVertexCache[1] = V2(0, 0);
							world->particleVertexCache[2] =
							    V2(particle->size.w, 0);
							world->particleVertexCache[3] = particle->size;
						}

						particle->vertexPoints    = world->particleVertexCache;
						particle->numVertexPoints = 4;

						particle->type  = entitytype_particle;
						particle->color = V4(1.0f, 0.0f, 0, 1.0f);
					}
				}

				ASSERT(colliderB->type == entitytype_bullet);

				deleteEntity(world, collisionIndex);

				deleteEntity(world, i--);
				world->asteroidCounter--;

				ASSERT(world->asteroidCounter >= 0);

				AudioRenderer *audioRenderer = getFreeAudioRenderer(world);
				if (audioRenderer)
				{
					char *sound;
					i32 choice = rand() % 3;
					if (choice == 0)
					{
						sound = "bang_small";
					}
					else if (choice == 1)
					{
						sound = "bang_medium";
					}
					else
					{
						sound = "bang_large";
					}

					AudioVorbis *explode =
					    asset_getVorbis(&state->assetManager, sound);
					audio_playVorbis(&state->transientArena,
					                 &state->audioManager, audioRenderer,
					                 explode, 1);
				}

				continue;
			}
		}

		RenderFlags flags = renderflag_wireframe | renderflag_no_texture;
		renderer_entity(&state->renderer, &state->transientArena, world->camera,
		                entity, V2(0, 0), 0,
		                collideColor, flags);
	}

	for (i32 i = 0; i < world->numAudioRenderers; i++)
	{
		AudioRenderer *audioRenderer = &world->audioRenderer[i];
		audio_updateAndPlay(&state->transientArena, &state->audioManager,
		                    audioRenderer);
	}

#if 1
	debug_drawUi(state, dt);
	debug_clearCounter();
#endif

	renderer_renderGroups(&state->renderer);
}
