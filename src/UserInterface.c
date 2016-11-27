#include "Dengine/UserInterface.h"
#include "Dengine/AssetManager.h"
#include "Dengine/Assets.h"
#include "Dengine/Asteroid.h"
#include "Dengine/Debug.h"
#include "Dengine/Renderer.h"

void userInterface_beginState(UiState *state)
{
	state->hotItem = 0;
}

void userInterface_endState(UiState *state, InputBuffer *input)
{
	if (!common_isSet(input->keys[keycode_mouseLeft].flags,
	                  keystateflag_ended_down))
	{
		state->activeItem = 0;
	}
	else if (state->activeItem == 0)
	{
		state->activeItem = -1;
	}

	if (state->keyEntered == keycode_tab) state->kbdItem = 0;

	state->keyEntered = keycode_null;
	state->keyChar    = keycode_null;
}

i32 userInterface_button(UiState *const uiState, MemoryArena_ *const arena,
                         AssetManager *const assetManager,
                         Renderer *const renderer, Font *const font,
                         const InputBuffer input, const i32 id, const Rect rect,
                         const char *const label)
{
	if (math_rect_contains_p(rect, input.mouseP))
	{
		uiState->hotItem = id;
		if (uiState->activeItem == 0)
		{
			if (common_isSet(input.keys[keycode_mouseLeft].flags,
			                 keystateflag_ended_down))
			{
				uiState->activeItem = id;
			}
		}

	}

#if 0
	// Draw shadow
	renderer_staticRect(renderer, v2_add(V2(1, 1), rect.min), rect.size,
	                    V2(0, 0), 0, renderTex, V4(0, 0, 0, 1));
#endif

	v2 buttonOffset = V2(0, 0);
	v4 buttonColor  = V4(0, 0, 0, 1);
	if (uiState->hotItem == id)
	{
		if (uiState->activeItem == id)
		{
			buttonOffset = V2(1, 1);
			buttonColor = V4(0.8f, 0.8f, 0.8f, 1);
		}
		else
		{
			// NOTE(doyle): Hover effect
			buttonColor = V4(0.05f, 0.05f, 0.05f, 1);
		}
	}

	/* If no widget has keyboard focus, take it */
	if (uiState->kbdItem == 0)
		uiState->kbdItem = id;

	v2 rectSize = math_rect_get_size(rect);
	/* If we have keyboard focus, show it */
	if (uiState->kbdItem == id)
	{
		// Draw outline
		renderer_staticRect(renderer,
		                    v2_add(V2(-2, -2), v2_add(buttonOffset, rect.min)),
		                    v2_add(V2(4, 4), rectSize), V2(0, 0), 0, NULL,
		                    buttonColor, renderflag_no_texture);
	}

	renderer_staticRect(renderer, v2_add(buttonOffset, rect.min),
	                    rectSize, V2(0, 0), 0, NULL,
	                    buttonColor, renderflag_no_texture);

	if (label)
	{
		v2 labelDim = asset_stringDimInPixels(font, label);
		v2 labelPos = rect.min;

		// Initially position the label to half the width of the button
		labelPos.x += (rectSize.w * 0.5f);

		// Move the label pos back half the length of the string (i.e.
		// center it)
		labelPos.x -= (CAST(f32) labelDim.w * 0.5f);

		if (labelDim.h < rectSize.h)
		{
			labelPos.y += (rectSize.h * 0.5f);
			labelPos.y -= (CAST(f32)labelDim.h * 0.5f);
		}

		// TODO(doyle): We're using odd colors to overcome z-sorting by forcing
		// button text into another render group
		labelPos = v2_add(labelPos, buttonOffset);
		renderer_staticString(renderer, arena, font, label, labelPos, V2(0, 0),
		                      0, V4(0.9f, 0.9f, 0.9f, 0.9f), 0);
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
	if (!common_isSet(input.keys[keycode_mouseLeft].flags,
	                  keystateflag_ended_down) &&
	    uiState->hotItem == id && uiState->activeItem == id)
	{
		return id;
	}

	return 0;
}

i32 userInterface_scrollbar(UiState *const uiState,
                            AssetManager *const assetManager,
                            Renderer *const renderer, const InputBuffer input,
                            const i32 id, const Rect scrollBarRect,
                            i32 *const value, const i32 maxValue)
{
#ifdef DENGINE_DEBUG
	ASSERT(*value <= maxValue);
#endif

	if (math_rect_contains_p(scrollBarRect, input.mouseP))
	{
		uiState->hotItem = id;
		if (uiState->activeItem == 0)
		{
			if (common_isSet(input.keys[keycode_mouseLeft].flags,
			                 keystateflag_ended_down))
			{
				uiState->activeItem = id;
			}
		}
	}

	RenderTex renderTex = renderer_createNullRenderTex(assetManager);

	/* If no widget has keyboard focus, take it */
	if (uiState->kbdItem == 0)
		uiState->kbdItem = id;

	v2 rectSize = math_rect_get_size(scrollBarRect);
	/* If we have keyboard focus, show it */
	if (uiState->kbdItem == id)
	{
		// Draw outline
		renderer_staticRect(renderer, v2_add(V2(-2, -2), scrollBarRect.min),
		                    v2_add(V2(4, 4), rectSize), V2(0, 0), 0,
		                    &renderTex, V4(1, 0, 0, 1), 0);
	}

	// Render scroll bar background
	renderer_staticRect(renderer, scrollBarRect.min, rectSize,
	                    V2(0, 0), 0, &renderTex, V4(0.75f, 0.5f, 0.5f, 1), 0);

	// Render scroll bar slider
	v2 sliderSize   = V2(16, 16);
	v4 sliderColor  = V4(0, 0, 0, 1);

	f32 sliderPercentageOffset = (CAST(f32) *value / CAST(f32) maxValue);
	f32 sliderYOffsetToBar =
	    (rectSize.h - sliderSize.h) * sliderPercentageOffset;
	v2 sliderPos   = v2_add(scrollBarRect.min, V2(0, sliderYOffsetToBar));

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
		f32 mouseYRelToRect = input.mouseP.y - scrollBarRect.min.y;

		// Bounds check
		if (mouseYRelToRect < 0)
			mouseYRelToRect = 0;
		else if (mouseYRelToRect > rectSize.h)
			mouseYRelToRect = rectSize.h;

		f32 newSliderPercentOffset =
		    (CAST(f32) mouseYRelToRect / rectSize.h);

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
                            InputBuffer input, const i32 id, const Rect rect,
                            char *const string)
{
	i32 strLen = common_strlen(string);
	b32 changed = FALSE;

	if (math_rect_contains_p(rect, input.mouseP))
	{
		uiState->hotItem = id;
		if (uiState->activeItem == 0)
		{
			if (common_isSet(input.keys[keycode_mouseLeft].flags,
			                 keystateflag_ended_down))
			{
				uiState->activeItem = id;
			}
		}
	}

	/* If no widget has keyboard focus, take it */
	if (uiState->kbdItem == 0)
		uiState->kbdItem = id;

	v2 rectSize = math_rect_get_size(rect);
	/* If we have keyboard focus, show it */
	if (uiState->kbdItem == id)
	{
		// Draw outline
		renderer_staticRect(renderer, v2_add(V2(-2, -2), rect.min),
		                    v2_add(V2(4, 4), rectSize), V2(0, 0), 0,
		                    NULL, V4(1.0f, 0, 0, 1), 0);
	}

	// Render text field
	renderer_staticRect(renderer, rect.min, rectSize, V2(0, 0), 0, NULL,
	                    V4(0.75f, 0.5f, 0.5f, 1), 0);

	if (uiState->activeItem == id || uiState->hotItem == id)
	{
		renderer_staticRect(renderer, rect.min, rectSize, V2(0, 0), 0,
		                    NULL, V4(0.75f, 0.75f, 0.0f, 1), 0);
	}
	else
	{
		renderer_staticRect(renderer, rect.min, rectSize, V2(0, 0), 0,
		                    NULL, V4(0.5f, 0.5f, 0.5f, 1), 0);
	}

	v2 strPos = rect.min;
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

	if (!common_isSet(input.keys[keycode_mouseLeft].flags,
	                  keystateflag_ended_down) &&
	    uiState->hotItem == id && uiState->activeItem == id)
	{
		uiState->kbdItem = id;
	}

	uiState->lastWidget = id;

	if (changed) return id;
	return 0;
}
