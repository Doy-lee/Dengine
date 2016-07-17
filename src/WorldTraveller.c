#include "WorldTraveller/WorldTraveller.h"

#include "Dengine/Debug.h"
#include "Dengine/Platform.h"

enum State
{
	state_active,
	state_menu,
	state_win,
};

INTERNAL void attackEntity(Entity *attacker, Entity *defender)
{
	// TODO(doyle): Use attacker stats in battle equations
	defender->stats->health--;
}

INTERNAL Entity *addEntity(World *world, v2 pos, v2 size, enum EntityType type,
                           enum Direction direction, Texture *tex, b32 collides)
{

#ifdef DENGINE_DEBUG
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

	switch(type)
	{
		case entitytype_hero:
		case entitytype_mob:
	    {
			entity.stats = PLATFORM_MEM_ALLOC(1, EntityStats);
			entity.stats->maxHealth = 100;
			entity.stats->health = entity.stats->maxHealth;
			entity.stats->actionRate = 100;
			entity.stats->actionTimer = entity.stats->actionRate;
			entity.stats->actionSpdMul = 100;
		    break;
	    }
		
		default:
			break;
	}

	world->entities[world->freeEntityIndex++] = entity;
	Entity *result = &world->entities[world->freeEntityIndex-1];

	return result;
}

