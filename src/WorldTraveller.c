#include "Dengine/WorldTraveller.h"
#include "Dengine/Audio.h"
#include "Dengine/Debug.h"
#include "Dengine/Entity.h"
#include "Dengine/Platform.h"
#include "Dengine/String.h"
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

INTERNAL void addGenericMob(MemoryArena *arena, AssetManager *assetManager,
                            World *world, v2 pos)
{
#ifdef DENGINE_DEBUG
	DEBUG_LOG("Mob entity spawned");
#endif

	Entity *hero = &world->entities[entity_getIndex(world, world->heroId)];

	v2 size              = V2(58.0f, 98.0f);
	f32 scale            = 2;
	enum EntityType type = entitytype_mob;
	enum Direction dir   = direction_west;
	Texture *tex         = asset_getTex(assetManager, "ClaudeSprite.png");
	b32 collides         = TRUE;
	Entity *mob =
	    entity_add(arena, world, pos, size, scale, type, dir, tex, collides);

	mob->numAudioRenderers = 4;
	mob->audioRenderer =
	    PLATFORM_MEM_ALLOC(arena, mob->numAudioRenderers, AudioRenderer);

	for (i32 i = 0; i < mob->numAudioRenderers; i++)
		mob->audioRenderer[i].sourceIndex = AUDIO_SOURCE_UNASSIGNED;

	/* Populate mob animation references */
	entity_addAnim(assetManager, mob, "claudeIdle");
	entity_addAnim(assetManager, mob, "claudeRun");
	entity_addAnim(assetManager, mob, "claudeBattleIdle");
	entity_addAnim(assetManager, mob, "claudeAttack");
	entity_setActiveAnim(mob, "claudeIdle");
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
	MemoryArena *arena         = &state->arena;

	i32 audioEntries         = 32;
	assetManager->audio.size = audioEntries;
	assetManager->audio.entries =
	    PLATFORM_MEM_ALLOC(arena, audioEntries, HashTableEntry);

	i32 texAtlasEntries         = 8;
	assetManager->texAtlas.size = texAtlasEntries;
	assetManager->texAtlas.entries =
	    PLATFORM_MEM_ALLOC(arena, texAtlasEntries, HashTableEntry);

	i32 texEntries          = 32;
	assetManager->textures.size = texEntries;
	assetManager->textures.entries =
	    PLATFORM_MEM_ALLOC(arena, texEntries, HashTableEntry);

	i32 animEntries          = 1024;
	assetManager->anims.size = animEntries;
	assetManager->anims.entries =
	    PLATFORM_MEM_ALLOC(arena, animEntries, HashTableEntry);

	/* Create empty 1x1 4bpp black texture */
	u32 bitmap   = (0xFF << 24) | (0xFF << 16) | (0xFF << 8) | (0xFF << 0);
	Texture *tex = asset_getFreeTexSlot(assetManager, arena, "nullTex");
	*tex         = texture_gen(1, 1, 4, CAST(u8 *)(&bitmap));

	/*
	 *********************************
	 * Load terrain texture atlas data
	 *********************************
	 */
	PlatformFileRead terrainXml = {0};
	i32 result = platform_readFileToBuffer(
	    arena, "data/textures/WorldTraveller/terrain.xml", &terrainXml);
	if (result)
	{
		DEBUG_LOG("Failed to read sprite sheet xml");
	}
	else
	{
		result = asset_loadXmlFile(assetManager, arena, &terrainXml);

		if (!result)
		{
			TexAtlas *terrainAtlas =
			    asset_getTexAtlas(assetManager, "terrain.png");

			i32 numSubTextures    = 1;
			f32 duration          = 1.0f;
			char *grassTerrain[1] = {"grass.png"};
			asset_addAnimation(assetManager, arena, "terrainGrass",
			                   terrainAtlas, grassTerrain, numSubTextures,
			                   duration);
		}
		else
		{
#ifdef DENGINE_DEBUG
			DEBUG_LOG("Failed to load terrain sprite xml data");
#endif
		}

		platform_closeFileRead(arena, &terrainXml);
	}

	/*
	 *********************************
	 * Load Claude texture atlas data
	 *********************************
	 */
	PlatformFileRead claudeXml = {0};
	result = platform_readFileToBuffer(
	    arena, "data/textures/WorldTraveller/ClaudeSprite.xml", &claudeXml);

	if (result)
	{
		DEBUG_LOG("Failed to read sprite sheet xml");
	}
	else
	{
		result = asset_loadXmlFile(assetManager, arena, &claudeXml);

		if (!result)
		{
			TexAtlas *claudeAtlas =
			    asset_getTexAtlas(assetManager, "ClaudeSprite.png");

			char *claudeIdle[1] = {"ClaudeSprite_Walk_Left_01"};
			f32 duration        = 1.0f;
			i32 numRects        = ARRAY_COUNT(claudeIdle);
			asset_addAnimation(assetManager, arena, "claudeIdle", claudeAtlas,
			                   claudeIdle, numRects, duration);

			// Run animation
			char *claudeRun[6] = {
			    "ClaudeSprite_Run_Left_01", "ClaudeSprite_Run_Left_02",
			    "ClaudeSprite_Run_Left_03", "ClaudeSprite_Run_Left_04",
			    "ClaudeSprite_Run_Left_05", "ClaudeSprite_Run_Left_06"};
			duration = 0.1f;
			numRects = ARRAY_COUNT(claudeRun);
			asset_addAnimation(assetManager, arena, "claudeRun", claudeAtlas,
			                   claudeRun, numRects, duration);

			// Battle Idle animation
			char *claudeBattleIdle[3] = {"ClaudeSprite_BattleIdle_Left_01",
			                             "ClaudeSprite_BattleIdle_Left_02",
			                             "ClaudeSprite_BattleIdle_Left_03"};
			numRects = ARRAY_COUNT(claudeBattleIdle);
			duration = 0.2f;
			asset_addAnimation(assetManager, arena, "claudeBattleIdle",
			                   claudeAtlas, claudeBattleIdle, numRects,
			                   duration);

			// Attack Left animation
			char *claudeAttack[6] = {
			    "ClaudeSprite_Attack_Left_01", "ClaudeSprite_Attack_Left_02",
			    "ClaudeSprite_Attack_Left_03", "ClaudeSprite_Attack_Left_04",
			    "ClaudeSprite_Attack_Left_05", "ClaudeSprite_Attack_Left_06"};
			numRects = ARRAY_COUNT(claudeAttack);
			duration = 0.1f;
			asset_addAnimation(assetManager, arena, "claudeAttack", claudeAtlas,
			                   claudeAttack, numRects, duration);

			//  Victory animation
			char *claudeVictory[8] = {"ClaudeSprite_Battle_Victory_01",
			                          "ClaudeSprite_Battle_Victory_02",
			                          "ClaudeSprite_Battle_Victory_03",
			                          "ClaudeSprite_Battle_Victory_04",
			                          "ClaudeSprite_Battle_Victory_05",
			                          "ClaudeSprite_Battle_Victory_06",
			                          "ClaudeSprite_Battle_Victory_07",
			                          "ClaudeSprite_Battle_Victory_08"};
			numRects = ARRAY_COUNT(claudeVictory);
			duration = 0.1f;
			asset_addAnimation(assetManager, arena, "claudeVictory",
			                   claudeAtlas, claudeVictory, numRects, duration);

			char *claudeEnergySword[6] = {"ClaudeSprite_Attack_EnergySword_01",
			                              "ClaudeSprite_Attack_EnergySword_02",
			                              "ClaudeSprite_Attack_EnergySword_03",
			                              "ClaudeSprite_Attack_EnergySword_04",
			                              "ClaudeSprite_Attack_EnergySword_05",
			                              "ClaudeSprite_Attack_EnergySword_06"};
			numRects = ARRAY_COUNT(claudeEnergySword);
			duration = 0.1f;
			asset_addAnimation(assetManager, arena, "claudeEnergySword",
			                   claudeAtlas, claudeEnergySword, numRects,
			                   duration);

			char *claudeAirSlash[7] = {"ClaudeSprite_Attack_AirSlash_01",
			                           "ClaudeSprite_Attack_AirSlash_02",
			                           "ClaudeSprite_Attack_AirSlash_03",
			                           "ClaudeSprite_Attack_AirSlash_04",
			                           "ClaudeSprite_Attack_AirSlash_05",
			                           "ClaudeSprite_Attack_AirSlash_06",
			                           "ClaudeSprite_Attack_AirSlash_07"};
			numRects = ARRAY_COUNT(claudeAirSlash);
			duration = 0.075f;
			asset_addAnimation(assetManager, arena, "claudeAirSlash",
			                   claudeAtlas, claudeAirSlash, numRects,
			                   duration);

			char *claudeAirSlashVfx[7] = {"ClaudeSprite_Attack_AirSlash_Vfx_01",
			                           "ClaudeSprite_Attack_AirSlash_Vfx_02",
			                           "ClaudeSprite_Attack_AirSlash_Vfx_03",
			                           "ClaudeSprite_Attack_AirSlash_Vfx_04",
			                           "ClaudeSprite_Attack_AirSlash_Vfx_05",
			                           "ClaudeSprite_Attack_AirSlash_Vfx_06",
			                           "ClaudeSprite_Attack_AirSlash_Vfx_07"};
			numRects = ARRAY_COUNT(claudeAirSlashVfx);
			duration = 0.075f;
			asset_addAnimation(assetManager, arena, "claudeAirSlashVfx",
			                   claudeAtlas, claudeAirSlashVfx, numRects,
			                   duration);

			char *claudeSword[1] = {
			    "ClaudeSprite_Sword_01",
			};

			numRects = ARRAY_COUNT(claudeSword);
			duration = 0.4f;
			asset_addAnimation(assetManager, arena, "claudeSword",
			                   claudeAtlas, claudeSword, numRects,
			                   duration);

			char *claudeAttackSlashLeft[4] = {
			    "ClaudeSprite_Attack_Slash_Left_01",
			    "ClaudeSprite_Attack_Slash_Left_02",
			    "ClaudeSprite_Attack_Slash_Left_03",
			    "ClaudeSprite_Attack_Slash_Left_04",
			};

			numRects = ARRAY_COUNT(claudeAttackSlashLeft);
			duration = 0.1f;
			asset_addAnimation(assetManager, arena, "claudeAttackSlashLeft",
			                   claudeAtlas, claudeAttackSlashLeft, numRects,
			                   duration);
		}
		else
		{
#ifdef DENGINE_DEBUG
			DEBUG_LOG("Failed to load claude sprite xml data");
#endif
		}

		platform_closeFileRead(arena, &claudeXml);
	}


#ifdef DENGINE_DEBUG
	DEBUG_LOG("Animations created");
#endif

	/* Load shaders */
	asset_loadShaderFiles(assetManager, arena, "data/shaders/sprite.vert.glsl",
	                      "data/shaders/sprite.frag.glsl",
	                      shaderlist_sprite);

	result =
	    asset_loadTTFont(assetManager, arena, "C:/Windows/Fonts/Arialbd.ttf");

#ifdef DENGINE_DEBUG
	if (result) DEBUG_LOG("Font loading failed");
	GL_CHECK_ERROR();
	DEBUG_LOG("Assets loaded");
#endif

	/* Load sound */

	i32 before = arena->bytesAllocated;

	char *sfxListPath = "data/audio/sfx/sfx.txt";
	PlatformFileRead sfxList = {0};
	result = platform_readFileToBuffer(arena, sfxListPath, &sfxList);

	char *sfxAudioNames[256];
	i32 sfxAudioIndex = 0;
	if (!result)
	{
		char string[256] = {0};
		i32 stringIndex  = 0;
		for (i32 i = 0; i < sfxList.size; i++)
		{
			char c = (CAST(char *)sfxList.buffer)[i];
			switch(c)
			{
				case 0x0a:
				{
				    i32 actualStrLen = common_strlen(string) + 1;
				    sfxAudioNames[sfxAudioIndex] =
				        PLATFORM_MEM_ALLOC(arena, actualStrLen, char);
				    common_strncpy(sfxAudioNames[sfxAudioIndex++], string,
				                   actualStrLen);

				    common_memset(string, 0, ARRAY_COUNT(string));
				    stringIndex = 0;
				    break;
			    }
				default:
				{
					if (c >= ' ' && c <= '~')
					{
					    string[stringIndex++] = c;
				    }
					break;
				}
			}
		}
	}

	char *sfxDir        = "data/audio/sfx/";
	char *sfxExtension  = ".ogg";
	i32 sfxDirLen       = common_strlen(sfxDir);
	i32 sfxExtensionLen = common_strlen(sfxExtension);

	for (i32 i = 0; i < sfxAudioIndex; i++)
	{
		char *sfxName  = sfxAudioNames[i];
		i32 sfxNameLen = common_strlen(sfxName);

		i32 sfxFullPathLen = sfxDirLen + sfxExtensionLen + sfxNameLen + 1;
		char *sfxFullPath  = PLATFORM_MEM_ALLOC(arena, sfxFullPathLen, char);

		common_strncat(sfxFullPath, sfxDir, sfxDirLen);
		common_strncat(sfxFullPath, sfxName, sfxNameLen);
		common_strncat(sfxFullPath, sfxExtension, sfxExtensionLen);

		i32 result = asset_loadVorbis(assetManager, arena, sfxFullPath, sfxName);
		if (result) DEBUG_LOG("Failed to load sfx file");

		// TODO(doyle): Need better string type to account for null terminating
		// character, having to remember to +1 on allocation AND freeing since
		// strlen only counts until null char is going to leave memory leaks
		// everywhere
		PLATFORM_MEM_FREE(arena, sfxName, sfxNameLen * sizeof(char) + 1);
		PLATFORM_MEM_FREE(arena, sfxFullPath, sfxFullPathLen * sizeof(char));
	}

	platform_closeFileRead(arena, &sfxList);

	char *audioPath =
	    "data/audio/Motoi Sakuraba - Stab the sword of justice.ogg";
	asset_loadVorbis(assetManager, arena, audioPath, "audio_battle");
	audioPath = "data/audio/Motoi Sakuraba - Field of Exper.ogg";
	asset_loadVorbis(assetManager, arena, audioPath, "audio_overworld");
	audioPath = "data/audio/nuindependent_hit22.ogg";
	asset_loadVorbis(assetManager, arena, audioPath, "audio_tackle");

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
		world->bounds =
		    math_getRect(V2(0, 0), v2_scale(worldDimensionInTiles,
		                                    CAST(f32) state->tileSize));
		world->uniqueIdAccumulator = 0;

#if 1
		TexAtlas *const atlas = asset_getTexAtlas(assetManager, "terrain.png");

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
				f32 scale = 1.0f;
				enum EntityType type = entitytype_tile;
				enum Direction dir = direction_null;
				Texture *tex = asset_getTex(assetManager, "terrain.png");
				b32 collides = FALSE;
				Entity *tile = entity_add(arena, world, pos, size, scale, type,
				                          dir, tex, collides);

				entity_addAnim(assetManager, tile, "terrainGrass");
				entity_setActiveAnim(tile, "terrainGrass");
			}
		}
