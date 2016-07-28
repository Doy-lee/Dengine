#include "Dengine/WorldTraveller.h"
#include "Dengine/Audio.h"
#include "Dengine/Debug.h"
#include "Dengine/Entity.h"
#include "Dengine/Platform.h"

enum State
{
	state_active,
	state_menu,
	state_win,
};

Entity *getHeroEntity(World *world)
{
	Entity *result = &world->entities[world->heroIndex];
	return result;
}

INTERNAL void rendererInit(GameState *state, v2 windowSize)
{
	AssetManager *assetManager = &state->assetManager;
	Renderer *renderer         = &state->renderer;
	renderer->size             = windowSize;
	// NOTE(doyle): Value to map a screen coordinate to NDC coordinate
	renderer->vertexNdcFactor =
	    V2(1.0f / renderer->size.w, 1.0f / renderer->size.h);
	renderer->shader = asset_getShader(assetManager, shaderlist_sprite);
	shader_use(renderer->shader);

	const mat4 projection =
	    mat4_ortho(0.0f, renderer->size.w, 0.0f, renderer->size.h, 0.0f, 1.0f);
	shader_uniformSetMat4fv(renderer->shader, "projection", projection);
	GL_CHECK_ERROR();

	/* Create buffers */
	glGenVertexArrays(1, &renderer->vao);
	glGenBuffers(1, &renderer->vbo);
	GL_CHECK_ERROR();

	/* Bind buffers */
	glBindBuffer(GL_ARRAY_BUFFER, renderer->vbo);
	glBindVertexArray(renderer->vao);

	/* Configure VAO */
	const GLuint numVertexElements = 4;
	const GLuint vertexSize        = sizeof(v4);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, numVertexElements, GL_FLOAT, GL_FALSE, vertexSize,
	                      (GLvoid *)0);
	GL_CHECK_ERROR();

	/* Unbind */
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	GL_CHECK_ERROR();
}

