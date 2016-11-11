#include "Dengine/UserInterface.h"
#include "Dengine/AssetManager.h"
#include "Dengine/Assets.h"
#include "Dengine/Debug.h"
#include "Dengine/Renderer.h"

i32 userInterface_button(UiState *const uiState, MemoryArena_ *const arena,
                         AssetManager *const assetManager,
                         Renderer *const renderer, Font *const font,
                         const KeyInput input, const i32 id, const Rect rect,
                         const char *const label)
{
	if (math_pointInRect(rect, input.mouseP))
	{
		uiState->hotItem = id;

		// NOTE(doyle): UI windows are drawn first, they steal focus on mouse
		// click since window logic is paired with the window rendering. If
		// a UI element resides over our mouse, we allow the element to override
		// the windows focus
		// TODO(doyle): Make a window list to iterate over window ids
		if (uiState->activeItem == uiState->statWindow.id ||
		    uiState->activeItem == uiState->debugWindow.id ||
			uiState->activeItem == 0)
		{
			if (input.keys[keycode_mouseLeft].endedDown)
			{
				uiState->activeItem = id;
			}
		}

	}

	RenderTex renderTex = renderer_createNullRenderTex(assetManager);

#if 0
	// Draw shadow
	renderer_staticRect(renderer, v2_add(V2(1, 1), rect.pos), rect.size,
	                    V2(0, 0), 0, renderTex, V4(0, 0, 0, 1));
#endif

	v2 buttonOffset = V2(0, 0);
	v4 buttonColor = {0};
	if (uiState->hotItem == id)
	{
		if (uiState->activeItem == id)
		{
			buttonOffset = V2(1, 1);
			buttonColor = V4(1, 1, 1, 1);
		}
		else
		{
			// TODO(doyle): Optional add effect on button hover
			buttonColor = V4(1, 1, 1, 1);
		}
	}
	else
	{
		buttonColor = V4(0.5f, 0.5f, 0.5f, 1);
	}

	/* If no widget has keyboard focus, take it */
	if (uiState->kbdItem == 0)
		uiState->kbdItem = id;

	/* If we have keyboard focus, show it */
	if (uiState->kbdItem == id)
	{
		// Draw outline
		renderer_staticRect(renderer,
		                    v2_add(V2(-2, -2), v2_add(buttonOffset, rect.pos)),
		                    v2_add(V2(4, 4), rect.size), V2(0, 0), 0, &renderTex,
		                    buttonColor, 0);
	}

	renderer_staticRect(renderer, v2_add(buttonOffset, rect.pos), rect.size,
	                    V2(0, 0), 0, &renderTex, buttonColor, 0);

	if (label)
	{
		v2 labelDim = asset_stringDimInPixels(font, label);
		v2 labelPos = rect.pos;

		// Initially position the label to half the width of the button
		labelPos.x += (rect.size.w * 0.5f);

		// Move the label pos back half the length of the string (i.e.
		// center it)
		labelPos.x -= (CAST(f32) labelDim.w * 0.5f);

		if (labelDim.h < rect.size.h)
		{
			labelPos.y += (rect.size.h * 0.5f);
			labelPos.y -= (CAST(f32)labelDim.h * 0.5f);
		}

		labelPos = v2_add(labelPos, buttonOffset);
		renderer_staticString(renderer, arena, font, label, labelPos, V2(0, 0),
		                      0, V4(0, 0, 0, 1), 0);
	}

	// After renderering before click check, see if we need to process keys
	if (uiState->kbdItem == id)
	{
		switch (uiState->keyEntered)
		{
		case keycode_tab:
			// Set focus to nothing and let next widget get focus
			uiState->kbdItem = 0;
			if (uiState->keyMod == keycode_leftShift)
				uiState->kbdItem = uiState->lastWidget;

			// Clear key state so next widget doesn't auto grab
			uiState->keyEntered = keycode_null;
			break;
		case keycode_enter:
			return 1;
		default:
			break;
		}
	}

	uiState->lastWidget = id;

	// If button is hot and active, but mouse button is not
	// down, the user must have clicked the button.
	if (!input.keys[keycode_mouseLeft].endedDown &&
	    uiState->hotItem == id &&
	    uiState->activeItem == id)
	{
		return id;
	}

	return 0;
}