#endif
	}

	World *const world = &state->world[state->currWorldIndex];
	world->cameraPos = V2(0.0f, 0.0f);

	/* Add world soundscape */
	Renderer *renderer   = &state->renderer;
	v2 size              = V2(10.0f, 10.0f);
	v2 pos               = V2(0, 0);
	f32 scale            = 0.0f;
	enum EntityType type = entitytype_soundscape;
	enum Direction dir   = direction_null;
	Texture *tex         = NULL;
	b32 collides         = FALSE;
	Entity *soundscape =
	    entity_add(arena, world, pos, size, scale, type, dir, tex, collides);

	world->soundscape = soundscape;
	soundscape->numAudioRenderers = 1;
	soundscape->audioRenderer =
	    PLATFORM_MEM_ALLOC(arena, soundscape->numAudioRenderers, AudioRenderer);
	for (i32 i = 0; i < soundscape->numAudioRenderers; i++)
		soundscape->audioRenderer[i].sourceIndex = AUDIO_SOURCE_UNASSIGNED;

	/* Init hero entity */
	size            = V2(58.0f, 98.0f);
	pos             = V2(size.x, CAST(f32) state->tileSize);
	scale           = 2.0f;
	type            = entitytype_hero;
	dir             = direction_east;
	tex             = asset_getTex(assetManager, "ClaudeSprite.png");
	collides        = TRUE;
	Entity *hero =
	    entity_add(arena, world, pos, size, scale, type, dir, tex, collides);

