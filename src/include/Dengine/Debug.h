#ifndef DENGINE_DEBUG_H
#define DENGINE_DEBUG_H

#include "Dengine/Assets.h"
#include "Dengine/Common.h"
#include "Dengine/Math.h"
#include "Dengine/Renderer.h"
#include "Dengine/Entity.h"

#include "WorldTraveller/WorldTraveller.h"

#define INVALID_CODE_PATH 0
enum DebugCallCount
{
	debugcallcount_drawArrays,
	debugcallcount_num,
};

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

extern DebugState GLOBAL_debug;

inline void debug_callCountIncrement(i32 id)
{
	ASSERT(id < debugcallcount_num);
	GLOBAL_debug.callCount[id]++;
}

inline void debug_clearCallCounter()
{
	for (i32 i = 0; i < debugcallcount_num; i++)
		GLOBAL_debug.callCount[i] = 0;
}

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


void debug_init(MemoryArena *arena, v2 windowSize, Font font);

#define DEBUG_LOG(string) debug_consoleLog(string, __FILE__, __LINE__);
void debug_consoleLog(char *string, char *file, int lineNum);

#define DEBUG_PUSH_STRING(string) debug_pushString(string, NULL, "char")
#define DEBUG_PUSH_VAR(formatString, data, type)                            \
	debug_pushString(formatString, CAST(void *)&data, type)
void debug_pushString(char *formatString, void *data, char *dataType);

void debug_drawUi(GameState *state, f32 dt);

#endif
