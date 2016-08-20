#include "Dengine/WorldTraveller.h"
#include "Dengine/Audio.h"
#include "Dengine/Debug.h"
#include "Dengine/Entity.h"
#include "Dengine/Platform.h"
#include "Dengine/UserInterface.h"

enum State
{
	state_active,
	state_menu,
	state_win,
};

enum EventType
{
	eventtype_null = 0,
	eventtype_start_attack,
	eventtype_end_attack,
	eventtype_start_anim,
	eventtype_end_anim,
	eventtype_entity_died,
	eventtype_count,
};

typedef struct Event
{
	enum EventType type;
	void *data;
} Event;

typedef struct EventQueue
{
	Event queue[1024];
	i32 numEvents;
} EventQueue;

INTERNAL Entity *getHeroEntity(World *world)
{
	Entity *result = &world->entities[entity_getIndex(world, world->heroId)];
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

#ifdef DENGINE_DEBUG
	DEBUG_LOG("Renderer initialised");
#endif
}

INTERNAL void assetInit(GameState *state)
{
	AssetManager *assetManager = &state->assetManager;
	MemoryArena *arena = &state->arena;

	/* Create empty 1x1 4bpp black texture */
	u32 bitmap       = (0xFF << 24) | (0xFF << 16) | (0xFF << 8) | (0xFF << 0);
	Texture emptyTex = texture_gen(1, 1, 4, CAST(u8 *)(&bitmap));
	assetManager->textures[texlist_empty] = emptyTex;

	/* Load textures */
	asset_loadTextureImage(assetManager,
	                       "data/textures/WorldTraveller/TerraSprite1024.png",
	                       texlist_hero);
	TexAtlas *heroAtlas = asset_getTextureAtlas(assetManager, texlist_hero);
	heroAtlas->texRect[herorects_idle]       = V4(746, 920, 804, 1018);
	heroAtlas->texRect[herorects_walkA]      = V4(641, 920, 699, 1018);
	heroAtlas->texRect[herorects_walkB]      = V4(849, 920, 904, 1018);
	heroAtlas->texRect[herorects_head]       = V4(108, 975, 159, 1024);
	heroAtlas->texRect[herorects_waveA]      = V4(944, 816, 1010, 918);
	heroAtlas->texRect[herorects_waveB]      = V4(944, 710, 1010, 812);
	heroAtlas->texRect[herorects_battlePose] = V4(8, 814, 71, 910);
	heroAtlas->texRect[herorects_castA]      = V4(428, 814, 493, 910);
	heroAtlas->texRect[herorects_castB]      = V4(525, 816, 590, 919);
	heroAtlas->texRect[herorects_castC]      = V4(640, 816, 698, 916);

	asset_loadTextureImage(assetManager,
	                       "data/textures/WorldTraveller/Terrain.png",
	                       texlist_terrain);
	TexAtlas *terrainAtlas =
	    asset_getTextureAtlas(assetManager, texlist_terrain);
	f32 atlasTileSize = 128.0f;
	const i32 texSize = 1024;
	v2 texOrigin = V2(0, 768);
	terrainAtlas->texRect[terrainrects_ground] =
	    V4(texOrigin.x, texOrigin.y, texOrigin.x + atlasTileSize,
	       texOrigin.y + atlasTileSize);

	/* Load shaders */
	asset_loadShaderFiles(assetManager, arena, "data/shaders/sprite.vert.glsl",
	                      "data/shaders/sprite.frag.glsl",
	                      shaderlist_sprite);

	i32 result =
	    asset_loadTTFont(assetManager, arena, "C:/Windows/Fonts/Arialbd.ttf");

#ifdef DENGINE_DEBUG
	if (result) DEBUG_LOG("Font loading failed");
	GL_CHECK_ERROR();
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
}