// TODO(doyle): Remove and implement own random generator!
#include <time.h>
#include <stdlib.h>
void worldTraveller_gameInit(GameState *state, v2 windowSize)
{
	AssetManager *assetManager = &state->assetManager;
	MemoryArena *arena = &state->arena;

	/*
	 ************************
	 * INITIALISE GAME AUDIO
	 ************************
	 */
	i32 result = audio_init(&state->audioManager);
	if (result)
	{
#ifdef DENGINE_DEBUG
		ASSERT(INVALID_CODE_PATH);
#endif
	}

	/*
	 *******************
	 * INITIALISE ASSETS
	 *******************
	 */
	/* Create empty 1x1 4bpp black texture */
	u32 bitmap       = (0xFF << 24) | (0xFF << 16) | (0xFF << 8) | (0xFF << 0);
	Texture emptyTex = texture_gen(1, 1, 4, CAST(u8 *)(&bitmap));
	assetManager->textures[texlist_empty] = emptyTex;

	/* Load textures */
	asset_loadTextureImage(assetManager,
	                       "data/textures/WorldTraveller/TerraSprite1024.png",
	                       texlist_hero);
	TexAtlas *heroAtlas = asset_getTextureAtlas(assetManager, texlist_hero);
	heroAtlas->texRect[herorects_idle]       = V4(746, 1018, 804, 920);
	heroAtlas->texRect[herorects_walkA]      = V4(641, 1018, 699, 920);
	heroAtlas->texRect[herorects_walkB]      = V4(849, 1018, 904, 920);
	heroAtlas->texRect[herorects_head]       = V4(108, 1024, 159, 975);
	heroAtlas->texRect[herorects_waveA]      = V4(944, 918, 1010, 816);
	heroAtlas->texRect[herorects_waveB]      = V4(944, 812, 1010, 710);
	heroAtlas->texRect[herorects_battlePose] = V4(8, 910, 71, 814);
	heroAtlas->texRect[herorects_castA]      = V4(428, 910, 493, 814);
	heroAtlas->texRect[herorects_castB]      = V4(525, 919, 590, 816);
	heroAtlas->texRect[herorects_castC]      = V4(640, 916, 698, 816);

	asset_loadTextureImage(assetManager,
	                       "data/textures/WorldTraveller/Terrain.png",
	                       texlist_terrain);
	TexAtlas *terrainAtlas =
	    asset_getTextureAtlas(assetManager, texlist_terrain);
	f32 atlasTileSize = 128.0f;
	const i32 texSize = 1024;
	v2 texOrigin = V2(0, CAST(f32)(texSize - 128));
	terrainAtlas->texRect[terrainrects_ground] =
	    V4(texOrigin.x, texOrigin.y, texOrigin.x + atlasTileSize,
	       texOrigin.y - atlasTileSize);

	/* Load shaders */
	asset_loadShaderFiles(assetManager, arena, "data/shaders/sprite.vert.glsl",
	                      "data/shaders/sprite.frag.glsl",
	                      shaderlist_sprite);

	result =
	    asset_loadTTFont(assetManager, arena, "C:/Windows/Fonts/Arialbd.ttf");
	if (result) DEBUG_LOG("Font loading failed");

	GL_CHECK_ERROR();

#ifdef DENGINE_DEBUG
	DEBUG_LOG("Assets loaded");
#endif

	/* Load animations */
	f32 duration = 1.0f;
	i32 numRects = 1;
	v4 *animRects = PLATFORM_MEM_ALLOC(arena, numRects, v4);
	i32 terrainAnimAtlasIndexes[1] = {terrainrects_ground};

	// TODO(doyle): Optimise animation storage, we waste space having 1:1 with
	// animlist when some textures don't have certain animations
	asset_addAnimation(assetManager, arena, texlist_terrain, animlist_terrain,
	                   terrainAnimAtlasIndexes, numRects, duration);

	// Idle animation
	duration      = 1.0f;
	numRects      = 1;
	i32 idleAnimAtlasIndexes[1] = {herorects_idle};
	asset_addAnimation(assetManager, arena, texlist_hero, animlist_hero_idle,
	                   idleAnimAtlasIndexes, numRects, duration);

	// Walk animation
	duration          = 0.10f;
	numRects          = 3;
	i32 walkAnimAtlasIndexes[3] = {herorects_walkA, herorects_idle,
	                               herorects_walkB};
	asset_addAnimation(assetManager, arena, texlist_hero, animlist_hero_walk,
	                   walkAnimAtlasIndexes, numRects, duration);

	// Wave animation
	duration          = 0.30f;
	numRects          = 2;
	i32 waveAnimAtlasIndexes[2] = {herorects_waveA, herorects_waveB};
	asset_addAnimation(assetManager, arena, texlist_hero, animlist_hero_wave,
	                   waveAnimAtlasIndexes, numRects, duration);

	// Battle Stance animation
	duration          = 1.0f;
	numRects          = 1;
	i32 battleStanceAnimAtlasIndexes[1] = {herorects_battlePose};
	asset_addAnimation(assetManager, arena, texlist_hero, animlist_hero_battlePose,
	                   battleStanceAnimAtlasIndexes, numRects, duration);

	// Battle tackle animation
	duration          = 0.15f;
	numRects          = 3;
	i32 tackleAnimAtlasIndexes[3] = {herorects_castA, herorects_castB,
	                                 herorects_castC};
	asset_addAnimation(assetManager, arena, texlist_hero, animlist_hero_tackle,
	                   tackleAnimAtlasIndexes, numRects, duration);
#ifdef DENGINE_DEBUG
	DEBUG_LOG("Animations created");
#endif

	/* Load sound */

	char *audioPath = "data/audio/Nobuo Uematsu - Battle 1.ogg";
	asset_loadVorbis(assetManager, arena, audioPath, audiolist_battle);
	audioPath = "data/audio/Yuki Kajiura - Swordland.ogg";
	asset_loadVorbis(assetManager, arena, audioPath, audiolist_overworld);
	audioPath = "data/audio/nuindependent_hit22.ogg";
	asset_loadVorbis(assetManager, arena, audioPath, audiolist_tackle);

#ifdef DENGINE_DEBUG
	DEBUG_LOG("Sound assets initialised");
#endif

	state->state          = state_active;
	state->currWorldIndex = 0;
	state->tileSize       = 64;

	/* Init renderer */
	rendererInit(state, windowSize);
#ifdef DENGINE_DEBUG
	DEBUG_LOG("Renderer initialised");
#endif

	/* Init world */
	const i32 targetWorldWidth  = 100 * METERS_TO_PIXEL;
	const i32 targetWorldHeight = 10 * METERS_TO_PIXEL;
	v2 worldDimensionInTiles    = V2i(targetWorldWidth / state->tileSize,
	                               targetWorldHeight / state->tileSize);

	for (i32 i = 0; i < ARRAY_COUNT(state->world); i++)
	{
		World *const world = &state->world[i];
		world->maxEntities = 16384;
		world->entities = PLATFORM_MEM_ALLOC(arena, world->maxEntities, Entity);
		world->entityIdInBattle =
		    PLATFORM_MEM_ALLOC(arena, world->maxEntities, i32);
		world->numEntitiesInBattle = 0;
		world->texType             = texlist_terrain;
		world->bounds =
		    math_getRect(V2(0, 0), v2_scale(worldDimensionInTiles,
		                                    CAST(f32) state->tileSize));
		world->uniqueIdAccumulator = 0;

		TexAtlas *const atlas =
		    asset_getTextureAtlas(assetManager, world->texType);

		for (i32 y = 0; y < worldDimensionInTiles.y; y++)
		{
			for (i32 x = 0; x < worldDimensionInTiles.x; x++)
			{
#ifdef DENGINE_DEBUG
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
				Entity *tile = entity_add(arena, world, pos, size, type, dir,
				                          tex, collides);

				entity_addAnim(assetManager, tile, animlist_terrain);
				tile->currAnimId = animlist_terrain;
			}
		}
	}

	World *const world = &state->world[state->currWorldIndex];
	world->cameraPos = V2(0.0f, 0.0f);

	/* Add world soundscape */
	Renderer *renderer   = &state->renderer;
	v2 size              = V2(10.0f, 10.0f);
	v2 pos               = V2(0, 0);
	enum EntityType type = entitytype_soundscape;
	enum Direction dir   = direction_null;
	Texture *tex         = NULL;
	b32 collides         = FALSE;
	Entity *soundscape =
	    entity_add(arena, world, pos, size, type, dir, tex, collides);

	world->soundscape = soundscape;
	soundscape->audioRenderer = PLATFORM_MEM_ALLOC(arena, 1, AudioRenderer);
	soundscape->audioRenderer->sourceIndex = AUDIO_SOURCE_UNASSIGNED;

	/* Init hero entity */
	world->heroIndex   = world->freeEntityIndex;

	size            = V2(58.0f, 98.0f);
	pos             = V2(size.x, CAST(f32) state->tileSize);
	type            = entitytype_hero;
	dir             = direction_east;
	tex             = asset_getTexture(assetManager, texlist_hero);
	collides        = TRUE;
	Entity *hero = entity_add(arena, world, pos, size, type, dir, tex, collides);
	hero->audioRenderer = PLATFORM_MEM_ALLOC(arena, 1, AudioRenderer);
	hero->audioRenderer->sourceIndex = AUDIO_SOURCE_UNASSIGNED;

	/* Populate hero animation references */
	entity_addAnim(assetManager, hero, animlist_hero_idle);
	entity_addAnim(assetManager, hero, animlist_hero_walk);
	entity_addAnim(assetManager, hero, animlist_hero_wave);
	entity_addAnim(assetManager, hero, animlist_hero_battlePose);
	entity_addAnim(assetManager, hero, animlist_hero_tackle);
	hero->currAnimId = animlist_hero_idle;

	/* Create a NPC */
	pos         = V2(hero->pos.x * 3, CAST(f32) state->tileSize);
	size        = hero->hitboxSize;
	type        = entitytype_npc;
	dir         = direction_null;
	tex         = hero->tex;
	collides    = FALSE;
	Entity *npc = entity_add(arena, world, pos, size, type, dir, tex, collides);

	/* Populate npc animation references */
	entity_addAnim(assetManager, npc, animlist_hero_wave);
	npc->currAnimId = animlist_hero_wave;

	/* Create a Mob */
	pos = V2(renderer->size.w - (renderer->size.w / 3.0f),
	         CAST(f32) state->tileSize);
	entity_addGenericMob(arena, assetManager, world, pos);
#ifdef DENGINE_DEBUG
	DEBUG_LOG("World populated");
#endif

	srand(CAST(u32)(time(NULL)));
}

