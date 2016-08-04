#include "Dengine/Debug.h"
#include "Dengine/AssetManager.h"
#include "Dengine/Entity.h"
#include "Dengine/MemoryArena.h"
#include "Dengine/Platform.h"
#include "Dengine/Renderer.h"
#include "Dengine/WorldTraveller.h"

typedef struct DebugState
{
	Font font;
	i32 *callCount;
	f32 stringLineGap;

	/* Debug strings rendered in top left corner */
	char debugStrings[64][128];
	i32 numDebugStrings;
	f32 stringUpdateTimer;
	f32 stringUpdateRate;
	v2 initialStringP;
	v2 currStringP;

	/* Debug gui console log */
	char console[20][128];
	i32 consoleIndex;
	v2 initialConsoleP;
} DebugState;

GLOBAL_VAR DebugState GLOBAL_debug;

inline char *debug_entitystate_string(i32 val)
{
	char *string;
	switch(val)
	{
		case entitystate_idle:
			string = "EntityState_Idle";
			break;
		case entitystate_battle:
			string = "EntityState_Battle";
			break;
		case entitystate_attack:
			string = "EntityState_Attack";
			break;
		case entitystate_dead:
			string = "EntityState_Dead";
			break;
		case entitystate_count:
			string = "EntityState_Count (Error!)";
			break;
		case entitystate_invalid:
			string = "EntityState_Invalid (Error!)";
			break;
		default:
			string = "EntityState Unknown (NOT DEFINED)";
	}
	return string;
}

inline char *debug_entityattack_string(i32 val)
{
	char *string;
	switch(val)
	{
		case entityattack_tackle:
			string = "EntityAttack_Tackle";
			break;
		case entityattack_count:
			string = "EntityAttack_Count (Error!)";
			break;
		case entityattack_invalid:
			string = "EntityAttack_Invalid";
			break;
		default:
			string = "EntityAttack Unknown (NOT DEFINED)";
	}
	return string;
}

void debug_init(MemoryArena *arena, v2 windowSize, Font font)
{
	GLOBAL_debug.font                 = font;
	GLOBAL_debug.callCount = PLATFORM_MEM_ALLOC(arena, debugcallcount_num, i32);
	GLOBAL_debug.stringLineGap = 1.1f * asset_getVFontSpacing(font.metrics);

	/* Init debug string stack */
	GLOBAL_debug.numDebugStrings   = 0;
	GLOBAL_debug.stringUpdateTimer = 0.0f;
	GLOBAL_debug.stringUpdateRate  = 0.15f;

	GLOBAL_debug.initialStringP =
	    V2(0.0f, (windowSize.h - 1.8f * GLOBAL_debug.stringLineGap));
	GLOBAL_debug.currStringP = GLOBAL_debug.initialStringP;

	/* Init gui console */
	i32 maxConsoleStrLen = ARRAY_COUNT(GLOBAL_debug.console[0]);
	GLOBAL_debug.consoleIndex = 0;

	// TODO(doyle): Font max size not entirely correct? using 1 * font.maxSize.w
	// reveals around 4 characters ..
	f32 consoleXPos = font.maxSize.w * 20;
	f32 consoleYPos = windowSize.h - 1.8f * GLOBAL_debug.stringLineGap;
	GLOBAL_debug.initialConsoleP = V2(consoleXPos, consoleYPos);
}

void debug_callCountIncrement(i32 id)
{
	ASSERT(id < debugcallcount_num);
	GLOBAL_debug.callCount[id]++;
}

void debug_clearCallCounter()
{
	for (i32 i = 0; i < debugcallcount_num; i++)
		GLOBAL_debug.callCount[i] = 0;
}