INTERNAL void entityInit(GameState *state, v2 windowSize)
{
	AssetManager *assetManager = &state->assetManager;
	MemoryArena *arena = &state->arena;

	/* Init world */
	const i32 targetWorldWidth  = 100 * METERS_TO_PIXEL;
	const i32 targetWorldHeight = 10 * METERS_TO_PIXEL;
#if 0
	v2 worldDimensionInTiles    = V2i(targetWorldWidth / state->tileSize,
	                               targetWorldHeight / state->tileSize);
#else
	v2 worldDimensionInTiles =
	    V2i(CAST(i32)(windowSize.w / state->tileSize) * 2,
	        CAST(i32) windowSize.h / state->tileSize);
#endif

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

		for (i32 y = 0; y < 1; y++)
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
	size            = V2(58.0f, 98.0f);
	pos             = V2(size.x, CAST(f32) state->tileSize);
	type            = entitytype_hero;
	dir             = direction_east;
	tex             = asset_getTexture(assetManager, texlist_hero);
	collides        = TRUE;
	Entity *hero =
	    entity_add(arena, world, pos, size, type, dir, tex, collides);

	hero->audioRenderer = PLATFORM_MEM_ALLOC(arena, 1, AudioRenderer);
	hero->audioRenderer->sourceIndex = AUDIO_SOURCE_UNASSIGNED;
	world->heroId       = hero->id;

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
}

// TODO(doyle): Remove and implement own random generator!
#include <time.h>
#include <stdlib.h>
void worldTraveller_gameInit(GameState *state, v2 windowSize)
{
	i32 result = audio_init(&state->audioManager);
	if (result)
	{
#ifdef DENGINE_DEBUG
		ASSERT(INVALID_CODE_PATH);
#endif
	}

	state->state              = state_active;
	state->currWorldIndex     = 0;
	state->tileSize           = 64;
	state->uiState.keyEntered = keycode_null;
	state->uiState.keyMod     = keycode_null;
	state->uiState.keyChar    = keycode_null;

	common_strncpy(state->uiState.statWindow.title, "Stat Menu",
	               common_strlen("Stat Menu"));
	state->uiState.statWindow.id                  = 99;
	state->uiState.statWindow.rect.pos            = V2(300, 400);
	state->uiState.statWindow.rect.size           = V2(300, 400);
	state->uiState.statWindow.prevFrameWindowHeld = FALSE;
	state->uiState.statWindow.windowHeld          = FALSE;

	common_strncpy(state->uiState.debugWindow.title, "Debug Menu",
	               common_strlen("Debug Menu"));
	state->uiState.debugWindow.id                  = 98;
	state->uiState.debugWindow.rect.pos            = V2(800, 400);
	state->uiState.debugWindow.rect.size           = V2(750, 400);
	state->uiState.debugWindow.prevFrameWindowHeld = FALSE;
	state->uiState.debugWindow.windowHeld          = FALSE;

	state->config.playWorldAudio   = FALSE;
	state->config.showDebugDisplay = TRUE;


	assetInit(state);
	rendererInit(state, windowSize);
	entityInit(state, windowSize);

	srand(CAST(u32)(time(NULL)));
}

