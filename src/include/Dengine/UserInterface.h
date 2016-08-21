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

typedef struct WindowState
{
	char title[64];
	i32 id;

	UiItem childUiItems[16];
	i32 numChildUiItems;

	Rect rect;
	// TODO(doyle): Store this in the input data not window?
	v2 prevMouseP;

	b32 prevFrameWindowHeld;
	b32 windowHeld;
} WindowState;

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

	WindowState statWindow;
	WindowState debugWindow;
} UiState;

inline i32 userInterface_generateId(UiState *const uiState)
{
	i32 result = uiState->uniqueId++;
	return result;
}

i32 userInterface_button(UiState *const uiState,
                         MemoryArena *const arena,
                         AssetManager *const assetManager,
                         Renderer *const renderer,
                         Font *const font,
                         const KeyInput input,
                         const i32 id, const Rect rect,
                         const char *const label);

i32 userInterface_textField(UiState *const uiState,
                            MemoryArena *const arena,
                            AssetManager *const assetManager,
                            Renderer *const renderer, Font *const font,
                            KeyInput input, const i32 id, const Rect rect,
                            char *const string);

i32 userInterface_scrollbar(UiState *const uiState,
                            AssetManager *const assetManager,
                            Renderer *const renderer, const KeyInput input,
                            const i32 id, const Rect scrollBarRect,
                            i32 *const value, const i32 maxValue);

i32 userInterface_window(UiState *const uiState, MemoryArena *const arena,
                         AssetManager *const assetManager,
                         Renderer *const renderer, Font *const font,
                         const KeyInput input, WindowState *window);
#endif