INTERNAL inline v4 getEntityScreenRect(Entity entity)
{
	v4 result = math_getRect(entity.pos, entity.hitboxSize);
	return result;
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
	Entity *hero       = &world->entities[world->heroIndex];
	v2 ddPos           = V2(0, 0);

	if (hero->stats->busyDuration <= 0)
	{
		// TODO(doyle): As we need to handle more key spam input, we want to
		// track
		// if a button ended down
		LOCAL_PERSIST b32 spaceBarWasDown = FALSE;

		if (state->keys[GLFW_KEY_RIGHT])
		{
			ddPos.x         = 1.0f;
			hero->direction = direction_east;
		}

		if (state->keys[GLFW_KEY_LEFT])
		{
			ddPos.x         = -1.0f;
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
			// NOTE(doyle): Cheese it and pre-compute the vector for diagonal
			// using
			// pythagoras theorem on a unit triangle
			// 1^2 + 1^2 = c^2
			ddPos = v2_scale(ddPos, 0.70710678118f);
		}

		LOCAL_PERSIST b32 toggleFlag = TRUE;
		// TODO(doyle): Revisit key input with state checking for last ended down
		if (state->keys[GLFW_KEY_SPACE] && spaceBarWasDown == FALSE)
		{
			Renderer *renderer = &state->renderer;
			f32 yPos = CAST(f32)(rand() % CAST(i32)renderer->size.h);
			f32 xModifier =  5.0f - CAST(f32)(rand() % 3);

			v2 pos = V2(renderer->size.w - (renderer->size.w / xModifier), yPos);
			entity_addGenericMob(&state->arena, &state->assetManager, world,
			                     pos);
			spaceBarWasDown = TRUE;
		}
		else if (!state->keys[GLFW_KEY_SPACE])
		{
			spaceBarWasDown = FALSE;
		}
	}

	// NOTE(doyle): Clipping threshold for snapping velocity to 0
	f32 epsilon    = 0.5f;
	v2 epsilonDpos = v2_sub(V2(epsilon, epsilon),
	                        V2(ABS(hero->dPos.x), ABS(hero->dPos.y)));

	if (epsilonDpos.x >= 0.0f && epsilonDpos.y >= 0.0f)
	{
		hero->dPos = V2(0.0f, 0.0f);
		if (hero->currAnimId == animlist_hero_walk)
		{
			entity_setActiveAnim(hero, animlist_hero_idle);
		}
	}
	else if (hero->currAnimId == animlist_hero_idle)
	{
		entity_setActiveAnim(hero, animlist_hero_walk);
	}

	f32 heroSpeed = 6.2f * METERS_TO_PIXEL;
	if (state->keys[GLFW_KEY_LEFT_SHIFT])
		heroSpeed = CAST(f32)(22.0f * 10.0f * METERS_TO_PIXEL);

	ddPos = v2_scale(ddPos, heroSpeed);
	// TODO(doyle): Counteracting force on player's acceleration is arbitrary
    ddPos = v2_sub(ddPos, v2_scale(hero->dPos, 5.5f));

	/*
	   NOTE(doyle): Calculate new position from acceleration with old velocity
	   new Position     = (a/2) * (t^2) + (v*t) + p,
	   acceleration     = (a/2) * (t^2)
	   old velocity     =                 (v*t)
	 */
	v2 ddPosNew = v2_scale(v2_scale(ddPos, 0.5f), SQUARED(dt));
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
			if (entity.state == entitystate_dead) continue;

			if (entity.collides)
			{
				v4 heroRect =
				    V4(newHeroP.x, newHeroP.y, (newHeroP.x + hero->hitboxSize.x),
				       (newHeroP.y + hero->hitboxSize.y));
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
		offsetFromHeroToOrigin.x += (hero->hitboxSize.x * 0.5f);
		world->cameraPos = offsetFromHeroToOrigin;
	}
}

