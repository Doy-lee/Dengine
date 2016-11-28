#include "Dengine/Asteroid.h"
#include "Dengine/Debug.h"

INTERNAL void loadGameAssets(GameState *state)
{
	AssetManager *assetManager = &state->assetManager;
	MemoryArena_ *arena        = &state->persistentArena;

	{ // Init font assets
#if 0
		i32 result =
		    asset_fontLoadTTF(assetManager, arena, &state->transientArena,
		                     "C:/Windows/Fonts/Arialbd.ttf", "Arial", 15);
#endif

		asset_fontLoadTTF(assetManager, arena, &state->transientArena,
		                  "F:/Workspace/Dropbox/Apps/Fonts/"
		                  "league-spartan-master/_webfonts/"
		                  "leaguespartan-bold.ttf",
		                  "Arial", 15);
	}

	{ // Init shaders assets
		asset_shaderLoad(
		    assetManager, arena, "data/shaders/default_tex.vert.glsl",
		    "data/shaders/default_tex.frag.glsl", shaderlist_default);

		asset_shaderLoad(
		    assetManager, arena, "data/shaders/default_no_tex.vert.glsl",
		    "data/shaders/default_no_tex.frag.glsl", shaderlist_default_no_tex);
	}

	{ // Init audio assets
		i32 result = asset_vorbisLoad(assetManager, arena,
		                              "data/audio/Asteroids/bang_large.ogg",
		                              "bang_large");
		ASSERT(!result);
		result = asset_vorbisLoad(assetManager, arena,
		                          "data/audio/Asteroids/bang_medium.ogg",
		                          "bang_medium");
		ASSERT(!result);
		result = asset_vorbisLoad(assetManager, arena,
		                          "data/audio/Asteroids/bang_small.ogg",
		                          "bang_small");
		ASSERT(!result);
		result = asset_vorbisLoad(assetManager, arena,
		                          "data/audio/Asteroids/beat1.ogg", "beat1");
		ASSERT(!result);
		result = asset_vorbisLoad(assetManager, arena,
		                          "data/audio/Asteroids/beat2.ogg", "beat2");
		ASSERT(!result);
		result = asset_vorbisLoad(assetManager, arena,
		                          "data/audio/Asteroids/extra_ship.ogg",
		                          "extra_ship");
		ASSERT(!result);
		result = asset_vorbisLoad(assetManager, arena,
		                          "data/audio/Asteroids/fire.ogg", "fire");
		ASSERT(!result);
		result = asset_vorbisLoad(assetManager, arena,
		                          "data/audio/Asteroids/saucer_big.ogg",
		                          "saucer_big");
		ASSERT(!result);
		result = asset_vorbisLoad(assetManager, arena,
		                          "data/audio/Asteroids/saucer_small.ogg",
		                          "saucer_small");
		ASSERT(!result);
		result = asset_vorbisLoad(assetManager, arena,
		                          "data/audio/Asteroids/thrust.ogg", "thrust");
		ASSERT(!result);
	}
}