i32 userInterface_scrollbar(UiState *const uiState,
                            AssetManager *const assetManager,
                            Renderer *const renderer, const KeyInput input,
                            const i32 id, const Rect scrollBarRect,
                            i32 *const value, const i32 maxValue)
{
#ifdef DENGINE_DEBUG
	ASSERT(*value <= maxValue);
#endif

	if (math_pointInRect(scrollBarRect, input.mouseP))
	{
		uiState->hotItem = id;
		if (uiState->activeItem == uiState->statWindow.id ||
		    uiState->activeItem == uiState->debugWindow.id ||
			uiState->activeItem == 0)
		{
			if (input.keys[keycode_mouseLeft].endedDown)
			{
				uiState->activeItem = id;
			}
		}
	}

	RenderTex renderTex = renderer_createNullRenderTex(assetManager);

	/* If no widget has keyboard focus, take it */
	if (uiState->kbdItem == 0)
		uiState->kbdItem = id;

	/* If we have keyboard focus, show it */
	if (uiState->kbdItem == id)
	{
		// Draw outline
		renderer_staticRect(renderer, v2_add(V2(-2, -2), scrollBarRect.pos),
		                    v2_add(V2(4, 4), scrollBarRect.size), V2(0, 0), 0,
		                    &renderTex, V4(1, 0, 0, 1), 0);
	}

	// Render scroll bar background
	renderer_staticRect(renderer, scrollBarRect.pos, scrollBarRect.size,
	                    V2(0, 0), 0, &renderTex, V4(0.75f, 0.5f, 0.5f, 1), 0);

	// Render scroll bar slider
	v2 sliderSize   = V2(16, 16);
	v4 sliderColor  = V4(0, 0, 0, 1);

	f32 sliderPercentageOffset = (CAST(f32) *value / CAST(f32) maxValue);
	f32 sliderYOffsetToBar =
	    (scrollBarRect.size.h - sliderSize.h) * sliderPercentageOffset;
	v2 sliderPos   = v2_add(scrollBarRect.pos, V2(0, sliderYOffsetToBar));

	if (uiState->hotItem == id || uiState->activeItem == id)
		sliderColor = V4(1.0f, 0, 0, 1);
	else
		sliderColor = V4(0.0f, 1.0f, 0, 1);

	renderer_staticRect(renderer, sliderPos, sliderSize, V2(0, 0), 0, &renderTex,
	                    sliderColor, 0);

	if (uiState->kbdItem == id)
	{
		switch (uiState->keyEntered)
		{
		case keycode_tab:
			uiState->kbdItem = 0;
			if (uiState->keyMod == keycode_leftShift)
				uiState->kbdItem = uiState->lastWidget;

			// Clear key state so next widget doesn't auto grab
			uiState->keyEntered = keycode_null;
			break;
		case keycode_up:
			// TODO(doyle): Fix input for this to work, i.e. proper rate limited input poll
			if (*value < maxValue)
			{
				(*value)++;
				return id;
			}
		case keycode_down:
			if (*value > 0)
			{
				(*value)--;
				return id;
			}
		default:
			break;
		}
	}

	uiState->lastWidget = id;

	if (uiState->activeItem == id)
	{
		f32 mouseYRelToRect = input.mouseP.y - scrollBarRect.pos.y;

		// Bounds check
		if (mouseYRelToRect < 0)
			mouseYRelToRect = 0;
		else if (mouseYRelToRect > scrollBarRect.size.h)
			mouseYRelToRect = scrollBarRect.size.h;

		f32 newSliderPercentOffset =
		    (CAST(f32) mouseYRelToRect / scrollBarRect.size.h);

		i32 newValue = CAST(i32)(newSliderPercentOffset * CAST(f32)maxValue);
		if (newValue != *value)
		{
			*value = newValue;
			return id;
		}
	}

	return 0;
}

i32 userInterface_textField(UiState *const uiState, MemoryArena_ *const arena,
                            AssetManager *const assetManager,
                            Renderer *const renderer, Font *const font,
                            KeyInput input, const i32 id, const Rect rect,
                            char *const string)
{
	i32 strLen = common_strlen(string);
	b32 changed = FALSE;

	if (math_pointInRect(rect, input.mouseP))
	{
		uiState->hotItem = id;
		if (uiState->activeItem == uiState->statWindow.id ||
		    uiState->activeItem == uiState->debugWindow.id ||
			uiState->activeItem == 0)
		{
			if (input.keys[keycode_mouseLeft].endedDown)
			{
				uiState->activeItem = id;
			}
		}
	}

	/* If no widget has keyboard focus, take it */
	if (uiState->kbdItem == 0)
		uiState->kbdItem = id;

	RenderTex renderTex = renderer_createNullRenderTex(assetManager);
	/* If we have keyboard focus, show it */
	if (uiState->kbdItem == id)
	{
		// Draw outline
		renderer_staticRect(renderer, v2_add(V2(-2, -2), rect.pos),
		                    v2_add(V2(4, 4), rect.size), V2(0, 0), 0,
		                    &renderTex, V4(1.0f, 0, 0, 1), 0);
	}

	// Render text field
	renderer_staticRect(renderer, rect.pos, rect.size, V2(0, 0), 0,
	                    &renderTex, V4(0.75f, 0.5f, 0.5f, 1), 0);

	if (uiState->activeItem == id || uiState->hotItem == id)
	{
		renderer_staticRect(renderer, rect.pos, rect.size, V2(0, 0), 0,
		                    &renderTex, V4(0.75f, 0.75f, 0.0f, 1), 0);
	}
	else
	{
		renderer_staticRect(renderer, rect.pos, rect.size, V2(0, 0), 0,
		                    &renderTex, V4(0.5f, 0.5f, 0.5f, 1), 0);
	}

	v2 strPos = rect.pos;
	renderer_staticString(renderer, arena, font, string, strPos, V2(0, 0), 0,
	                      V4(0, 0, 0, 1), 0);

	if (uiState->kbdItem == id)
	{
		switch (uiState->keyEntered)
		{
		case keycode_tab:
			uiState->kbdItem = 0;
			if (uiState->keyMod == keycode_leftShift)
				uiState->kbdItem = uiState->lastWidget;

			uiState->keyEntered = keycode_null;
			break;

		case keycode_backspace:
			if (strLen > 0)
			{
				string[--strLen] = 0;
				changed        = TRUE;
			}
			break;
		default:
			break;
		}

		if (uiState->keyChar >= keycode_space &&
		    uiState->keyChar <= keycode_tilda && strLen < 30)
		{
			string[strLen++] = uiState->keyChar + ' ';
			string[strLen] = 0;
			changed = TRUE;
		}
	}

	if (!input.keys[keycode_mouseLeft].endedDown &&
	    uiState->hotItem == id &&
	    uiState->activeItem == id)
	{
		uiState->kbdItem = id;
	}

	uiState->lastWidget = id;

	if (changed) return id;
	return 0;
}

