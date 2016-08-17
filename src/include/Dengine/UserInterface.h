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

typedef struct UiState
{
	i32 hotItem;
	i32 activeItem;

	i32 kbdItem;
	enum KeyCode keyEntered;
	enum KeyCode keyMod;
	enum KeyCode keyChar;

	i32 lastWidget;
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
