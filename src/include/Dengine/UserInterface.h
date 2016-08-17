#ifndef DENGINE_USER_INTERFACE_H
#define DENGINE_USER_INTERFACE_H

#include "Dengine/Common.h"
#include "Dengine/Math.h"
#include "Dengine/Platform.h"

/* Forward Declaration */
typedef struct AssetManager AssetManager;
typedef struct Font Font;
typedef struct MemoryArena MemoryArena;
typedef struct Renderer Renderer;

enum UiType
{
	uitype_button,
	uitype_scrollbar,
	uitype_textfield,
	uitype_string,
	uitype_count,
};

typedef struct UiItem
{
	i32 id;
	Rect rect;
	enum UiType type;
} UiItem;

typedef struct UiState
{
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

i32 userInterface_button(UiState *const uiState,
                         AssetManager *const assetManager,
                         Renderer *const renderer, const KeyInput input,
                         const i32 id, const Rect rect);

i32 userInterface_textField(UiState *const uiState, MemoryArena *arena,
                            AssetManager *const assetManager,
                            Renderer *const renderer, Font *const font,
                            KeyInput input, const i32 id, v2 pos,
                            char *const string);

i32 userInterface_scrollBar(UiState *const uiState,
                            AssetManager *const assetManager,
                            Renderer *const renderer, const KeyInput input,
                            const i32 id, const Rect scrollBarRect,
                            i32 *const value, const i32 maxValue);
#endif