#if 0
	Entity *heroWeapon =
	    entity_add(arena, world, pos, V2(0, 0), entitytype_weapon,
	               hero->direction, hero->tex, FALSE);
	heroWeapon->rotation = -90.0f;
	heroWeapon->invisible = TRUE;
	entity_addAnim(assetManager, heroWeapon, "claudeSword");

	hero->stats->weapon = heroWeapon;
#endif

	hero->numAudioRenderers = 4;
	hero->audioRenderer =
	    PLATFORM_MEM_ALLOC(arena, hero->numAudioRenderers, AudioRenderer);

	for (i32 i = 0; i < hero->numAudioRenderers; i++)
		hero->audioRenderer[i].sourceIndex = AUDIO_SOURCE_UNASSIGNED;

	world->heroId                    = hero->id;
	world->cameraFollowingId         = hero->id;

	/* Populate hero animation references */
	entity_addAnim(assetManager, hero, "claudeIdle");
	entity_addAnim(assetManager, hero, "claudeRun");
	entity_addAnim(assetManager, hero, "claudeBattleIdle");
	entity_addAnim(assetManager, hero, "claudeAttack");
	entity_addAnim(assetManager, hero, "claudeEnergySword");
	entity_addAnim(assetManager, hero, "claudeAirSlash");
	entity_setActiveAnim(hero, "claudeIdle");

	/* Create a NPC */
	pos         = V2(hero->pos.x * 3, CAST(f32) state->tileSize);
	size        = hero->hitboxSize;
	type        = entitytype_npc;
	dir         = direction_null;
	tex         = hero->tex;
	collides    = FALSE;
	Entity *npc =
	    entity_add(arena, world, pos, size, scale, type, dir, tex, collides);

	/* Populate npc animation references */
	entity_addAnim(assetManager, npc, "claudeVictory");
	entity_setActiveAnim(npc, "claudeVictory");

	/* Create a Mob */
	pos = V2(renderer->size.w - (renderer->size.w / 3.0f),
	         CAST(f32) state->tileSize);
	addGenericMob(arena, assetManager, world, pos);