INTERNAL void addAnim(Entity *entity, enum EntityAnimId animId, v4 *rects,
                      i32 numRects, f32 duration)
{

#ifdef DENGINE_DEBUG
	ASSERT(rects && numRects >= 0)
	ASSERT(animId < entityanimid_count);
#endif

	EntityAnim result   = {0};
	result.rect         = rects;
	result.numRects     = numRects;
	result.duration     = duration;
	result.currDuration = duration;

	entity->anim[animId] = result;
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

void worldTraveller_gameInit(GameState *state, v2 windowSize)
{
	AssetManager *assetManager = &state->assetManager;
	/* Initialise assets */
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
	terrainAtlas->texRect[terraincoords_ground] =
	    V4(texOrigin.x, texOrigin.y, texOrigin.x + atlasTileSize,
	       texOrigin.y - atlasTileSize);

	/* Load shaders */
	asset_loadShaderFiles(assetManager, "data/shaders/sprite.vert.glsl",
	                      "data/shaders/sprite.frag.glsl",
	                      shaderlist_sprite);

	asset_loadTTFont(assetManager, "C:/Windows/Fonts/Arialbd.ttf");
	glCheckError();

	state->state          = state_active;
	state->currWorldIndex = 0;
	state->tileSize       = 64;

	/* Init renderer */
	rendererInit(state, windowSize);

	/* Init world */
	const i32 targetWorldWidth  = 500 * METERS_TO_PIXEL;
	const i32 targetWorldHeight = 15 * METERS_TO_PIXEL;
	v2 worldDimensionInTiles    = V2i(targetWorldWidth / state->tileSize,
	                               targetWorldHeight / state->tileSize);

	for (i32 i = 0; i < ARRAY_COUNT(state->world); i++)
	{
		World *const world = &state->world[i];
		world->maxEntities = 8192;
		world->entities = PLATFORM_MEM_ALLOC(world->maxEntities, Entity);
		world->texType = texlist_terrain;

		world->bounds =
		    math_getRect(V2(0, 0), v2_scale(worldDimensionInTiles,
		                                    CAST(f32) state->tileSize));

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
				Entity *tile =
				    addEntity(world, pos, size, type, dir, tex, collides);

				f32 duration = 1.0f;
				i32 numRects = 1;
				v4 *animRects = PLATFORM_MEM_ALLOC(numRects, v4);
				animRects[0] = atlas->texRect[terraincoords_ground];
				addAnim(tile, entityanimid_idle, animRects, numRects, duration);
			}
		}
	}

	World *const world = &state->world[state->currWorldIndex];
	world->cameraPos = V2(0.0f, 0.0f);

	/* Init hero entity */
	world->heroIndex   = world->freeEntityIndex;

	Renderer *renderer   = &state->renderer;
	v2 size              = V2(58.0f, 98.0f);
	v2 pos               = V2(size.x, CAST(f32) state->tileSize);
	enum EntityType type = entitytype_hero;
	enum Direction dir   = direction_east;
	Texture *tex         = asset_getTexture(assetManager, texlist_hero);
	b32 collides         = TRUE;
	Entity *hero = addEntity(world, pos, size, type, dir, tex, collides);

	/* Add idle animation */
	f32 duration      = 1.0f;
	i32 numRects      = 1;
	v4 *heroIdleRects = PLATFORM_MEM_ALLOC(numRects, v4);
	heroIdleRects[0]  = heroAtlas->texRect[herorects_idle];
	addAnim(hero, entityanimid_idle, heroIdleRects, numRects, duration);
	hero->currAnimId = entityanimid_idle;

	/* Add walking animation */
	duration          = 0.10f;
	numRects          = 3;
	v4 *heroWalkRects = PLATFORM_MEM_ALLOC(numRects, v4);
	heroWalkRects[0]  = heroAtlas->texRect[herorects_walkA];
	heroWalkRects[1]  = heroAtlas->texRect[herorects_idle];
	heroWalkRects[2]  = heroAtlas->texRect[herorects_walkB];
	addAnim(hero, entityanimid_walk, heroWalkRects, numRects, duration);

	/* Add hero waving animation */
	duration          = 0.30f;
	numRects          = 2;
	v4 *heroWaveRects = PLATFORM_MEM_ALLOC(numRects, v4);
	heroWaveRects[0]  = heroAtlas->texRect[herorects_waveA];
	heroWaveRects[1]  = heroAtlas->texRect[herorects_waveB];
	addAnim(hero, entityanimid_wave, heroWaveRects, numRects, duration);

	/* Add hero battle stance animation */
	duration          = 1.0f;
	numRects          = 1;
	v4 *heroBattlePoseRects = PLATFORM_MEM_ALLOC(numRects, v4);
	heroBattlePoseRects[0]   = heroAtlas->texRect[herorects_battlePose];
	addAnim(hero, entityanimid_battlePose, heroBattlePoseRects, numRects,
	        duration);

	/* Add hero battle tackle animation */
	duration            = 0.15f;
	numRects            = 3;
	v4 *heroTackleRects = PLATFORM_MEM_ALLOC(numRects, v4);
	heroTackleRects[0]  = heroAtlas->texRect[herorects_castA];
	heroTackleRects[1]  = heroAtlas->texRect[herorects_castB];
	heroTackleRects[2]  = heroAtlas->texRect[herorects_castC];
	addAnim(hero, entityanimid_tackle, heroTackleRects, numRects,
	        duration);

	/* Create a NPC */
	pos         = V2(hero->pos.x * 3, CAST(f32) state->tileSize);
	size        = hero->size;
	type        = entitytype_npc;
	dir         = direction_null;
	tex         = hero->tex;
	collides    = FALSE;
	Entity *npc = addEntity(world, pos, size, type, dir, tex, collides);

	/* Add npc waving animation */
	duration           = 0.30f;
	numRects           = 2;
	v4 *npcWavingRects = PLATFORM_MEM_ALLOC(numRects, v4);
	npcWavingRects[0]  = heroAtlas->texRect[herorects_waveA];
	npcWavingRects[1]  = heroAtlas->texRect[herorects_waveB];
	addAnim(npc, entityanimid_wave, npcWavingRects, numRects, duration);
	npc->currAnimId = entityanimid_wave;

	/* Create a Mob */
	pos         = V2(renderer->size.w - (renderer->size.w / 3.0f),
	                 CAST(f32) state->tileSize);
	size        = hero->size;
	type        = entitytype_mob;
	dir         = direction_west;
	tex         = hero->tex;
	collides    = TRUE;
	Entity *mob = addEntity(world, pos, size, type, dir, tex, collides);

	/* Add mob idle animation */
	duration         = 1.0f;
	numRects         = 1;
	v4 *mobIdleRects = PLATFORM_MEM_ALLOC(numRects, v4);
	mobIdleRects[0]  = heroIdleRects[0];
	addAnim(mob, entityanimid_idle, mobIdleRects, numRects, duration);
	mob->currAnimId = entityanimid_idle;

	/* Add mob walking animation */
	duration         = 0.10f;
	numRects         = 3;
	v4 *mobWalkRects = PLATFORM_MEM_ALLOC(numRects, v4);
	mobWalkRects[0]  = heroWalkRects[0];
	mobWalkRects[1]  = heroWalkRects[1];
	mobWalkRects[2]  = heroWalkRects[2];
	addAnim(mob, entityanimid_walk, mobWalkRects, numRects, duration);
}

