#include "Dengine/Debug.h"
#include "Dengine/Platform.h"
#include "Dengine/AssetManager.h"

DebugState GLOBAL_debug;

void debug_init()
{
	GLOBAL_debug.totalMemoryAllocated = 0;
	GLOBAL_debug.callCount = PLATFORM_MEM_ALLOC(debugcallcount_num, i32);

	GLOBAL_debug.numDebugStrings   = 0;
	GLOBAL_debug.stringUpdateTimer = 0.0f;
	GLOBAL_debug.stringUpdateRate  = 0.15f;

	GLOBAL_debug.stringLineGap = -1;
}

void debug_pushString(char *formatString, void *data, char *dataType)
{
	if (GLOBAL_debug.stringUpdateTimer <= 0)
	{
		i32 numDebugStrings = GLOBAL_debug.numDebugStrings;
		if (common_strcmp(dataType, "v2") == 0)
		{
			v2 val = *(CAST(v2 *) data);
			snprintf(GLOBAL_debug.debugStrings[numDebugStrings],
			         ARRAY_COUNT(GLOBAL_debug.debugStrings[0]),
			         formatString, val.x, val.y);
		}
		else if (common_strcmp(dataType, "i32") == 0)
		{
			i32 val = *(CAST(i32 *) data);
			snprintf(GLOBAL_debug.debugStrings[numDebugStrings],
			         ARRAY_COUNT(GLOBAL_debug.debugStrings[0]),
			         formatString, val);
		}
		else if (common_strcmp(dataType, "f32") == 0)
		{
			f32 val = *(CAST(f32 *) data);
			snprintf(GLOBAL_debug.debugStrings[numDebugStrings],
			         ARRAY_COUNT(GLOBAL_debug.debugStrings[0]),
			         formatString, val);
		}
		else if (common_strcmp(dataType, "char") == 0)
		{
			if (data)
			{
				char *val = CAST(char *) data;

				snprintf(GLOBAL_debug.debugStrings[numDebugStrings],
				         ARRAY_COUNT(GLOBAL_debug.debugStrings[0]),
				         formatString, val);
			}
			else
			{
				snprintf(GLOBAL_debug.debugStrings[numDebugStrings],
				         ARRAY_COUNT(GLOBAL_debug.debugStrings[0]),
				         formatString);
			}
		}
		
		else
		{
			ASSERT(INVALID_CODE_PATH);
		}
		GLOBAL_debug.numDebugStrings++;
	}
}

void debug_stringUpdateAndRender(Renderer *renderer, Font *font, f32 dt)
{
	if (GLOBAL_debug.stringLineGap == -1)
	{
		GLOBAL_debug.stringLineGap =
		    1.1f * asset_getVFontSpacing(font->metrics);
		GLOBAL_debug.initialStringPos =
		    V2(0.0f,
		       (renderer->size.y - 1.8f * GLOBAL_debug.stringLineGap));
		GLOBAL_debug.stringPos = GLOBAL_debug.initialStringPos;
	}

	for (i32 i = 0; i < GLOBAL_debug.numDebugStrings; i++)
	{
		f32 rotate = 0;
		v4 color   = V4(0, 0, 0, 1);
		renderer_staticString(renderer, font, GLOBAL_debug.debugStrings[i],
		                      GLOBAL_debug.stringPos, rotate, color);
		GLOBAL_debug.stringPos.y -=
		    (0.9f * GLOBAL_debug.stringLineGap);
	}

	if (GLOBAL_debug.stringUpdateTimer <= 0)
	{
		GLOBAL_debug.stringUpdateTimer =
		    GLOBAL_debug.stringUpdateRate;
	}
	else
	{
		GLOBAL_debug.stringUpdateTimer -= dt;
		if (GLOBAL_debug.stringUpdateTimer <= 0)
		{
			GLOBAL_debug.numDebugStrings = 0;
		}
	}

	GLOBAL_debug.stringPos = GLOBAL_debug.initialStringPos;
}

