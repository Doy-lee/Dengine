#include "WorldTraveller/WorldTraveller.h"

#include "Dengine/Debug.h"
#include "Dengine/Platform.h"

enum State
{
	state_active,
	state_menu,
	state_win,
};

INTERNAL Entity *addEntity(World *world, v2 pos, v2 size, enum EntityType type,
                           enum Direction direction, Texture *tex, b32 collides)
{

#ifdef DENGINE_DEBUG
	ASSERT(tex && world);
	ASSERT(world->freeEntityIndex < world->maxEntities);
	ASSERT(type < entitytype_count);
#endif

	Entity entity     = {0};
	entity.id         = world->uniqueIdAccumulator++;
	entity.pos        = pos;
	entity.hitboxSize = size;
	entity.renderSize = size;
	entity.type       = type;
	entity.direction  = direction;
	entity.tex        = tex;
	entity.collides   = collides;

	switch(type)
	{
		case entitytype_hero:
		case entitytype_mob:
	    {
		    entity.stats                   = PLATFORM_MEM_ALLOC(1, EntityStats);
		    entity.stats->maxHealth        = 100;
		    entity.stats->health           = entity.stats->maxHealth;
		    entity.stats->actionRate       = 100;
		    entity.stats->actionTimer      = entity.stats->actionRate;
		    entity.stats->actionSpdMul     = 100;
		    entity.stats->entityIdToAttack = -1;
		    entity.stats->queuedAttack     = entityattack_invalid;
		    entity.state                   = entitystate_idle;
		    break;
	    }
		
		default:
			break;
	}