INTERNAL inline v4 getEntityScreenRect(Entity entity)
{
	v4 result = math_getRect(entity.pos, entity.hitboxSize);
	return result;
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
	Entity *hero = &world->entities[entity_getIndex(world, world->heroId)];
	v2 ddPos     = V2(0, 0);

	KeyState *keys = state->input.keys;
	for (enum KeyCode i = 0; i < keycode_count; i++)
	{
		KeyState *currKey = &keys[i];
		if (currKey->newHalfTransitionCount > currKey->oldHalfTransitionCount)
		{
			i32 numTransitions = currKey->newHalfTransitionCount -
			                     currKey->oldHalfTransitionCount;

			if (!IS_EVEN(numTransitions))
			{
				currKey->endedDown = ~currKey->endedDown;
			}

		}

		// Parse ui input
		// TODO(doyle): Multi press within frame overrides UI input parsing
		if ((i >= keycode_A && i <= keycode_Z) ||
		    (i >= keycode_a && i <= keycode_z))
		{

			if (getKeyStatus(currKey, readkeytype_repeat, 0.15f, dt))
			{
				state->uiState.keyChar = i;
			}
		}
	}

	if (hero->stats->busyDuration <= 0)
	{
		if (getKeyStatus(&keys[keycode_right], readkeytype_repeat,
		                 KEY_DELAY_NONE, dt))
		{
			ddPos.x         = 1.0f;
			hero->direction = direction_east;
		}

		if (getKeyStatus(&keys[keycode_left], readkeytype_repeat,
		                 KEY_DELAY_NONE, dt))
		{
			ddPos.x         = -1.0f;
			hero->direction = direction_west;
		}

		if (getKeyStatus(&keys[keycode_up], readkeytype_repeat, KEY_DELAY_NONE,
		                 dt))
		{
			ddPos.y = 1.0f;
		}

		if (getKeyStatus(&keys[keycode_down], readkeytype_repeat,
		                 KEY_DELAY_NONE, dt))
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

		if (getKeyStatus(&keys[keycode_enter], readkeytype_oneShot,
		                 KEY_DELAY_NONE, dt))
		{
			state->uiState.keyEntered = keycode_enter;
		}

		if (getKeyStatus(&keys[keycode_tab], readkeytype_delayedRepeat, 0.25f,
		                 dt))
		{
			state->uiState.keyEntered = keycode_tab;
		}

		if (getKeyStatus(&keys[keycode_space], readkeytype_delayedRepeat, 0.25f,
		                 dt))
		{
			state->uiState.keyChar = keycode_space;
		}

		if (getKeyStatus(&keys[keycode_i], readkeytype_delayedRepeat, 0.25f,
		                 dt))
		{
			state->uiState.keyChar = keycode_i;
		}

		if (getKeyStatus(&keys[keycode_backspace], readkeytype_delayedRepeat,
		                 0.1f, dt))
		{
			state->uiState.keyEntered = keycode_backspace;
		}

		if (getKeyStatus(&keys[keycode_left_square_bracket],
		                 readkeytype_delayedRepeat, 0.25f, dt))
		{

			Renderer *renderer = &state->renderer;
			f32 yPos = CAST(f32)(rand() % CAST(i32)renderer->size.h);
			f32 xModifier =  5.0f - CAST(f32)(rand() % 3);

			v2 pos = V2(renderer->size.w - (renderer->size.w / xModifier), yPos);
			entity_addGenericMob(&state->arena, &state->assetManager, world,
			                     pos);
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
	if (getKeyStatus(&state->input.keys[keycode_leftShift], readkeytype_repeat,
	                 KEY_DELAY_NONE, dt))
	{
		// TODO: Context sensitive command separation
		state->uiState.keyMod = keycode_leftShift;
		heroSpeed = CAST(f32)(22.0f * 10.0f * METERS_TO_PIXEL);
	}
	else
	{
		state->uiState.keyMod = keycode_null;
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
			Entity entity = world->entities[i];
			if (entity.state == entitystate_dead) continue;
			if (entity.id == world->heroId) continue;

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

		// NOTE(doyle): Set the camera such that the hero is centered
		v2 offsetFromHeroToCameraOrigin =
		    V2((hero->pos.x - (0.5f * state->renderer.size.w)), (0.0f));

		// NOTE(doyle): Account for the hero's origin being the bottom left
		offsetFromHeroToCameraOrigin.x += (hero->hitboxSize.x * 0.5f);
		world->cameraPos = offsetFromHeroToCameraOrigin;
	}
}

INTERNAL Rect createWorldBoundedCamera(World *world, v2 size)
{
	Rect camera = {world->cameraPos, size};
	// NOTE(doyle): Lock camera if it passes the bounds of the world
	if (camera.pos.x <= world->bounds.x)
		camera.pos.x = world->bounds.x;

	// TODO(doyle): Allow Y panning when we need it
	f32 cameraTopBoundInPixels = camera.pos.y + camera.size.h;
	if (cameraTopBoundInPixels >= world->bounds.w)
		camera.pos.y = (world->bounds.w - camera.size.h);

	f32 cameraRightBoundInPixels = camera.pos.x + camera.size.w;
	if (cameraRightBoundInPixels >= world->bounds.z)
		camera.pos.x = (world->bounds.z - camera.size.w);

	if (camera.pos.y <= world->bounds.y) camera.pos.y = world->bounds.y;
	return camera;
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
		Entity hero = world->entities[entity_getIndex(world, world->heroId)];

		if (hero.state == entitystate_dead) result = ENTITY_NULL_ID;
		else result = hero.id;

		return result;
	}

	/* Attacker is hero */
	Entity hero = attacker;
	for (i32 i = world->maxEntities; i >= 0; i--)
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

INTERNAL registerEvent(EventQueue *eventQueue, enum EventType type, void *data)
{
#ifdef DENGINE_DEBUG
	ASSERT(eventQueue && type < eventtype_count);
	ASSERT(eventQueue->numEvents+1 < ARRAY_COUNT(eventQueue->queue));
#endif
	i32 currIndex = eventQueue->numEvents++;
	eventQueue->queue[currIndex].type = type;
	eventQueue->queue[currIndex].data = data;
}

INTERNAL void entityStateSwitch(EventQueue *eventQueue, World *world,
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
			registerEvent(eventQueue, eventtype_entity_died, CAST(void *)entity);
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
			break;
		}
		case entitystate_idle:
			resetEntityState(world, entity);
			break;
		case entitystate_dead:
			registerEvent(eventQueue, eventtype_entity_died, CAST(void *)entity);
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
			entity_setActiveAnim(entity, animlist_hero_battlePose);
			entity->stats->actionTimer  = entity->stats->actionRate;
			entity->stats->busyDuration = 0;
			break;
		// NOTE(doyle): Entity has been forced out of an attack (out of range)
		case entitystate_idle:
			resetEntityState(world, entity);
			break;
		case entitystate_dead:
			registerEvent(eventQueue, eventtype_entity_died, CAST(void *)entity);
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

typedef struct AttackSpec
{
	Entity *attacker;
	Entity *defender;
	i32 damage;
} AttackSpec;

INTERNAL void beginAttack(EventQueue *eventQueue, World *world,
                          Entity *attacker)
{
#ifdef DENGINE_DEBUG
	ASSERT(attacker->stats->entityIdToAttack != ENTITY_NULL_ID);
	ASSERT(attacker->state == entitystate_battle);
#endif

	entityStateSwitch(eventQueue, world, attacker, entitystate_attack);
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

// TODO(doyle): MemArena here is temporary until we incorporate AttackSpec to
// battle in general!
INTERNAL void endAttack(MemoryArena *arena, EventQueue *eventQueue,
                        World *world, Entity *attacker)
{
#ifdef DENGINE_DEBUG
	ASSERT(attacker->stats->entityIdToAttack != ENTITY_NULL_ID);
#endif

	entityStateSwitch(eventQueue, world, attacker, entitystate_battle);
	switch (attacker->stats->queuedAttack)
	{
	case entityattack_tackle:
		// TODO(doyle): Move animation offsets out and into animation type
		if (attacker->direction == direction_east)
			attacker->dPos.x -= (1.0f * METERS_TO_PIXEL);
		else
			attacker->dPos.x += (1.0f * METERS_TO_PIXEL);

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
		i32 entityIdToAttack = attacker->stats->entityIdToAttack;
		i32 entityIndex      = entity_getIndex(world, entityIdToAttack);

		if (entityIndex != -1)
		{
			defender = &world->entities[entityIndex];
#ifdef DENGINE_DEBUG
			ASSERT(defender->type == entitytype_mob ||
			       defender->type == entitytype_hero);
#endif
		}
		else
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
		i32 damage = 50;

		AttackSpec *attackSpec = PLATFORM_MEM_ALLOC(arena, 1, AttackSpec);
		attackSpec->attacker   = attacker;
		attackSpec->defender   = defender;
		attackSpec->damage     = damage;

		registerEvent(eventQueue, eventtype_end_attack, attackSpec);
		// TODO(doyle): Use attacker stats in battle equations
		if (attacker->type == entitytype_hero)
		{
			defender->stats->health -= damage;
		}

		if (defender->stats->health <= 0)
		{
			entityStateSwitch(eventQueue, world, defender, entitystate_dead);
			entityStateSwitch(eventQueue, world, attacker, entitystate_idle);
		}
	}
}

INTERNAL void sortWorldEntityList(World *world, i32 numDeadEntities)
{
	b32 listHasChanged       = TRUE;
	i32 numUnsortedEntities  = world->freeEntityIndex;
	while (listHasChanged)
	{
		listHasChanged = FALSE;
		for (i32 i = 0; i < numUnsortedEntities-1; i++)
		{
			// TODO(doyle): Better sort algorithm
			Entity *entityA = &world->entities[i];
			Entity *entityB = &world->entities[i+1];
			if (entityA->type == entitytype_null ||
			    (entityB->type != entitytype_null && entityA->id > entityB->id))
			{
				Entity tempEntity         = world->entities[i];
				world->entities[i]        = world->entities[i + 1];
				world->entities[i + 1]    = tempEntity;
				listHasChanged = TRUE;
			}
		}
		numUnsortedEntities--;
	}

	world->freeEntityIndex -= numDeadEntities;
}

typedef struct DamageDisplay
{
	char damageStr[12];
	v2 pos;
	f32 lifetime;
} DamageDisplay;

typedef struct BattleState
{
	DamageDisplay damageDisplay[128];
} BattleState;

GLOBAL_VAR BattleState battleState = {0};

enum RectBaseline
{
	rectbaseline_top,
	rectbaseline_topLeft,
	rectbaseline_topRight,
	rectbaseline_bottom,
	rectbaseline_bottomRight,
	rectbaseline_bottomLeft,
	rectbaseline_left,
	rectbaseline_right,
	rectbaseline_center,
	rectbaseline_count,

};

INTERNAL v2 getPosRelativeToRect(Rect rect, v2 offset,
                                 enum RectBaseline baseline)
{
#ifdef DENGINE_DEBUG
	ASSERT(baseline < rectbaseline_count);
#endif
	v2 result = {0};

	v2 posToOffsetFrom = rect.pos;
	switch (baseline)
	{
		case rectbaseline_top:
			posToOffsetFrom.y += (rect.size.h);
			posToOffsetFrom.x += (rect.size.w * 0.5f);
		    break;
		case rectbaseline_topLeft:
			posToOffsetFrom.y += (rect.size.h);
			break;
		case rectbaseline_topRight:
			posToOffsetFrom.y += (rect.size.h);
			posToOffsetFrom.x += (rect.size.w);
			break;
		case rectbaseline_bottom:
			posToOffsetFrom.x += (rect.size.w * 0.5f);
			break;
		case rectbaseline_bottomRight:
			posToOffsetFrom.x += (rect.size.w);
			break;
		case rectbaseline_left:
			posToOffsetFrom.y += (rect.size.h * 0.5f);
			break;
		case rectbaseline_right:
			posToOffsetFrom.x += (rect.size.w);
			posToOffsetFrom.y += (rect.size.h * 0.5f);
			break;

		case rectbaseline_bottomLeft:
		    break;
		default:
#ifdef DENGINE_DEBUG
		    DEBUG_LOG(
		        "getPosRelativeToRect() warning: baseline enum not recognised");
#endif
			break;
	}

	result = v2_add(posToOffsetFrom, offset);
	return result;
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
	EventQueue eventQueue = {0};
	Rect camera           = createWorldBoundedCamera(world, renderer->size);
	AudioManager *audioManager = &state->audioManager;
	ASSERT(world->freeEntityIndex < world->maxEntities);
	for (i32 i = 0; i < world->freeEntityIndex; i++)
	{
		Entity *const entity = &world->entities[i];
		if (entity->state == entitystate_dead) continue;

		if (entity->type == entitytype_soundscape)
		{
			AudioRenderer *audioRenderer = entity->audioRenderer;
			if (!state->config.playWorldAudio)
			{
				// TODO(doyle): Use is playing flag, not just streaming flag
				if (audioRenderer->state == audiostate_playing)
				{
					audio_pauseVorbis(&state->audioManager, audioRenderer);
				}
			}
			else
			{
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
							audio_streamPlayVorbis(
							    arena, &state->audioManager, audioRenderer,
							    overworldTheme, AUDIO_REPEAT_INFINITE);
						}
					}
					else
					{
						audio_streamPlayVorbis(arena, &state->audioManager,
						                       audioRenderer, overworldTheme,
						                       AUDIO_REPEAT_INFINITE);
					}
				}

				if (audioRenderer->state == audiostate_paused)
				{
					audio_resumeVorbis(&state->audioManager, audioRenderer);
				}
			}
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
			Entity *hero        = getHeroEntity(world);
			f32 distance        = v2_magnitude(hero->pos, entity->pos);

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

			entityStateSwitch(&eventQueue, world, entity, newState);
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
					beginAttack(&eventQueue, world, entity);
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
					endAttack(&state->arena, &eventQueue, world, entity);
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
			renderer_entity(renderer, camera, entity, V2(0, 0), 0,
			                V4(1, 1, 1, 1));
		}
	}

	i32 numDeadEntities = 0;
	for (i32 i = 0; i < eventQueue.numEvents; i++)
	{
		Event event = eventQueue.queue[i];
		switch(event.type)
		{
		case eventtype_end_attack:
		{
			if (!event.data) continue;

			AttackSpec *attackSpec = (CAST(AttackSpec *) event.data);
			Entity *attacker       = attackSpec->attacker;
			Entity *defender       = attackSpec->defender;

			audio_playVorbis(arena, audioManager, attacker->audioRenderer,
			                 asset_getVorbis(assetManager, audiolist_tackle),
			                 1);

			/* Get first free string position and store the damage str data */
			for (i32 i = 0; i < ARRAY_COUNT(battleState.damageDisplay); i++)
			{
				if (battleState.damageDisplay[i].lifetime <= 0.0f)
				{
					common_memset(
					    battleState.damageDisplay[i].damageStr, 0,
					    ARRAY_COUNT(battleState.damageDisplay[i].damageStr));

					battleState.damageDisplay[i].lifetime = 1.0f;
					battleState.damageDisplay[i].pos      = defender->pos;
					common_itoa(
					    attackSpec->damage,
					    battleState.damageDisplay[i].damageStr,
					    ARRAY_COUNT(battleState.damageDisplay[i].damageStr));

					break;
				}
			}

			PLATFORM_MEM_FREE(&state->arena, attackSpec, sizeof(AttackSpec));
			break;
		}
		// NOTE(doyle): We delete dead entities at the end of the update
		// loop incase a later indexed entity deletes an earlier indexed
		// entity, the entity will exist for an additional frame
		case eventtype_entity_died:
		{
#ifdef DEGINE_DEBUG
			DEBUG_LOG("Entity died: being deleted");
#endif
			if (!event.data) continue;

			Entity *entity = (CAST(Entity *) event.data);
			audio_stopVorbis(&state->arena, audioManager,
			                 entity->audioRenderer);
			entity_clearData(&state->arena, world, entity);
			numDeadEntities++;
			break;
		}
		default:
		{
			break;
		}
		}
	}

	/* Render all damage strings */
	for (i32 i = 0; i < ARRAY_COUNT(battleState.damageDisplay); i++)
	{
		if (battleState.damageDisplay[i].lifetime > 0.0f)
		{
			battleState.damageDisplay[i].lifetime -= dt;

			char *damageString = battleState.damageDisplay[i].damageStr;
			v2 damagePos       = battleState.damageDisplay[i].pos;
			f32 lifetime       = battleState.damageDisplay[i].lifetime;

			// TODO(doyle): Incorporate UI into entity list or it's own list
			// with a rendering lifetime value
			renderer_string(renderer, &state->arena, camera, font,
			                damageString, damagePos, V2(0, 0), 0,
			                V4(1, 1, 1, lifetime));
		}
	}

	// TODO(doyle): Dead hero not accounted for here
	Entity *hero = getHeroEntity(world);
	if (world->numEntitiesInBattle > 0)
	{
		// NOTE(doyle): If battle entities is 1 then only the hero left
		if (hero->state == entitystate_battle &&
		    world->numEntitiesInBattle == 1)
			entityStateSwitch(&eventQueue, world, hero, entitystate_idle);
		else if (hero->state != entitystate_attack)
		{
			entityStateSwitch(&eventQueue, world, hero, entitystate_battle);
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

	// INIT IMGUI
	state->uiState.hotItem = 0;

	/* Draw ui */
	if (state->uiState.keyChar == keycode_i)
	{
		state->config.showStatMenu =
		    (state->config.showStatMenu == TRUE) ? FALSE : TRUE;
	}

	if (state->config.showStatMenu)
	{
		WindowState *statWindow = &state->uiState.statWindow;
		userInterface_window(&state->uiState, &state->arena, assetManager,
		                     renderer, font, state->input, statWindow);
	}

	/* Draw debug window */
	WindowState *debugWindow = &state->uiState.debugWindow;
	userInterface_window(&state->uiState, &state->arena, assetManager, renderer,
	                     font, state->input, debugWindow);

	// TODO(doyle): Bug in font rendering once button reaches 700-800+ pixels
	Rect toggleAudioButtonRect = {0};
	toggleAudioButtonRect.size = V2(100, 50);
	toggleAudioButtonRect.pos  = getPosRelativeToRect(
	    debugWindow->rect, V2(10, -65.0f), rectbaseline_topLeft);

	b32 toggleAudioClicked = userInterface_button(
	    &state->uiState, &state->arena, assetManager, renderer, font,
	    state->input, 1, toggleAudioButtonRect, "Toggle Music");

	if (toggleAudioClicked)
	{
		state->config.playWorldAudio =
		    (state->config.playWorldAudio == TRUE) ? FALSE : TRUE;
	}

	Rect toggleDebugButtonRect = {0};
	toggleDebugButtonRect.size = V2(100, 50);
	toggleDebugButtonRect.pos  = getPosRelativeToRect(
	    toggleAudioButtonRect, V2(25, 0), rectbaseline_bottomRight);
	b32 toggleDebugClicked = userInterface_button(
	    &state->uiState, &state->arena, assetManager, renderer, font,
	    state->input, 2, toggleDebugButtonRect, "Toggle Debug Display");

	if (toggleDebugClicked)
	{
		state->config.showDebugDisplay =
		    (state->config.showDebugDisplay == TRUE) ? FALSE : TRUE;
	}

	LOCAL_PERSIST i32 scrollValue = 30;
	Rect scrollRectA              = {V2(0, 0), V2(16, 255)};
	scrollRectA.pos = getPosRelativeToRect(debugWindow->rect, V2(-20, -260),
	                                       rectbaseline_topRight);
	userInterface_scrollBar(&state->uiState, assetManager, renderer,
	                        state->input, 3, scrollRectA, &scrollValue, 160);

	LOCAL_PERSIST char fieldString[80] = "Hello world";
	v2 textFieldP = getPosRelativeToRect(toggleAudioButtonRect, V2(0, -20),
	                                     rectbaseline_bottomLeft);
	userInterface_textField(&state->uiState, &state->arena, assetManager,
	                        renderer, font, state->input, 4, textFieldP,
	                        fieldString);

	// RESET IMGUI
	if (!state->input.keys[keycode_mouseLeft].endedDown)
		state->uiState.activeItem = 0;
	else if (state->uiState.activeItem == 0)
		state->uiState.activeItem = -1;

	if (state->uiState.keyEntered == keycode_tab) state->uiState.kbdItem = 0;
	state->uiState.keyEntered = keycode_null;
	state->uiState.keyChar    = keycode_null;

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

			v2 heroCenter = v2_add(hero->pos, v2_scale(hero->hitboxSize, 0.5f));
			RenderTex renderTex = renderer_createNullRenderTex(assetManager);
			f32 distance        = v2_magnitude(hero->pos, entity->pos);
			renderer_rect(&state->renderer, camera, heroCenter,
			              V2(distance, 2.0f), V2(0, 0), angle, renderTex,
			              V4(1, 0, 0, 1.0f));
		}
	}

	for (i32 i = 0; i < keycode_count; i++)
	{
		state->input.keys[i].oldHalfTransitionCount =
		    state->input.keys[i].newHalfTransitionCount;
	}

	sortWorldEntityList(world, numDeadEntities);

#ifdef DENGINE_DEBUG
	for (i32 i = 0; i < world->freeEntityIndex-1; i++)
	{
		ASSERT(world->entities[i].type != entitytype_null);
		ASSERT(world->entities[i].id < world->entities[i+1].id);
	}
#endif

#ifdef DENGINE_DEBUG
	if (state->config.showDebugDisplay)
	{
		debug_drawUi(state, dt);
	}
#endif
}