INTERNAL v4 createCameraBounds(World *world, v2 size)
{
	v4 result = math_getRect(world->cameraPos, size);
	// NOTE(doyle): Lock camera if it passes the bounds of the world
	if (result.x <= world->bounds.x)
	{
		result.x = world->bounds.x;
		result.z = result.x + size.w;
	}

	// TODO(doyle): Do the Y component when we need it
	if (result.y >= world->bounds.y) result.y = world->bounds.y;

	if (result.z >= world->bounds.z)
	{
		result.z = world->bounds.z;
		result.x = result.z - size.w;
	}

	if (result.w <= world->bounds.w) result.w = world->bounds.w;
	return result;
}

#define ENTITY_IN_BATTLE TRUE
#define ENTITY_NOT_IN_BATTLE FALSE
#define ENTITY_NULL_ID -1
INTERNAL i32 findBestEntityToAttack(World *world, Entity attacker)
{
#ifdef DENGINE_DEBUG
	ASSERT(world);
	ASSERT(attacker.type == entitytype_hero || attacker.type == entitytype_mob);
#endif
	i32 result = 0;

	// TODO(doyle): If attacker is mob- retrieve hero entity id directly, change
	// when we have party members!
	if (attacker.type == entitytype_mob)
	{
		Entity hero = world->entities[world->heroIndex];

		if (hero.state == entitystate_dead) result = ENTITY_NULL_ID;
		else result = hero.id;

		return result;
	}

	/* Attacker is hero */
	Entity hero = attacker;
	for (i32 i = 0; i < world->maxEntities; i++)
	{
		Entity targetEntity = world->entities[i];
		if (hero.id == targetEntity.id) continue;
		if (world->entityIdInBattle[targetEntity.id] == ENTITY_IN_BATTLE)
		{
			result = targetEntity.id;
			return result;
		}
	}
	
	// NOTE(doyle): Not all "battling" entities have been enumerated yet in the
	// update loop, guard against when using function
	return ENTITY_NULL_ID;
}