i32 userInterface_window(UiState *const uiState, MemoryArena_ *const arena,
                         AssetManager *const assetManager,
                         Renderer *const renderer, Font *const font,
                         const KeyInput input, WindowState *window)
{
	if (math_pointInRect(window->rect, input.mouseP))
	{
		uiState->hotItem = window->id;
		if (uiState->activeItem == 0 && input.keys[keycode_mouseLeft].endedDown)
			uiState->activeItem = window->id;
	}

	Rect rect = window->rect;
	RenderTex nullRenderTex = renderer_createNullRenderTex(assetManager);
	renderer_staticRect(renderer, rect.pos, rect.size, V2(0, 0), 0,
	                    &nullRenderTex, V4(0.25f, 0.25f, 0.5f, 0.5f), 0);

	v2 menuTitleP = v2_add(rect.pos, V2(0, rect.size.h - 10));
	renderer_staticString(renderer, arena, font, window->title, menuTitleP,
	                      V2(0, 0), 0, V4(0, 0, 0, 1), 0);

	/* Draw window elements */
	i32 firstActiveChildId = -1;
	for (i32 i = 0; i < window->numChildUiItems; i++)
	{
		UiItem *childUi = &window->childUiItems[i];

		// TODO(doyle): Redundant? If we can only have 1 active child at a time
		// What about overlapping elements?
		i32 getChildActiveState = -1;
		switch(childUi->type)
		{
		case uitype_button:
			// TODO(doyle): Bug in font rendering once button reaches 700-800+
			// pixels
			getChildActiveState = userInterface_button(
			    uiState, arena, assetManager, renderer, font, input,
			    childUi->id, childUi->rect, childUi->label);
			break;

		case uitype_scrollbar:
			getChildActiveState = userInterface_scrollbar(
			    uiState, assetManager, renderer, input, childUi->id,
			    childUi->rect, &childUi->value, childUi->maxValue);
			break;

		case uitype_textField:
			getChildActiveState = userInterface_textField(
			    uiState, arena, assetManager, renderer, font, input,
			    childUi->id, childUi->rect, childUi->string);
			break;

		default:
			DEBUG_LOG(
			    "userInterface_window() warning: Enum uitype unrecognised");
			break;
		}

		// NOTE(doyle): Capture only the first active id, but keep loop going to
		// render the rest of the window ui
		if (firstActiveChildId == -1 && getChildActiveState > 0)
			firstActiveChildId = getChildActiveState;
	}

	if (firstActiveChildId != -1)
		return firstActiveChildId;

	// NOTE(doyle): activeItem captures mouse click within the UI bounds, but if
	// the user drags the mouse outside the bounds quicker than the game updates
	// then we use a second flag which only "unclicks" when the mouse is let go
	if (uiState->activeItem == window->id) window->windowHeld = TRUE;

	if (window->windowHeld)
	{
		if (!input.keys[keycode_mouseLeft].endedDown)
		{
			window->windowHeld          = FALSE;
			window->prevFrameWindowHeld = FALSE;
		}
	}

	if (window->windowHeld)
	{
		// NOTE(doyle): If this is the first window click we don't process
		// movement and store the current position to delta from next cycle
		if (window->prevFrameWindowHeld)
		{
			// NOTE(doyle): Window clicked and held
			v2 deltaP = v2_sub(input.mouseP, window->prevMouseP);

			for (i32 i = 0; i < window->numChildUiItems; i++)
			{
				UiItem *childUi = &window->childUiItems[i];
				childUi->rect.pos = v2_add(deltaP, childUi->rect.pos);
			}

			DEBUG_PUSH_VAR("Delta Pos %4.2f, %4.2f", deltaP, "v2");
			window->rect.pos = v2_add(deltaP, window->rect.pos);
		}
		else
		{
			window->prevFrameWindowHeld = TRUE;
		}

		window->prevMouseP = input.mouseP;
	}


	if (!input.keys[keycode_mouseLeft].endedDown &&
	    uiState->hotItem == window->id &&
	    uiState->activeItem == window->id)
	{
		return window->id;
	}

	return 0;
}

