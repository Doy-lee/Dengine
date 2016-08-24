#include "Dengine/Entity.h"
#include "Dengine/Debug.h"
#include "Dengine/Platform.h"
#include "Dengine/WorldTraveller.h"

void entity_setActiveAnim(Entity *entity, char *animName)
{
	/* Reset current anim data */
	EntityAnim *currEntityAnim = &entity->animList[entity->currAnimId];
	currEntityAnim->currDuration = currEntityAnim->anim->frameDuration;
	currEntityAnim->currFrame    = 0;

	/* Set entity active animation */
	for (i32 i = 0; i < ARRAY_COUNT(entity->animList); i++)
	{
		Animation *anim = entity->animList[i].anim;
		if (anim)
		{
			if (common_strcmp(anim->key, animName) == 0)
			{
				entity->currAnimId = i;
				return;
			}
		}
	}
	
	DEBUG_LOG("Entity does not have access to desired anim");
}

void entity_updateAnim(Entity *entity, f32 dt)
{
	if (!entity->tex)
		return;

	EntityAnim *currEntityAnim = &entity->animList[entity->currAnimId];
	Animation *anim = currEntityAnim->anim;

	currEntityAnim->currDuration -= dt;
	if (currEntityAnim->currDuration <= 0.0f)
	{
		currEntityAnim->currFrame++;
		currEntityAnim->currFrame    = currEntityAnim->currFrame % anim->numFrames;
		currEntityAnim->currDuration = anim->frameDuration;
	}

	// NOTE(doyle): If humanoid entity, let animation dictate render size which
	// may exceed the hitbox size of the entity
	switch (entity->type)
	{
	case entitytype_hero:
	case entitytype_mob:
	case entitytype_npc:
		char *frameName = anim->frameList[currEntityAnim->currFrame];
		Rect texRect =
		    asset_getAtlasSubTexRect(anim->atlas, frameName);
		entity->renderSize = texRect.size;
	default:
		break;
	}
}

void entity_addAnim(AssetManager *assetManager, Entity *entity, char *animName)
{
	i32 freeAnimIndex = 0;
	for (i32 i = 0; i < ARRAY_COUNT(entity->animList); i++)
	{
		EntityAnim *entityAnim = &entity->animList[i];
		if (!entityAnim->anim)
		{
			entityAnim->anim         = asset_getAnim(assetManager, animName);
			entityAnim->currFrame    = 0;
			entityAnim->currDuration = entityAnim->anim->frameDuration;
			return;
		}
	}

	DEBUG_LOG("No more free entity animation slots");
}

void entity_addGenericMob(MemoryArena *arena, AssetManager *assetManager,
                          World *world, v2 pos)
{
#ifdef DENGINE_DEBUG
	DEBUG_LOG("Mob entity spawned");
#endif

	Entity *hero = &world->entities[entity_getIndex(world, world->heroId)];

	v2 size              = V2(58.0f, 98.0f);
	enum EntityType type = entitytype_mob;
	enum Direction dir   = direction_west;
	Texture *tex         = asset_getTexture(assetManager, texlist_hero);
	b32 collides         = TRUE;
	Entity *mob = entity_add(arena, world, pos, size, type, dir, tex, collides);

	mob->audioRenderer = PLATFORM_MEM_ALLOC(arena, 1, AudioRenderer);
	mob->audioRenderer->sourceIndex = AUDIO_SOURCE_UNASSIGNED;

	/* Populate mob animation references */
	entity_addAnim(assetManager, mob, "Claude idle");
}

Entity *entity_add(MemoryArena *arena, World *world, v2 pos, v2 size,
                   enum EntityType type, enum Direction direction, Texture *tex,
                   b32 collides)
{

#ifdef DENGINE_DEBUG
	ASSERT(world);
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

	switch (type)
	{
	case entitytype_hero:
		entity.stats               = PLATFORM_MEM_ALLOC(arena, 1, EntityStats);
		entity.stats->maxHealth    = 100;
		entity.stats->health       = entity.stats->maxHealth;
		entity.stats->actionRate   = 100;
		entity.stats->actionTimer  = entity.stats->actionRate;
		entity.stats->actionSpdMul = 100;
		entity.stats->entityIdToAttack = -1;
		entity.stats->queuedAttack     = entityattack_invalid;
		entity.state                   = entitystate_idle;
		break;
	case entitytype_mob:
	{
		entity.stats               = PLATFORM_MEM_ALLOC(arena, 1, EntityStats);
		entity.stats->maxHealth    = 100;
		entity.stats->health       = entity.stats->maxHealth;
		entity.stats->actionRate   = 80;
		entity.stats->actionTimer  = entity.stats->actionRate;
		entity.stats->actionSpdMul = 100;
		entity.stats->entityIdToAttack = -1;
		entity.stats->queuedAttack     = entityattack_invalid;
		entity.state                   = entitystate_idle;
		break;
	}

	default:
		break;
	}

	world->entities[world->freeEntityIndex++] = entity;
	Entity *result = &world->entities[world->freeEntityIndex - 1];

	return result;
}

void entity_clearData(MemoryArena *arena, World *world, Entity *entity)
{
	if (entity->stats)
		PLATFORM_MEM_FREE(arena, entity->stats, sizeof(EntityStats));

	if (entity->audioRenderer)
		PLATFORM_MEM_FREE(arena, entity->audioRenderer, sizeof(AudioRenderer));

	entity->type = entitytype_null;
}

i32 entity_getIndex(World *world, i32 entityId)
{
	i32 first = 0;
	i32 last  = world->freeEntityIndex - 1;

	while (first <= last)
	{
		i32 middle = (first + last) / 2;

		if (world->entities[middle].id > entityId)
			last = middle - 1;
		else if (world->entities[middle].id < entityId)
			first = middle + 1;
		else
			return middle;
	}

#ifdef DENGINE_DEBUG
	ASSERT(INVALID_CODE_PATH);
#endif
	return -1;
}

