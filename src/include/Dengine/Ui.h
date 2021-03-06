#ifndef DENGINE_USER_INTERFACE_H
#define DENGINE_USER_INTERFACE_H

#include "Dengine/Common.h"
#include "Dengine/Math.h"
#include "Dengine/Platform.h"

/* Forward Declaration */
typedef struct AssetManager AssetManager;
typedef struct Font Font;
typedef struct MemoryArena MemoryArena_;
typedef struct Renderer Renderer;
typedef struct GameState GameState;

enum UiType
{
	uitype_button,
	uitype_scrollbar,
	uitype_textField,
	uitype_string,
	uitype_count,
};

typedef struct UiItem
{
	i32 id;
	char label[64];
	enum UiType type;

	Rect rect;

	// TODO(doyle): ECS this? Not all elements need
	i32 value;
	i32 maxValue;
	char string[80];
} UiItem;

typedef struct UiState
{
	i32 uniqueId;

	UiItem uiList[128];
	i32 numItems;

	i32 hotItem;
	i32 activeItem;
	i32 lastWidget;

	i32 kbdItem;
	enum KeyCode keyEntered;
	enum KeyCode keyMod;
	enum KeyCode keyChar;
} UiState;

inline i32 ui_generateId(UiState *const uiState)
{
	i32 result = uiState->uniqueId++;
	return result;
}

void ui_beginState(UiState *state);
void ui_endState(UiState *state, InputBuffer *input);

i32 ui_button(UiState *const uiState, MemoryArena_ *const arena,
              AssetManager *const assetManager, Renderer *const renderer,
              Font *const font, const InputBuffer input, const i32 id,
              const Rect rect, const char *const label, i32 zDepth);

i32 ui_textfield(UiState *const uiState, MemoryArena_ *const arena,
                 AssetManager *const assetManager, Renderer *const renderer,
                 Font *const font, InputBuffer input, const i32 id,
                 const Rect rect, char *const string, i32 zDepth);

i32 ui_scrollbar(UiState *const uiState, AssetManager *const assetManager,
                 Renderer *const renderer, const InputBuffer input,
                 const i32 id, const Rect scrollBarRect, i32 *const value,
                 const i32 maxValue, i32 zDepth);
#endif
