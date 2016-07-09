#include "Dengine/Debug.h"

DebugState GLOBAL_debugState;

void debug_init()
{
	GLOBAL_debugState.numDebugStrings   = 0;
	GLOBAL_debugState.stringUpdateTimer = 0.0f;
	GLOBAL_debugState.stringUpdateRate  = 0.15f;

	GLOBAL_debugState.stringLineGap = -1;
}

void debug_pushString(char *formatString, void *data, char *dataType)
{
	if (GLOBAL_debugState.stringUpdateTimer <= 0)
	{
		i32 numDebugStrings = GLOBAL_debugState.numDebugStrings;
		if (common_strcmp(dataType, "v2") == 0)
		{
			v2 val = *(CAST(v2 *) data);
			snprintf(GLOBAL_debugState.debugStrings[numDebugStrings],
			         ARRAY_COUNT(GLOBAL_debugState.debugStrings[0]),
			         formatString, val.x, val.y);
		}
		else if (common_strcmp(dataType, "i32") == 0)
		{
			i32 val = *(CAST(i32 *) data);
			snprintf(GLOBAL_debugState.debugStrings[numDebugStrings],
			         ARRAY_COUNT(GLOBAL_debugState.debugStrings[0]),
			         formatString, val);
		}
		else
		{
			ASSERT(INVALID_CODE_PATH);
		}
		GLOBAL_debugState.numDebugStrings++;
	}
}

void debug_stringUpdateAndRender(Renderer *renderer, Font *font, f32 dt)
{
	if (GLOBAL_debugState.stringLineGap == -1)
	{
		GLOBAL_debugState.stringLineGap =
		    1.1f * asset_getVFontSpacing(font->metrics);
		GLOBAL_debugState.initialStringPos =
		    V2(0.0f,
		       (renderer->size.y - 1.8f * GLOBAL_debugState.stringLineGap));
		GLOBAL_debugState.stringPos = GLOBAL_debugState.initialStringPos;
	}

	for (i32 i = 0; i < GLOBAL_debugState.numDebugStrings; i++)
	{
		f32 rotate = 0;
		v4 color   = V4(0, 0, 0, 1);
		renderer_string(renderer, font, GLOBAL_debugState.debugStrings[i],
		                GLOBAL_debugState.stringPos, rotate, color);
		GLOBAL_debugState.stringPos.y -=
		    (0.9f * GLOBAL_debugState.stringLineGap);
	}

	if (GLOBAL_debugState.stringUpdateTimer <= 0)
	{
		GLOBAL_debugState.stringUpdateTimer =
		    GLOBAL_debugState.stringUpdateRate;
	}
	else
	{
		GLOBAL_debugState.stringUpdateTimer -= dt;
		if (GLOBAL_debugState.stringUpdateTimer <= 0)
		{
			GLOBAL_debugState.numDebugStrings = 0;
		}
	}

	GLOBAL_debugState.stringPos = GLOBAL_debugState.initialStringPos;
}