#include <stdlib.h>
#include <time.h>
INTERNAL v2 *createAsteroidVertexList(MemoryArena_ *arena, i32 iterations,
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

INTERNAL v2 *createNormalEdgeList(MemoryArena_ *transientArena, v2 *vertexList,
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

INTERNAL v2 calculateProjectionRangeForEdge(v2 *vertexList, i32 vertexListSize,
                                            v2 edgeNormal)
{
	v2 result  = {0};
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

INTERNAL b32 checkEdgeProjectionOverlap(v2 *vertexList, i32 listSize,
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

INTERNAL u32 moveEntity(GameWorldState *world, MemoryArena_ *transientArena,
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

INTERNAL void addAsteroidWithSpec(GameWorldState *world,
                                  enum AsteroidSize asteroidSize,
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
		if (math_rectContainsP(topLeftQuadrant, newP))
		{
			newP.y += midpoint.y;
		}
		else if (math_rectContainsP(botLeftQuadrant, newP))
		{
			newP.x -= midpoint.x;
		}
		else if (math_rectContainsP(topRightQuadrant, newP))
		{
			newP.y -= midpoint.y;
		}
		else if (math_rectContainsP(botRightQuadrant, newP))
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
	asteroid->color = V4(1.0f, 1.0f, 1.0f, 1.0f);
}

INTERNAL void addAsteroid(GameWorldState *world, enum AsteroidSize asteroidSize)
{
	addAsteroidWithSpec(world, asteroidSize, NULL);
}

INTERNAL void addBullet(GameWorldState *world, Entity *shooter)
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

INTERNAL void setCollisionRule(GameWorldState *world, enum EntityType a,
                               enum EntityType b, b32 rule)
{
	ASSERT(a <= entitytype_count);
	ASSERT(b <= entitytype_count);
	world->collisionTable[a][b] = rule;
	world->collisionTable[b][a] = rule;
}

INTERNAL AudioRenderer *getFreeAudioRenderer(GameWorldState *world,
                                             AudioVorbis *vorbis,
                                             i32 maxSimultaneousPlayers)
{
	i32 freeIndex = -1;
	i32 sameAudioPlayingCount = 0;

	AudioRenderer *result = NULL;
	for (i32 i = 0; i < world->numAudioRenderers; i++)
	{
		AudioRenderer *renderer = &world->audioRenderer[i];
		if (renderer->state == audiostate_playing &&
		    common_strcmp(renderer->audio->key, vorbis->key) == 0)
		{
			sameAudioPlayingCount++;
		}
		else if (renderer->state == audiostate_stopped && freeIndex == -1)
		{
			freeIndex = i;
		}
	}

	if (sameAudioPlayingCount < maxSimultaneousPlayers && freeIndex != -1)
	{
		result = &world->audioRenderer[freeIndex];
	}

	return result;
}

INTERNAL void addPlayer(GameWorldState *world)
{
	Entity *ship = &world->entityList[world->entityIndex++];
	ship->id     = world->entityIdCounter++;
	ship->pos    = math_rectGetCentre(world->camera);
	ship->size   = V2(25.0f, 50.0f);
	ship->hitbox = ship->size;
	ship->offset = v2_scale(ship->size, -0.5f);

	ship->numVertexPoints = 3;
	ship->vertexPoints    = memory_pushBytes(&world->entityArena,
	                                      sizeof(v2) * ship->numVertexPoints);

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

INTERNAL void deleteEntity(GameWorldState *world, i32 entityIndex)
{
	ASSERT(entityIndex > 0);
	ASSERT(entityIndex < ARRAY_COUNT(world->entityList));

	/* Last entity replaces the entity to delete */
	world->entityList[entityIndex] = world->entityList[world->entityIndex - 1];

	/* Make sure the replaced entity from end of list is cleared out */
	Entity emptyEntity = {0};
	world->entityList[--world->entityIndex] = emptyEntity;
}

#define GET_STATE_DATA(state, arena, type)                                     \
	(type *)getStateData_(state, arena, appstate_##type)
INTERNAL void *getStateData_(GameState *state, MemoryArena_ *persistentArena,
                             enum AppState appState)
{
	void *result = NULL;
	switch (appState)
	{
	case appstate_StartMenuState:
	{
		if (!state->appStateData[appState])
		{
			state->appStateData[appState] =
			    MEMORY_PUSH_STRUCT(persistentArena, StartMenuState);
		}
	}
	break;

	case appstate_GameWorldState:
	{
		if (!state->appStateData[appState])
		{
			state->appStateData[appState] =
			    MEMORY_PUSH_STRUCT(persistentArena, GameWorldState);
		}
	}
	break;

	default:
	{
		ASSERT(INVALID_CODE_PATH);
	}
	break;
	}

	ASSERT(state->appStateData[appState]);
	result = state->appStateData[appState];
	return result;
}

INTERNAL void gameUpdate(GameState *state, Memory *memory, f32 dt)
{
	GameWorldState *world =
	    GET_STATE_DATA(state, &state->persistentArena, GameWorldState);

	if (!world->init)
	{
		world->pixelsPerMeter = 70.0f;

		MemoryIndex entityArenaSize =
		    (MemoryIndex)((f32)memory->transientSize * 0.5f);

		u8 *arenaBase = state->transientArena.base + state->transientArena.size;
		memory_arenaInit(&world->entityArena, arenaBase, entityArenaSize);

		world->camera.min               = V2(0, 0);
		world->camera.max               = state->renderer.size;
		world->worldSize                = state->renderer.size;

		{ // Init null entity
			Entity *nullEntity = &world->entityList[world->entityIndex++];
			nullEntity->id     = world->entityIdCounter++;
		}

		{ // Init asteroid entities
			world->numAsteroids = 15;
		}

		{ // Init audio renderer
			world->numAudioRenderers = 8;
			world->audioRenderer     = MEMORY_PUSH_ARRAY(
			    &world->entityArena, world->numAudioRenderers, AudioRenderer);
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

		world->numStarP = 100;
		world->starPList =
		    MEMORY_PUSH_ARRAY(&state->persistentArena, world->numStarP, v2);

		for (i32 i = 0; i < world->numStarP; i++)
		{
			i32 randX = rand() % (i32)world->worldSize.x;
			i32 randY = rand() % (i32)world->worldSize.y;

			world->starPList[i] = V2i(randX, randY);
		}

		world->init = TRUE;
	}

	for (u32 i = world->asteroidCounter; i < world->numAsteroids; i++)
		addAsteroid(world, (rand() % asteroidsize_count));

	Radians starRotation = DEGREES_TO_RADIANS(45.0f);
	v2 starSize = V2(2, 2);
	for (i32 i = 0; i < world->numStarP; i++)
	{
		renderer_rect(&state->renderer, world->camera, world->starPList[i],
		              starSize, V2(0, 0), starRotation, NULL,
		              V4(1.0f, 1.0f, 0, 0.5f),
		              renderflag_no_texture | renderflag_wireframe);
	}

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

					AudioVorbis *fire =
					    asset_vorbisGet(&state->assetManager, "fire");
				    AudioRenderer *audioRenderer =
				        getFreeAudioRenderer(world, fire, 2);
				    if (audioRenderer)
				    {
					    // TODO(doyle): Atm transient arena is not used, this is
					    // just to fill out the arguments
					    audio_vorbisPlay(&state->transientArena,
					                     &state->audioManager, audioRenderer,
					                     fire, 1);
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
			if (!math_rectContainsP(world->camera, entity->pos))
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
				    asset_vorbisGet(&state->assetManager, sound);
				AudioRenderer *audioRenderer =
				    getFreeAudioRenderer(world, explode, 3);
				if (audioRenderer)
				{
					audio_vorbisPlay(&state->transientArena,
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
}

INTERNAL void startMenuUpdate(GameState *state, Memory *memory, f32 dt)
{
	AssetManager *assetManager   = &state->assetManager;
	InputBuffer *inputBuffer     = &state->input;
	Renderer *renderer           = &state->renderer;
	MemoryArena_ *transientArena = &state->transientArena;
	UiState *uiState             = &state->uiState;
	StartMenuState *menuState =
	    GET_STATE_DATA(state, &state->persistentArena, StartMenuState);

	Font *arial15 = asset_fontGetOrCreateOnDemand(
	    assetManager, &state->persistentArena, transientArena, "Arial", 15);
	Font *arial25 = asset_fontGetOrCreateOnDemand(
	    assetManager, &state->persistentArena, transientArena, "Arial", 40);

	v2 screenCenter = v2_scale(renderer->size, 0.5f);

	const char *const title = "Asteroids";
	v2 titleDim     = asset_fontStringDimInPixels(arial25, title);
	v2 halfTitleDim = v2_scale(titleDim, 0.5f);
	v2 titleP       = v2_add(screenCenter, V2(0, 40));
	titleP          = v2_sub(titleP, halfTitleDim);

	renderer_stringFixed(renderer, transientArena, arial25, title, titleP,
	                     V2(0, 0), 0, V4(1, 0, 0, 1), 0);

	ui_beginState(uiState);

	{ // Draw blinking Start Game prompt
		menuState->startMenuGameStartBlinkTimer -= dt;
		if (menuState->startMenuGameStartBlinkTimer < 0.0f)
		{
			menuState->startMenuGameStartBlinkTimer = 1.0f;
			menuState->startMenuToggleShow =
			    (menuState->startMenuToggleShow) ? FALSE : TRUE;
		}

		if (menuState->startMenuToggleShow)
		{
			const char *const gameStart = "Press enter to start";
			v2 gameStartDim = asset_fontStringDimInPixels(arial25, gameStart);
			v2 halfGameStartDim = v2_scale(gameStartDim, 0.5f);
			v2 gameStartP       = v2_add(screenCenter, V2(0, -40));
			gameStartP          = v2_sub(gameStartP, halfGameStartDim);

			renderer_stringFixed(renderer, transientArena, arial25, gameStart,
			                     gameStartP, V2(0, 0), 0, V4(1, 1, 0, 1), 0);
		}
	}

	{
		const char *const optionPrompt = "Press [o] for options ";
		v2 dim     = asset_fontStringDimInPixels(arial25, optionPrompt);
		v2 halfDim = v2_scale(dim, 0.5f);
		v2 p       = v2_add(screenCenter, V2(0, -120));
		p          = v2_sub(p, halfDim);
		renderer_stringFixed(renderer, transientArena, arial25, optionPrompt, p,
		                     V2(0, 0), 0, V4(1, 1, 0, 1), 0);
	}

	if (platform_queryKey(&inputBuffer->keys[keycode_enter],
	                      readkeytype_one_shot, KEY_DELAY_NONE))
	{
		state->currState = appstate_GameWorldState;

		GameWorldState *world =
		    GET_STATE_DATA(state, &state->persistentArena, GameWorldState);
		addPlayer(world);
	}

	ui_endState(uiState, inputBuffer);
}

#define ASTEROID_GET_STATE_DATA(state, type)                                   \
	(type *)asteroid_getStateData_(state, appstate_##type)
void *asteroid_getStateData_(GameState *state, enum AppState appState)
{
	void *result = state->appStateData[appState];
	return result;
}

void asteroid_gameUpdateAndRender(GameState *state, Memory *memory,
                                  v2 windowSize, f32 dt)
{
	MemoryIndex globalTransientArenaSize =
	    (MemoryIndex)((f32)memory->transientSize * 0.5f);
	memory_arenaInit(&state->transientArena, memory->transient,
	                 globalTransientArenaSize);

	if (!state->init)
	{
		srand((u32)time(NULL));
		asset_init(&state->assetManager, &state->persistentArena);
		audio_init(&state->audioManager);
		
		// NOTE(doyle): Load game assets must be before init_renderer so that
		// shaders are available for the renderer configuration
		loadGameAssets(state);
		renderer_init(&state->renderer, &state->assetManager,
		              &state->persistentArena, windowSize);

		Font *arial15 = asset_fontGet(&state->assetManager, "Arial", 15);
		debug_init(&state->persistentArena, windowSize, *arial15);

		state->currState = appstate_StartMenuState;
		state->init      = TRUE;
	}

	platform_inputBufferProcess(&state->input, dt);

	switch (state->currState)
	{
	case appstate_StartMenuState:
	{
		// NOTE(doyle): Let menu overlay the game menu. We add player on "enter"
		// So fall through to appstate_game is valid here!
		startMenuUpdate(state, memory, dt);
	}
	case appstate_GameWorldState:
	{
		gameUpdate(state, memory, dt);
	}
	break;

	default:
	{
		ASSERT(INVALID_CODE_PATH);
	}
	break;
	}

	debug_drawUi(state, dt);
	renderer_renderGroups(&state->renderer);
}