#ifdef DENGINE_DEBUG
	DEBUG_LOG("World populated");
#endif
}

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

INTERNAL void unitTest(MemoryArena *arena)
{
	ASSERT(common_atoi("-2", common_strlen("-2")) == -2);
	ASSERT(common_atoi("100", common_strlen("100")) == 100);
	ASSERT(common_atoi("1", common_strlen("1")) == 1);
	ASSERT(common_atoi("954 32", common_strlen("954 32")) == 954);

	ASSERT(common_atoi("", 0) == -1);
	ASSERT(common_atoi(" 32", common_strlen(" 32")) == -1);
	ASSERT(common_atoi("+32", common_strlen("+32")) == 32);
	ASSERT(common_atoi("+ 32", common_strlen("+ 32")) == 0);
	asset_unitTest(arena);

	i32 memBefore  = arena->bytesAllocated;
	String *hello = string_make(arena, "hello, ");
	String *world = string_make(arena, "world");
	ASSERT(string_len(hello) == 7);
	ASSERT(string_len(world) == 5);

	hello = string_append(arena, hello, world, string_len(world));
	ASSERT(string_len(hello) == 12);
	string_free(arena, hello);
	string_free(arena, world);

	hello = string_make(arena, "");
	world = string_make(arena, "");
	hello = string_append(arena, hello, world, string_len(world));
	ASSERT(string_len(hello) == 0);
	ASSERT(string_len(world) == 0);

	string_free(arena, hello);
	string_free(arena, world);

	i32 memAfter = arena->bytesAllocated;

	ASSERT(memBefore == memAfter);
}

// TODO(doyle): Remove and implement own random generator!
#include <time.h>
#include <stdlib.h>
void worldTraveller_gameInit(GameState *state, v2 windowSize)
{
#ifdef DENGINE_DEBUG
	unitTest(&state->arena);
#endif

	i32 result = audio_init(&state->audioManager);
	if (result)
	{
#ifdef DENGINE_DEBUG
		ASSERT(INVALID_CODE_PATH);
#endif
	}

	state->state              = state_active;
	state->currWorldIndex     = 0;
	state->tileSize           = 70;

	state->uiState.uniqueId   = 1;
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


	WindowState *debugWindow = &state->uiState.debugWindow;
	common_strncpy(debugWindow->title, "Debug Menu",
	               common_strlen("Debug Menu"));
	debugWindow->id                  = 98;
	debugWindow->numChildUiItems     = 0;
	debugWindow->rect.size           = V2(400, 200);
	debugWindow->rect.pos = V2(windowSize.w - debugWindow->rect.size.w - 10,
	                           windowSize.h - debugWindow->rect.size.h - 10);
	debugWindow->prevFrameWindowHeld = FALSE;
	debugWindow->windowHeld          = FALSE;

	UiItem *audioBtn =
	    (debugWindow->childUiItems) + debugWindow->numChildUiItems++;
	common_strncpy(audioBtn->label, "Toggle Music",
	               common_strlen("Toggle Music"));
	audioBtn->id        = userInterface_generateId(&state->uiState);
	audioBtn->rect.size = V2(100, 50);
	audioBtn->rect.pos = getPosRelativeToRect(debugWindow->rect, V2(10, -65.0f),
	                                          rectbaseline_topLeft);
	audioBtn->type = uitype_button;

	UiItem *debugBtn =
	    (debugWindow->childUiItems) + debugWindow->numChildUiItems++;
	common_strncpy(debugBtn->label, "Toggle Debug",
	               common_strlen("Toggle Debug"));
	debugBtn->id        = userInterface_generateId(&state->uiState);
	debugBtn->rect.size = V2(100, 50);
	debugBtn->rect.pos  = getPosRelativeToRect(audioBtn->rect, V2(25, 0),
	                                          rectbaseline_bottomRight);
	debugBtn->type = uitype_button;

	UiItem *scrollbar =
	    (debugWindow->childUiItems) + debugWindow->numChildUiItems++;
	scrollbar->id     = userInterface_generateId(&state->uiState);

	scrollbar->rect.size = V2(16, debugWindow->rect.size.h);
	scrollbar->rect.pos =
	    getPosRelativeToRect(debugWindow->rect, V2(-scrollbar->rect.size.w, 0),
	                         rectbaseline_bottomRight);
	scrollbar->value    = 0;
	scrollbar->maxValue = 160;
	scrollbar->type = uitype_scrollbar;

	UiItem *textField =
	    (debugWindow->childUiItems) + debugWindow->numChildUiItems++;
	textField->id = userInterface_generateId(&state->uiState);
	textField->rect.size = V2(200, 20);
	textField->rect.pos  = getPosRelativeToRect(
	    audioBtn->rect, V2(0, -textField->rect.size.h - 10),
	    rectbaseline_bottomLeft);

	common_strncpy(textField->string, "Hello world",
	               common_strlen("Hello world"));
	textField->type = uitype_textField;

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
	entity_setActiveAnim(entity, "claudeIdle");
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
			entity_setActiveAnim(entity, "claudeIdle");
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
			entity_setActiveAnim(entity, "claudeBattleIdle");
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

INTERNAL void beginAttack(AssetManager *assetManager, MemoryArena *arena,
                          EventQueue *eventQueue, World *world,
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
	{
		entity_setActiveAnim(attacker, "claudeAttack");
		if (attacker->stats->weapon)
		{
			attacker->stats->weapon->invisible = FALSE;
			entity_setActiveAnim(attacker->stats->weapon,
			                     "claudeAttackSlashLeft");
		}
		if (attacker->direction == direction_east)
			attacker->dPos.x += (1.0f * METERS_TO_PIXEL);
		else
			attacker->dPos.x -= (1.0f * METERS_TO_PIXEL);
		break;
	}

	case entityattack_energySword:
	{
		entity_setActiveAnim(attacker, "claudeEnergySword");
		break;
	}

	case entityattack_airSlash:
	{
		entity_setActiveAnim(attacker, "claudeAirSlash");
		f32 scale = 1.5f;
		v2 size = V2(20, 20);
		Entity *projectile = entity_add(
		    arena, world, attacker->pos, size, scale,
		    entitytype_projectile, attacker->direction, attacker->tex, TRUE);

		projectile->collidesWith[entitytype_hero] = FALSE;
		projectile->collidesWith[entitytype_mob]  = TRUE;

		projectile->stats->entityIdToAttack = attacker->stats->entityIdToAttack;
		entity_addAnim(assetManager, projectile, "claudeAirSlashVfx");
		entity_setActiveAnim(projectile, "claudeAirSlashVfx");

		v2 initialOffset      = V2(size.x * 0.5f, 0);
		f32 deltaScale        = 0.3f;
		projectile->numChilds = 3;
		for (i32 i = 0; i < projectile->numChilds; i++)
		{
			v2 childOffset = v2_scale(initialOffset, CAST(f32) i + 1);
			scale -= deltaScale;

			Entity *child =
			    entity_add(arena, world, v2_sub(projectile->pos, childOffset),
			               V2(20, 20), scale, entitytype_projectile,
			               projectile->direction, projectile->tex, FALSE);

			child->collidesWith[entitytype_hero] = FALSE;
			child->collidesWith[entitytype_mob]  = TRUE;

			child->stats->entityIdToAttack =
			    projectile->stats->entityIdToAttack;
			entity_addAnim(assetManager, child, "claudeAirSlashVfx");
			entity_setActiveAnim(child, "claudeAirSlashVfx");
			projectile->childIds[i] = child->id;
		}
		break;
	}

	default:
#ifdef DENGINE_DEBUG
		ASSERT(INVALID_CODE_PATH);
#endif
		break;
	}

	EntityAnim attackAnim = attacker->animList[attacker->currAnimId];
	f32 busyDuration =
	    attackAnim.anim->frameDuration * CAST(f32) attackAnim.anim->numFrames;
	attacker->stats->busyDuration = busyDuration;
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

		if (attacker->stats->weapon)
		{
			attacker->stats->weapon->invisible = TRUE;
		}

		if (attacker->direction == direction_east)
			attacker->dPos.x -= (1.0f * METERS_TO_PIXEL);
		else
			attacker->dPos.x += (1.0f * METERS_TO_PIXEL);

		break;

	case entityattack_airSlash:
		break;

	case entityattack_energySword:
		attacker->stats->health += 80;
		AttackSpec *attackSpec = PLATFORM_MEM_ALLOC(arena, 1, AttackSpec);
		attackSpec->attacker   = attacker;
		attackSpec->defender   = attacker;
		attackSpec->damage     = 30;
		registerEvent(eventQueue, eventtype_end_attack, attackSpec);
		return;

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
		i32 damage = 25;

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
		else if (attacker->type == entitytype_mob)
		{
			defender->stats->health -= damage * 0.25f;
		}

		if (defender->stats->health <= 0.0f) defender->stats->health = 10.0f;

		if (defender->stats->health <= 0)
		{
			entityStateSwitch(eventQueue, world, defender, entitystate_dead);
			entityStateSwitch(eventQueue, world, attacker, entitystate_idle);
		}
	}
}

