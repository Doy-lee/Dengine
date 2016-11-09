#include "Dengine/Debug.h"
#include "Dengine/AssetManager.h"
#include "Dengine/MemoryArena.h"
#include "Dengine/Platform.h"
#include "Dengine/Renderer.h"
#include "Dengine/Asteroid.h"

typedef struct DebugState
{
	b32 init;
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

void debug_init(MemoryArena_ *arena, v2 windowSize, Font font)
{
	GLOBAL_debug.font          = font;
	GLOBAL_debug.callCount =
	    memory_pushBytes(arena, debugcount_num * sizeof(i32));
	GLOBAL_debug.stringLineGap = CAST(f32) font.verticalSpacing;

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

	GLOBAL_debug.init = TRUE;
}

void debug_recursivePrintXmlTree(XmlNode *root, i32 levelsDeep)
{
	if (!root)
	{
		return;
	}
	else
	{
		for (i32 i = 0; i < levelsDeep; i++)
		{
			printf("-");
		}

		printf("%s ", root->name);

		XmlAttribute *attribute = &root->attribute;
		printf("| %s = %s", attribute->name, attribute->value);

		while (attribute->next)
		{
			attribute = attribute->next;
			printf("| %s = %04s", attribute->name, attribute->value);
		}
		printf("\n");

		debug_recursivePrintXmlTree(root->child, levelsDeep+1);
		debug_recursivePrintXmlTree(root->next, levelsDeep);
	}
}

void debug_countIncrement(i32 id)
{
	if (GLOBAL_debug.init == FALSE) return;

	ASSERT(id < debugcount_num);
	GLOBAL_debug.callCount[id]++;
}

void debug_clearCounter()
{
	for (i32 i = 0; i < debugcount_num; i++)
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

INTERNAL void updateAndRenderDebugStack(Renderer *renderer, MemoryArena_ *arena,
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

INTERNAL void renderConsole(Renderer *renderer, MemoryArena_ *arena)
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
	debug_clearCounter();
}
