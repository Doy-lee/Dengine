#ifndef DENGINE_DEBUG_H
#define DENGINE_DEBUG_H

#include "Dengine/Renderer.h"

#define INVALID_CODE_PATH TRUE
#define DEBUG_PUSH_STRING(formatString, data, type)                            \
	debug_pushString(formatString, CAST(void *) data, type)

enum DebugCallCount
{
	debugcallcount_drawArrays,
	debugcallcount_num,
};

typedef struct DebugState
{
	/* Debug strings rendered in top left corner */
	char debugStrings[256][64];
	i32 numDebugStrings;
	f32 stringUpdateTimer;
	f32 stringUpdateRate;

	v2 initialStringPos;
	v2 stringPos;

	f32 stringLineGap;
} DebugState;

extern DebugState GLOBAL_debugState;

void debug_init();
void debug_pushString(char *formatString, void *data, char *dataType);
void debug_stringUpdateAndRender(Renderer *renderer, Font *font, f32 dt);
#endif
