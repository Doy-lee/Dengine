#include "Dengine/UserInterface.h"
#include "Dengine/AssetManager.h"
#include "Dengine/Assets.h"
#include "Dengine/Renderer.h"
#include "Dengine/Debug.h"

i32 userInterface_button(UiState *const uiState,
                         MemoryArena *const arena,
                         AssetManager *const assetManager,
                         Renderer *const renderer,
                         Font *const font,
                         const KeyInput input,
                         const i32 id, const Rect rect, const char *const label)
{
	if (math_pointInRect(rect, input.mouseP))
	{
		uiState->hotItem = id;
		if (uiState->activeItem == 0 && input.keys[keycode_mouseLeft].endedDown)
			uiState->activeItem = id;
	}

	RenderTex renderTex = renderer_createNullRenderTex(assetManager);

	/* If no widget has keyboard focus, take it */
	if (uiState->kbdItem == 0)
		uiState->kbdItem = id;

	/* If we have keyboard focus, show it */
	if (uiState->kbdItem == id)
	{
		// Draw outline
		renderer_staticRect(renderer, v2_add(V2(-6, -6), rect.pos),
		                    v2_add(V2(20, 20), rect.size), V2(0, 0), 0, renderTex,
		                    V4(1.0f, 0, 0, 1));
	}

	// Draw shadow
	renderer_staticRect(renderer, v2_add(V2(8, 8), rect.pos), rect.size,
	                    V2(0, 0), 0, renderTex, V4(0, 0, 0, 1));

	v2 buttonOffset = V2(0, 0);
	if (uiState->hotItem == id)
	{
		if (uiState->activeItem == id)
		{
			buttonOffset = V2(2, 2);
			renderer_staticRect(renderer, v2_add(buttonOffset, rect.pos),
			                    rect.size, V2(0, 0), 0, renderTex,
			                    V4(1, 1, 1, 1));
		}
		else
		{
			renderer_staticRect(renderer, v2_add(buttonOffset, rect.pos),
			                    rect.size, V2(0, 0), 0, renderTex,
			                    V4(1, 1, 1, 1));
		}
	}
	else
	{
		renderer_staticRect(renderer, v2_add(buttonOffset, rect.pos), rect.size,
		                    V2(0, 0), 0, renderTex, V4(0.5f, 0.5f, 0.5f, 1));
	}

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
		                      0, V4(0, 0, 0, 1));
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

	if (!input.keys[keycode_mouseLeft].endedDown &&
	    uiState->hotItem == id &&
	    uiState->activeItem == id)
	{
		return 1;
	}

	return 0;
}

i32 userInterface_scrollBar(UiState *const uiState,
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
		if (uiState->activeItem == 0 && input.keys[keycode_mouseLeft].endedDown)
			uiState->activeItem = id;
	}

	RenderTex renderTex = renderer_createNullRenderTex(assetManager);

	/* If no widget has keyboard focus, take it */
	if (uiState->kbdItem == 0)
		uiState->kbdItem = id;

	/* If we have keyboard focus, show it */
	if (uiState->kbdItem == id)
	{
		// Draw outline
		renderer_staticRect(renderer, v2_add(V2(-6, -6), scrollBarRect.pos),
		                    v2_add(V2(20, 20), scrollBarRect.size), V2(0, 0), 0,
		                    renderTex, V4(1.0f, 0, 0, 1));
	}

	// Render scroll bar background
	renderer_staticRect(renderer, scrollBarRect.pos, scrollBarRect.size,
	                    V2(0, 0), 0, renderTex, V4(0.75f, 0.5f, 0.5f, 1));

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

	renderer_staticRect(renderer, sliderPos, sliderSize, V2(0, 0), 0, renderTex,
	                    sliderColor);

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
				return 1;
			}
		case keycode_down:
			if (*value > 0)
			{
				(*value)--;
				return 1;
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
			return 1;
		}
	}

	return 0;
}

i32 userInterface_textField(UiState *const uiState, MemoryArena *const arena,
                            AssetManager *const assetManager,
                            Renderer *const renderer, Font *const font,
                            KeyInput input, const i32 id, v2 pos,
                            char *const string)
{
	i32 strLen = common_strlen(string);
	b32 changed = FALSE;

	Rect textRect = {0};
	textRect.pos = pos;
	textRect.size = V2(30 * font->maxSize.w, font->maxSize.h);
	if (math_pointInRect(textRect, input.mouseP))
	{
		uiState->hotItem = id;
		if (uiState->activeItem == 0 && input.keys[keycode_mouseLeft].endedDown)
		{
			uiState->activeItem = id;
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
		renderer_staticRect(renderer, v2_add(V2(-4, -4), textRect.pos),
		                    v2_add(V2(16, 16), textRect.size), V2(0, 0), 0,
		                    renderTex, V4(1.0f, 0, 0, 1));
	}

	// Render text field
	renderer_staticRect(renderer, textRect.pos, textRect.size, V2(0, 0), 0,
	                    renderTex, V4(0.75f, 0.5f, 0.5f, 1));

	if (uiState->activeItem == id || uiState->hotItem == id)
	{
		renderer_staticRect(renderer, textRect.pos, textRect.size, V2(0, 0), 0,
		                    renderTex, V4(0.75f, 0.75f, 0.0f, 1));
	}
	else
	{
		renderer_staticRect(renderer, textRect.pos, textRect.size, V2(0, 0), 0,
		                    renderTex, V4(0.5f, 0.5f, 0.5f, 1));
	}

	v2 strPos = textRect.pos;

	renderer_staticString(renderer, arena, font, string, strPos, V2(0, 0), 0,
	                      V4(0, 0, 0, 1));

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
	return changed;
}