void debug_consoleLog(char *string, char *file, int lineNum)
{

	i32 maxConsoleStrLen = ARRAY_COUNT(GLOBAL_debug.console[0]);

	/* Completely clear out current console line */
	for (i32 i = 0; i < maxConsoleStrLen; i++)
		GLOBAL_debug.console[GLOBAL_debug.consoleIndex][i] = 0;

	i32 strIndex = 0;
	i32 fileStrLen = common_strlen(file);
	for (i32 count = 0; strIndex < maxConsoleStrLen; strIndex++, count++)
	{
		if (fileStrLen <= count) break;
		GLOBAL_debug.console[GLOBAL_debug.consoleIndex][strIndex] = file[count];
	}

	if (strIndex < maxConsoleStrLen)
		GLOBAL_debug.console[GLOBAL_debug.consoleIndex][strIndex++] = ':';

	char line[12] = {0};
	common_itoa(lineNum, line, ARRAY_COUNT(line));
	i32 lineStrLen = common_strlen(line);
	for (i32 count = 0; strIndex < maxConsoleStrLen; strIndex++, count++)
	{
		if (lineStrLen <= count) break;
		GLOBAL_debug.console[GLOBAL_debug.consoleIndex][strIndex] = line[count];
	}

	if (strIndex < maxConsoleStrLen)
		GLOBAL_debug.console[GLOBAL_debug.consoleIndex][strIndex++] = ':';

	i32 stringStrLen = common_strlen(string);
	for (i32 count = 0; strIndex < maxConsoleStrLen; strIndex++, count++)
	{
		if (stringStrLen <= count) break;
		GLOBAL_debug.console[GLOBAL_debug.consoleIndex][strIndex] = string[count];
	}

	if (strIndex >= maxConsoleStrLen)
	{
		GLOBAL_debug.console[GLOBAL_debug.consoleIndex][maxConsoleStrLen-4] = '.';
		GLOBAL_debug.console[GLOBAL_debug.consoleIndex][maxConsoleStrLen-3] = '.';
		GLOBAL_debug.console[GLOBAL_debug.consoleIndex][maxConsoleStrLen-2] = '.';
		GLOBAL_debug.console[GLOBAL_debug.consoleIndex][maxConsoleStrLen-1] = 0;
	}

	i32 maxConsoleLines = ARRAY_COUNT(GLOBAL_debug.console);
	GLOBAL_debug.consoleIndex++;

	if (GLOBAL_debug.consoleIndex >= maxConsoleLines)
		GLOBAL_debug.consoleIndex = 0;
}