	world->entities[world->freeEntityIndex++] = entity;
	Entity *result = &world->entities[world->freeEntityIndex-1];

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

INTERNAL void addAnim(AssetManager *assetManager, i32 animId, Entity *entity)
{
	Animation *anim = asset_getAnim(assetManager, animId);
	entity->anim[animId].anim = anim;
	entity->anim[animId].currFrame = 0;
	entity->anim[animId].currDuration = anim->frameDuration;
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
	terrainAtlas->texRect[terrainrects_ground] =
	    V4(texOrigin.x, texOrigin.y, texOrigin.x + atlasTileSize,
	       texOrigin.y - atlasTileSize);

	/* Load shaders */
	asset_loadShaderFiles(assetManager, "data/shaders/sprite.vert.glsl",
	                      "data/shaders/sprite.frag.glsl",
	                      shaderlist_sprite);

	asset_loadTTFont(assetManager, "C:/Windows/Fonts/Arialbd.ttf");
	glCheckError();

	/* Load animations */
	f32 duration = 1.0f;
	i32 numRects = 1;
	v4 *animRects = PLATFORM_MEM_ALLOC(numRects, v4);
	i32 terrainAnimAtlasIndexes[1] = {terrainrects_ground};

	// TODO(doyle): Optimise animation storage, we waste space having 1:1 with
	// animlist when some textures don't have certain animations
	asset_addAnimation(assetManager, texlist_terrain, animlist_terrain,
	                   terrainAnimAtlasIndexes, numRects, duration);

	// Idle animation
	duration      = 1.0f;
	numRects      = 1;
	i32 idleAnimAtlasIndexes[1] = {herorects_idle};
	asset_addAnimation(assetManager, texlist_hero, animlist_hero_idle,
	                   idleAnimAtlasIndexes, numRects, duration);

	// Walk animation
	duration          = 0.10f;
	numRects          = 3;
	i32 walkAnimAtlasIndexes[3] = {herorects_walkA, herorects_idle,
	                               herorects_walkB};
	asset_addAnimation(assetManager, texlist_hero, animlist_hero_walk,
	                   walkAnimAtlasIndexes, numRects, duration);

	// Wave animation
	duration          = 0.30f;
	numRects          = 2;
	i32 waveAnimAtlasIndexes[2] = {herorects_waveA, herorects_waveB};
	asset_addAnimation(assetManager, texlist_hero, animlist_hero_wave,
	                   waveAnimAtlasIndexes, numRects, duration);

	// Battle Stance animation
	duration          = 1.0f;
	numRects          = 1;
	i32 battleStanceAnimAtlasIndexes[1] = {herorects_battlePose};
	asset_addAnimation(assetManager, texlist_hero, animlist_hero_battlePose,
	                   battleStanceAnimAtlasIndexes, numRects, duration);

	// Battle tackle animation
	duration          = 0.15f;
	numRects          = 3;
	i32 tackleAnimAtlasIndexes[3] = {herorects_castA, herorects_castB,
	                                 herorects_castC};
	asset_addAnimation(assetManager, texlist_hero, animlist_hero_tackle,
	                   tackleAnimAtlasIndexes, numRects, duration);


	state->state          = state_active;
	state->currWorldIndex = 0;
	state->tileSize       = 64;

	/* Init renderer */
	rendererInit(state, windowSize);

	/* Init world */
	const i32 targetWorldWidth  = 100 * METERS_TO_PIXEL;
	const i32 targetWorldHeight = 10 * METERS_TO_PIXEL;
	v2 worldDimensionInTiles    = V2i(targetWorldWidth / state->tileSize,
	                               targetWorldHeight / state->tileSize);

	for (i32 i = 0; i < ARRAY_COUNT(state->world); i++)
	{
		World *const world = &state->world[i];
		world->maxEntities = 16384;
		world->entities = PLATFORM_MEM_ALLOC(world->maxEntities, Entity);
		world->entityIdInBattle = PLATFORM_MEM_ALLOC(world->maxEntities, i32);
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
				Entity *tile =
				    addEntity(world, pos, size, type, dir, tex, collides);

				addAnim(assetManager, animlist_terrain, tile);
				tile->currAnimId = animlist_terrain;
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

	/* Populate hero animation references */
	addAnim(assetManager, animlist_hero_idle, hero);
	addAnim(assetManager, animlist_hero_walk, hero);
	addAnim(assetManager, animlist_hero_wave, hero);
	addAnim(assetManager, animlist_hero_battlePose, hero);
	addAnim(assetManager, animlist_hero_tackle, hero);
	hero->currAnimId = animlist_hero_idle;

	/* Create a NPC */
	pos         = V2(hero->pos.x * 3, CAST(f32) state->tileSize);
	size        = hero->hitboxSize;
	type        = entitytype_npc;
	dir         = direction_null;
	tex         = hero->tex;
	collides    = FALSE;
	Entity *npc = addEntity(world, pos, size, type, dir, tex, collides);

	/* Populate npc animation references */
	addAnim(assetManager, animlist_hero_wave, npc);
	npc->currAnimId = animlist_hero_wave;

	/* Create a Mob */
	pos         = V2(renderer->size.w - (renderer->size.w / 3.0f),
	                 CAST(f32) state->tileSize);
	size        = hero->hitboxSize;
	type        = entitytype_mob;
	dir         = direction_west;
	tex         = hero->tex;
	collides    = TRUE;
	Entity *mob = addEntity(world, pos, size, type, dir, tex, collides);

	/* Populate mob animation references */
	addAnim(assetManager, animlist_hero_idle, mob);
	addAnim(assetManager, animlist_hero_walk, mob);
	addAnim(assetManager, animlist_hero_battlePose, mob);
	addAnim(assetManager, animlist_hero_tackle, mob);
	hero->currAnimId = animlist_hero_idle;
}

INTERNAL inline void setActiveEntityAnim(Entity *entity,
                                         enum EntityAnimId animId)
{
#ifdef DENGINE_DEBUG
	ASSERT(animId < animlist_count);
	ASSERT(entity->anim[animId].anim);
#endif

	/* Reset current anim data */
	EntityAnim_ *currAnim   = &entity->anim[entity->currAnimId];
	currAnim->currDuration  = currAnim->anim->frameDuration;
	currAnim->currFrame = 0;

	/* Set entity active animation */
	entity->currAnimId = animId;
	entity->currAnimCyclesCompleted = 0;
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

		// TODO(doyle): Revisit key input with state checking for last ended down
#if 0
		if (state->keys[GLFW_KEY_SPACE] && !spaceBarWasDown)
		{
			if (!(hero->currAnimId == entityanimid_tackle &&
				  hero->currAnimCyclesCompleted == 0))
			{
				spaceBarWasDown = TRUE;
			}
		}
		else if (!state->keys[GLFW_KEY_SPACE])
		{
			spaceBarWasDown = FALSE;
		}
#endif
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
			setActiveEntityAnim(hero, animlist_hero_idle);
		}
	}
	else if (hero->currAnimId == animlist_hero_idle)
	{
		setActiveEntityAnim(hero, animlist_hero_walk);
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

INTERNAL void updateEntityAnim(Entity *entity, f32 dt)
{
	// TODO(doyle): Recheck why we have this twice
	EntityAnim_ *entityAnim = &entity->anim[entity->currAnimId];
	Animation anim         = *entityAnim->anim;
	i32 frameIndex         = anim.frameIndex[entityAnim->currFrame];
	v4 texRect             = anim.atlas->texRect[frameIndex];

	entityAnim->currDuration -= dt;
	if (entityAnim->currDuration <= 0.0f)
	{
		if (++entityAnim->currFrame >= anim.numFrames)
			entity->currAnimCyclesCompleted++;

		entityAnim->currFrame = entityAnim->currFrame % anim.numFrames;
		frameIndex = entityAnim->anim->frameIndex[entityAnim->currFrame];
		texRect    = anim.atlas->texRect[frameIndex];
		entityAnim->currDuration = anim.frameDuration;
	}

	// NOTE(doyle): If humanoid entity, let animation dictate render size which
	// may exceed the hitbox size of the entity
	switch (entity->type)
	{
		case entitytype_hero:
		case entitytype_mob:
		case entitytype_npc:
			entity->renderSize = math_getRectSize(texRect);
		default:
			break;
	}
}

INTERNAL void beginAttack(Entity *attacker)
{
	attacker->state = entitystate_attack;
	switch (attacker->stats->queuedAttack)
	{
	case entityattack_tackle:
		EntityAnim_ attackAnim = attacker->anim[animlist_hero_tackle];
		f32 busyDuration = attackAnim.anim->frameDuration *
		                   CAST(f32) attackAnim.anim->numFrames;

		attacker->stats->busyDuration = busyDuration;
		setActiveEntityAnim(attacker, animlist_hero_tackle);

		if (attacker->direction == direction_east)
			attacker->dPos.x += (1.0f * METERS_TO_PIXEL);
		else
			attacker->dPos.x -= (1.0f * METERS_TO_PIXEL);

		break;
	default:
#ifdef DENGINE_DEBUG
		ASSERT(INVALID_CODE_PATH);
#endif
	}
}

// TODO(doyle): Calculate the battle damage, transition back into battle pose ..
// etc
INTERNAL void endAttack(World *world, Entity *attacker)
{
	switch (attacker->stats->queuedAttack)
	{
	case entityattack_tackle:
		if (attacker->direction == direction_east)
			attacker->dPos.x -= (1.0f * METERS_TO_PIXEL);
		else
			attacker->dPos.x += (1.0f * METERS_TO_PIXEL);

		break;
	default:
#ifdef DENGINE_DEBUG
		ASSERT(INVALID_CODE_PATH);
#endif
	}

	// TODO(doyle): Use attacker stats in battle equations
	attacker->state               = entitystate_battle;
	attacker->stats->actionTimer  = attacker->stats->actionRate;
	attacker->stats->busyDuration = 0;

	setActiveEntityAnim(attacker, animlist_hero_battlePose);

	Entity *defender = &world->entities[attacker->stats->entityIdToAttack];
	defender->stats->health--;
}

// TODO(doyle): Exposed because of debug .. rework debug system so it we don't
// need to expose any game API to it.
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
INTERNAL inline void updateWorldBattleEntities(World *world, Entity *entity,
                                               b32 isInBattle)
{
	world->entityIdInBattle[entity->id] = isInBattle;

	if (isInBattle)
		world->numEntitiesInBattle++;
	else
		world->numEntitiesInBattle--;
}

INTERNAL void entityStateSwitch(World *world, Entity *entity,
                                enum EntityState newState)
{
	if (entity->state == newState) return;

	switch(entity->state)
	{
	case entitystate_idle:
		switch (newState)
		{
		case entitystate_battle:
			updateWorldBattleEntities(world, entity, ENTITY_IN_BATTLE);
		case entitystate_attack:
		case entitystate_dead:
			break;
		default:
			ASSERT(INVALID_CODE_PATH);
		}
		break;
	case entitystate_battle:
		switch (newState)
		{
		case entitystate_idle:
			updateWorldBattleEntities(world, entity, ENTITY_NOT_IN_BATTLE);
			entity->stats->actionTimer = entity->stats->actionRate;
			entity->stats->queuedAttack = entityattack_invalid;
			setActiveEntityAnim(entity, animlist_hero_idle);
			break;
		case entitystate_attack:
		case entitystate_dead:
			break;
		default:
			ASSERT(INVALID_CODE_PATH);
		}
	case entitystate_attack:
		switch (newState)
		{
		case entitystate_idle:
		case entitystate_battle:
		case entitystate_dead:
			break;
		default:
			ASSERT(INVALID_CODE_PATH);
		}
	case entitystate_dead:
		switch (newState)
		{
		case entitystate_idle:
		case entitystate_battle:
		case entitystate_attack:
			break;
		default:
			ASSERT(INVALID_CODE_PATH);
		}
	}

	entity->state = newState;
}

INTERNAL void updateEntityAndRender(Renderer *renderer, World *world, f32 dt)
{
	for (i32 i = 0; i < world->freeEntityIndex; i++)
	{
		Entity *const entity  = &world->entities[i];
		Entity *hero          = &world->entities[world->heroIndex];

		switch(entity->type)
		{
		case entitytype_mob:
		{
			// TODO(doyle): Currently calculated in pixels, how about meaningful
			// game units?
			f32 battleThreshold = 500.0f;
			f32 distance = v2_magnitude(hero->pos, entity->pos);

			enum EntityState newState = entitystate_invalid;
			if (distance <= battleThreshold)
				newState = entitystate_battle;
			else
				newState = entitystate_idle;

			entityStateSwitch(world, entity, newState);
		}
		// NOTE(doyle): Allow fall through to entitytype_hero here
		case entitytype_hero:
		{
			if (entity->state == entitystate_battle ||
			    entity->state == entitystate_attack)
			{
				EntityStats *stats = entity->stats;
				if (stats->health > 0)
				{
					if (entity->state == entitystate_battle)
					{
						if (stats->actionTimer > 0)
							stats->actionTimer -= dt * stats->actionSpdMul;

						if (stats->actionTimer < 0)
						{
							stats->actionTimer = 0;
							if (stats->queuedAttack == entityattack_invalid)
								stats->queuedAttack = entityattack_tackle;

							beginAttack(entity);
						}
					}
					else
					{
						stats->busyDuration -= dt;
						if (stats->busyDuration <= 0)
							endAttack(world, entity);
					}
				}
				else
				{
					// TODO(doyle): Generalise for all entities
					hero->stats->entityIdToAttack = -1;
					hero->state                   = entitystate_idle;
					entity->state                 = entitystate_dead;
				}
			}
			break;
		}
		default:
			break;
		}

		if (world->numEntitiesInBattle > 0)
		{
			if (hero->state == entitystate_idle)
			{
				hero->state = entitystate_battle;
				world->entityIdInBattle[hero->id] = TRUE;
			}

			if (hero->stats->entityIdToAttack == -1)
				hero->stats->entityIdToAttack = i;
		}
		else
		{
			if (hero->state == entitystate_battle)
			{
				hero->state                       = entitystate_idle;
				world->entityIdInBattle[hero->id] = FALSE;
				setActiveEntityAnim(hero, animlist_hero_idle);
			}
			hero->stats->entityIdToAttack = -1;
			hero->stats->actionTimer      = hero->stats->actionRate;
			hero->stats->busyDuration     = 0;
		}

		updateEntityAnim(entity, dt);

		/* Calculate region to render */
		v4 cameraBounds = createCameraBounds(world, renderer->size);
		renderer_entity(renderer, cameraBounds, entity, 0, V4(1, 1, 1, 1));
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
	Font *font                 = &assetManager->font;

	ASSERT(world->freeEntityIndex < world->maxEntities);
	updateEntityAndRender(renderer, world, dt);

	/* Draw ui */
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
	debug_drawUi(state, dt);
#endif
}