INTERNAL inline void updateWorldBattleEntities(World *world, Entity *entity,
                                               b32 isInBattle)
{
#ifdef DENGINE_DEBUG
	ASSERT(isInBattle == ENTITY_IN_BATTLE ||
	       isInBattle == ENTITY_NOT_IN_BATTLE);
	ASSERT(world && entity);
#endif
	world->entityIdInBattle[entity->id] = isInBattle;

	if (isInBattle)
	{
		world->numEntitiesInBattle++;
	}
	else
	{
		world->numEntitiesInBattle--;
	}

#ifdef DENGINE_DEBUG
	ASSERT(world->numEntitiesInBattle >= 0);
#endif
}

// TODO(doyle): Function too vague
INTERNAL inline void resetEntityState(World *world, Entity *entity)
{
	updateWorldBattleEntities(world, entity, ENTITY_NOT_IN_BATTLE);
	entity_setActiveAnim(entity, animlist_hero_idle);
	entity->stats->busyDuration     = 0;
	entity->stats->actionTimer      = entity->stats->actionRate;
	entity->stats->queuedAttack     = entityattack_invalid;
	entity->stats->entityIdToAttack = ENTITY_NULL_ID;
}

INTERNAL void beginAttack(World *world, Entity *attacker)
{
#ifdef DENGINE_DEBUG
	ASSERT(attacker->stats->entityIdToAttack != ENTITY_NULL_ID);
	ASSERT(attacker->state == entitystate_battle);
#endif
	switch (attacker->stats->queuedAttack)
	{
	case entityattack_tackle:
		EntityAnim_ attackAnim = attacker->anim[animlist_hero_tackle];
		f32 busyDuration       = attackAnim.anim->frameDuration *
		                   CAST(f32) attackAnim.anim->numFrames;
		attacker->stats->busyDuration = busyDuration;
		entity_setActiveAnim(attacker, animlist_hero_tackle);
		if (attacker->direction == direction_east)
			attacker->dPos.x += (1.0f * METERS_TO_PIXEL);
		else
			attacker->dPos.x -= (1.0f * METERS_TO_PIXEL);
		break;
	default:
#ifdef DENGINE_DEBUG
		ASSERT(INVALID_CODE_PATH);
#endif
		break;
	}
}

INTERNAL void endAttack(MemoryArena *arena, AssetManager *assetManager,
                        AudioManager *audioManager, World *world,
                        Entity *attacker)
{
#ifdef DENGINE_DEBUG
	ASSERT(attacker->stats->entityIdToAttack != ENTITY_NULL_ID);
#endif

	switch (attacker->stats->queuedAttack)
	{
	case entityattack_tackle:
		// TODO(doyle): Move animation offsets out and into animation type
		if (attacker->direction == direction_east)
			attacker->dPos.x -= (1.0f * METERS_TO_PIXEL);
		else
			attacker->dPos.x += (1.0f * METERS_TO_PIXEL);

#if 1
		audio_streamPlayVorbis(arena, audioManager, attacker->audioRenderer,
		                       asset_getVorbis(assetManager, audiolist_tackle),
		                       1);
#endif

		break;
	default:
#ifdef DENGINE_DEBUG
		ASSERT(INVALID_CODE_PATH);
#endif
		break;
	}

	/* Compute attack damage */
	Entity *defender = NULL;

	// TODO(doyle): Implement faster lookup for entity id in entity table
	b32 noMoreValidTargets = FALSE;
	do
	{
		/* Get target entity to attack */
		for (i32 i = 0; i < world->maxEntities; i++)
		{
			i32 entityIdToAttack = attacker->stats->entityIdToAttack;
			if (world->entities[i].id == entityIdToAttack)
			{
				defender = &world->entities[i];
#ifdef DENGINE_DEBUG
				ASSERT(defender->type == entitytype_mob ||
				       defender->type == entitytype_hero);
#endif
				break;
			}
		}

		/* If no longer exists, find next best */
		if (!defender)
		{
			i32 entityIdToAttack = findBestEntityToAttack(world, *attacker);
			if (entityIdToAttack == ENTITY_NULL_ID)
			{
				noMoreValidTargets = TRUE;
			}
			else
			{
				attacker->stats->entityIdToAttack =
				    findBestEntityToAttack(world, *attacker);
			}
		}
	} while (!defender && noMoreValidTargets == TRUE);

	if (!noMoreValidTargets)
	{
		i32 damage = -10;
		// TODO(doyle): Use attacker stats in battle equations
		if (attacker->type == entitytype_hero)
		{
			defender->stats->health += damage;
		}
		else
		{
			// defender->stats->health--;
		}
	}
}