void debug_pushString(char *formatString, void *data, char *dataType)
{
	if (GLOBAL_debug.numDebugStrings >=
	    ARRAY_COUNT(GLOBAL_debug.debugStrings))
	{
		DEBUG_LOG("Debug string stack is full, DEBUG_PUSH() failed");
		return;
	}

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
		else if (common_strcmp(dataType, "v3") == 0)
		{
			v3 val = *(CAST(v3 *) data);
			snprintf(GLOBAL_debug.debugStrings[numDebugStrings],
			         ARRAY_COUNT(GLOBAL_debug.debugStrings[0]),
			         formatString, val.x, val.y, val.z);
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

INTERNAL void updateAndRenderDebugStack(Renderer *renderer, MemoryArena *arena,
                                        f32 dt)
{
	for (i32 i = 0; i < GLOBAL_debug.numDebugStrings; i++)
	{
		f32 rotate = 0;
		v4 color   = V4(0, 0, 0, 1);
		renderer_staticString(renderer, arena, &GLOBAL_debug.font,
		                      GLOBAL_debug.debugStrings[i],
		                      GLOBAL_debug.currStringP, V2(0, 0), rotate, color);
		GLOBAL_debug.currStringP.y -= (0.9f * GLOBAL_debug.stringLineGap);
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

	GLOBAL_debug.currStringP = GLOBAL_debug.initialStringP;

}

INTERNAL void renderConsole(Renderer *renderer, MemoryArena *arena)
{
	i32 maxConsoleLines = ARRAY_COUNT(GLOBAL_debug.console);
	v2 consoleStrP = GLOBAL_debug.initialConsoleP;
	for (i32 i = 0; i < maxConsoleLines; i++)
	{
		f32 rotate = 0;
		v4 color   = V4(0, 0, 0, 1);
		renderer_staticString(renderer, arena, &GLOBAL_debug.font,
		                      GLOBAL_debug.console[i], consoleStrP,
		                      V2(0, 0), rotate, color);
		consoleStrP.y -= (0.9f * GLOBAL_debug.stringLineGap);
	}
}

void debug_drawUi(GameState *state, f32 dt)
{
	AssetManager *assetManager = &state->assetManager;
	Renderer *renderer         = &state->renderer;
	World *const world         = &state->world[state->currWorldIndex];
	Entity *hero = &world->entities[entity_getIndex(world, world->heroId)];

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

	Font *font = &GLOBAL_debug.font;
	if (world->numEntitiesInBattle > 0)
	{
		v4 color        = V4(1.0f, 0, 0, 1);
		char *battleStr = "IN-BATTLE RANGE";
		f32 strLenInPixels =
		    CAST(f32)(font->maxSize.w * common_strlen(battleStr));
		v2 strPos = V2((renderer->size.w * 0.5f) - (strLenInPixels * 0.5f),
		               renderer->size.h - 300.0f);
		renderer_staticString(&state->renderer, &state->arena, font, battleStr,
		                      strPos, V2(0, 0), 0, color);
	}

	for (i32 i = 0; i < world->freeEntityIndex; i++)
	{
		Entity *const entity  = &world->entities[i];
		if (entity->state == entitystate_dead) continue;

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

			renderer_string(&state->renderer, &state->arena, cameraBounds, font,
			                debugString, strPos, V2(0, 0), 0, color);

			f32 stringLineGap = 1.1f * asset_getVFontSpacing(font->metrics);
			strPos.y -= GLOBAL_debug.stringLineGap;

			char entityPosStr[128];
			snprintf(entityPosStr, ARRAY_COUNT(entityPosStr), "%06.2f, %06.2f",
			         entity->pos.x, entity->pos.y);
			renderer_string(&state->renderer, &state->arena, cameraBounds, font,
			                entityPosStr, strPos, V2(0, 0), 0, color);

			strPos.y -= GLOBAL_debug.stringLineGap;
			char entityIDStr[32];
			snprintf(entityIDStr, ARRAY_COUNT(entityIDStr), "ID: %4d/%d", entity->id,
			         world->uniqueIdAccumulator-1);
			renderer_string(&state->renderer, &state->arena, cameraBounds, font,
			                entityIDStr, strPos, V2(0, 0), 0, color);

			if (entity->stats)
			{
				strPos.y -= GLOBAL_debug.stringLineGap;
				char entityHealth[32];
				snprintf(entityHealth, ARRAY_COUNT(entityHealth), "HP: %3.0f/%3.0f",
				         entity->stats->health, entity->stats->maxHealth);
				renderer_string(&state->renderer, &state->arena, cameraBounds,
				                font, entityHealth, strPos, V2(0, 0), 0, color);

				strPos.y -= GLOBAL_debug.stringLineGap;
				char entityTimer[32];
				snprintf(entityTimer, ARRAY_COUNT(entityTimer), "ATB: %3.0f/%3.0f",
				         entity->stats->actionTimer, entity->stats->actionRate);
				renderer_string(&state->renderer, &state->arena, cameraBounds,
				                font, entityTimer, strPos, V2(0, 0), 0, color);

				strPos.y -= GLOBAL_debug.stringLineGap;
				char entityIdTarget[32];
				snprintf(entityIdTarget, ARRAY_COUNT(entityIdTarget),
				         "Targetting ID: %d", entity->stats->entityIdToAttack);
				renderer_string(&state->renderer, &state->arena, cameraBounds,
				                font, entityIdTarget, strPos, V2(0, 0), 0, color);
			}

			strPos.y -= GLOBAL_debug.stringLineGap;
			char *entityStateStr = debug_entitystate_string(entity->state);
			renderer_string(&state->renderer, &state->arena, cameraBounds, font,
			                entityStateStr, strPos, V2(0, 0), 0, color);

			if (entity->audioRenderer)
			{
				strPos.y -= GLOBAL_debug.stringLineGap;
				char entityAudioSourceIndex[32];
				snprintf(entityAudioSourceIndex,
				         ARRAY_COUNT(entityAudioSourceIndex),
				         "AudioSource Index: %d",
				         entity->audioRenderer->sourceIndex);
				renderer_string(&state->renderer, &state->arena, cameraBounds,
				                font, entityAudioSourceIndex, strPos, V2(0, 0),
				                0, color);
			}
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
	DEBUG_PUSH_VAR("Mouse Pos: %06.2f, %06.2f", state->input.mouse, "v2");

	i32 debug_bAllocated = state->arena.bytesAllocated;
	DEBUG_PUSH_VAR("TotalMemoryAllocated: %db", debug_bAllocated, "i32");
	i32 debug_kbAllocated = state->arena.bytesAllocated / 1024;
	DEBUG_PUSH_VAR("TotalMemoryAllocated: %dkb", debug_kbAllocated, "i32");

	AudioManager *audioManager = &state->audioManager;
	DEBUG_PUSH_STRING("== Audio System ==");
	for (i32 i = 0; i < ARRAY_COUNT(audioManager->sourceList); i++)
	{
		if (audioManager->sourceList[i].isFree) continue;
		v3 tmp = V3i(i, audioManager->sourceList[i].id,
		             audioManager->sourceList[i].isFree);
		DEBUG_PUSH_VAR("Source ID[%02.0f].id[%02.0f].isFree: %1.0f", tmp, "v3");
	}

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

	updateAndRenderDebugStack(&state->renderer, &state->arena, dt);
	renderConsole(&state->renderer, &state->arena);
	debug_clearCallCounter();
}
