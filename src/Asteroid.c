#include "Dengine/Asteroid.h"
#include "Dengine/Debug.h"

void initAssetManager(GameState *state)
{
	AssetManager *assetManager = &state->assetManager;
	MemoryArena_ *arena        = &state->persistentArena;

	i32 audioEntries         = 32;
	assetManager->audio.size = audioEntries;
	assetManager->audio.entries =
	    memory_pushBytes(arena, audioEntries * sizeof(HashTableEntry));

	i32 texAtlasEntries         = 8;
	assetManager->texAtlas.size = texAtlasEntries;
	assetManager->texAtlas.entries =
	    memory_pushBytes(arena, texAtlasEntries * sizeof(HashTableEntry));

	i32 texEntries              = 32;
	assetManager->textures.size = texEntries;
	assetManager->textures.entries =
	    memory_pushBytes(arena, texEntries * sizeof(HashTableEntry));

	i32 animEntries          = 1024;
	assetManager->anims.size = animEntries;
	assetManager->anims.entries =
	    memory_pushBytes(arena, animEntries * sizeof(HashTableEntry));

	/* Create empty 1x1 4bpp black texture */
	u32 bitmap   = (0xFF << 24) | (0xFF << 16) | (0xFF << 8) | (0xFF << 0);
	Texture *tex = asset_getFreeTexSlot(assetManager, arena, "nullTex");
	*tex         = texture_gen(1, 1, 4, CAST(u8 *)(&bitmap));

	/* Load shaders */
	asset_loadShaderFiles(
	    assetManager, arena, "data/shaders/default_tex.vert.glsl",
	    "data/shaders/default_tex.frag.glsl", shaderlist_default);

	asset_loadShaderFiles(
	    assetManager, arena, "data/shaders/default_no_tex.vert.glsl",
	    "data/shaders/default_no_tex.frag.glsl", shaderlist_default_no_tex);

	i32 result =
	    asset_loadTTFont(assetManager, arena, "C:/Windows/Fonts/Arialbd.ttf");
	if (result) ASSERT(TRUE);
}

