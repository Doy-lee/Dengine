#ifndef DENGINE_DEBUG_H
#define DENGINE_DEBUG_H

#include "Dengine/Assets.h"
#include "Dengine/Common.h"
#include "Dengine/Math.h"

/* Forward Declaration */
typedef struct GameState GameState;
typedef struct MemoryArena MemoryArena_;

#define INVALID_CODE_PATH 0
enum DebugCount
{
	debugcount_drawArrays,
	debugcount_platformMemAlloc,
	debugcount_platformMemFree,
	debugcount_numVertex,
	debugcount_renderGroups,
	debugcount_num,
};

void debug_init(v2 windowSize, Font font);

#define DEBUG_RECURSIVE_PRINT_XML_TREE(sig) debug_recursivePrintXmlTree(sig, 1)
void debug_recursivePrintXmlTree(XmlNode *root, i32 levelsDeep);

void debug_countIncrement(enum DebugCount id);

#define DEBUG_LOG(string) debug_consoleLog(string, __FILE__, __LINE__);
void debug_consoleLog(char *string, char *file, int lineNum);

#define DEBUG_PUSH_STRING(string) debug_pushString(string, NULL, "char")
#define DEBUG_PUSH_VAR(formatString, data, type)                               \
	debug_pushString(formatString, CAST(void *) & data, type)
void debug_pushString(char *formatString, void *data, char *dataType);

void debug_drawUi(GameState *state, f32 dt);

#endif