void debug_drawUi(GameState *state, f32 dt)
{
	AssetManager *assetManager = &state->assetManager;
	Renderer *renderer         = &state->renderer;
	World *const world         = &state->world[state->currWorldIndex];
	Entity *hero               = &world->entities[world->heroIndex];
	Font *font                 = &assetManager->font;

	// TODO(doyle): Dumb copy function from game so we don't expose api
	v4 cameraBounds = math_getRect(world->cameraPos, renderer->size);
	if (cameraBounds.x <= world->bounds.x)
	{
		cameraBounds.x = world->bounds.x;
		cameraBounds.z = cameraBounds.x + renderer->size.w;
	}

	if (cameraBounds.y >= world->bounds.y) cameraBounds.y = world->bounds.y;

	if (cameraBounds.z >= world->bounds.z)
	{
		cameraBounds.z = world->bounds.z;
		cameraBounds.x = cameraBounds.z - renderer->size.w;
	}
	if (cameraBounds.w <= world->bounds.w) cameraBounds.w = world->bounds.w;

#if 0
	Texture *emptyTex = asset_getTexture(assetManager, texlist_empty);
	v2 heroCenter     = v2_add(hero->pos, v2_scale(hero->hitboxSize, 0.5f));
	RenderTex renderTex = {emptyTex, V4(0, 1, 1, 0)};
	renderer_rect(&state->renderer, cameraBounds, heroCenter,
	              V2(distance, 5.0f), 0, renderTex, V4(1, 0, 0, 0.25f));
#endif

	if (world->numEntitiesInBattle > 0)
	{
		v4 color        = V4(1.0f, 0, 0, 1);
		char *battleStr = "IN-BATTLE RANGE";
		f32 strLenInPixels =
		    CAST(f32)(font->maxSize.w * common_strlen(battleStr));
		v2 strPos = V2((renderer->size.w * 0.5f) - (strLenInPixels * 0.5f),
		               renderer->size.h - 300.0f);
		renderer_staticString(&state->renderer, font, battleStr, strPos, 0,
		                      color);
	}

	for (i32 i = 0; i < world->maxEntities; i++)
	{
		Entity *const entity  = &world->entities[i];
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
			v2 strPos = v2_add(entity->pos, entity->hitboxSize);
			i32 indexOfLowerAInMetrics = 'a' - CAST(i32) font->codepointRange.x;
			strPos.y += font->charMetrics[indexOfLowerAInMetrics].offset.y;

			renderer_string(&state->renderer, cameraBounds, font, debugString,
			                strPos, 0, color);

			f32 stringLineGap = 1.1f * asset_getVFontSpacing(font->metrics);
			strPos.y -= GLOBAL_debug.stringLineGap;

			char entityPosStr[128];
			snprintf(entityPosStr, ARRAY_COUNT(entityPosStr), "%06.2f, %06.2f",
			         entity->pos.x, entity->pos.y);
			renderer_string(&state->renderer, cameraBounds, font, entityPosStr,
			                strPos, 0, color);

			strPos.y -= GLOBAL_debug.stringLineGap;
			char entityIDStr[32];
			snprintf(entityIDStr, ARRAY_COUNT(entityIDStr), "ID: %4d/%d", entity->id,
			         world->uniqueIdAccumulator-1);
			renderer_string(&state->renderer, cameraBounds, font, entityIDStr,
			                strPos, 0, color);

			if (entity->stats)
			{
				strPos.y -= GLOBAL_debug.stringLineGap;
				char entityHealth[32];
				snprintf(entityHealth, ARRAY_COUNT(entityHealth), "HP: %3.0f/%3.0f",
				         entity->stats->health, entity->stats->maxHealth);
				renderer_string(&state->renderer, cameraBounds, font,
				                entityHealth, strPos, 0, color);

				strPos.y -= GLOBAL_debug.stringLineGap;
				char entityTimer[32];
				snprintf(entityTimer, ARRAY_COUNT(entityTimer), "ATB: %3.0f/%3.0f",
				         entity->stats->actionTimer, entity->stats->actionRate);
				renderer_string(&state->renderer, cameraBounds, font,
				                entityTimer, strPos, 0, color);
			}

			strPos.y -= GLOBAL_debug.stringLineGap;
			char *entityStateStr = debug_entitystate_string(entity->state);
			renderer_string(&state->renderer, cameraBounds, font,
			                entityStateStr, strPos, 0, color);
		}
	}

	/* Render debug info stack */
	DEBUG_PUSH_STRING("== Hero Properties == ");
	DEBUG_PUSH_VAR("Hero Pos: %06.2f, %06.2f", hero->pos, "v2");
	DEBUG_PUSH_VAR("Hero dPos: %06.2f, %06.2f", hero->dPos, "v2");
	DEBUG_PUSH_VAR("Hero Busy Duration: %05.3f", hero->stats->busyDuration, "f32");
	char *heroStateString = debug_entitystate_string(hero->state);
	DEBUG_PUSH_VAR("Hero State: %s", *heroStateString, "char");
	char *heroQueuedAttackStr =
	    debug_entityattack_string(hero->stats->queuedAttack);
	DEBUG_PUSH_VAR("Hero QueuedAttack: %s", *heroQueuedAttackStr, "char");

	DEBUG_PUSH_STRING("== State Properties == ");
	DEBUG_PUSH_VAR("FreeEntityIndex: %d", world->freeEntityIndex, "i32");
	DEBUG_PUSH_VAR("glDrawArray Calls: %d",
	               GLOBAL_debug.callCount[debugcallcount_drawArrays], "i32");
	i32 debug_kbAllocated = GLOBAL_debug.totalMemoryAllocated / 1024;
	DEBUG_PUSH_VAR("TotalMemoryAllocated: %dkb", debug_kbAllocated, "i32");

	DEBUG_PUSH_STRING("== EntityIDs in Battle List == ");
	DEBUG_PUSH_VAR("NumEntitiesInBattle: %d", world->numEntitiesInBattle,
	               "i32");
	if (world->numEntitiesInBattle > 0)
	{
		for (i32 i = 0; i < world->maxEntities; i++)
		{
			if (world->entityIdInBattle[i])
				DEBUG_PUSH_VAR("Entity ID: %d", i, "i32");
		}
	}
	else
	{
		DEBUG_PUSH_STRING("-none-");
	}

	debug_stringUpdateAndRender(&state->renderer, font, dt);
	debug_clearCallCounter();
}