void initRenderer(GameState *state, v2 windowSize) {
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

enum ReadKeyType
{
	readkeytype_oneShot,
	readkeytype_delayedRepeat,
	readkeytype_repeat,
	readkeytype_count,
};

#define KEY_DELAY_NONE 0.0f
INTERNAL b32 getKeyStatus(KeyState *key, enum ReadKeyType readType,
                          f32 delayInterval, f32 dt)
{

	if (!key->endedDown) return FALSE;

	switch(readType)
	{
	case readkeytype_oneShot:
	{
		if (key->newHalfTransitionCount > key->oldHalfTransitionCount)
			return TRUE;
		break;
	}
	case readkeytype_repeat:
	case readkeytype_delayedRepeat:
	{
		if (key->newHalfTransitionCount > key->oldHalfTransitionCount)
		{
			if (readType == readkeytype_delayedRepeat)
			{
				// TODO(doyle): Let user set arbitrary delay after initial input
				key->delayInterval = 2 * delayInterval;
			}
			else
			{
				key->delayInterval = delayInterval;
			}
			return TRUE;
		}
		else if (key->delayInterval <= 0.0f)
		{
			key->delayInterval = delayInterval;
			return TRUE;
		}
		else
		{
			key->delayInterval -= dt;
		}
		break;
	}
	default:
#ifdef DENGINE_DEBUG
		DEBUG_LOG("getKeyStatus() error: Invalid ReadKeyType enum");
		ASSERT(INVALID_CODE_PATH);
#endif
		break;
	}

	return FALSE;
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

INTERNAL void addAsteroid(World *world, v2 windowSize)
{
	world->asteroidCounter++;

	Entity *asteroid = &world->entityList[world->entityIndex++];
	asteroid->id     = world->entityIdCounter++;

	i32 randValue = rand();
	i32 randX     = (randValue % (i32)windowSize.w);
	i32 randY     = (randValue % (i32)windowSize.h);
	asteroid->pos = V2i(randX, randY);

	asteroid->size       = V2(100.0f, 100.0f);
	asteroid->hitbox     = asteroid->size;
	asteroid->offset     = v2_scale(asteroid->size, -0.5f);
	asteroid->type       = entitytype_asteroid;
	asteroid->direction  = direction_null;
	asteroid->renderMode = rendermode_polygon;

	asteroid->numVertexPoints = 10;

	i32 cacheIndex = randValue % ARRAY_COUNT(world->asteroidVertexCache);
	if (!world->asteroidVertexCache[cacheIndex])
	{
		world->asteroidVertexCache[cacheIndex] = createAsteroidVertexList(
		    &world->entityArena, asteroid->numVertexPoints,
		    (i32)(asteroid->size.w * 0.5f));
	}

	asteroid->vertexPoints = world->asteroidVertexCache[cacheIndex];
	asteroid->color = V4(0.0f, 0.5f, 0.5f, 1.0f);
}

INTERNAL void addBullet(World *world, Entity *shooter)
{
	Entity *bullet = &world->entityList[world->entityIndex++];
	bullet->id     = world->entityIdCounter++;

	bullet->offset     = v2_scale(bullet->size, -0.5f);
	bullet->pos        = v2_add(shooter->pos, bullet->offset);
	bullet->hitbox     = bullet->size;
	bullet->size       = V2(2.0f, 20.0f);
	bullet->rotation   = shooter->rotation;
	bullet->renderMode = rendermode_quad;

	if (!world->bulletVertexCache)
	{
		world->bulletVertexCache =
		    memory_pushBytes(&world->entityArena, sizeof(v2) * 4);
		world->bulletVertexCache[0] = V2(0, bullet->size.h);
		world->bulletVertexCache[1] = V2(0, 0);
		world->bulletVertexCache[2] = V2(bullet->size.w, 0);
		world->bulletVertexCache[3] = bullet->size;
	}

	bullet->vertexPoints = world->bulletVertexCache;
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
			ship->direction  = direction_null;
			ship->renderMode = rendermode_polygon;
			ship->color      = V4(1.0f, 0.5f, 0.5f, 1.0f);
		}

		{ // Global Collision Rules
			setCollisionRule(world, entitytype_ship, entitytype_asteroid, TRUE);
			setCollisionRule(world, entitytype_bullet, entitytype_asteroid,
			                 TRUE);
		}

		world->camera.min = V2(0, 0);
		world->camera.max = state->renderer.size;
		world->worldSize  = windowSize;

		state->init       = TRUE;

		debug_init(&state->persistentArena, windowSize,
		           state->assetManager.font);
	}

	for (u32 i = world->asteroidCounter; i < world->numAsteroids; i++)
		addAsteroid(world, windowSize);

	{
		KeyState *keys = state->input.keys;
		for (enum KeyCode code = 0; code < keycode_count; code++)
		{
			KeyState *keyState = &keys[code];

			u32 halfTransitionCount = keyState->newHalfTransitionCount -
			                          keyState->oldHalfTransitionCount;

			if (halfTransitionCount > 0)
			{
				b32 transitionCountIsOdd = ((halfTransitionCount & 1) == 1);

				if (transitionCountIsOdd)
				{
					if (keyState->endedDown) keyState->endedDown = FALSE;
					else keyState->endedDown = TRUE;
				}

				keyState->oldHalfTransitionCount =
				    keyState->newHalfTransitionCount;
			}
		}
	}

	if (getKeyStatus(&state->input.keys[keycode_left_square_bracket],
	                 readkeytype_repeat, 0.2f, dt))
	{
		addAsteroid(world, windowSize);
	}

	ASSERT(world->entityList[0].id == NULL_ENTITY_ID);
	for (i32 i = 1; i < world->entityIndex; i++)
	{
		Entity *entity = &world->entityList[i];
		ASSERT(entity->type != entitytype_invalid);

		v2 pivotPoint = {0};
		f32 ddPSpeedInMs = 0;
		v2 ddP           = {0};
		if (entity->type == entitytype_ship)
		{
			if (getKeyStatus(&state->input.keys[keycode_up], readkeytype_repeat,
			                 0.0f, dt))
			{
				// TODO(doyle): Renderer creates upfacing triangles by default,
				// but we need to offset rotation so that our base "0 degrees"
				// is right facing for trig to work
				Radians rotation =
				    DEGREES_TO_RADIANS((entity->rotation + 90.0f));

				v2 direction = V2(math_cosf(rotation), math_sinf(rotation));
				ddP          = direction;
			}

			Degrees rotationsPerSecond = 180.0f;
			if (getKeyStatus(&state->input.keys[keycode_left],
			                 readkeytype_repeat, 0.0f, dt))
			{
				entity->rotation += (rotationsPerSecond) * dt;
			}

			if (getKeyStatus(&state->input.keys[keycode_right],
			                 readkeytype_repeat, 0.0f, dt))
			{
				entity->rotation -= (rotationsPerSecond) * dt;
			}

			if (getKeyStatus(&state->input.keys[keycode_space],
			                 readkeytype_delayedRepeat, 0.05f, dt))
			{
				addBullet(world, entity);
			}

			if (ddP.x > 0.0f && ddP.y > 0.0f)
			{
				// NOTE(doyle): Cheese it and pre-compute the vector for
				// diagonal using pythagoras theorem on a unit triangle 1^2
				// + 1^2 = c^2
				ddP = v2_scale(ddP, 0.70710678118f);
			}

			ddPSpeedInMs = 25;
			DEBUG_PUSH_VAR("Pos: %5.2f, %5.2f", entity->pos, "v2");
			DEBUG_PUSH_VAR("Velocity: %5.2f, %5.2f", entity->dP, "v2");
			DEBUG_PUSH_VAR("Rotation: %5.2f", entity->rotation, "f32");

			renderer_rect(&state->renderer, world->camera, entity->pos,
			              V2(5, 5), V2(0, 0),
			              DEGREES_TO_RADIANS(entity->rotation), NULL,
			              V4(1.0f, 1.0f, 1.0f, 1.0f), renderflag_no_texture);
		}
		else if (entity->type == entitytype_asteroid)
		{

			i32 randValue = rand();
			if (entity->direction == direction_null)
			{
				entity->direction = randValue % direction_count;
			}

			v2 ddP = {0};
			switch (entity->direction)
			{
			case direction_north:
			{
				ddP.y = 1.0f;
			}
			break;

			case direction_northwest:
			{
				ddP.x = 1.0f;
				ddP.y = 1.0f;
			}
			break;

			case direction_west:
			{
				ddP.x = -1.0f;
			}
			break;

			case direction_southwest:
			{
				ddP.x = -1.0f;
				ddP.y = -1.0f;
			}
			break;

			case direction_south:
			{
				ddP.y = -1.0f;
			}
			break;

			case direction_southeast:
			{
				ddP.x = 1.0f;
				ddP.y = -1.0f;
			}
			break;

			case direction_east:
			{
				ddP.x = 1.0f;
			}
			break;

			case direction_northeast:
			{
				ddP.x = 1.0f;
				ddP.y = 1.0f;
			}
			break;

			default:
			{
				ASSERT(INVALID_CODE_PATH);
			}
			break;
			}

			f32 dirOffset = ((randValue % 10) + 1) / 100.0f;
			v2_scale(ddP, dirOffset);

			// NOTE(doyle): Make asteroids start and move at constant speed by
			// ensuring that dP is "refreshed" with non-decaying acceleration
			ddPSpeedInMs = 1;
			entity->dP   = v2_scale(ddP, world->pixelsPerMeter * ddPSpeedInMs);
			entity->rotation += (60 * dt);
		}
		else if (entity->type == entitytype_bullet)
		{
			if (!math_pointInRect(world->camera, entity->pos))
			{
				world->entityList[i--] =
				    world->entityList[--world->entityIndex];
				continue;
			}

			ddPSpeedInMs     = 25;
			Radians rotation = DEGREES_TO_RADIANS((entity->rotation + 90.0f));
			ddP              = V2(math_cosf(rotation), math_sinf(rotation));
			entity->dP = v2_scale(ddP, world->pixelsPerMeter * ddPSpeedInMs);

		}

		// Loop entity around world
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

			if (colliderA->type == entitytype_asteroid)
			{
				ASSERT(colliderB->type == entitytype_bullet);

				world->entityList[collisionIndex] =
				    world->entityList[--world->entityIndex];

				world->entityList[i--] =
				    world->entityList[--world->entityIndex];

				world->asteroidCounter--;

				ASSERT(world->asteroidCounter >= 0);
				continue;
			}
		}

		RenderFlags flags = renderflag_wireframe | renderflag_no_texture;
		renderer_entity(&state->renderer, &state->transientArena, world->camera,
		                entity, V2(0, 0), 0,
		                collideColor, flags);
	}

#if 1
	debug_drawUi(state, dt);
	debug_clearCounter();
#endif

	renderer_renderGroups(&state->renderer);
}