INTERNAL void sortWorldEntityList(World *world)
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
}

INTERNAL enum EntityAttack selectBestAttack(Entity *entity)
{
	if (entity->stats->health <= 50.0f && entity->type == entitytype_hero)
	{
		return entityattack_energySword;
	}
	else
	{
		enum EntityAttack attack = entityattack_tackle;;
		if (entity->type == entitytype_hero)
		{
			b32 choice = rand() % 2;
			attack =
			    (choice == TRUE) ? entityattack_airSlash : entityattack_tackle;
			//attack = entityattack_tackle;
		}

		return attack;
	}
}

INTERNAL i32 entityGetFreeAudioRendererIndex(Entity *entity)
{
	i32 result = -1;
	for (i32 i = 0; i < entity->numAudioRenderers; i++)
	{
		if (entity->audioRenderer[i].state == audiostate_stopped)
		{
			result = i;
			break;
		}
	}

	return result;
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

INTERNAL b32 checkCollision(Entity *a, Entity *b)
{
	b32 result = FALSE;
	if (a->collides && b->collides && a->collidesWith[b->type])
	{
		Rect aRect = {a->pos, a->hitboxSize};

		v2 aTopLeftP =
		    getPosRelativeToRect(aRect, V2(0, 0), rectbaseline_topLeft);
		v2 aTopRightP =
		    getPosRelativeToRect(aRect, V2(0, 0), rectbaseline_topRight);
		v2 aBottomLeftP =
		    getPosRelativeToRect(aRect, V2(0, 0), rectbaseline_bottomLeft);
		v2 aBottomRightP =
		    getPosRelativeToRect(aRect, V2(0, 0), rectbaseline_bottomRight);

		Rect bRect = {b->pos, b->hitboxSize};

		if (math_pointInRect(bRect, aTopLeftP) ||
		    math_pointInRect(bRect, aTopRightP) ||
		    math_pointInRect(bRect, aBottomLeftP) ||
		    math_pointInRect(bRect, aBottomRightP))
		{
			result = TRUE;
			return result;
		}
	}

	return result;
}

INTERNAL b32 moveEntityAndReturnCollision(World *world, Entity *entity,
                                          v2 ddPos, f32 speedInMs, f32 dt)
{
	/*
	 **************************
	 * Calculate Hero Speed
	 **************************
	 */
	// NOTE(doyle): Clipping threshold for snapping velocity to 0
	f32 epsilon = 15.0f;
	v2 epsilonDpos = v2_sub(V2(epsilon, epsilon),
	                        V2(ABS(entity->dPos.x), ABS(entity->dPos.y)));

	if (epsilonDpos.x >= 0.0f && epsilonDpos.y >= 0.0f)
		entity->dPos = V2(0.0f, 0.0f);

	f32 speedInPixels = speedInMs * METERS_TO_PIXEL;

	ddPos = v2_scale(ddPos, speedInPixels);
	// TODO(doyle): Counteracting force on player's acceleration is
	// arbitrary
	ddPos = v2_sub(ddPos, v2_scale(entity->dPos, 5.5f));

	/*
	   NOTE(doyle): Calculate new position from acceleration with old
	   velocity
	   new Position     = (a/2) * (t^2) + (v*t) + p,
	   acceleration     = (a/2) * (t^2)
	   old velocity     =                 (v*t)
	 */
	v2 ddPosNew = v2_scale(v2_scale(ddPos, 0.5f), SQUARED(dt));
	v2 dPos     = v2_scale(entity->dPos, dt);

	v2 oldP = entity->pos;
	v2 newP = v2_add(v2_add(ddPosNew, dPos), entity->pos);
	entity->pos = newP;

	/*
	 **************************
	 * Collision Detection
	 **************************
	 */
	// TODO(doyle): Only check collision for entities within small
	// bounding box of the hero
	b32 entityCollided = FALSE;
	if (entity->collides)
	{
		for (i32 i = 0; i < world->maxEntities; i++)
		{
			Entity *collider = &world->entities[i];
			if (collider->state == entitystate_dead) continue;
			if (collider->id == entity->id) continue;

			entityCollided = checkCollision(entity, collider);
			if (entityCollided) break;
		}
	}

	if (entityCollided)
	{
		entity->dPos = V2(0.0f, 0.0f);
		entity->pos = oldP;
	}
	else
	{
		// f'(t) = curr velocity = a*t + v, where v is old velocity
		entity->dPos = v2_add(entity->dPos, v2_scale(ddPos, dt));
		entity->pos  = newP;
	}

	return entityCollided;
}

void worldTraveller_gameUpdateAndRender(GameState *state, f32 dt)
{
	// TODO(doyle): use proper dt limiting techniques
	if (dt >= 1.0f) dt = 1.0f;
	GL_CHECK_ERROR();

	AssetManager *assetManager = &state->assetManager;
	Renderer *renderer         = &state->renderer;
	World *const world         = &state->world[state->currWorldIndex];
	Font *font                 = &assetManager->font;
	MemoryArena *arena         = &state->arena;

	/*
	 **********************
	 * Parse Input Keys
	 **********************
	 */
	{
		/*
		 **********************
		 * Parse Alphabet Keys
		 **********************
		 */
		KeyState *keys = state->input.keys;
		for (enum KeyCode code = 0; code < keycode_count; code++)
		{
			b32 parseKey = FALSE;

			KeyState *key = &keys[code];
			if (key->newHalfTransitionCount > key->oldHalfTransitionCount)
			{
				i32 numTransitions =
				    key->newHalfTransitionCount - key->oldHalfTransitionCount;

				if (!IS_EVEN(numTransitions))
					key->endedDown = (key->endedDown) ? FALSE : TRUE;
			}

			if (code >= keycode_a && code <= keycode_z) parseKey = TRUE;
			if (code >= keycode_A && code <= keycode_Z) parseKey = TRUE;
			if (code >= keycode_0 && code <= keycode_9) parseKey = TRUE;

			if (!parseKey) continue;

			if (getKeyStatus(key, readkeytype_repeat, 0.15f, dt))
			{
				// TODO(doyle): Multi press within frame overrides ui parsing
				state->uiState.keyChar = code;
			}
		}

		/*
		 **************************
		 * Parse Game Control Keys
		 **************************
		 */
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

		if (getKeyStatus(&keys[keycode_backspace], readkeytype_delayedRepeat,
		                 0.1f, dt))
		{
			state->uiState.keyEntered = keycode_backspace;
		}

		if (getKeyStatus(&keys[keycode_left_square_bracket],
		                 readkeytype_delayedRepeat, 0.25f, dt))
		{

			Renderer *renderer = &state->renderer;
			f32 yPos           = CAST(f32)(rand() % CAST(i32) renderer->size.h);
			f32 xModifier      = 5.0f - CAST(f32)(rand() % 3);

			v2 pos =
			    V2(renderer->size.w - (renderer->size.w / xModifier), yPos);
			addGenericMob(&state->arena, &state->assetManager, world, pos);
		}
	}

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

		/* Reposition camera if entity is targeted id */
		if (world->cameraFollowingId == entity->id)
		{
			// NOTE(doyle): Set the camera such that the hero is centered
			v2 offsetFromEntityToCameraOrigin =
			    V2((entity->pos.x - (0.5f * state->renderer.size.w)), (0.0f));

			// NOTE(doyle): Account for the hero's origin being the bottom left
			offsetFromEntityToCameraOrigin.x += (entity->hitboxSize.x * 0.5f);
			world->cameraPos = offsetFromEntityToCameraOrigin;
		}

		if (entity->state == entitystate_dead) continue;
		if (entity->invisible == TRUE) continue;

		if (entity->type == entitytype_soundscape)
		{
			AudioRenderer *audioRenderer = &entity->audioRenderer[0];
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
					    asset_getVorbis(assetManager, "audio_battle");
					if (audioRenderer->audio)
					{
						if (common_strcmp(audioRenderer->audio->key,
						                  "audio_battle") != 0)
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
					    asset_getVorbis(assetManager, "audio_overworld");
					if (audioRenderer->audio)
					{
						if (common_strcmp(audioRenderer->audio->key,
						                  "audio_overworld") != 0)
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
		 * Calculate Mob In Battle Range
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

			i32 numEntitiesInBattleBefore = world->numEntitiesInBattle;
			entityStateSwitch(&eventQueue, world, entity, newState);

			if (numEntitiesInBattleBefore == 0 &&
			    world->numEntitiesInBattle > 0)
			{
				i32 freeAudioIndex = entityGetFreeAudioRendererIndex(hero);
				if (freeAudioIndex != -1)
				{
					char *battleTaunt[11] = {
					    "Battle_come_on",
					    "Battle_heh",
					    "Battle_heres_the_enemy",
					    "Battle_hey",
					    "Battle_oh_its_just_them",
					    "Battle_shouldnt_you_run_away",
					    "Battle_things_will_work_out_somehow",
					    "Battle_things_will_work_out",
					    "Battle_were_gonna_win",
					    "Battle_we_can_win_this",
					    "Battle_you_think_you_can_win_over_the_hero_of_light"};

					i32 battleTauntSfxIndex = rand() % ARRAY_COUNT(battleTaunt);
					audio_playVorbis(
					    arena, audioManager,
					    &hero->audioRenderer[freeAudioIndex],
					    asset_getVorbis(assetManager,
					                    battleTaunt[battleTauntSfxIndex]),
					    1);
				}
			}
		}
		else if (entity->type == entitytype_hero)
		{
			/*
			 **************************
			 * Calculate Hero Movement
			 **************************
			 */

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

			Entity *hero   = entity;
			KeyState *keys = state->input.keys;

			v2 ddPos = V2(0, 0);
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

				if (getKeyStatus(&keys[keycode_up], readkeytype_repeat,
				                 KEY_DELAY_NONE, dt))
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
					// NOTE(doyle): Cheese it and pre-compute the vector for
					// diagonal using pythagoras theorem on a unit triangle 1^2
					// + 1^2 = c^2
					ddPos = v2_scale(ddPos, 0.70710678118f);
				}
			}

			f32 heroSpeed = 6.2f;
			if (getKeyStatus(&state->input.keys[keycode_leftShift],
			                 readkeytype_repeat, KEY_DELAY_NONE, dt))
			{
				// TODO: Context sensitive command separation
				state->uiState.keyMod = keycode_leftShift;
				heroSpeed *= 10.0f;
			}
			else
			{
				state->uiState.keyMod = keycode_null;
			}

			moveEntityAndReturnCollision(world, hero, ddPos, heroSpeed, dt);

			char *currAnimName = hero->animList[hero->currAnimId].anim->key;
			if (ABS(entity->dPos.x) > 0.0f || ABS(entity->dPos.y) > 0.0f)
			{
				if (common_strcmp(currAnimName, "claudeIdle") == 0)
				{
					entity_setActiveAnim(hero, "claudeRun");
				}
			}
			else
			{
				if (common_strcmp(currAnimName, "claudeRun") == 0)
				{
					entity_setActiveAnim(hero, "claudeIdle");
				}
			}

		}
		else if (entity->type == entitytype_projectile)
		{
			Entity *projectile = entity;
			i32 targetIndex =
			    entity_getIndex(world, projectile->stats->entityIdToAttack);
			Entity *target = &world->entities[targetIndex];

			v2 ddPos = V2(0, 0);
			f32 projectileSpeed = 10.0f;

			v2 difference = v2_sub(target->pos, projectile->pos);
			f32 longSide  = (ABS(difference.x) > ABS(difference.y))
			                   ? ABS(difference.x)
			                   : ABS(difference.y);

			ddPos.x = (difference.x / longSide);
			ddPos.y = (difference.y / longSide);

			if (ddPos.x != 0.0f && ddPos.y != 0.0f)
			{
				// NOTE(doyle): Cheese it and pre-compute the vector for
				// diagonal using pythagoras theorem on a unit triangle 1^2
				// + 1^2 = c^2
				ddPos = v2_scale(ddPos, 0.70710678118f);
			}

			b32 collided = moveEntityAndReturnCollision(
			    world, projectile, ddPos, projectileSpeed, dt);

			if (collided)
			{
				// TODO(doyle): Unify concept of dead entity for mobs and
				// projectiles
				projectile->state = entitystate_dead;
				registerEvent(&eventQueue, eventtype_entity_died, projectile);
			}
		}

		/*
		 **************************************************
		 * Conduct Battle For Mobs In Range
		 **************************************************
		 */
		if (entity->type == entitytype_mob || entity->type == entitytype_hero)
		{
			EntityStats *stats = entity->stats;

			if (stats->weapon)
			{
				stats->weapon->pos = entity->pos;
			}

			if (entity->state == entitystate_battle)
			{
				if (stats->actionTimer > 0)
					stats->actionTimer -= dt * stats->actionSpdMul;

				if (stats->actionTimer <= 0)
				{
					stats->actionTimer = 0;
					if (stats->queuedAttack == entityattack_invalid)
					{
						enum EntityAttack attack = selectBestAttack(entity);
						stats->queuedAttack      = attack;
					}

					/* Launch up attack animation */
					beginAttack(assetManager, &state->arena, &eventQueue, world,
					            entity);
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

		/*
		 ****************
		 * Update Entity
		 ****************
		 */
		for (i32 i = 0; i < entity->numAudioRenderers; i++)
		{
			audio_updateAndPlay(&state->arena, &state->audioManager,
			                    &entity->audioRenderer[i]);
		}

		if (entity->tex)
		{
			entity_updateAnim(entity, dt);
			/* Calculate region to render */
			renderer_entity(renderer, camera, entity,
			                v2_scale(entity->renderSize, 0.5f), 0,
			                V4(1, 1, 1, 1));
		}
	}

	/*
	 *****************************************
	 * Process Events From Entity Update Loop
	 *****************************************
	 */
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

			if (attacker->stats->queuedAttack == entityattack_tackle)
			{
				i32 freeAudioIndex = entityGetFreeAudioRendererIndex(attacker);
				if (freeAudioIndex != -1)
				{
					char *attackSfx[11] = {
					    "Attack_1",   "Attack_2",      "Attack_3",
					    "Attack_4",   "Attack_5",      "Attack_6",
					    "Attack_7",   "Attack_hit_it", "Attack_take_that",
					    "Attack_hya", "Attack_burn"};

					i32 attackSfxIndex = rand() % ARRAY_COUNT(attackSfx);
					audio_playVorbis(arena, audioManager,
					                 &attacker->audioRenderer[freeAudioIndex],
					                 asset_getVorbis(assetManager,
					                                 attackSfx[attackSfxIndex]),
					                 1);
				}

				freeAudioIndex = entityGetFreeAudioRendererIndex(defender);
				if (freeAudioIndex != -1)
				{
					char *hurtSfx[10] = {"Hurt_1",         "Hurt_2",
					                     "Hurt_3",         "Hurt_hows_this",
					                     "Hurt_battlecry", "Hurt_ow",
					                     "Hurt_uh_oh",     "Hurt_ugh",
					                     "Hurt_woah",      "Hurt_yearning"};

					i32 hurtSfxIndex = rand() % ARRAY_COUNT(hurtSfx);

					audio_playVorbis(
					    arena, audioManager,
					    &defender->audioRenderer[freeAudioIndex],
					    asset_getVorbis(assetManager, hurtSfx[hurtSfxIndex]),
					    1);
				}
			}
			else if (attacker->stats->queuedAttack == entityattack_energySword)
			{
				i32 freeAudioIndex = entityGetFreeAudioRendererIndex(attacker);
				if (freeAudioIndex != -1)
				{
					audio_playVorbis(
					    arena, audioManager,
					    &attacker->audioRenderer[freeAudioIndex],
					    asset_getVorbis(assetManager, "Attack_energy_sword"),
					    1);
				}
			}
			else if (attacker->stats->queuedAttack == entityattack_airSlash)
			{
				i32 freeAudioIndex = entityGetFreeAudioRendererIndex(attacker);
				if (freeAudioIndex != -1)
				{
					audio_playVorbis(
					    arena, audioManager,
					    &attacker->audioRenderer[freeAudioIndex],
					    asset_getVorbis(assetManager, "Attack_air_slash"),
					    1);
				}
			}
			else
			{
				//ASSERT(INVALID_CODE_PATH);
			}

			attacker->stats->queuedAttack = entityattack_invalid;

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
			for (i32 i = 0; i < entity->numAudioRenderers; i++)
			{
				audio_stopVorbis(&state->arena, audioManager,
				                 &entity->audioRenderer[i]);
			}
			entity_clearData(&state->arena, world, entity);
			numDeadEntities++;

			for (i32 i = 0; i < entity->numChilds; i++)
			{
				Entity *child = entity_get(world, entity->childIds[i]);
				if (child)
				{
					for (i32 i = 0; i < child->numAudioRenderers; i++)
					{
						audio_stopVorbis(&state->arena, audioManager,
						                 &child->audioRenderer[i]);
					}
					entity_clearData(&state->arena, world, child);
					numDeadEntities++;
				}
				else
				{
					DEBUG_LOG("Entity child expected but not found");
				}
			}
			break;
		}
		default:
		{
			break;
		}
		}
	}

	/*
	 ****************************
	 * Update World From Results
	 ****************************
	 */
	// TODO(doyle): Dead hero not accounted for here
	Entity *hero = getHeroEntity(world);
	if (world->numEntitiesInBattle > 0)
	{
		// NOTE(doyle): If battle entities is 1 then only the hero left
		if (hero->state == entitystate_battle &&
		    world->numEntitiesInBattle == 1)
		{
			entityStateSwitch(&eventQueue, world, hero, entitystate_idle);
		}
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
			entity_setActiveAnim(hero, "claudeIdle");
		}
		hero->stats->entityIdToAttack = -1;
		hero->stats->actionTimer      = hero->stats->actionRate;
		hero->stats->busyDuration     = 0;
	}

	sortWorldEntityList(world);
	world->freeEntityIndex -= numDeadEntities;

	/*
	 ********************
	 * UI Rendering Code
	 ********************
	 */
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
	i32 activeId =
	    userInterface_window(&state->uiState, &state->arena, assetManager,
	                         renderer, font, state->input, debugWindow);

	// TODO(doyle): Name lookup to user interface id
	if (activeId == 1)
	{
		state->config.playWorldAudio =
		    (state->config.playWorldAudio == TRUE) ? FALSE : TRUE;
	}
	else if (activeId == 2)
	{
		state->config.showDebugDisplay =
		    (state->config.showDebugDisplay == TRUE) ? FALSE : TRUE;
	}

	// RESET IMGUI
	if (!state->input.keys[keycode_mouseLeft].endedDown)
		state->uiState.activeItem = 0;
	else if (state->uiState.activeItem == 0)
		state->uiState.activeItem = -1;

	if (state->uiState.keyEntered == keycode_tab) state->uiState.kbdItem = 0;
	state->uiState.keyEntered = keycode_null;
	state->uiState.keyChar    = keycode_null;

	/* Draw hero avatar */
	TexAtlas *heroAtlas =
	    asset_getTexAtlas(assetManager, "ClaudeSprite.png");
	Rect heroAvatarRect =
	    asset_getAtlasSubTex(heroAtlas, "ClaudeSprite_Avatar_01");
	v2 heroAvatarP =
	    V2(10.0f, (renderer->size.h * 0.5f) - (0.5f * heroAvatarRect.size.h));

	// TODO(doyle): Use rect in rendering not V4
	v4 heroAvatarTexRect      = {0};
	heroAvatarTexRect.vec2[0] = heroAvatarRect.pos;
	heroAvatarTexRect.vec2[1] = v2_add(heroAvatarRect.pos, heroAvatarRect.size);

	RenderTex heroRenderTex = {hero->tex, heroAvatarTexRect};
	renderer_staticRect(renderer, heroAvatarP, heroAvatarRect.size, V2(0, 0), 0,
	                    heroRenderTex, V4(1, 1, 1, 1));

	char heroAvatarStr[20];
	snprintf(heroAvatarStr, ARRAY_COUNT(heroAvatarStr), "HP: %3.0f/%3.0f",
	         hero->stats->health, hero->stats->maxHealth);
	f32 strLenInPixels =
	    CAST(f32)(font->maxSize.w * common_strlen(heroAvatarStr));
	v2 strPos =
	    V2(heroAvatarP.x, heroAvatarP.y - (0.5f * heroAvatarRect.size.h));
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

    /*
	 ********************
	 * DEBUG CODE
	 ********************
	 */
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