INTERNAL void entityStateSwitch(MemoryArena *arena, AssetManager *assetManager,
                                AudioManager *audioManager, World *world,
                                Entity *entity, enum EntityState newState)
{
#ifdef DENGINE_DEBUG
	ASSERT(world && entity)
	ASSERT(entity->type == entitytype_mob || entity->type == entitytype_hero)
#endif
	if (entity->state == newState) return;

	switch(entity->state)
	{
	case entitystate_idle:
		switch (newState)
		{
		case entitystate_battle:
			updateWorldBattleEntities(world, entity, ENTITY_IN_BATTLE);
			entity->stats->entityIdToAttack =
			    findBestEntityToAttack(world, *entity);
			break;

		// TODO(doyle): Corner case- if move out of range and entity has
		// switched to idle mode, we reach the attacker entity and they continue
		// attacking it since there's no check before attack if entity is idle
		// or not (i.e. has moved out of frame last frame).
		case entitystate_dead:
			entity_setActiveAnim(entity, animlist_hero_idle);
			entity->stats->busyDuration     = 0;
			entity->stats->actionTimer      = entity->stats->actionRate;
			entity->stats->queuedAttack     = entityattack_invalid;
			entity->stats->entityIdToAttack = ENTITY_NULL_ID;
			break;

		case entitystate_attack:
		default:
#ifdef DENGINE_DEBUG
			ASSERT(INVALID_CODE_PATH);
#endif
			break;
		}
		break;
	case entitystate_battle:
		switch (newState)
		{
		case entitystate_attack:
		{
			beginAttack(world, entity);
			break;
		}
		case entitystate_idle:
		case entitystate_dead:
			resetEntityState(world, entity);
			break;
		default:
#ifdef DENGINE_DEBUG
			ASSERT(INVALID_CODE_PATH);
#endif
			break;
		}
		break;
	case entitystate_attack:
		switch (newState)
		{
		case entitystate_battle:
			endAttack(arena, assetManager, audioManager, world, entity);
			entity_setActiveAnim(entity, animlist_hero_battlePose);
			entity->stats->actionTimer  = entity->stats->actionRate;
			entity->stats->busyDuration = 0;
			break;
		// NOTE(doyle): Entity has been forced out of an attack (out of range)
		case entitystate_idle:
		case entitystate_dead:
			resetEntityState(world, entity);
			break;
		default:
#ifdef DENGINE_DEBUG
			ASSERT(INVALID_CODE_PATH);
#endif
			break;
		}
		break;
	case entitystate_dead:
		switch (newState)
		{
		case entitystate_idle:
		case entitystate_battle:
		case entitystate_attack:
		default:
#ifdef DENGINE_DEBUG
			ASSERT(INVALID_CODE_PATH);
#endif
			break;
		}
		break;
	default:
#ifdef DENGINE_DEBUG
		ASSERT(INVALID_CODE_PATH);
#endif
		break;
	}

	entity->state = newState;
}

