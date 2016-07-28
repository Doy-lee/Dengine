#include "Dengine/AssetManager.h"
#include "Dengine/MemoryArena.h"
#include "Dengine/Platform.h"
#include "Dengine/Debug.h"

#include "Dengine/Entity.h"

#include "WorldTraveller/WorldTraveller.h"

void entity_setActiveAnim(Entity *entity, enum AnimList animId)
{
#ifdef DENGINE_DEBUG
	ASSERT(animId < animlist_count);
	ASSERT(entity->anim[animId].anim);
#endif

	/* Reset current anim data */
	EntityAnim_ *currAnim  = &entity->anim[entity->currAnimId];
	currAnim->currDuration = currAnim->anim->frameDuration;
	currAnim->currFrame    = 0;

	/* Set entity active animation */
	entity->currAnimId = animId;
}

void entity_updateAnim(Entity *entity, f32 dt)
{
	if (!entity->tex) return;

	// TODO(doyle): Recheck why we have this twice
	EntityAnim_ *entityAnim = &entity->anim[entity->currAnimId];
	Animation anim          = *entityAnim->anim;
	i32 frameIndex          = anim.frameIndex[entityAnim->currFrame];
	v4 texRect              = anim.atlas->texRect[frameIndex];

	entityAnim->currDuration -= dt;
	if (entityAnim->currDuration <= 0.0f)
	{
		entityAnim->currFrame++;
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

void entity_addAnim(AssetManager *assetManager, Entity *entity, i32 animId)
{
	Animation *anim = asset_getAnim(assetManager, animId);
	entity->anim[animId].anim = anim;
	entity->anim[animId].currFrame = 0;
	entity->anim[animId].currDuration = anim->frameDuration;
}

void entity_addGenericMob(MemoryArena *arena, AssetManager *assetManager,
                          World *world, v2 pos)
{
#ifdef DENGINE_DEBUG
	DEBUG_LOG("Mob entity spawned");
#endif

	Entity *hero = &world->entities[world->heroIndex];

	v2 size              = V2(58.0f, 98.0f);
	enum EntityType type = entitytype_mob;
	enum Direction dir   = direction_west;
	Texture *tex         = asset_getTexture(assetManager, texlist_hero);
	b32 collides         = TRUE;
	Entity *mob = entity_add(arena, world, pos, size, type, dir, tex, collides);

	/* Populate mob animation references */
	entity_addAnim(assetManager, mob, animlist_hero_idle);
	entity_addAnim(assetManager, mob, animlist_hero_walk);
	entity_addAnim(assetManager, mob, animlist_hero_wave);
	entity_addAnim(assetManager, mob, animlist_hero_battlePose);
	entity_addAnim(assetManager, mob, animlist_hero_tackle);
	mob->currAnimId = animlist_hero_idle;

}

Entity *entity_add(MemoryArena *arena, World *world, v2 pos, v2 size,
                           enum EntityType type, enum Direction direction,
                           Texture *tex, b32 collides)
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

	switch(type)
	{
		case entitytype_hero:
		    entity.stats = PLATFORM_MEM_ALLOC(arena, 1, EntityStats);
		    entity.stats->maxHealth        = 100;
		    entity.stats->health           = entity.stats->maxHealth;
		    entity.stats->actionRate       = 100;
		    entity.stats->actionTimer      = entity.stats->actionRate;
		    entity.stats->actionSpdMul     = 100;
		    entity.stats->entityIdToAttack = -1;
		    entity.stats->queuedAttack     = entityattack_invalid;
		    entity.state                   = entitystate_idle;
			break;
		case entitytype_mob:
	    {
		    entity.stats = PLATFORM_MEM_ALLOC(arena, 1, EntityStats);
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

void entity_delete(MemoryArena *arena, World *world, i32 entityIndex)
{
	Entity *entity = &world->entities[entityIndex];
	PLATFORM_MEM_FREE(arena, entity->stats, sizeof(EntityStats));

	// TODO(doyle): Inefficient shuffle down all elements
	for (i32 i = entityIndex; i < world->freeEntityIndex-1; i++)
		world->entities[i] = world->entities[i+1];

	world->freeEntityIndex--;
}