INTERNAL inline void setActiveEntityAnim(Entity *entity,
                                         enum EntityAnimId animId)
{
#ifdef DENGINE_DEBUG
	ASSERT(animId < entityanimid_count);
	ASSERT(entity->anim[animId].rect);
#endif

	/* Reset current anim data */
	EntityAnim *currAnim    = &entity->anim[entity->currAnimId];
	currAnim->currDuration  = currAnim->duration;
	currAnim->currRectIndex = 0;

	/* Set entity active animation */
	entity->currAnimId = animId;
	entity->currAnimCyclesCompleted = 0;
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

	// TODO(doyle): As we need to handle more key spam input, we want to track
	// if a button ended down
	LOCAL_PERSIST b32 spaceBarWasDown = FALSE;

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

	if (state->keys[GLFW_KEY_SPACE] && !spaceBarWasDown)
	{
		spaceBarWasDown = TRUE;
		setActiveEntityAnim(hero, entityanimid_tackle);
	}
	else if (!state->keys[GLFW_KEY_SPACE])
	{
		spaceBarWasDown = FALSE;
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
	                        V2(ABS(hero->dPos.x), ABS(hero->dPos.y)));

	if (epsilonDpos.x >= 0.0f && epsilonDpos.y >= 0.0f)
	{
		hero->dPos = V2(0.0f, 0.0f);
		if (hero->currAnimId == entityanimid_walk)
		{
			setActiveEntityAnim(hero, entityanimid_idle);
		}
	}
	else if (hero->currAnimId == entityanimid_idle)
	{
		setActiveEntityAnim(hero, entityanimid_walk);
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

INTERNAL void updateEntityAnim(Entity *entity, f32 dt)
{
	EntityAnim *anim = &entity->anim[entity->currAnimId];
	v4 texRect       = anim->rect[anim->currRectIndex];

	anim->currDuration -= dt;
	if (anim->currDuration <= 0.0f)
	{
		if (++anim->currRectIndex >= anim->numRects)
			entity->currAnimCyclesCompleted++;

		anim->currRectIndex = anim->currRectIndex % anim->numRects;
		texRect             = anim->rect[anim->currRectIndex];
		anim->currDuration  = anim->duration;
	}
}

void worldTraveller_gameUpdateAndRender(GameState *state, const f32 dt)
{
	/* Update */
	parseInput(state, dt);
	glCheckError();

	AssetManager *assetManager = &state->assetManager;
	Renderer *renderer         = &state->renderer;
	World *const world         = &state->world[state->currWorldIndex];
	Entity *hero               = &world->entities[world->heroIndex];
#ifdef DENGINE_DEBUG
	Font *font                 = &assetManager->font;
#endif

	/* Recalculate rendering bounds */
	v4 cameraBounds = math_getRect(world->cameraPos, renderer->size);
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

	/* Render entity and logic loop */
	ASSERT(world->freeEntityIndex < world->maxEntities);
	for (i32 i = 0; i < world->freeEntityIndex; i++)
	{
		/* Render entities */
		Entity *const entity = &world->entities[i];
		u32 oldAnimCycleCount = entity->currAnimCyclesCompleted;
		updateEntityAnim(entity, dt);

		v2 entityRenderSize = entity->size;
		switch (entity->type)
		{
			case entitytype_mob:
				break;

			case entitytype_hero:
#ifdef DENGINE_DEBUG
			    DEBUG_PUSH_STRING("HeroAnimCycleCount: %d",
			                      entity->currAnimCyclesCompleted, "i32");
#endif

			    // NOTE(doyle): If dynamic entity, allow animations to exceed
			    // the actual hitbox of character
			    EntityAnim *anim = &entity->anim[entity->currAnimId];
			    v4 texRect       = anim->rect[anim->currRectIndex];
			    entityRenderSize = math_getRectSize(texRect);

				if (oldAnimCycleCount != entity->currAnimCyclesCompleted)
				{
					if (entity->currAnimId == entityanimid_tackle)
					{
						setActiveEntityAnim(entity, entityanimid_idle);
					}
			    }
			    break;
			default:
				break;
		}

		f32 rotate = 0.0f;
		renderer_entity(&state->renderer, cameraBounds, entity,
		                entityRenderSize, rotate, V4(1, 1, 1, 1));

		/* Game logic */

		// TODO(doyle): Undefined behaviour when multiple entities on screen
		if (entity->type == entitytype_mob)
		{
			// TODO(doyle): Currently calculated in pixels, how about meaningful
			// game units?
			f32 distance = v2_magnitude(hero->pos, entity->pos);
#ifdef DENGINE_DEBUG
			DEBUG_PUSH_STRING("Hero to Entity Magnitude: %06.2f", distance,
			                  "f32");
#endif
			f32 battleThreshold = 500.0f;
			if (distance <= battleThreshold)
			{
#ifdef DENGINE_DEBUG
				v4 color        = V4(1.0f, 0, 0, 1);
				char *battleStr = "IN-BATTLE RANGE";
				f32 strLenInPixels =
				    CAST(f32)(font->maxSize.w * common_strlen(battleStr));
				v2 strPos =
				    V2((renderer->size.w * 0.5f) - (strLenInPixels * 0.5f),
				       renderer->size.h - 300.0f);
				renderer_staticString(&state->renderer, font, battleStr, strPos,
				                      0, color);
#endif
				/* Render targetting reticule */
				Texture *emptyTex =
				    asset_getTexture(assetManager, texlist_empty);
				v2 heroCenter = v2_add(hero->pos, v2_scale(hero->size, 0.5f));

				RenderTex renderTex = {emptyTex, V4(0, 1, 1, 0)};
				renderer_rect(renderer, cameraBounds, heroCenter,
				              V2(distance, 5.0f), 0, renderTex,
				              V4(1, 0, 0, 0.25f));

				/* Update action timer */
#ifdef DENGINE_DEBUG
				ASSERT(entity->stats)
#endif
				setActiveEntityAnim(hero, entityanimid_battlePose);

				hero->stats->actionTimer -= dt * hero->stats->actionSpdMul;
				entity->stats->actionTimer -= dt * entity->stats->actionSpdMul;

				if (hero->stats->health > 0)
				{
					if (hero->stats->actionTimer <= 0)
					{
						//attackEntity(hero, entity);
						hero->stats->actionTimer = hero->stats->actionRate;
					}
				}

				if (entity->stats->health > 0)
				{
					if (entity->stats->actionTimer <= 0)
					{
						entity->stats->actionTimer = entity->stats->actionRate;
					}
				}
			}
			else
			{
				entity->stats->actionTimer = entity->stats->actionRate;
				hero->stats->actionTimer   = hero->stats->actionRate;
			}
		}

#ifdef DENGINE_DEBUG
		/* Render debug markers on entities */
		v4 color          = V4(1, 1, 1, 1);
		char *debugString = NULL;
		switch (entity->type)
		{
			case entitytype_mob:
			color       = V4(1, 0, 0, 1);
			debugString = "MOB";
			break;

			case entitytype_hero:
			color       = V4(0, 0, 1.0f, 1);
			debugString = "HERO";
			break;

			case entitytype_npc:
			color       = V4(0, 1.0f, 0, 1);
			debugString = "NPC";
			break;

			default:
			break;
		}

		if (debugString)
		{
			v2 strPos                  = v2_add(entity->pos, entity->size);
			i32 indexOfLowerAInMetrics = 'a' - CAST(i32)font->codepointRange.x;
			strPos.y += font->charMetrics[indexOfLowerAInMetrics].offset.y;

			renderer_string(&state->renderer, cameraBounds, font, debugString,
			                strPos, 0, color);

			f32 stringLineGap = 1.1f * asset_getVFontSpacing(font->metrics);
			strPos.y -= GLOBAL_debugState.stringLineGap;

			char entityPosStr[128];
			snprintf(entityPosStr, ARRAY_COUNT(entityPosStr), "%06.2f, %06.2f",
			         entity->pos.x, entity->pos.y);
			renderer_string(&state->renderer, cameraBounds, font, entityPosStr,
			                strPos, 0, color);

			strPos.y -= GLOBAL_debugState.stringLineGap;
			char entityIDStr[32];
			snprintf(entityIDStr, ARRAY_COUNT(entityIDStr), "ID: %4d/%d", i,
			         world->maxEntities);
			renderer_string(&state->renderer, cameraBounds, font, entityIDStr,
			                strPos, 0, color);

			if (entity->stats)
			{
				strPos.y -= GLOBAL_debugState.stringLineGap;
				char entityHealth[32];
				snprintf(entityHealth, ARRAY_COUNT(entityHealth), "HP: %3.0f/%3.0f",
				         entity->stats->health, entity->stats->maxHealth);
				renderer_string(&state->renderer, cameraBounds, font,
				                entityHealth, strPos, 0, color);

				strPos.y -= GLOBAL_debugState.stringLineGap;
				char entityTimer[32];
				snprintf(entityTimer, ARRAY_COUNT(entityTimer), "ATB: %3.0f/%3.0f",
				         entity->stats->actionTimer, entity->stats->actionRate);
				renderer_string(&state->renderer, cameraBounds, font,
				                entityTimer, strPos, 0, color);
			}
		}
#endif
	}

	TexAtlas *heroAtlas  = asset_getTextureAtlas(assetManager, texlist_hero);
	v4 heroAvatarTexRect = heroAtlas->texRect[herorects_head];
	v2 heroAvatarSize    = math_getRectSize(heroAvatarTexRect);
	v2 heroAvatarP =
	    V2(10.0f, (renderer->size.h * 0.5f) - (0.5f * heroAvatarSize.h));

	RenderTex heroRenderTex = {hero->tex, heroAvatarTexRect};
	renderer_staticRect(renderer, heroAvatarP, heroAvatarSize, 0, heroRenderTex,
	                    V4(1, 1, 1, 1));

	char *heroAvatarStr = "HP: 100/100";
	f32 strLenInPixels =
	    CAST(f32)(font->maxSize.w * common_strlen(heroAvatarStr));
	v2 strPos = V2(heroAvatarP.x, heroAvatarP.y - (0.5f * heroAvatarSize.h));
	renderer_staticString(&state->renderer, font, heroAvatarStr, strPos, 0,
	                      V4(0, 0, 1, 1));

#ifdef DENGINE_DEBUG
	/* Render debug info stack */
	DEBUG_PUSH_STRING("Hero Pos: %06.2f, %06.2f", hero->pos, "v2");
	DEBUG_PUSH_STRING("Hero dPos: %06.2f, %06.2f", hero->dPos, "v2");
	DEBUG_PUSH_STRING("FreeEntityIndex: %d", world->freeEntityIndex, "i32");

	DEBUG_PUSH_STRING("glDrawArray Calls: %d",
	                  GLOBAL_debugState.callCount[debugcallcount_drawArrays],
	                  "i32");

	i32 debug_kbAllocated = GLOBAL_debugState.totalMemoryAllocated / 1024;
	DEBUG_PUSH_STRING("TotalMemoryAllocated: %dkb", debug_kbAllocated, "i32");
	debug_stringUpdateAndRender(&state->renderer, font, dt);
	debug_clearCallCounter();
#endif
}