void worldTraveller_gameUpdateAndRender(GameState *state, f32 dt)
{
	if (dt >= 1.0f) dt = 1.0f;
	/* Update */
	parseInput(state, dt);
	GL_CHECK_ERROR();

	AssetManager *assetManager = &state->assetManager;
	Renderer *renderer         = &state->renderer;
	World *const world         = &state->world[state->currWorldIndex];
	Font *font                 = &assetManager->font;
	MemoryArena *arena         = &state->arena;

	/*
	 ******************************
	 * Update entities and render
	 ******************************
	 */
	Entity *hero               = getHeroEntity(world);
	v4 cameraBounds            = createCameraBounds(world, renderer->size);
	AudioManager *audioManager = &state->audioManager;
	ASSERT(world->freeEntityIndex < world->maxEntities);
	for (i32 i = 0; i < world->freeEntityIndex; i++)
	{
		Entity *const entity = &world->entities[i];

		if (entity->type == entitytype_soundscape)
		{
			AudioRenderer *audioRenderer = entity->audioRenderer;
			if (world->numEntitiesInBattle > 0)
			{
				AudioVorbis *battleTheme =
				    asset_getVorbis(assetManager, audiolist_battle);
				if (audioRenderer->audio)
				{
					if (audioRenderer->audio->type != audiolist_battle)
					{
						audio_streamPlayVorbis(arena, &state->audioManager,
						                       audioRenderer, battleTheme,
						                       AUDIO_REPEAT_INFINITE);
					}
				}
				else
				{
					audio_streamPlayVorbis(arena, &state->audioManager,
					                       audioRenderer, battleTheme,
					                       AUDIO_REPEAT_INFINITE);
				}
			}
			else
			{
				AudioVorbis *overworldTheme =
				    asset_getVorbis(assetManager, audiolist_overworld);
				if (audioRenderer->audio)
				{
					if (audioRenderer->audio->type != audiolist_overworld)
					{
						audio_streamPlayVorbis(arena, &state->audioManager,
						                       audioRenderer, overworldTheme,
						                       AUDIO_REPEAT_INFINITE);
					}
				}
				else
				{
					audio_streamPlayVorbis(arena, &state->audioManager,
					                       audioRenderer, overworldTheme,
					                       AUDIO_REPEAT_INFINITE);
				}
			}
		}

		if (entity->state == entitystate_dead)
		{
#ifdef DENGINE_DEBUG
			switch(entity->type)
			{
			case entitytype_hero:
			case entitytype_mob:
				break;
			default:
				// TODO(doyle): Implement variable expansion in DEBUG_LOG
				DEBUG_LOG("Unexpected entity has been deleted");
			}
#endif
			// TODO(doyle): Accumulate all dead entities and delete at the
			// end. Hence resort/organise entity array once, not every time
			// an entity dies
#if 1
			i32 entityIndexInArray = i;
			audio_streamStopVorbis(arena, &state->audioManager,
			                       entity->audioRenderer);
			entity_delete(&state->arena, world, entityIndexInArray);

			// TODO(doyle): DeleteEntity moves elements down 1, so account for i
			i--;
#endif
			continue;
		}

		/*
		 *****************************************************
		 * Set mob to battle mode if within distance from hero
		 *****************************************************
		 */
		if (entity->type == entitytype_mob)
		{
			// TODO(doyle): Currently calculated in pixels, how about meaningful
			// game units?
			f32 battleThreshold = 500.0f;
			f32 distance = v2_magnitude(hero->pos, entity->pos);

			enum EntityState newState = entitystate_invalid;
			if (distance <= battleThreshold)
			{
				// NOTE(doyle): If in range but in battle, no state change
				if (entity->state == entitystate_battle ||
				    entity->state == entitystate_attack)
				{
					newState = entity->state;
				}
				else
				{
					newState = entitystate_battle;
				}
			}
			else
			{
				newState = entitystate_idle;
			}

			entityStateSwitch(arena, assetManager, audioManager, world, entity,
			                  newState);
		}

		/*
		 **************************************************
		 * Conduct battle for humanoid entities if in range
		 **************************************************
		 */
		if (entity->type == entitytype_mob || entity->type == entitytype_hero)
		{
			EntityStats *stats = entity->stats;
			if (entity->state == entitystate_battle)
			{
				if (stats->actionTimer > 0)
					stats->actionTimer -= dt * stats->actionSpdMul;

				if (stats->actionTimer <= 0)
				{
					stats->actionTimer = 0;
					if (stats->queuedAttack == entityattack_invalid)
						stats->queuedAttack = entityattack_tackle;

					/* Launch up attack animation */
					entityStateSwitch(arena, assetManager, audioManager, world,
					                  entity, entitystate_attack);
				}
			}
			else if (entity->state == entitystate_attack)
			{
				// TODO(doyle): Untested if the attacker and the defender same
				Entity *attacker = entity;
				stats->busyDuration -= dt;
				if (stats->busyDuration <= 0)
				{
					/* Apply attack damage */
					entityStateSwitch(arena, assetManager, audioManager, world,
					                  entity, entitystate_battle);

					/* Get target entity that was attacked */
					Entity *defender = NULL;
					for (i32 i = 0; i < world->maxEntities; i++)
					{
						i32 entityIdToAttack =
						    attacker->stats->entityIdToAttack;
						if (world->entities[i].id == entityIdToAttack)
						{
							defender = &world->entities[i];
#ifdef DENGINE_DEBUG
							ASSERT(defender->type == entitytype_mob ||
							       defender->type == entitytype_hero);
#endif
							break;
						}
					}

					if (defender->stats->health <= 0)
					{
#ifdef DENGINE_DEBUG
						DEBUG_LOG("Entity has died");
#endif
						entityStateSwitch(arena, assetManager, audioManager, world, defender,
						                  entitystate_dead);
						entityStateSwitch(arena, assetManager, audioManager, world, attacker,
						                  entitystate_idle);
					}
				}
			}
		}

		if (entity->audioRenderer)
		{
			audio_updateAndPlay(&state->arena, &state->audioManager,
			                    entity->audioRenderer);
		}

		/*
		 **************************************************
		 * Update animations and render entity
		 **************************************************
		 */
		if (entity->tex)
		{
			entity_updateAnim(entity, dt);
			/* Calculate region to render */
			renderer_entity(renderer, cameraBounds, entity, V2(0, 0), 0,
			                V4(1, 1, 1, 1));
		}
	}

	// TODO(doyle): Dead hero not accounted for here
	if (world->numEntitiesInBattle > 0)
	{
		// NOTE(doyle): If battle entities is 1 then only the hero left
		if (hero->state == entitystate_battle &&
		    world->numEntitiesInBattle == 1)
			entityStateSwitch(arena, assetManager, audioManager, world, hero, entitystate_idle);
		else if (hero->state != entitystate_attack)
		{
			entityStateSwitch(arena, assetManager, audioManager, world, hero, entitystate_battle);
		}
	}
	else
	{
		if (hero->state == entitystate_battle)
		{
			hero->state                       = entitystate_idle;
			world->entityIdInBattle[hero->id] = FALSE;
			entity_setActiveAnim(hero, animlist_hero_idle);
		}
		hero->stats->entityIdToAttack = -1;
		hero->stats->actionTimer      = hero->stats->actionRate;
		hero->stats->busyDuration     = 0;
	}

	/* Draw ui */

	/* Draw hero avatar */
	TexAtlas *heroAtlas  = asset_getTextureAtlas(assetManager, texlist_hero);
	v4 heroAvatarTexRect = heroAtlas->texRect[herorects_head];
	v2 heroAvatarSize    = math_getRectSize(heroAvatarTexRect);
	v2 heroAvatarP =
	    V2(10.0f, (renderer->size.h * 0.5f) - (0.5f * heroAvatarSize.h));

	RenderTex heroRenderTex = {hero->tex, heroAvatarTexRect};
	renderer_staticRect(renderer, heroAvatarP, heroAvatarSize, V2(0, 0), 0,
	                    heroRenderTex, V4(1, 1, 1, 1));

	char heroAvatarStr[20];
	snprintf(heroAvatarStr, ARRAY_COUNT(heroAvatarStr), "HP: %3.0f/%3.0f",
	         hero->stats->health, hero->stats->maxHealth);
	f32 strLenInPixels =
	    CAST(f32)(font->maxSize.w * common_strlen(heroAvatarStr));
	v2 strPos = V2(heroAvatarP.x, heroAvatarP.y - (0.5f * heroAvatarSize.h));
	renderer_staticString(&state->renderer, &state->arena, font, heroAvatarStr,
	                      strPos, V2(0, 0), 0, V4(0, 0, 1, 1));

	for (i32 i = 0; i < world->maxEntities; i++)
	{
		Entity *entity = &world->entities[i];
		if (entity->id == hero->id)
			continue;

		if (entity->state == entitystate_attack ||
		    entity->state == entitystate_battle)
		{
			v2 difference    = v2_sub(entity->pos, hero->pos);
			f32 angle        = math_atan2f(difference.y, difference.x);
			f32 angleDegrees = RADIANS_TO_DEGREES(angle);

			Texture *emptyTex = asset_getTexture(assetManager, texlist_empty);
			v2 heroCenter = v2_add(hero->pos, v2_scale(hero->hitboxSize, 0.5f));
			RenderTex renderTex = {emptyTex, V4(0, 1, 1, 0)};
			f32 distance        = v2_magnitude(hero->pos, entity->pos);
			renderer_rect(&state->renderer, cameraBounds, heroCenter,
			              V2(distance, 2.0f), V2(0, 0), angle, renderTex,
			              V4(1, 0, 0, 1.0f));
		}
	}
#ifdef DENGINE_DEBUG
	debug_drawUi(state, dt);
#endif
}
