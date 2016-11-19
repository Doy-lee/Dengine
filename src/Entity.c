#include "Dengine/Entity.h"
#include "Dengine/AssetManager.h"
#include "Dengine/Debug.h"

SubTexture entity_getActiveSubTexture(Entity *const entity)
{
	EntityAnim *entityAnim = &entity->animList[entity->animListIndex];
	Animation *anim        = entityAnim->anim;
	char *frameName        = anim->frameList[entityAnim->currFrame];

	SubTexture result = asset_getAtlasSubTex(anim->atlas, frameName);
	return result;
}

void entity_setActiveAnim(Entity *const entity, const char *const animName)
{
	/* Reset current anim data */
	EntityAnim *currEntityAnim     = &entity->animList[entity->animListIndex];
	currEntityAnim->currDuration   = currEntityAnim->anim->frameDuration;
	currEntityAnim->currFrame      = 0;
	currEntityAnim->timesCompleted = 0;

	/* Set entity active animation */
	for (i32 i = 0; i < ARRAY_COUNT(entity->animList); i++)
	{
		Animation *anim = entity->animList[i].anim;
		if (anim)
		{
			// TODO(doyle): Linear search, but not a problem if list is small
			if (common_strcmp(anim->key, animName) == 0)
			{
				entity->animListIndex = i;
				EntityAnim *newEntityAnim = &entity->animList[i];
				newEntityAnim->currDuration =
				    newEntityAnim->anim->frameDuration;
				newEntityAnim->currFrame = 0;
				return;
			}
		}
	}
	
	DEBUG_LOG("Entity does not have access to desired anim");
}

void entity_updateAnim(Entity *const entity, const f32 dt)
{
	if (!entity->tex)
		return;

	EntityAnim *currEntityAnim = &entity->animList[entity->animListIndex];
	Animation *anim = currEntityAnim->anim;

	currEntityAnim->currDuration -= dt;
	if (currEntityAnim->currDuration <= 0.0f)
	{
		currEntityAnim->currFrame++;
		currEntityAnim->currFrame = currEntityAnim->currFrame % anim->numFrames;
		if (currEntityAnim->currFrame == 0)
		{
			currEntityAnim->timesCompleted++;
		}

		currEntityAnim->currDuration = anim->frameDuration;
	}

	char *frameName    = anim->frameList[currEntityAnim->currFrame];
	SubTexture texRect = asset_getAtlasSubTex(anim->atlas, frameName);
	entity->size       = v2_scale(texRect.rect.max, entity->scale);
}

void entity_addAnim(AssetManager *const assetManager, Entity *const entity,
                    const char *const animName)
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
