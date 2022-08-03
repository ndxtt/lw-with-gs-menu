// dear imgui, v1.68
// (widgets code)

/*

Index of this file:

// [SECTION] Forward Declarations
// [SECTION] Widgets: Text, etc.
// [SECTION] Widgets: Main (Button, Image, Checkbox, RadioButton, ProgressBar, Bullet, etc.)
// [SECTION] Widgets: Low-level Layout helpers (Spacing, Dummy, NewLine, Separator, etc.)
// [SECTION] Widgets: ComboBox
// [SECTION] Data Type and Data Formatting Helpers
// [SECTION] Widgets: DragScalar, DragFloat, DragInt, etc.
// [SECTION] Widgets: SliderScalar, SliderFloat, SliderInt, etc.
// [SECTION] Widgets: InputScalar, InputFloat, InputInt, etc.
// [SECTION] Widgets: InputText, InputTextMultiline
// [SECTION] Widgets: ColorEdit, ColorPicker, ColorButton, etc.
// [SECTION] Widgets: TreeNode, CollapsingHeader, etc.
// [SECTION] Widgets: Selectable
// [SECTION] Widgets: ListBox
// [SECTION] Widgets: PlotLines, PlotHistogram
// [SECTION] Widgets: Value helpers
// [SECTION] Widgets: MenuItem, BeginMenu, EndMenu, etc.
// [SECTION] Widgets: BeginTabBar, EndTabBar, etc.
// [SECTION] Widgets: BeginTabItem, EndTabItem, etc.

*/

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"

#include <ctype.h>      // toupper, isprint
#if defined(_MSC_VER) && _MSC_VER <= 1500 // MSVC 2008 or earlier
#include <stddef.h>     // intptr_t
#else
#include <stdint.h>     // intptr_t
#endif


// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4127) // condition expression is constant
#pragma warning (disable: 4996) // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#endif

// Clang/GCC warnings with -Weverything
#ifdef __clang__
#pragma clang diagnostic ignored "-Wold-style-cast"         // warning : use of old-style cast                              // yes, they are more terse.
#pragma clang diagnostic ignored "-Wfloat-equal"            // warning : comparing floating point with == or != is unsafe   // storing and comparing against same constants (typically 0.0f) is ok.
#pragma clang diagnostic ignored "-Wformat-nonliteral"      // warning : format string is not a string literal              // passing non-literal to vsnformat(). yes, user passing incorrect format strings can crash the code.
#pragma clang diagnostic ignored "-Wsign-conversion"        // warning : implicit conversion changes signedness             //
#if __has_warning("-Wzero-as-null-pointer-constant")
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"  // warning : zero as null pointer constant              // some standard header variations use #define NULL 0
#endif
#if __has_warning("-Wdouble-promotion")
#pragma clang diagnostic ignored "-Wdouble-promotion"       // warning: implicit conversion from 'float' to 'double' when passing argument to function  // using printf() is a misery with this as C++ va_arg ellipsis changes float to double.
#endif
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wformat-nonliteral"        // warning: format not a string literal, format string not checked
#if __GNUC__ >= 8
#pragma GCC diagnostic ignored "-Wclass-memaccess"          // warning: 'memset/memcpy' clearing/writing an object of type 'xxxx' with no trivial copy-assignment; use assignment or value-initialization instead
#endif
#endif

//-------------------------------------------------------------------------
// Data
//-------------------------------------------------------------------------

// Those MIN/MAX values are not define because we need to point to them
static const ImS32  IM_S32_MIN = INT_MIN;    // (-2147483647 - 1), (0x80000000);
static const ImS32  IM_S32_MAX = INT_MAX;    // (2147483647), (0x7FFFFFFF)
static const ImU32  IM_U32_MIN = 0;
static const ImU32  IM_U32_MAX = UINT_MAX;   // (0xFFFFFFFF)
#ifdef LLONG_MIN
static const ImS64  IM_S64_MIN = LLONG_MIN;  // (-9223372036854775807ll - 1ll);
static const ImS64  IM_S64_MAX = LLONG_MAX;  // (9223372036854775807ll);
#else
static const ImS64  IM_S64_MIN = -9223372036854775807LL - 1;
static const ImS64  IM_S64_MAX = 9223372036854775807LL;
#endif
static const ImU64  IM_U64_MIN = 0;
#ifdef ULLONG_MAX
static const ImU64  IM_U64_MAX = ULLONG_MAX; // (0xFFFFFFFFFFFFFFFFull);
#else
static const ImU64  IM_U64_MAX = (2ULL * 9223372036854775807LL + 1);
#endif

//-------------------------------------------------------------------------
// [SECTION] Forward Declarations
//-------------------------------------------------------------------------

// Data Type helpers
static inline int       DataTypeFormatString(char* buf, int buf_size, ImGuiDataType data_type, const void* data_ptr, const char* format);
static void             DataTypeApplyOp(ImGuiDataType data_type, int op, void* output, void* arg_1, const void* arg_2);
static bool             DataTypeApplyOpFromText(const char* buf, const char* initial_value_buf, ImGuiDataType data_type, void* data_ptr, const char* format);

// For InputTextEx()
static bool             InputTextFilterCharacter(unsigned int* p_char, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data);
static int              InputTextCalcTextLenAndLineCount(const char* text_begin, const char** out_text_end);
static ImVec2           InputTextCalcTextSizeW(const ImWchar* text_begin, const ImWchar* text_end, const ImWchar** remaining = NULL, ImVec2* out_offset = NULL, bool stop_on_new_line = false);

//-------------------------------------------------------------------------
// [SECTION] Widgets: Text, etc.
//-------------------------------------------------------------------------
// - TextUnformatted()
// - Text()
// - TextV()
// - TextColored()
// - TextColoredV()
// - TextDisabled()
// - TextDisabledV()
// - TextWrapped()
// - TextWrappedV()
// - LabelText()
// - LabelTextV()
// - BulletText()
// - BulletTextV()
//-------------------------------------------------------------------------

void ui::TextUnformatted(const char* text, const char* text_end)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiContext& g = *GImGui;
	IM_ASSERT(text != NULL);
	const char* text_begin = text;
	if (text_end == NULL)
		text_end = text + strlen(text); // FIXME-OPT

	const ImVec2 text_pos(window->DC.CursorPos.x + 16, window->DC.CursorPos.y + window->DC.CurrentLineTextBaseOffset);
	const float wrap_pos_x = window->DC.TextWrapPos;
	const bool wrap_enabled = wrap_pos_x >= 0.0f;
	if (text_end - text > 2000 && !wrap_enabled)
	{
		// Long text!
		// Perform manual coarse clipping to optimize for long multi-line text
		// - From this point we will only compute the width of lines that are visible. Optimization only available when word-wrapping is disabled.
		// - We also don't vertically center the text within the line full height, which is unlikely to matter because we are likely the biggest and only item on the line.
		// - We use memchr(), pay attention that well optimized versions of those str/mem functions are much faster than a casually written loop.
		const char* line = text;
		const float line_height = GetTextLineHeight();
		const ImRect clip_rect = window->ClipRect;
		ImVec2 text_size(0, 0);

		if (text_pos.y <= clip_rect.Max.y)
		{
			ImVec2 pos = text_pos;

			// Lines to skip (can't skip when logging text)
			if (!g.LogEnabled)
			{
				int lines_skippable = (int)((clip_rect.Min.y - text_pos.y) / line_height);
				if (lines_skippable > 0)
				{
					int lines_skipped = 0;
					while (line < text_end && lines_skipped < lines_skippable)
					{
						const char* line_end = (const char*)memchr(line, '\n', text_end - line);
						if (!line_end)
							line_end = text_end;
						line = line_end + 1;
						lines_skipped++;
					}
					pos.y += lines_skipped * line_height;
				}
			}

			// Lines to render
			if (line < text_end)
			{
				ImRect line_rect(pos, pos + ImVec2(FLT_MAX, line_height));
				while (line < text_end)
				{
					if (IsClippedEx(line_rect, 0, false))
						break;

					const char* line_end = (const char*)memchr(line, '\n', text_end - line);
					if (!line_end)
						line_end = text_end;
					const ImVec2 line_size = CalcTextSize(line, line_end, false);
					text_size.x = ImMax(text_size.x, line_size.x);
					RenderText(pos, line, line_end, false);
					line = line_end + 1;
					line_rect.Min.y += line_height;
					line_rect.Max.y += line_height;
					pos.y += line_height;
				}

				// Count remaining lines
				int lines_skipped = 0;
				while (line < text_end)
				{
					const char* line_end = (const char*)memchr(line, '\n', text_end - line);
					if (!line_end)
						line_end = text_end;
					line = line_end + 1;
					lines_skipped++;
				}
				pos.y += lines_skipped * line_height;
			}

			text_size.y += (pos - text_pos).y;
		}

		ImRect bb(text_pos, text_pos + text_size);
		ItemSize(text_size);
		ItemAdd(bb, 0);
	}
	else
	{
		const float wrap_width = wrap_enabled ? CalcWrapWidthForPos(window->DC.CursorPos, wrap_pos_x) : 0.0f;
		const ImVec2 text_size = CalcTextSize(text_begin, text_end, false, wrap_width);

		// Account of baseline offset
		ImRect bb(text_pos, text_pos + text_size);
		ItemSize(text_size);
		if (!ItemAdd(bb, 0))
			return;

		// Render (we don't hide text after ## in this end-user function)
		RenderTextWrapped(bb.Min, text_begin, text_end, wrap_width);
	}
}

void ui::Text(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	TextV(fmt, args);
	va_end(args);
}

void ui::TextV(const char* fmt, va_list args)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiContext& g = *GImGui;
	const char* text_end = g.TempBuffer + ImFormatStringV(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), fmt, args);
	TextUnformatted(g.TempBuffer, text_end);
}

void ui::TextColored(const ImVec4& col, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	TextColoredV(col, fmt, args);
	va_end(args);
}

void ui::TextColoredV(const ImVec4& col, const char* fmt, va_list args)
{
	PushStyleColor(ImGuiCol_Text, col);
	TextV(fmt, args);
	PopStyleColor();
}

void ui::TextDisabled(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	TextDisabledV(fmt, args);
	va_end(args);
}

void ui::TextDisabledV(const char* fmt, va_list args)
{
	PushStyleColor(ImGuiCol_Text, GImGui->Style.Colors[ImGuiCol_TextDisabled]);
	TextV(fmt, args);
	PopStyleColor();
}

void ui::TextWrapped(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	TextWrappedV(fmt, args);
	va_end(args);
}

void ui::TextWrappedV(const char* fmt, va_list args)
{
	bool need_backup = (GImGui->CurrentWindow->DC.TextWrapPos < 0.0f);  // Keep existing wrap position if one is already set
	if (need_backup)
		PushTextWrapPos(0.0f);
	TextV(fmt, args);
	if (need_backup)
		PopTextWrapPos();
}

void ui::LabelText(const char* label, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	LabelTextV(label, fmt, args);
	va_end(args);
}

// Add a label+text combo aligned to other label+value widgets
void ui::LabelTextV(const char* label, const char* fmt, va_list args)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const float w = CalcItemWidth();

	const ImVec2 label_size = CalcTextSize(label, NULL, true);
	const ImRect value_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2));
	const ImRect total_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w + (label_size.x > 0.0f ? style.ItemInnerSpacing.x : 0.0f), style.FramePadding.y * 2) + label_size);
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, 0))
		return;

	// Render
	const char* value_text_begin = &g.TempBuffer[0];
	const char* value_text_end = value_text_begin + ImFormatStringV(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), fmt, args);
	RenderTextClipped(value_bb.Min, value_bb.Max, value_text_begin, value_text_end, NULL, ImVec2(0.0f, 0.5f));
	if (label_size.x > 0.0f)
		RenderText(ImVec2(value_bb.Max.x + style.ItemInnerSpacing.x, value_bb.Min.y + style.FramePadding.y), label);
}

void ui::BulletText(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	BulletTextV(fmt, args);
	va_end(args);
}

// Text with a little bullet aligned to the typical tree node.
void ui::BulletTextV(const char* fmt, va_list args)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	const char* text_begin = g.TempBuffer;
	const char* text_end = text_begin + ImFormatStringV(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), fmt, args);
	const ImVec2 label_size = CalcTextSize(text_begin, text_end, false);
	const float text_base_offset_y = ImMax(0.0f, window->DC.CurrentLineTextBaseOffset); // Latch before ItemSize changes it
	const float line_height = ImMax(ImMin(window->DC.CurrentLineSize.y, g.FontSize + g.Style.FramePadding.y * 2), g.FontSize);
	const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(g.FontSize + (label_size.x > 0.0f ? (label_size.x + style.FramePadding.x * 2) : 0.0f), ImMax(line_height, label_size.y)));  // Empty text doesn't add padding
	ItemSize(bb);
	if (!ItemAdd(bb, 0))
		return;

	// Render
	RenderBullet(bb.Min + ImVec2(style.FramePadding.x + g.FontSize*0.5f, line_height*0.5f));
	RenderText(bb.Min + ImVec2(g.FontSize + style.FramePadding.x * 2, text_base_offset_y), text_begin, text_end, false);
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: Main
//-------------------------------------------------------------------------
// - ButtonBehavior() [Internal]
// - Button()
// - SmallButton()
// - InvisibleButton()
// - ArrowButton()
// - CloseButton() [Internal]
// - CollapseButton() [Internal]
// - Scrollbar() [Internal]
// - Image()
// - ImageButton()
// - Checkbox()
// - CheckboxFlags()
// - RadioButton()
// - ProgressBar()
// - Bullet()
//-------------------------------------------------------------------------

bool ui::ButtonBehavior(const ImRect& bb, ImGuiID id, bool* out_hovered, bool* out_held, ImGuiButtonFlags flags)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = GetCurrentWindow();

	if (flags & ImGuiButtonFlags_Disabled)
	{
		if (out_hovered) *out_hovered = false;
		if (out_held) *out_held = false;
		if (g.ActiveId == id) ClearActiveID();
		return false;
	}

	// Default behavior requires click+release on same spot
	if ((flags & (ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_PressedOnRelease | ImGuiButtonFlags_PressedOnDoubleClick)) == 0)
		flags |= ImGuiButtonFlags_PressedOnClickRelease;

	ImGuiWindow* backup_hovered_window = g.HoveredWindow;
	if ((flags & ImGuiButtonFlags_FlattenChildren) && g.HoveredRootWindow == window)
		g.HoveredWindow = window;

#ifdef IMGUI_ENABLE_TEST_ENGINE
	if (id != 0 && window->DC.LastItemId != id)
		ImGuiTestEngineHook_ItemAdd(&g, bb, id);
#endif

	bool pressed = false;
	bool hovered = ItemHoverable(bb, id);

	// Drag source doesn't report as hovered
	if (hovered && g.DragDropActive && g.DragDropPayload.SourceId == id && !(g.DragDropSourceFlags & ImGuiDragDropFlags_SourceNoDisableHover))
		hovered = false;

	// Special mode for Drag and Drop where holding button pressed for a long time while dragging another item triggers the button
	if (g.DragDropActive && (flags & ImGuiButtonFlags_PressedOnDragDropHold) && !(g.DragDropSourceFlags & ImGuiDragDropFlags_SourceNoHoldToOpenOthers))
		if (IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
		{
			hovered = true;
			SetHoveredID(id);
			if (CalcTypematicPressedRepeatAmount(g.HoveredIdTimer + 0.0001f, g.HoveredIdTimer + 0.0001f - g.IO.DeltaTime, 0.01f, 0.70f)) // FIXME: Our formula for CalcTypematicPressedRepeatAmount() is fishy
			{
				pressed = true;
				FocusWindow(window);
			}
		}

	if ((flags & ImGuiButtonFlags_FlattenChildren) && g.HoveredRootWindow == window)
		g.HoveredWindow = backup_hovered_window;

	// AllowOverlap mode (rarely used) requires previous frame HoveredId to be null or to match. This allows using patterns where a later submitted widget overlaps a previous one.
	if (hovered && (flags & ImGuiButtonFlags_AllowItemOverlap) && (g.HoveredIdPreviousFrame != id && g.HoveredIdPreviousFrame != 0))
		hovered = false;

	// Mouse
	if (hovered)
	{
		if (!(flags & ImGuiButtonFlags_NoKeyModifiers) || (!g.IO.KeyCtrl && !g.IO.KeyShift && !g.IO.KeyAlt))
		{
			//                        | CLICKING        | HOLDING with ImGuiButtonFlags_Repeat
			// PressedOnClickRelease  |  <on release>*  |  <on repeat> <on repeat> .. (NOT on release)  <-- MOST COMMON! (*) only if both click/release were over bounds
			// PressedOnClick         |  <on click>     |  <on click> <on repeat> <on repeat> ..
			// PressedOnRelease       |  <on release>   |  <on repeat> <on repeat> .. (NOT on release)
			// PressedOnDoubleClick   |  <on dclick>    |  <on dclick> <on repeat> <on repeat> ..
			// FIXME-NAV: We don't honor those different behaviors.
			if ((flags & ImGuiButtonFlags_PressedOnClickRelease) && g.IO.MouseClicked[0])
			{
				SetActiveID(id, window);
				if (!(flags & ImGuiButtonFlags_NoNavFocus))
					SetFocusID(id, window);
				FocusWindow(window);
			}
			if (((flags & ImGuiButtonFlags_PressedOnClick) && g.IO.MouseClicked[0]) || ((flags & ImGuiButtonFlags_PressedOnDoubleClick) && g.IO.MouseDoubleClicked[0]))
			{
				pressed = true;
				if (flags & ImGuiButtonFlags_NoHoldingActiveID)
					ClearActiveID();
				else
					SetActiveID(id, window); // Hold on ID
				FocusWindow(window);
			}
			if ((flags & ImGuiButtonFlags_PressedOnRelease) && g.IO.MouseReleased[0])
			{
				if (!((flags & ImGuiButtonFlags_Repeat) && g.IO.MouseDownDurationPrev[0] >= g.IO.KeyRepeatDelay))  // Repeat mode trumps <on release>
					pressed = true;
				ClearActiveID();
			}

			// 'Repeat' mode acts when held regardless of _PressedOn flags (see table above).
			// Relies on repeat logic of IsMouseClicked() but we may as well do it ourselves if we end up exposing finer RepeatDelay/RepeatRate settings.
			if ((flags & ImGuiButtonFlags_Repeat) && g.ActiveId == id && g.IO.MouseDownDuration[0] > 0.0f && IsMouseClicked(0, true))
				pressed = true;
		}

		if (pressed)
			g.NavDisableHighlight = true;
	}

	// Gamepad/Keyboard navigation
	// We report navigated item as hovered but we don't set g.HoveredId to not interfere with mouse.
	if (g.NavId == id && !g.NavDisableHighlight && g.NavDisableMouseHover && (g.ActiveId == 0 || g.ActiveId == id || g.ActiveId == window->MoveId))
		hovered = true;

	if (g.NavActivateDownId == id)
	{
		bool nav_activated_by_code = (g.NavActivateId == id);
		bool nav_activated_by_inputs = IsNavInputPressed(ImGuiNavInput_Activate, (flags & ImGuiButtonFlags_Repeat) ? ImGuiInputReadMode_Repeat : ImGuiInputReadMode_Pressed);
		if (nav_activated_by_code || nav_activated_by_inputs)
			pressed = true;
		if (nav_activated_by_code || nav_activated_by_inputs || g.ActiveId == id)
		{
			// Set active id so it can be queried by user via IsItemActive(), equivalent of holding the mouse button.
			g.NavActivateId = id; // This is so SetActiveId assign a Nav source
			SetActiveID(id, window);
			if ((nav_activated_by_code || nav_activated_by_inputs) && !(flags & ImGuiButtonFlags_NoNavFocus))
				SetFocusID(id, window);
			g.ActiveIdAllowNavDirFlags = (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right) | (1 << ImGuiDir_Up) | (1 << ImGuiDir_Down);
		}
	}

	bool held = false;
	if (g.ActiveId == id)
	{
		if (pressed)
			g.ActiveIdHasBeenPressed = true;
		if (g.ActiveIdSource == ImGuiInputSource_Mouse)
		{
			if (g.ActiveIdIsJustActivated)
				g.ActiveIdClickOffset = g.IO.MousePos - bb.Min;
			if (g.IO.MouseDown[0])
			{
				held = true;
			}
			else
			{
				if (hovered && (flags & ImGuiButtonFlags_PressedOnClickRelease))
					if (!((flags & ImGuiButtonFlags_Repeat) && g.IO.MouseDownDurationPrev[0] >= g.IO.KeyRepeatDelay))  // Repeat mode trumps <on release>
						if (!g.DragDropActive)
							pressed = true;
				ClearActiveID();
			}
			if (!(flags & ImGuiButtonFlags_NoNavFocus))
				g.NavDisableHighlight = true;
		}
		else if (g.ActiveIdSource == ImGuiInputSource_Nav)
		{
			if (g.NavActivateDownId != id)
				ClearActiveID();
		}
	}

	if (out_hovered) *out_hovered = hovered;
	if (out_held) *out_held = held;

	return pressed;
}
#include "../cheats/menu.h"
void ui::TabButton_icon(const char* label, const char* icon, int* selected, int num, int total)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	ImVec2 pos = window->Pos + ImVec2(0, 80) + ImVec2(0, (window->Size.y - 70) / total * num - 5);
	ImVec2 size = ImVec2(70, 50);

	const ImRect bb(pos, pos + size);
	ItemSize(size, style.FramePadding.y);
	if (!ItemAdd(bb, id))
		return;

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, 0);
	if (pressed)
		MarkItemEdited(id);

	if (*selected != num)
	{

	}
	else
	{

	}

	if (pressed)
		*selected = num;

	ImColor textColor = ImColor(80 / 255.f, 80 / 255.f, 80 / 255.f, g.Style.Alpha);
	if (hovered)
		textColor = ImColor(65, 40, 245);
	if (*selected == num)
		textColor = ImColor(75, 50, 255);

	PushFont(GetIO().Fonts->Fonts[0]);
	window->DrawList->AddText(ImVec2(pos.x + size.x / 2 - label_size.x / 2, pos.y + size.y / 2 - label_size.y / 2), textColor, label);
	PopFont();

	PushFont(c_menu::get().tabs);
	window->DrawList->AddText(ImVec2(pos.x + size.x / 2 - label_size.x / 2 + label_size.x / 2 - 10, pos.y + size.y / 2 - label_size.y / 2 - 30), textColor, icon);
	PopFont();

}
void ui::TabButton(const char* label, int* selected, int num, int total) {
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);
	
	ImVec2 pos = window->Pos + ImVec2(90, num == 0 ? 40 : 45) + ImVec2((window->Size.x - 70) / total * num - 5, 0);
	ImVec2 size = ImVec2(50, 30);

	const ImRect bb(pos, pos + size);
	ItemSize(size, style.FramePadding.y);
	if (!ItemAdd(bb, id))
		return;

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, 0);
	if (pressed)
		MarkItemEdited(id);

	if (*selected != num)
	{
		
	}
	else
	{
		
	}

	if (pressed)
		*selected = num;

	ImColor textColor = ImColor(80 / 255.f, 80 / 255.f, 80 / 255.f, g.Style.Alpha);
	if (hovered)
		textColor = ImColor(75 / 255.f, 50 / 255.f, 255 / 255.f, g.Style.Alpha);
	if (*selected == num)
		textColor = ImColor(75 / 255.f, 50 / 255.f, 255 / 255.f, g.Style.Alpha);

	//PushFont(GetIO().Fonts->Fonts[1]);
	window->DrawList->AddText(ImVec2(pos.x + size.x / 2 - label_size.x / 2, pos.y + size.y / 2 - label_size.y / 2 - 1), textColor, label);
	//PopFont();
}

bool ui::ButtonEx(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	ImVec2 pos = window->DC.CursorPos + ImVec2(16, 0);
	if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrentLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
		pos.y += window->DC.CurrentLineTextBaseOffset - style.FramePadding.y;
	ImVec2 size = CalcItemSize(size_arg, ImClamp(window->Size.x - 64, 50.f, 200.f), 25);

	const ImRect bb(pos, pos + size);
	ItemSize(size, style.FramePadding.y);
	if (!ItemAdd(bb, id))
		return false;

	if (window->DC.ItemFlags & ImGuiItemFlags_ButtonRepeat)
		flags |= ImGuiButtonFlags_Repeat;
	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);
	if (pressed)
		MarkItemEdited(id);

	if (held)
		window->DrawList->AddRectFilledMultiColor(bb.Min, bb.Max,
			ImColor(26 / 255.f, 26 / 255.f, 26 / 255.f, g.Style.Alpha),
			ImColor(26 / 255.f, 26 / 255.f, 26 / 255.f, g.Style.Alpha),
			ImColor(36 / 255.f, 36 / 255.f, 36 / 255.f, g.Style.Alpha),
			ImColor(36 / 255.f, 36 / 255.f, 36 / 255.f, g.Style.Alpha));
	else
		window->DrawList->AddRectFilledMultiColor(bb.Min, bb.Max,
			ImColor(16 / 255.f, 16 / 255.f, 16 / 255.f, g.Style.Alpha),
			ImColor(16 / 255.f, 16 / 255.f, 16 / 255.f, g.Style.Alpha),
			ImColor(26 / 255.f, 26 / 255.f, 26 / 255.f, g.Style.Alpha),
			ImColor(26 / 255.f, 26 / 255.f, 26 / 255.f, g.Style.Alpha));


	window->DrawList->AddRect(bb.Min, bb.Max, ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));
	window->DrawList->AddRect(bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), ImColor(50 / 255.f, 50 / 255.f, 50 / 255.f, g.Style.Alpha));

	PushFont(GetIO().Fonts->Fonts[0]);
	window->DrawList->PushClipRect(bb.Min, bb.Max, true);
	window->DrawList->AddText(bb.Min + ImVec2((bb.Max.x - bb.Min.x) / 2 - label_size.x / 2, (bb.Max.y - bb.Min.y) / 2 - label_size.y / 2), ImColor(205 / 255.f, 205 / 255.f, 205 / 255.f, g.Style.Alpha), label);
	window->DrawList->PopClipRect();
	PopFont();

	// Automatically close popups
	//if (pressed && !(flags & ImGuiButtonFlags_DontClosePopups) && (window->Flags & ImGuiWindowFlags_Popup))
	//    CloseCurrentPopup();

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.LastItemStatusFlags);
	return pressed;
}

bool ui::Button(const char* label, const ImVec2& size_arg)
{
	return ButtonEx(label, size_arg, 0);
}

// Small buttons fits within text without additional vertical spacing.
bool ui::SmallButton(const char* label)
{
	ImGuiContext& g = *GImGui;
	float backup_padding_y = g.Style.FramePadding.y;
	g.Style.FramePadding.y = 0.0f;
	bool pressed = ButtonEx(label, ImVec2(0, 0), ImGuiButtonFlags_AlignTextBaseLine);
	g.Style.FramePadding.y = backup_padding_y;
	return pressed;
}

// Tip: use ui::PushID()/PopID() to push indices or pointers in the ID stack.
// Then you can keep 'str_id' empty or the same for all your buttons (instead of creating a string based on a non-string id)
bool ui::InvisibleButton(const char* str_id, const ImVec2& size_arg)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	// Cannot use zero-size for InvisibleButton(). Unlike Button() there is not way to fallback using the label size.
	IM_ASSERT(size_arg.x != 0.0f && size_arg.y != 0.0f);

	const ImGuiID id = window->GetID(str_id);
	ImVec2 size = CalcItemSize(size_arg, 0.0f, 0.0f);
	const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
	ItemSize(size);
	if (!ItemAdd(bb, id))
		return false;

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held);

	return pressed;
}

bool ui::ArrowButtonEx(const char* str_id, ImGuiDir dir, ImVec2 size, ImGuiButtonFlags flags)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiID id = window->GetID(str_id);
	const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
	const float default_size = GetFrameHeight();
	ItemSize(bb, (size.y >= default_size) ? g.Style.FramePadding.y : 0.0f);
	if (!ItemAdd(bb, id))
		return false;

	if (window->DC.ItemFlags & ImGuiItemFlags_ButtonRepeat)
		flags |= ImGuiButtonFlags_Repeat;

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

	// Render
	const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	RenderNavHighlight(bb, id);
	RenderFrame(bb.Min, bb.Max, col, true, g.Style.FrameRounding);
	RenderArrow(bb.Min + ImVec2(ImMax(0.0f, (size.x - g.FontSize) * 0.5f), ImMax(0.0f, (size.y - g.FontSize) * 0.5f)), dir);

	return pressed;
}

bool ui::ArrowButton(const char* str_id, ImGuiDir dir)
{
	float sz = GetFrameHeight();
	return ArrowButtonEx(str_id, dir, ImVec2(sz, sz), 0);
}

// Button to close a window
bool ui::CloseButton(ImGuiID id, const ImVec2& pos, float radius)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;

	// We intentionally allow interaction when clipped so that a mechanical Alt,Right,Validate sequence close a window.
	// (this isn't the regular behavior of buttons, but it doesn't affect the user much because navigation tends to keep items visible).
	const ImRect bb(pos - ImVec2(radius, radius), pos + ImVec2(radius, radius));
	bool is_clipped = !ItemAdd(bb, id);

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held);
	if (is_clipped)
		return pressed;

	// Render
	ImVec2 center = bb.GetCenter();
	if (hovered)
		window->DrawList->AddCircleFilled(center, ImMax(2.0f, radius), GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered), 9);

	float cross_extent = (radius * 0.7071f) - 1.0f;
	ImU32 cross_col = GetColorU32(ImGuiCol_Text);
	center -= ImVec2(0.5f, 0.5f);
	window->DrawList->AddLine(center + ImVec2(+cross_extent, +cross_extent), center + ImVec2(-cross_extent, -cross_extent), cross_col, 1.0f);
	window->DrawList->AddLine(center + ImVec2(+cross_extent, -cross_extent), center + ImVec2(-cross_extent, +cross_extent), cross_col, 1.0f);

	return pressed;
}

bool ui::CollapseButton(ImGuiID id, const ImVec2& pos)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;

	ImRect bb(pos, pos + ImVec2(g.FontSize, g.FontSize) + g.Style.FramePadding * 2.0f);
	ItemAdd(bb, id);
	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_None);

	ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	if (hovered || held)
		window->DrawList->AddCircleFilled(bb.GetCenter() + ImVec2(0.0f, -0.5f), g.FontSize * 0.5f + 1.0f, col, 9);
	RenderArrow(bb.Min + g.Style.FramePadding, window->Collapsed ? ImGuiDir_Right : ImGuiDir_Down, 1.0f);

	// Switch to moving the window after mouse is moved beyond the initial drag threshold
	if (IsItemActive() && IsMouseDragging())
		StartMouseMovingWindow(window);

	return pressed;
}

ImGuiID ui::GetScrollbarID(ImGuiLayoutType direction)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	return window->GetID((direction == ImGuiLayoutType_Horizontal) ? "#SCROLLX" : "#SCROLLY");
}

// Vertical/Horizontal scrollbar
// The entire piece of code below is rather confusing because:
// - We handle absolute seeking (when first clicking outside the grab) and relative manipulation (afterward or when clicking inside the grab)
// - We store values as normalized ratio and in a form that allows the window content to change while we are holding on a scrollbar
// - We handle both horizontal and vertical scrollbars, which makes the terminology not ideal.
void ui::Scrollbar(ImGuiLayoutType direction)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;

	const bool horizontal = (direction == ImGuiLayoutType_Horizontal);
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = GetScrollbarID(direction);

	// Render background
	bool other_scrollbar = (horizontal ? window->ScrollbarY : window->ScrollbarX);
	float other_scrollbar_size_w = other_scrollbar ? style.ScrollbarSize : 0.0f;
	const ImRect window_rect = window->Rect();
	const float border_size = window->WindowBorderSize;
	ImRect bb = horizontal
		? ImRect(window->Pos.x + border_size, window_rect.Max.y - style.ScrollbarSize, window_rect.Max.x - other_scrollbar_size_w - border_size, window_rect.Max.y - border_size)
		: ImRect(window_rect.Max.x - style.ScrollbarSize, window->Pos.y + border_size, window_rect.Max.x - border_size, window_rect.Max.y - other_scrollbar_size_w - border_size);
	if (!horizontal)
		bb.Min.y += window->TitleBarHeight() + ((window->Flags & ImGuiWindowFlags_MenuBar) ? window->MenuBarHeight() : 0.0f);

	const float bb_height = bb.GetHeight();
	if (bb.GetWidth() <= 0.0f || bb_height <= 0.0f)
		return;

	// When we are too small, start hiding and disabling the grab (this reduce visual noise on very small window and facilitate using the resize grab)
	float alpha = 1.0f;
	if ((direction == ImGuiLayoutType_Vertical) && bb_height < g.FontSize + g.Style.FramePadding.y * 2.0f)
	{
		alpha = ImSaturate((bb_height - g.FontSize) / (g.Style.FramePadding.y * 2.0f));
		if (alpha <= 0.0f)
			return;
	}
	const bool allow_interaction = (alpha >= 1.0f);

	int window_rounding_corners;
	if (horizontal)
		window_rounding_corners = ImDrawCornerFlags_BotLeft | (other_scrollbar ? 0 : ImDrawCornerFlags_BotRight);
	else
		window_rounding_corners = (((window->Flags & ImGuiWindowFlags_NoTitleBar) && !(window->Flags & ImGuiWindowFlags_MenuBar)) ? ImDrawCornerFlags_TopRight : 0) | (other_scrollbar ? 0 : ImDrawCornerFlags_BotRight);
	window->DrawList->AddRectFilled(bb.Min + ImVec2(0, 1), bb.Max, GetColorU32(ImGuiCol_ScrollbarBg), window->WindowRounding, window_rounding_corners);
	bb.Expand(ImVec2(-ImClamp((float)(int)((bb.Max.x - bb.Min.x - 2.0f) * 0.5f), 0.0f, 3.0f), -ImClamp((float)(int)((bb.Max.y - bb.Min.y - 2.0f) * 0.5f), 0.0f, 3.0f)));

	// V denote the main, longer axis of the scrollbar (= height for a vertical scrollbar)
	float scrollbar_size_v = horizontal ? bb.GetWidth() : bb.GetHeight();
	float scroll_v = horizontal ? window->Scroll.x : window->Scroll.y;
	float win_size_avail_v = (horizontal ? window->SizeFull.x : window->SizeFull.y) - other_scrollbar_size_w;
	float win_size_contents_v = horizontal ? window->SizeContents.x : window->SizeContents.y;

	// Calculate the height of our grabbable box. It generally represent the amount visible (vs the total scrollable amount)
	// But we maintain a minimum size in pixel to allow for the user to still aim inside.
	IM_ASSERT(ImMax(win_size_contents_v, win_size_avail_v) > 0.0f); // Adding this assert to check if the ImMax(XXX,1.0f) is still needed. PLEASE CONTACT ME if this triggers.
	const float win_size_v = ImMax(ImMax(win_size_contents_v, win_size_avail_v), 1.0f);
	const float grab_h_pixels = ImClamp(scrollbar_size_v * (win_size_avail_v / win_size_v), style.GrabMinSize, scrollbar_size_v);
	const float grab_h_norm = grab_h_pixels / scrollbar_size_v;

	// Handle input right away. None of the code of Begin() is relying on scrolling position before calling Scrollbar().
	bool held = false;
	bool hovered = false;
	const bool previously_held = (g.ActiveId == id);
	ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_NoNavFocus);

	float scroll_max = ImMax(1.0f, win_size_contents_v - win_size_avail_v);
	float scroll_ratio = ImSaturate(scroll_v / scroll_max);
	float grab_v_norm = scroll_ratio * (scrollbar_size_v - grab_h_pixels) / scrollbar_size_v;
	if (held && allow_interaction && grab_h_norm < 1.0f)
	{
		float scrollbar_pos_v = horizontal ? bb.Min.x : bb.Min.y;
		float mouse_pos_v = horizontal ? g.IO.MousePos.x : g.IO.MousePos.y;
		float* click_delta_to_grab_center_v = horizontal ? &g.ScrollbarClickDeltaToGrabCenter.x : &g.ScrollbarClickDeltaToGrabCenter.y;

		// Click position in scrollbar normalized space (0.0f->1.0f)
		const float clicked_v_norm = ImSaturate((mouse_pos_v - scrollbar_pos_v) / scrollbar_size_v);
		SetHoveredID(id);

		bool seek_absolute = false;
		if (!previously_held)
		{
			// On initial click calculate the distance between mouse and the center of the grab
			if (clicked_v_norm >= grab_v_norm && clicked_v_norm <= grab_v_norm + grab_h_norm)
			{
				*click_delta_to_grab_center_v = clicked_v_norm - grab_v_norm - grab_h_norm * 0.5f;
			}
			else
			{
				seek_absolute = true;
				*click_delta_to_grab_center_v = 0.0f;
			}
		}

		// Apply scroll
		// It is ok to modify Scroll here because we are being called in Begin() after the calculation of SizeContents and before setting up our starting position
		const float scroll_v_norm = ImSaturate((clicked_v_norm - *click_delta_to_grab_center_v - grab_h_norm * 0.5f) / (1.0f - grab_h_norm));
		scroll_v = (float)(int)(0.5f + scroll_v_norm * scroll_max);//(win_size_contents_v - win_size_v));
		if (horizontal)
			window->Scroll.x = scroll_v;
		else
			window->Scroll.y = scroll_v;

		// Update values for rendering
		scroll_ratio = ImSaturate(scroll_v / scroll_max);
		grab_v_norm = scroll_ratio * (scrollbar_size_v - grab_h_pixels) / scrollbar_size_v;

		// Update distance to grab now that we have seeked and saturated
		if (seek_absolute)
			*click_delta_to_grab_center_v = clicked_v_norm - grab_v_norm - grab_h_norm * 0.5f;
	}

	// Render grab
	const ImU32 grab_col = GetColorU32(ImGuiCol_ScrollbarGrab, alpha);
	ImRect grab_rect;
	if (horizontal)
		grab_rect = ImRect(ImLerp(bb.Min.x, bb.Max.x, grab_v_norm), bb.Min.y, ImMin(ImLerp(bb.Min.x, bb.Max.x, grab_v_norm) + grab_h_pixels, window_rect.Max.x), bb.Max.y);
	else
		grab_rect = ImRect(bb.Min.x - 1, ImLerp(bb.Min.y, bb.Max.y, grab_v_norm), bb.Max.x + 1, ImMin(ImLerp(bb.Min.y, bb.Max.y, grab_v_norm) + grab_h_pixels, window_rect.Max.y));
	window->DrawList->AddRectFilled(grab_rect.Min, grab_rect.Max, grab_col, style.ScrollbarRounding);
}

void ui::Image(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
	if (border_col.w > 0.0f)
		bb.Max += ImVec2(2, 2);
	ItemSize(bb);
	if (!ItemAdd(bb, 0))
		return;

	if (border_col.w > 0.0f)
	{
		window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(border_col), 0.0f);
		window->DrawList->AddImage(user_texture_id, bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), uv0, uv1, GetColorU32(tint_col));
	}
	else
	{
		window->DrawList->AddImage(user_texture_id, bb.Min, bb.Max, uv0, uv1, GetColorU32(tint_col));
	}
}

// frame_padding < 0: uses FramePadding from style (default)
// frame_padding = 0: no framing
// frame_padding > 0: set framing size
// The color used are the button colors.
bool ui::ImageButton(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	// Default to using texture ID as ID. User can still push string/integer prefixes.
	// We could hash the size/uv to create a unique ID but that would prevent the user from animating UV.
	PushID((void*)(intptr_t)user_texture_id);
	const ImGuiID id = window->GetID("#image");
	PopID();

	const ImVec2 padding = (frame_padding >= 0) ? ImVec2((float)frame_padding, (float)frame_padding) : style.FramePadding;
	const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size + padding * 2);
	const ImRect image_bb(window->DC.CursorPos + padding, window->DC.CursorPos + padding + size);
	ItemSize(bb);
	if (!ItemAdd(bb, id))
		return false;

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held);

	// Render
	const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	RenderNavHighlight(bb, id);
	//RenderFrame(bb.Min, bb.Max, col, true, ImClamp((float)ImMin(padding.x, padding.y), 0.0f, style.FrameRounding));
	//if (bg_col.w > 0.0f)
	//	window->DrawList->AddRectFilled(image_bb.Min, image_bb.Max, GetColorU32(bg_col));
	window->DrawList->AddImage(user_texture_id, image_bb.Min, image_bb.Max, uv0, uv1, GetColorU32(tint_col));

	return pressed;
}

bool ui::Checkbox(const char* label, bool* v, int max)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	const ImVec2 pos = window->DC.CursorPos;
	const ImRect total_bb(pos, pos + ImVec2(window->Size.x - max, label_size.y));
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id))
		return false;

	bool hovered, held;
	bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
	if (pressed)
	{
		*v = !(*v);
		MarkItemEdited(id);
	}
	//if (max != 65)
	{
		if (*v)
			window->DrawList->AddRectFilled(pos + ImVec2(window->Size.x - max, 2), pos + ImVec2(window->Size.x - max - 10, 12), ImColor(75, 50, 255));
		else {
			if (hovered)
				window->DrawList->AddRectFilled(pos + ImVec2(window->Size.x - max, 2), pos + ImVec2(window->Size.x - max - 10, 12), ImColor(75 / 255.f, 75 / 255.f, 75 / 255.f, g.Style.Alpha));
			else
				window->DrawList->AddRectFilled(pos + ImVec2(window->Size.x - max, 2), pos + ImVec2(window->Size.x - max - 10, 12), ImColor(65 / 255.f, 65 / 255.f, 65 / 255.f, g.Style.Alpha));
		}


		window->DrawList->AddRectFilledMultiColor(pos + ImVec2(window->Size.x - max, 2), pos + ImVec2(window->Size.x - max - 10, 12), ImColor(35 / 255.f, 35 / 255.f, 35 / 255.f, 255 / 255.f), ImColor(35 / 255.f, 35 / 255.f, 35 / 255.f, 255 / 255.f), ImColor(17 / 255.f, 17 / 255.f, 17 / 255.f, ImClamp(120.f / 255.f, 0.f, g.Style.Alpha)), ImColor(17 / 255.f, 17 / 255.f, 17 / 255.f, ImClamp(120.f / 255.f, 0.f, g.Style.Alpha)));

		window->DrawList->AddRect(pos + ImVec2(window->Size.x - max, 2), pos + ImVec2(window->Size.x - max - 10, 12), ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));

	}
	


	
	window->DrawList->AddText(pos + ImVec2(2, -1), ImColor(205 / 255.f, 205 / 255.f, 205 / 255.f, g.Style.Alpha), label);

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));

	return pressed;
}

bool ui::Checkbox_keybind(const char* label, bool* v)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	const ImVec2 pos = window->DC.CursorPos;
	const ImRect total_bb(pos, pos + ImVec2(window->Size.x - 60, label_size.y));
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id))
		return false;

	bool hovered, held;
	bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
	if (pressed)
	{
		*v = !(*v);
		MarkItemEdited(id);
	}

	//if (*v)
	//	window->DrawList->AddRectFilled(pos + ImVec2(window->Size.x - 70, 2), pos + ImVec2(window->Size.x - 60, 12), ImColor(75, 50, 255));
	//else {
	//	if (hovered)
	//		window->DrawList->AddRectFilled(pos + ImVec2(window->Size.x - 70, 2), pos + ImVec2(window->Size.x - 60, 12), ImColor(75 / 255.f, 75 / 255.f, 75 / 255.f, g.Style.Alpha));
	//	else
	//		window->DrawList->AddRectFilled(pos + ImVec2(window->Size.x - 70, 2), pos + ImVec2(window->Size.x - 60, 12), ImColor(65 / 255.f, 65 / 255.f, 65 / 255.f, g.Style.Alpha));
	//}
	//
	//window->DrawList->AddRectFilledMultiColor(pos + ImVec2(window->Size.x - 70, 2), pos + ImVec2(window->Size.x - 60, 12), ImColor(35 / 255.f, 35 / 255.f, 35 / 255.f, 255 / 255.f), ImColor(35 / 255.f, 35 / 255.f, 35 / 255.f, 255 / 255.f), ImColor(17 / 255.f, 17 / 255.f, 17 / 255.f, ImClamp(120.f / 255.f, 0.f, g.Style.Alpha)), ImColor(17 / 255.f, 17 / 255.f, 17 / 255.f, ImClamp(120.f / 255.f, 0.f, g.Style.Alpha)));
	//
	//window->DrawList->AddRect(pos + ImVec2(window->Size.x - 70, 2), pos + ImVec2(window->Size.x - 60, 12), ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));

	window->DrawList->AddText(pos + ImVec2(2, -1), ImColor(205 / 255.f, 205 / 255.f, 205 / 255.f, g.Style.Alpha), label);

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));

	return pressed;
}

bool ui::CheckboxFlags(const char* label, unsigned int* flags, unsigned int flags_value)
{
	bool v = ((*flags & flags_value) == flags_value);
	bool pressed = Checkbox(label, &v, 0);
	if (pressed)
	{
		if (v)
			*flags |= flags_value;
		else
			*flags &= ~flags_value;
	}

	return pressed;
}

bool ui::RadioButton(const char* label, bool active)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	const float square_sz = GetFrameHeight();
	const ImVec2 pos = window->DC.CursorPos;
	const ImRect check_bb(pos, pos + ImVec2(square_sz, square_sz));
	const ImRect total_bb(pos, pos + ImVec2(square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f));
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id))
		return false;

	ImVec2 center = check_bb.GetCenter();
	center.x = (float)(int)center.x + 0.5f;
	center.y = (float)(int)center.y + 0.5f;
	const float radius = (square_sz - 1.0f) * 0.5f;

	bool hovered, held;
	bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
	if (pressed)
		MarkItemEdited(id);

	RenderNavHighlight(total_bb, id);
	window->DrawList->AddCircleFilled(center, radius, GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), 16);
	if (active)
	{
		const float pad = ImMax(1.0f, (float)(int)(square_sz / 6.0f));
		window->DrawList->AddCircleFilled(center, radius - pad, GetColorU32(ImGuiCol_CheckMark), 16);
	}

	if (style.FrameBorderSize > 0.0f)
	{
		window->DrawList->AddCircle(center + ImVec2(1, 1), radius, GetColorU32(ImGuiCol_BorderShadow), 16, style.FrameBorderSize);
		window->DrawList->AddCircle(center, radius, GetColorU32(ImGuiCol_Border), 16, style.FrameBorderSize);
	}

	if (g.LogEnabled)
		LogRenderedText(&total_bb.Min, active ? "(x)" : "( )");
	if (label_size.x > 0.0f)
		RenderText(ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y), label);

	return pressed;
}

bool ui::RadioButton(const char* label, int* v, int v_button)
{
	const bool pressed = RadioButton(label, *v == v_button);
	if (pressed)
		*v = v_button;
	return pressed;
}

// size_arg (for each axis) < 0.0f: align to end, 0.0f: auto, > 0.0f: specified size
void ui::ProgressBar(float fraction, const ImVec2& size_arg, const char* overlay)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	ImVec2 pos = window->DC.CursorPos;
	ImRect bb(pos, pos + CalcItemSize(size_arg, CalcItemWidth(), g.FontSize + style.FramePadding.y*2.0f));
	ItemSize(bb, style.FramePadding.y);
	if (!ItemAdd(bb, 0))
		return;

	// Render
	fraction = ImSaturate(fraction);
	RenderFrame(bb.Min, bb.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);
	bb.Expand(ImVec2(-style.FrameBorderSize, -style.FrameBorderSize));
	const ImVec2 fill_br = ImVec2(ImLerp(bb.Min.x, bb.Max.x, fraction), bb.Max.y);
	RenderRectFilledRangeH(window->DrawList, bb, GetColorU32(ImGuiCol_PlotHistogram), 0.0f, fraction, style.FrameRounding);

	// Default displaying the fraction as percentage string, but user can override it
	char overlay_buf[32];
	if (!overlay)
	{
		ImFormatString(overlay_buf, IM_ARRAYSIZE(overlay_buf), "%.0f%%", fraction * 100 + 0.01f);
		overlay = overlay_buf;
	}

	ImVec2 overlay_size = CalcTextSize(overlay, NULL);
	if (overlay_size.x > 0.0f)
		RenderTextClipped(ImVec2(ImClamp(fill_br.x + style.ItemSpacing.x, bb.Min.x, bb.Max.x - overlay_size.x - style.ItemInnerSpacing.x), bb.Min.y), bb.Max, overlay, NULL, &overlay_size, ImVec2(0.0f, 0.5f), &bb);
}

void ui::Bullet()
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const float line_height = ImMax(ImMin(window->DC.CurrentLineSize.y, g.FontSize + g.Style.FramePadding.y * 2), g.FontSize);
	const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(g.FontSize, line_height));
	ItemSize(bb);
	if (!ItemAdd(bb, 0))
	{
		SameLine(0, style.FramePadding.x * 2);
		return;
	}

	// Render and stay on same line
	RenderBullet(bb.Min + ImVec2(style.FramePadding.x + g.FontSize*0.5f, line_height*0.5f));
	SameLine(0, style.FramePadding.x * 2);
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: Low-level Layout helpers
//-------------------------------------------------------------------------
// - Spacing()
// - Dummy()
// - NewLine()
// - AlignTextToFramePadding()
// - Separator()
// - VerticalSeparator() [Internal]
// - SplitterBehavior() [Internal]
//-------------------------------------------------------------------------

void ui::Spacing()
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;
	ItemSize(ImVec2(0, 0));
}

void ui::Dummy(const ImVec2& size)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
	ItemSize(bb);
	ItemAdd(bb, 0);
}

void ui::NewLine()
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiContext& g = *GImGui;
	const ImGuiLayoutType backup_layout_type = window->DC.LayoutType;
	window->DC.LayoutType = ImGuiLayoutType_Vertical;
	if (window->DC.CurrentLineSize.y > 0.0f)     // In the event that we are on a line with items that is smaller that FontSize high, we will preserve its height.
		ItemSize(ImVec2(0, 0));
	else
		ItemSize(ImVec2(0.0f, g.FontSize));
	window->DC.LayoutType = backup_layout_type;
}

void ui::AlignTextToFramePadding()
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiContext& g = *GImGui;
	window->DC.CurrentLineSize.y = ImMax(window->DC.CurrentLineSize.y, g.FontSize + g.Style.FramePadding.y * 2);
	window->DC.CurrentLineTextBaseOffset = ImMax(window->DC.CurrentLineTextBaseOffset, g.Style.FramePadding.y);
}

// Horizontal/vertical separating line
void ui::Separator()
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;
	ImGuiContext& g = *GImGui;

	// Those flags should eventually be overridable by the user
	ImGuiSeparatorFlags flags = (window->DC.LayoutType == ImGuiLayoutType_Horizontal) ? ImGuiSeparatorFlags_Vertical : ImGuiSeparatorFlags_Horizontal;
	IM_ASSERT(ImIsPowerOfTwo((int)(flags & (ImGuiSeparatorFlags_Horizontal | ImGuiSeparatorFlags_Vertical))));   // Check that only 1 option is selected
	if (flags & ImGuiSeparatorFlags_Vertical)
	{
		VerticalSeparator();
		return;
	}

	// Horizontal Separator
	if (window->DC.ColumnsSet)
		PopClipRect();

	float x1 = window->Pos.x;
	float x2 = window->Pos.x + window->Size.x;
	if (!window->DC.GroupStack.empty())
		x1 += window->DC.Indent.x;

	const ImRect bb(ImVec2(x1, window->DC.CursorPos.y), ImVec2(x2, window->DC.CursorPos.y + 1.0f));
	ItemSize(ImVec2(0.0f, 0.0f)); // NB: we don't provide our width so that it doesn't get feed back into AutoFit, we don't provide height to not alter layout.
	if (!ItemAdd(bb, 0))
	{
		if (window->DC.ColumnsSet)
			PushColumnClipRect();
		return;
	}

	window->DrawList->AddLine(bb.Min, ImVec2(bb.Max.x, bb.Min.y), GetColorU32(ImGuiCol_Separator));

	if (g.LogEnabled)
		LogRenderedText(&bb.Min, "--------------------------------");

	if (window->DC.ColumnsSet)
	{
		PushColumnClipRect();
		window->DC.ColumnsSet->LineMinY = window->DC.CursorPos.y;
	}
}

void ui::VerticalSeparator()
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;
	ImGuiContext& g = *GImGui;

	float y1 = window->DC.CursorPos.y;
	float y2 = window->DC.CursorPos.y + window->DC.CurrentLineSize.y;
	const ImRect bb(ImVec2(window->DC.CursorPos.x, y1), ImVec2(window->DC.CursorPos.x + 1.0f, y2));
	ItemSize(ImVec2(bb.GetWidth(), 0.0f));
	if (!ItemAdd(bb, 0))
		return;

	window->DrawList->AddLine(ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Min.x, bb.Max.y), GetColorU32(ImGuiCol_Separator));
	if (g.LogEnabled)
		LogText(" |");
}
// Helper for ColorPicker4()
static void RenderArrowsForVerticalBar(ImDrawList* draw_list, ImVec2 pos, ImVec2 half_sz, float bar_w)
{
	draw_list->AddRect(pos - ImVec2(0, 2), pos + ImVec2(20, 2), ImColor(0.f, 0.f, 0.f, 1.f));
}

// Using 'hover_visibility_delay' allows us to hide the highlight and mouse cursor for a short time, which can be convenient to reduce visual noise.
bool ui::SplitterBehavior(const ImRect& bb, ImGuiID id, ImGuiAxis axis, float* size1, float* size2, float min_size1, float min_size2, float hover_extend, float hover_visibility_delay)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;

	const ImGuiItemFlags item_flags_backup = window->DC.ItemFlags;
	window->DC.ItemFlags |= ImGuiItemFlags_NoNav | ImGuiItemFlags_NoNavDefaultFocus;
	bool item_add = ItemAdd(bb, id);
	window->DC.ItemFlags = item_flags_backup;
	if (!item_add)
		return false;

	bool hovered, held;
	ImRect bb_interact = bb;
	bb_interact.Expand(axis == ImGuiAxis_Y ? ImVec2(0.0f, hover_extend) : ImVec2(hover_extend, 0.0f));
	ButtonBehavior(bb_interact, id, &hovered, &held, ImGuiButtonFlags_FlattenChildren | ImGuiButtonFlags_AllowItemOverlap);
	if (g.ActiveId != id)
		SetItemAllowOverlap();

	if (held || (g.HoveredId == id && g.HoveredIdPreviousFrame == id && g.HoveredIdTimer >= hover_visibility_delay))
		SetMouseCursor(axis == ImGuiAxis_Y ? ImGuiMouseCursor_ResizeNS : ImGuiMouseCursor_ResizeEW);

	ImRect bb_render = bb;
	if (held)
	{
		ImVec2 mouse_delta_2d = g.IO.MousePos - g.ActiveIdClickOffset - bb_interact.Min;
		float mouse_delta = (axis == ImGuiAxis_Y) ? mouse_delta_2d.y : mouse_delta_2d.x;

		// Minimum pane size
		float size_1_maximum_delta = ImMax(0.0f, *size1 - min_size1);
		float size_2_maximum_delta = ImMax(0.0f, *size2 - min_size2);
		if (mouse_delta < -size_1_maximum_delta)
			mouse_delta = -size_1_maximum_delta;
		if (mouse_delta > size_2_maximum_delta)
			mouse_delta = size_2_maximum_delta;

		// Apply resize
		if (mouse_delta != 0.0f)
		{
			if (mouse_delta < 0.0f)
				IM_ASSERT(*size1 + mouse_delta >= min_size1);
			if (mouse_delta > 0.0f)
				IM_ASSERT(*size2 - mouse_delta >= min_size2);
			*size1 += mouse_delta;
			*size2 -= mouse_delta;
			bb_render.Translate((axis == ImGuiAxis_X) ? ImVec2(mouse_delta, 0.0f) : ImVec2(0.0f, mouse_delta));
			MarkItemEdited(id);
		}
	}

	// Render
	const ImU32 col = GetColorU32(held ? ImGuiCol_SeparatorActive : (hovered && g.HoveredIdTimer >= hover_visibility_delay) ? ImGuiCol_SeparatorHovered : ImGuiCol_Separator);
	window->DrawList->AddRectFilled(bb_render.Min, bb_render.Max, col, g.Style.FrameRounding);

	return held;
}

static float CalcMaxPopupHeightFromItemCount(int items_count)
{
	ImGuiContext& g = *GImGui;
	if (items_count <= 0)
		return FLT_MAX;
	return 19 * items_count + items_count % 4;
}

const char* keys[] = {
	"[-]",
	"[M1]",
	"[M2]",
	"[CN]",
	"[M3]",
	"[M4]",
	"[M5]",
	"[-]",
	"[BAC]",
	"[TAB]",
	"[-]",
	"[-]",
	"[CLR]",
	"[RET]",
	"[-]",
	"[-]",
	"[SHI]",
	"[CTL]",
	"[MEN]",
	"[PAU]",
	"[CAP]",
	"[KAN]",
	"[-]",
	"[JUN]",
	"[FIN]",
	"[KAN]",
	"[-]",
	"[ESC]",
	"[CON]",
	"[NCO]",
	"[ACC]",
	"[MAD]",
	"[SPA]",
	"[PGU]",
	"[PGD]",
	"[END]",
	"[HOM]",
	"[LEF]",
	"[UP]",
	"[RIG]",
	"[DOW]",
	"[SEL]",
	"[PRI]",
	"[EXE]",
	"[PRI]",
	"[INS]",
	"[DEL]",
	"[HEL]",
	"[0]",
	"[1]",
	"[2]",
	"[3]",
	"[4]",
	"[5]",
	"[6]",
	"[7]",
	"[8]",
	"[9]",
	"[-]",
	"[-]",
	"[-]",
	"[-]",
	"[-]",
	"[-]",
	"[-]",
	"[A]",
	"[B]",
	"[C]",
	"[D]",
	"[E]",
	"[F]",
	"[G]",
	"[H]",
	"[I]",
	"[J]",
	"[K]",
	"[L]",
	"[M]",
	"[N]",
	"[O]",
	"[P]",
	"[Q]",
	"[R]",
	"[S]",
	"[T]",
	"[U]",
	"[V]",
	"[W]",
	"[X]",
	"[Y]",
	"[Z]",
	"[WIN]",
	"[WIN]",
	"[APP]",
	"[-]",
	"[SLE]",
	"[NUM]",
	"[NUM]",
	"[NUM]",
	"[NUM]",
	"[NUM]",
	"[NUM]",
	"[NUM]",
	"[NUM]",
	"[NUM]",
	"[NUM]",
	"[MUL]",
	"[ADD]",
	"[SEP]",
	"[MIN]",
	"[DEC]",
	"[DIV]",
	"[F1]",
	"[F2]",
	"[F3]",
	"[F4]",
	"[F5]",
	"[F6]",
	"[F7]",
	"[F8]",
	"[F9]",
	"[F10]",
	"[F11]",
	"[F12]",
	"[F13]",
	"[F14]",
	"[F15]",
	"[F16]",
	"[F17]",
	"[F18]",
	"[F19]",
	"[F20]",
	"[F21]",
	"[F22]",
	"[F23]",
	"[F24]",
	"[-]",
	"[-]",
	"[-]",
	"[-]",
	"[-]",
	"[-]",
	"[-]",
	"[-]",
	"[NUM]",
	"[SCR]",
	"[EQU]",
	"[MAS]",
	"[TOY]",
	"[OYA]",
	"[OYA]",
	"[-]",
	"[-]",
	"[-]",
	"[-]",
	"[-]",
	"[-]",
	"[-]",
	"[-]",
	"[-]",
	"[SHI]",
	"[SHI]",
	"[CTR]",
	"[CTR]",
	"[ALT]",
	"[ALT]"
};

#define VK_LBUTTON        0x01
#define VK_RBUTTON        0x02
#define VK_MBUTTON        0x04
#define VK_XBUTTON1       0x05
#define VK_XBUTTON2       0x06
#define VK_BACK           0x08
#define VK_RMENU          0xA5




bool ui::Keybind(const char* str_id, int* current_key, int* key_style) {
	//ImGuiWindow* window = GetCurrentWindow();
	//if (window->SkipItems)
	//	return false;
	//
	//SameLine(window->Size.x - 40);
	//
	//ImGuiContext& g = *GImGui;
	//
	//const ImGuiStyle& style = g.Style;
	//const ImGuiID id = window->GetID(str_id);
	//ImGuiIO* io = &GetIO();
	//
	//const ImVec2 label_size = CalcTextSize(keys[*current_key]);
	//const ImRect frame_bb(window->DC.CursorPos - ImVec2(0, 5), window->DC.CursorPos + ImVec2(30, 15));
	//const ImRect total_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(window->Pos.x + window->Size.x - window->DC.CursorPos.x, label_size.y));
	//ItemSize(total_bb, style.FramePadding.y);
	//if (!ItemAdd(total_bb, id, &frame_bb))
	//	return false;
	//
	//const bool hovered = IsItemHovered();
	//const bool edit_requested = hovered && io->MouseClicked[0];
	//const bool style_requested = hovered && io->MouseClicked[1];
	//
	//if (edit_requested) {
	//	if (g.ActiveId != id) {
	//		memset(io->MouseDown, 0, sizeof(io->MouseDown));
	//		memset(io->KeysDown, 0, sizeof(io->KeysDown));
	//		*current_key = 0;
	//	}
	//
	//	SetActiveID(id, window);
	//	FocusWindow(window);
	//}
	//else if (!hovered && io->MouseClicked[0] && g.ActiveId == id)
	//	ClearActiveID();
	//
	//bool value_changed = false;
	//int key = *current_key;
	//
	//if (g.ActiveId == id) {
	//	for (auto i = 0; i < 5; i++) {
	//		if (io->MouseDown[i]) {
	//			switch (i) {
	//			case 0:
	//				key = VK_LBUTTON;
	//				break;
	//			case 1:
	//				key = VK_RBUTTON;
	//				break;
	//			case 2:
	//				key = VK_MBUTTON;
	//				break;
	//			case 3:
	//				key = VK_XBUTTON1;
	//				break;
	//			case 4:
	//				key = VK_XBUTTON2;
	//			}
	//			value_changed = true;
	//			ClearActiveID();
	//		}
	//	}
	//
	//	if (!value_changed) {
	//		for (auto i = VK_BACK; i <= VK_RMENU; i++) {
	//			if (io->KeysDown[i]) {
	//				key = i;
	//				value_changed = true;
	//				ClearActiveID();
	//			}
	//		}
	//	}
	//
	//	if (IsKeyPressedMap(ImGuiKey_Escape)) {
	//		*current_key = 0;
	//		ClearActiveID();
	//	}
	//	else
	//		*current_key = key;
	//}
	//else {
	//	if (key_style) {
	//		bool popup_open = IsPopupOpen(id);
	//
	//		if (style_requested && !popup_open)
	//			OpenPopupEx(id);
	//
	//		if (popup_open) {
	//			SetNextWindowSize(ImVec2(100, CalcMaxPopupHeightFromItemCount(4)));
	//
	//			char name[16];
	//			ImFormatString(name, IM_ARRAYSIZE(name), "##Combo_%02d", g.BeginPopupStack.Size); // Recycle windows based on depth
	//
	//			// Peak into expected window size so we can position it
	//			if (ImGuiWindow* popup_window = FindWindowByName(name))
	//				if (popup_window->WasActive)
	//				{
	//					ImVec2 size_expected = CalcWindowExpectedSize(popup_window);
	//					ImRect r_outer = GetWindowAllowedExtentRect(popup_window);
	//					ImVec2 pos = FindBestWindowPosForPopupEx(frame_bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, frame_bb, ImGuiPopupPositionPolicy_ComboBox);
	//					SetNextWindowPos(pos);
	//				}
	//
	//			// Horizontally align ourselves with the framed text
	//			ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
	//			PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(style.FramePadding.x, style.WindowPadding.y));
	//			bool ret = Begin(name, NULL, window_flags);
	//			PopStyleVar();
	//
	//			if (Selectable("Always On", *key_style == 0))
	//				*key_style = 0;
	//
	//			if (Selectable("On Hotkey", *key_style == 1))
	//				*key_style = 1;
	//
	//			if (Selectable("Toggle", *key_style == 2))
	//				*key_style = 2;
	//
	//			if (Selectable("Off Hotkey", *key_style == 3))
	//				*key_style = 3;
	//
	//			EndPopup();
	//		}
	//	}
	//}
	//
	//char buf_display[64] = "-";
	//
	//if (*current_key != 0 && g.ActiveId != id)
	//	strcpy_s(buf_display, keys[*current_key]);
	//else if (g.ActiveId == id)
	//	strcpy_s(buf_display, "-");
	//
	//ImVec2 textsize = CalcTextSize(buf_display);
	//
	//window->DrawList->AddRectFilledMultiColor(ImVec2(frame_bb.Min), ImVec2(frame_bb.Min + textsize + ImVec2(5, 5)), ImColor(17, 17, 17, 255), ImColor(17, 17, 17, 255), ImColor(27, 27, 27, 255), ImColor(27, 27, 27, 255));
	//
	//window->DrawList->AddRect(ImVec2(frame_bb.Min), ImVec2(frame_bb.Min + textsize + ImVec2(5, 5)), ImColor(36, 36, 36, 255));
	//
	//PushFont(io->Fonts->Fonts[0]);
	//window->DrawList->AddText(frame_bb.Min + ImVec2(3, 1), g.ActiveId == id ? ImColor(75 / 255.f, 50 / 255.f, 255 / 255.f, g.Style.Alpha) : ImColor(90 / 255.f, 90 / 255.f, 90 / 255.f, g.Style.Alpha), buf_display);
	//PopFont();
	//
	//return value_changed;



ImGuiWindow* window = GetCurrentWindow();
if (window->SkipItems)
return false;

SameLine(window->Size.x - 28);

ImGuiContext& g = *GImGui;

const ImGuiStyle& style = g.Style;
const ImGuiID id = window->GetID(str_id);
ImGuiIO* io = &GetIO();

const ImVec2 label_size = CalcTextSize(keys[*current_key]);
const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + label_size);
const ImRect total_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(window->Pos.x + window->Size.x - window->DC.CursorPos.x, label_size.y));
ItemSize(total_bb, style.FramePadding.y);
if (!ItemAdd(total_bb, id, &frame_bb))
return false;

const bool hovered = IsItemHovered();
const bool edit_requested = hovered && io->MouseClicked[0];
const bool style_requested = hovered && io->MouseClicked[1];

if (edit_requested) {
	if (g.ActiveId != id) {
		memset(io->MouseDown, 0, sizeof(io->MouseDown));
		memset(io->KeysDown, 0, sizeof(io->KeysDown));
		*current_key = 0;
	}

	SetActiveID(id, window);
	FocusWindow(window);
}
else if (!hovered && io->MouseClicked[0] && g.ActiveId == id)
ClearActiveID();

bool value_changed = false;
int key = *current_key;

if (g.ActiveId == id) {
	for (auto i = 0; i < 5; i++) {
		if (io->MouseDown[i]) {
			switch (i) {
			case 0:
				key = VK_LBUTTON;
				break;
			case 1:
				key = VK_RBUTTON;
				break;
			case 2:
				key = VK_MBUTTON;
				break;
			case 3:
				key = VK_XBUTTON1;
				break;
			case 4:
				key = VK_XBUTTON2;
			}
			value_changed = true;
			ClearActiveID();
		}
	}

	if (!value_changed) {
		for (auto i = VK_BACK; i <= VK_RMENU; i++) {
			if (io->KeysDown[i]) {
				key = i;
				value_changed = true;
				ClearActiveID();
			}
		}
	}

	if (IsKeyPressedMap(ImGuiKey_Escape)) {
		*current_key = 0;
		ClearActiveID();
	}
	else
		*current_key = key;
}
else {
	if (key_style) {
		bool popup_open = IsPopupOpen(id);

		if (style_requested && !popup_open)
			OpenPopupEx(id);

		if (popup_open) {
			SetNextWindowSize(ImVec2(100, CalcMaxPopupHeightFromItemCount(4)));

			char name[16];
			ImFormatString(name, IM_ARRAYSIZE(name), "##Combo_%02d", g.BeginPopupStack.Size); // Recycle windows based on depth

			// Peak into expected window size so we can position it
			if (ImGuiWindow* popup_window = FindWindowByName(name))
				if (popup_window->WasActive)
				{
					ImVec2 size_expected = CalcWindowExpectedSize(popup_window);
					ImRect r_outer = GetWindowAllowedExtentRect(popup_window);
					ImVec2 pos = FindBestWindowPosForPopupEx(frame_bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, frame_bb, ImGuiPopupPositionPolicy_ComboBox);
					SetNextWindowPos(pos);
				}

			// Horizontally align ourselves with the framed text
			ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
			PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(style.FramePadding.x, style.WindowPadding.y));
			bool ret = Begin(name, NULL, window_flags);
			PopStyleVar();

			if (Selectable("Hold", *key_style == 0))
				*key_style = 0;

			if (Selectable("Toggle", *key_style == 1))
				*key_style = 1;

			//if (Selectable("Toggle", *key_style == 2))
			//	*key_style = 2;
			//
			//if (Selectable("Off Hotkey", *key_style == 3))
			//	*key_style = 3;

			EndPopup();
		}
	}
}

char buf_display[64] = "[-]";

if (*current_key != 0 && g.ActiveId != id)
strcpy_s(buf_display, keys[*current_key]);
else if (g.ActiveId == id)
strcpy_s(buf_display, "[-]");

PushFont(io->Fonts->Fonts[0]);
window->DrawList->AddText(frame_bb.Min - ImVec2(5,0), g.ActiveId == id ? ImColor(75 / 255.f, 50 / 255.f, 255 / 255.f, g.Style.Alpha) : ImColor(90 / 255.f, 90 / 255.f, 90 / 255.f, g.Style.Alpha), buf_display);
PopFont();

return value_changed;






}

//-------------------------------------------------------------------------
// [SECTION] Widgets: ComboBox
//-------------------------------------------------------------------------
// - BeginCombo()
// - EndCombo()
// - Combo()
//-------------------------------------------------------------------------



bool ui::BeginCombo(const char* label, const char* preview_value, ImGuiComboFlags flags, int items)
{
	ImGuiContext& g = *GImGui;
	ImGuiCond backup_next_window_size_constraint = g.NextWindowData.SizeConstraintCond;
	g.NextWindowData.SizeConstraintCond = 0;

	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) != (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)); // Can't use both flags together

	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);

	const float arrow_size = (flags & ImGuiComboFlags_NoArrowButton) ? 0.0f : GetFrameHeight();
	const ImVec2 label_size = CalcTextSize(label, NULL, true);
	const float w = ImClamp(window->Size.x - 64, 268.f, 313.f);
	const ImRect frame_bb(window->DC.CursorPos + ImVec2(window->Size.x - 140, 0), window->DC.CursorPos + ImVec2(window->Size.x - 40, label_size.x > 0 ? label_size.y : 2));
	const ImRect total_bb(frame_bb.Min, frame_bb.Max);
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id, &frame_bb))
		return false;

	bool hovered, held;
	bool pressed = ButtonBehavior(frame_bb, id, &hovered, &held);
	bool popup_open = IsPopupOpen(id);

	const ImRect value_bb(frame_bb.Min, frame_bb.Max - ImVec2(arrow_size, 0.0f));
	ImRect cb_bb = ImRect(frame_bb.Min + ImVec2(0, label_size.x > 0 ? label_size.y - 13 : -13), frame_bb.Max);

	if (!hovered && !popup_open)
		window->DrawList->AddRectFilledMultiColor(cb_bb.Min, cb_bb.Max,
			ImColor(17 / 255.f, 17 / 255.f, 17 / 255.f, g.Style.Alpha),
			ImColor(17 / 255.f, 17 / 255.f, 17 / 255.f, g.Style.Alpha),
			ImColor(47 / 255.f, 47 / 255.f, 47 / 255.f, g.Style.Alpha),
			ImColor(47 / 255.f, 47 / 255.f, 47 / 255.f, g.Style.Alpha));
	else
		window->DrawList->AddRectFilledMultiColor(cb_bb.Min, cb_bb.Max,
			ImColor(27 / 255.f, 27 / 255.f, 27 / 255.f, g.Style.Alpha),
			ImColor(27 / 255.f, 27 / 255.f, 27 / 255.f, g.Style.Alpha),
			ImColor(57 / 255.f, 57 / 255.f, 57 / 255.f, g.Style.Alpha),
			ImColor(57 / 255.f, 57 / 255.f, 57 / 255.f, g.Style.Alpha));

	window->DrawList->AddRect(cb_bb.Min, cb_bb.Max, ImColor(47 / 255.f, 47 / 255.f, 47 / 255.f, g.Style.Alpha));

	if (label_size.x > 0)
		window->DrawList->AddText(frame_bb.Min - ImVec2(149 - 41, 0), ImColor(182 / 255.f, 178 / 255.f, 180 / 255.f, g.Style.Alpha), label);

	window->DrawList->PushClipRect(cb_bb.Min, cb_bb.Max - ImVec2(20, 0), true);
	window->DrawList->AddText(cb_bb.Min + ImVec2(8, -1), ImColor(182 / 255.f, 178 / 255.f, 180 / 255.f, g.Style.Alpha), preview_value);
	window->DrawList->PopClipRect();

	if (!popup_open)
		RenderArrow(cb_bb.Max - ImVec2(16, 11), ImGuiDir_Down, 0.8f);
	else
		RenderArrow(cb_bb.Max - ImVec2(16, 11), ImGuiDir_Up, 0.8f);

	if ((pressed || g.NavActivateId == id) && !popup_open)
	{
		if (window->DC.NavLayerCurrent == 0)
			window->NavLastIds[0] = id;
		OpenPopupEx(id);
		popup_open = true;
	}

	if (!popup_open)
		return false;

	SetNextWindowSize(ImVec2(w - 174, CalcMaxPopupHeightFromItemCount(items) + 20));

	char name[16];
	ImFormatString(name, IM_ARRAYSIZE(name), "##Combo_%02d", g.BeginPopupStack.Size); // Recycle windows based on depth

	// Peak into expected window size so we can position it
	if (ImGuiWindow* popup_window = FindWindowByName(name))
		if (popup_window->WasActive)
		{
			ImVec2 size_expected = CalcWindowExpectedSize(popup_window);
			if (flags & ImGuiComboFlags_PopupAlignLeft)
				popup_window->AutoPosLastDirection = ImGuiDir_Left;
			ImRect r_outer = GetWindowAllowedExtentRect(popup_window);
			ImVec2 pos = FindBestWindowPosForPopupEx(frame_bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, frame_bb, ImGuiPopupPositionPolicy_ComboBox);
			SetNextWindowPos(pos);
		}

	// Horizontally align ourselves with the framed text
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
	PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(style.FramePadding.x, style.WindowPadding.y));
	bool ret = Begin(name, NULL, window_flags);
	PopStyleVar();
	if (!ret)
	{
		EndPopup();
		IM_ASSERT(0);   // This should never happen as we tested for IsPopupOpen() above
		return false;
	}
	return true;
}

void ui::EndCombo()
{
	EndPopup();
}
void ui::EndComboSkeet()
{
	EndPopupSkeet();
}
// Getter for the old Combo() API: const char*[]
static bool Items_ArrayGetter(void* data, int idx, const char** out_text)
{
	const char* const* items = (const char* const*)data;
	if (out_text)
		*out_text = items[idx];
	return true;
}

// Getter for the old Combo() API: "item1\0item2\0item3\0"
static bool Items_SingleStringGetter(void* data, int idx, const char** out_text)
{
	// FIXME-OPT: we could pre-compute the indices to fasten this. But only 1 active combo means the waste is limited.
	const char* items_separated_by_zeros = (const char*)data;
	int items_count = 0;
	const char* p = items_separated_by_zeros;
	while (*p)
	{
		if (idx == items_count)
			break;
		p += strlen(p) + 1;
		items_count++;
	}
	if (!*p)
		return false;
	if (out_text)
		*out_text = p;
	return true;
}

// Old API, prefer using BeginCombo() nowadays if you can.
bool ui::Combo(const char* label, int* current_item, bool(*items_getter)(void*, int, const char**), void* data, int items_count, int popup_max_height_in_items)
{
	ImGuiContext& g = *GImGui;

	// Call the getter to obtain the preview string which is a parameter to BeginCombo()
	const char* preview_value = NULL;
	if (*current_item >= 0 && *current_item < items_count)
		items_getter(data, *current_item, &preview_value);

	// The old Combo() API exposed "popup_max_height_in_items". The new more general BeginCombo() API doesn't have/need it, but we emulate it here.
	if (popup_max_height_in_items != -1 && !g.NextWindowData.SizeConstraintCond)
		SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, CalcMaxPopupHeightFromItemCount(popup_max_height_in_items)));

	if (!BeginCombo(label, preview_value, ImGuiComboFlags_None))
		return false;

	// Display items
	// FIXME-OPT: Use clipper (but we need to disable it on the appearing frame to make sure our call to SetItemDefaultFocus() is processed)
	bool value_changed = false;
	for (int i = 0; i < items_count; i++)
	{
		PushID((void*)(intptr_t)i);
		const bool item_selected = (i == *current_item);
		const char* item_text;
		if (!items_getter(data, i, &item_text))
			item_text = "*Unknown item*";
		if (Selectable(item_text, item_selected))
		{
			value_changed = true;
			*current_item = i;
		}
		if (item_selected)
			SetItemDefaultFocus();
		PopID();
	}

	EndCombo();
	return value_changed;
}

// Combo box helper allowing to pass an array of strings.
bool ui::Combo(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items)
{
	const bool value_changed = Combo(label, current_item, Items_ArrayGetter, (void*)items, items_count, height_in_items);
	return value_changed;
}

// Combo box helper allowing to pass all items in a single string literal holding multiple zero-terminated items "item1\0item2\0"
bool ui::Combo(const char* label, int* current_item, const char* items_separated_by_zeros, int height_in_items)
{
	int items_count = 0;
	const char* p = items_separated_by_zeros;       // FIXME-OPT: Avoid computing this, or at least only when combo is open
	while (*p)
	{
		p += strlen(p) + 1;
		items_count++;
	}
	bool value_changed = Combo(label, current_item, Items_SingleStringGetter, (void*)items_separated_by_zeros, items_count, height_in_items);
	return value_changed;
}

//-------------------------------------------------------------------------
// [SECTION] Data Type and Data Formatting Helpers [Internal]
//-------------------------------------------------------------------------
// - PatchFormatStringFloatToInt()
// - DataTypeFormatString()
// - DataTypeApplyOp()
// - DataTypeApplyOpFromText()
// - GetMinimumStepAtDecimalPrecision
// - RoundScalarWithFormat<>()
//-------------------------------------------------------------------------

struct ImGuiDataTypeInfo
{
	size_t      Size;
	const char* PrintFmt;   // Unused
	const char* ScanFmt;
};

static const ImGuiDataTypeInfo GDataTypeInfo[] =
{
	{ sizeof(int),          "%d",   "%d"    },
	{ sizeof(unsigned int), "%u",   "%u"    },
#ifdef _MSC_VER
	{ sizeof(ImS64),        "%I64d","%I64d" },
	{ sizeof(ImU64),        "%I64u","%I64u" },
#else
	{ sizeof(ImS64),        "%lld", "%lld"  },
	{ sizeof(ImU64),        "%llu", "%llu"  },
#endif
	{ sizeof(float),        "%f",   "%f"    },  // float are promoted to double in va_arg
	{ sizeof(double),       "%f",   "%lf"   },
};
IM_STATIC_ASSERT(IM_ARRAYSIZE(GDataTypeInfo) == ImGuiDataType_COUNT);

// FIXME-LEGACY: Prior to 1.61 our DragInt() function internally used floats and because of this the compile-time default value for format was "%.0f".
// Even though we changed the compile-time default, we expect users to have carried %f around, which would break the display of DragInt() calls.
// To honor backward compatibility we are rewriting the format string, unless IMGUI_DISABLE_OBSOLETE_FUNCTIONS is enabled. What could possibly go wrong?!
static const char* PatchFormatStringFloatToInt(const char* fmt)
{
	if (fmt[0] == '%' && fmt[1] == '.' && fmt[2] == '0' && fmt[3] == 'f' && fmt[4] == 0) // Fast legacy path for "%.0f" which is expected to be the most common case.
		return "%d";
	const char* fmt_start = ImParseFormatFindStart(fmt);    // Find % (if any, and ignore %%)
	const char* fmt_end = ImParseFormatFindEnd(fmt_start);  // Find end of format specifier, which itself is an exercise of confidence/recklessness (because snprintf is dependent on libc or user).
	if (fmt_end > fmt_start && fmt_end[-1] == 'f')
	{
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
		if (fmt_start == fmt && fmt_end[0] == 0)
			return "%d";
		ImGuiContext& g = *GImGui;
		ImFormatString(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), "%.*s%%d%s", (int)(fmt_start - fmt), fmt, fmt_end); // Honor leading and trailing decorations, but lose alignment/precision.
		return g.TempBuffer;
#else
		IM_ASSERT(0 && "DragInt(): Invalid format string!"); // Old versions used a default parameter of "%.0f", please replace with e.g. "%d"
#endif
	}
	return fmt;
}

static inline int DataTypeFormatString(char* buf, int buf_size, ImGuiDataType data_type, const void* data_ptr, const char* format)
{
	if (data_type == ImGuiDataType_S32 || data_type == ImGuiDataType_U32)   // Signedness doesn't matter when pushing the argument
		return ImFormatString(buf, buf_size, format, *(const ImU32*)data_ptr);
	if (data_type == ImGuiDataType_S64 || data_type == ImGuiDataType_U64)   // Signedness doesn't matter when pushing the argument
		return ImFormatString(buf, buf_size, format, *(const ImU64*)data_ptr);
	if (data_type == ImGuiDataType_Float)
		return ImFormatString(buf, buf_size, format, *(const float*)data_ptr);
	if (data_type == ImGuiDataType_Double)
		return ImFormatString(buf, buf_size, format, *(const double*)data_ptr);
	IM_ASSERT(0);
	return 0;
}

// FIXME: Adding support for clamping on boundaries of the data type would be nice.
static void DataTypeApplyOp(ImGuiDataType data_type, int op, void* output, void* arg1, const void* arg2)
{
	IM_ASSERT(op == '+' || op == '-');
	switch (data_type)
	{
	case ImGuiDataType_S32:
		if (op == '+')      *(int*)output = *(const int*)arg1 + *(const int*)arg2;
		else if (op == '-') *(int*)output = *(const int*)arg1 - *(const int*)arg2;
		return;
	case ImGuiDataType_U32:
		if (op == '+')      *(unsigned int*)output = *(const unsigned int*)arg1 + *(const ImU32*)arg2;
		else if (op == '-') *(unsigned int*)output = *(const unsigned int*)arg1 - *(const ImU32*)arg2;
		return;
	case ImGuiDataType_S64:
		if (op == '+')      *(ImS64*)output = *(const ImS64*)arg1 + *(const ImS64*)arg2;
		else if (op == '-') *(ImS64*)output = *(const ImS64*)arg1 - *(const ImS64*)arg2;
		return;
	case ImGuiDataType_U64:
		if (op == '+')      *(ImU64*)output = *(const ImU64*)arg1 + *(const ImU64*)arg2;
		else if (op == '-') *(ImU64*)output = *(const ImU64*)arg1 - *(const ImU64*)arg2;
		return;
	case ImGuiDataType_Float:
		if (op == '+')      *(float*)output = *(const float*)arg1 + *(const float*)arg2;
		else if (op == '-') *(float*)output = *(const float*)arg1 - *(const float*)arg2;
		return;
	case ImGuiDataType_Double:
		if (op == '+')      *(double*)output = *(const double*)arg1 + *(const double*)arg2;
		else if (op == '-') *(double*)output = *(const double*)arg1 - *(const double*)arg2;
		return;
	case ImGuiDataType_COUNT: break;
	}
	IM_ASSERT(0);
}

// User can input math operators (e.g. +100) to edit a numerical values.
// NB: This is _not_ a full expression evaluator. We should probably add one and replace this dumb mess..
static bool DataTypeApplyOpFromText(const char* buf, const char* initial_value_buf, ImGuiDataType data_type, void* data_ptr, const char* format)
{
	while (ImCharIsBlankA(*buf))
		buf++;

	// We don't support '-' op because it would conflict with inputing negative value.
	// Instead you can use +-100 to subtract from an existing value
	char op = buf[0];
	if (op == '+' || op == '*' || op == '/')
	{
		buf++;
		while (ImCharIsBlankA(*buf))
			buf++;
	}
	else
	{
		op = 0;
	}
	if (!buf[0])
		return false;

	// Copy the value in an opaque buffer so we can compare at the end of the function if it changed at all.
	IM_ASSERT(data_type < ImGuiDataType_COUNT);
	int data_backup[2];
	IM_ASSERT(GDataTypeInfo[data_type].Size <= sizeof(data_backup));
	memcpy(data_backup, data_ptr, GDataTypeInfo[data_type].Size);

	if (format == NULL)
		format = GDataTypeInfo[data_type].ScanFmt;

	int arg1i = 0;
	if (data_type == ImGuiDataType_S32)
	{
		int* v = (int*)data_ptr;
		int arg0i = *v;
		float arg1f = 0.0f;
		if (op && sscanf(initial_value_buf, format, &arg0i) < 1)
			return false;
		// Store operand in a float so we can use fractional value for multipliers (*1.1), but constant always parsed as integer so we can fit big integers (e.g. 2000000003) past float precision
		if (op == '+') { if (sscanf(buf, "%d", &arg1i)) *v = (int)(arg0i + arg1i); }                   // Add (use "+-" to subtract)
		else if (op == '*') { if (sscanf(buf, "%f", &arg1f)) *v = (int)(arg0i * arg1f); }                   // Multiply
		else if (op == '/') { if (sscanf(buf, "%f", &arg1f) && arg1f != 0.0f) *v = (int)(arg0i / arg1f); }  // Divide
		else { if (sscanf(buf, format, &arg1i) == 1) *v = arg1i; }                           // Assign constant
	}
	else if (data_type == ImGuiDataType_U32 || data_type == ImGuiDataType_S64 || data_type == ImGuiDataType_U64)
	{
		// Assign constant
		// FIXME: We don't bother handling support for legacy operators since they are a little too crappy. Instead we may implement a proper expression evaluator in the future.
		sscanf(buf, format, data_ptr);
	}
	else if (data_type == ImGuiDataType_Float)
	{
		// For floats we have to ignore format with precision (e.g. "%.2f") because sscanf doesn't take them in
		format = "%f";
		float* v = (float*)data_ptr;
		float arg0f = *v, arg1f = 0.0f;
		if (op && sscanf(initial_value_buf, format, &arg0f) < 1)
			return false;
		if (sscanf(buf, format, &arg1f) < 1)
			return false;
		if (op == '+') { *v = arg0f + arg1f; }                    // Add (use "+-" to subtract)
		else if (op == '*') { *v = arg0f * arg1f; }                    // Multiply
		else if (op == '/') { if (arg1f != 0.0f) *v = arg0f / arg1f; } // Divide
		else { *v = arg1f; }                            // Assign constant
	}
	else if (data_type == ImGuiDataType_Double)
	{
		format = "%lf"; // scanf differentiate float/double unlike printf which forces everything to double because of ellipsis
		double* v = (double*)data_ptr;
		double arg0f = *v, arg1f = 0.0;
		if (op && sscanf(initial_value_buf, format, &arg0f) < 1)
			return false;
		if (sscanf(buf, format, &arg1f) < 1)
			return false;
		if (op == '+') { *v = arg0f + arg1f; }                    // Add (use "+-" to subtract)
		else if (op == '*') { *v = arg0f * arg1f; }                    // Multiply
		else if (op == '/') { if (arg1f != 0.0f) *v = arg0f / arg1f; } // Divide
		else { *v = arg1f; }                            // Assign constant
	}
	return memcmp(data_backup, data_ptr, GDataTypeInfo[data_type].Size) != 0;
}

static float GetMinimumStepAtDecimalPrecision(int decimal_precision)
{
	static const float min_steps[10] = { 1.0f, 0.1f, 0.01f, 0.001f, 0.0001f, 0.00001f, 0.000001f, 0.0000001f, 0.00000001f, 0.000000001f };
	if (decimal_precision < 0)
		return FLT_MIN;
	return (decimal_precision < IM_ARRAYSIZE(min_steps)) ? min_steps[decimal_precision] : ImPow(10.0f, (float)-decimal_precision);
}

template<typename TYPE>
static const char* ImAtoi(const char* src, TYPE* output)
{
	int negative = 0;
	if (*src == '-') { negative = 1; src++; }
	if (*src == '+') { src++; }
	TYPE v = 0;
	while (*src >= '0' && *src <= '9')
		v = (v * 10) + (*src++ - '0');
	*output = negative ? -v : v;
	return src;
}

template<typename TYPE, typename SIGNEDTYPE>
TYPE ui::RoundScalarWithFormatT(const char* format, ImGuiDataType data_type, TYPE v)
{
	const char* fmt_start = ImParseFormatFindStart(format);
	if (fmt_start[0] != '%' || fmt_start[1] == '%') // Don't apply if the value is not visible in the format string
		return v;
	char v_str[64];
	ImFormatString(v_str, IM_ARRAYSIZE(v_str), fmt_start, v);
	const char* p = v_str;
	while (*p == ' ')
		p++;
	if (data_type == ImGuiDataType_Float || data_type == ImGuiDataType_Double)
		v = (TYPE)ImAtof(p);
	else
		ImAtoi(p, (SIGNEDTYPE*)&v);
	return v;
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: DragScalar, DragFloat, DragInt, etc.
//-------------------------------------------------------------------------
// - DragBehaviorT<>() [Internal]
// - DragBehavior() [Internal]
// - DragScalar()
// - DragScalarN()
// - DragFloat()
// - DragFloat2()
// - DragFloat3()
// - DragFloat4()
// - DragFloatRange2()
// - DragInt()
// - DragInt2()
// - DragInt3()
// - DragInt4()
// - DragIntRange2()
//-------------------------------------------------------------------------

// This is called by DragBehavior() when the widget is active (held by mouse or being manipulated with Nav controls)
template<typename TYPE, typename SIGNEDTYPE, typename FLOATTYPE>
bool ui::DragBehaviorT(ImGuiDataType data_type, TYPE* v, float v_speed, const TYPE v_min, const TYPE v_max, const char* format, float power, ImGuiDragFlags flags)
{
	ImGuiContext& g = *GImGui;
	const ImGuiAxis axis = (flags & ImGuiDragFlags_Vertical) ? ImGuiAxis_Y : ImGuiAxis_X;
	const bool is_decimal = (data_type == ImGuiDataType_Float) || (data_type == ImGuiDataType_Double);
	const bool has_min_max = (v_min != v_max);
	const bool is_power = (power != 1.0f && is_decimal && has_min_max && (v_max - v_min < FLT_MAX));

	// Default tweak speed
	if (v_speed == 0.0f && has_min_max && (v_max - v_min < FLT_MAX))
		v_speed = (float)((v_max - v_min) * g.DragSpeedDefaultRatio);

	// Inputs accumulates into g.DragCurrentAccum, which is flushed into the current value as soon as it makes a difference with our precision settings
	float adjust_delta = 0.0f;
	if (g.ActiveIdSource == ImGuiInputSource_Mouse && IsMousePosValid() && g.IO.MouseDragMaxDistanceSqr[0] > 1.0f*1.0f)
	{
		adjust_delta = g.IO.MouseDelta[axis];
		if (g.IO.KeyAlt)
			adjust_delta *= 1.0f / 100.0f;
		if (g.IO.KeyShift)
			adjust_delta *= 10.0f;
	}
	else if (g.ActiveIdSource == ImGuiInputSource_Nav)
	{
		int decimal_precision = is_decimal ? ImParseFormatPrecision(format, 3) : 0;
		adjust_delta = GetNavInputAmount2d(ImGuiNavDirSourceFlags_Keyboard | ImGuiNavDirSourceFlags_PadDPad, ImGuiInputReadMode_RepeatFast, 1.0f / 10.0f, 10.0f)[axis];
		v_speed = ImMax(v_speed, GetMinimumStepAtDecimalPrecision(decimal_precision));
	}
	adjust_delta *= v_speed;

	// For vertical drag we currently assume that Up=higher value (like we do with vertical sliders). This may become a parameter.
	if (axis == ImGuiAxis_Y)
		adjust_delta = -adjust_delta;

	// Clear current value on activation
	// Avoid altering values and clamping when we are _already_ past the limits and heading in the same direction, so e.g. if range is 0..255, current value is 300 and we are pushing to the right side, keep the 300.
	bool is_just_activated = g.ActiveIdIsJustActivated;
	bool is_already_past_limits_and_pushing_outward = has_min_max && ((*v >= v_max && adjust_delta > 0.0f) || (*v <= v_min && adjust_delta < 0.0f));
	bool is_drag_direction_change_with_power = is_power && ((adjust_delta < 0 && g.DragCurrentAccum > 0) || (adjust_delta > 0 && g.DragCurrentAccum < 0));
	if (is_just_activated || is_already_past_limits_and_pushing_outward || is_drag_direction_change_with_power)
	{
		g.DragCurrentAccum = 0.0f;
		g.DragCurrentAccumDirty = false;
	}
	else if (adjust_delta != 0.0f)
	{
		g.DragCurrentAccum += adjust_delta;
		g.DragCurrentAccumDirty = true;
	}

	if (!g.DragCurrentAccumDirty)
		return false;

	TYPE v_cur = *v;
	FLOATTYPE v_old_ref_for_accum_remainder = (FLOATTYPE)0.0f;

	if (is_power)
	{
		// Offset + round to user desired precision, with a curve on the v_min..v_max range to get more precision on one side of the range
		FLOATTYPE v_old_norm_curved = ImPow((FLOATTYPE)(v_cur - v_min) / (FLOATTYPE)(v_max - v_min), (FLOATTYPE)1.0f / power);
		FLOATTYPE v_new_norm_curved = v_old_norm_curved + (g.DragCurrentAccum / (v_max - v_min));
		v_cur = v_min + (TYPE)ImPow(ImSaturate((float)v_new_norm_curved), power) * (v_max - v_min);
		v_old_ref_for_accum_remainder = v_old_norm_curved;
	}
	else
	{
		v_cur += (TYPE)g.DragCurrentAccum;
	}

	// Round to user desired precision based on format string
	v_cur = RoundScalarWithFormatT<TYPE, SIGNEDTYPE>(format, data_type, v_cur);

	// Preserve remainder after rounding has been applied. This also allow slow tweaking of values.
	g.DragCurrentAccumDirty = false;
	if (is_power)
	{
		FLOATTYPE v_cur_norm_curved = ImPow((FLOATTYPE)(v_cur - v_min) / (FLOATTYPE)(v_max - v_min), (FLOATTYPE)1.0f / power);
		g.DragCurrentAccum -= (float)(v_cur_norm_curved - v_old_ref_for_accum_remainder);
	}
	else
	{
		g.DragCurrentAccum -= (float)((SIGNEDTYPE)v_cur - (SIGNEDTYPE)*v);
	}

	// Lose zero sign for float/double
	if (v_cur == (TYPE)-0)
		v_cur = (TYPE)0;

	// Clamp values (+ handle overflow/wrap-around for integer types)
	if (*v != v_cur && has_min_max)
	{
		if (v_cur < v_min || (v_cur > *v && adjust_delta < 0.0f && !is_decimal))
			v_cur = v_min;
		if (v_cur > v_max || (v_cur < *v && adjust_delta > 0.0f && !is_decimal))
			v_cur = v_max;
	}

	// Apply result
	if (*v == v_cur)
		return false;
	*v = v_cur;
	return true;
}

bool ui::DragBehavior(ImGuiID id, ImGuiDataType data_type, void* v, float v_speed, const void* v_min, const void* v_max, const char* format, float power, ImGuiDragFlags flags)
{
	ImGuiContext& g = *GImGui;
	if (g.ActiveId == id)
	{
		if (g.ActiveIdSource == ImGuiInputSource_Mouse && !g.IO.MouseDown[0])
			ClearActiveID();
		else if (g.ActiveIdSource == ImGuiInputSource_Nav && g.NavActivatePressedId == id && !g.ActiveIdIsJustActivated)
			ClearActiveID();
	}
	if (g.ActiveId != id)
		return false;

	switch (data_type)
	{
	case ImGuiDataType_S32:    return DragBehaviorT<ImS32, ImS32, float >(data_type, (ImS32*)v, v_speed, v_min ? *(const ImS32*)v_min : IM_S32_MIN, v_max ? *(const ImS32*)v_max : IM_S32_MAX, format, power, flags);
	case ImGuiDataType_U32:    return DragBehaviorT<ImU32, ImS32, float >(data_type, (ImU32*)v, v_speed, v_min ? *(const ImU32*)v_min : IM_U32_MIN, v_max ? *(const ImU32*)v_max : IM_U32_MAX, format, power, flags);
	case ImGuiDataType_S64:    return DragBehaviorT<ImS64, ImS64, double>(data_type, (ImS64*)v, v_speed, v_min ? *(const ImS64*)v_min : IM_S64_MIN, v_max ? *(const ImS64*)v_max : IM_S64_MAX, format, power, flags);
	case ImGuiDataType_U64:    return DragBehaviorT<ImU64, ImS64, double>(data_type, (ImU64*)v, v_speed, v_min ? *(const ImU64*)v_min : IM_U64_MIN, v_max ? *(const ImU64*)v_max : IM_U64_MAX, format, power, flags);
	case ImGuiDataType_Float:  return DragBehaviorT<float, float, float >(data_type, (float*)v, v_speed, v_min ? *(const float*)v_min : -FLT_MAX, v_max ? *(const float*)v_max : FLT_MAX, format, power, flags);
	case ImGuiDataType_Double: return DragBehaviorT<double, double, double>(data_type, (double*)v, v_speed, v_min ? *(const double*)v_min : -DBL_MAX, v_max ? *(const double*)v_max : DBL_MAX, format, power, flags);
	case ImGuiDataType_COUNT:  break;
	}
	IM_ASSERT(0);
	return false;
}

bool ui::DragScalar(const char* label, ImGuiDataType data_type, void* v, float v_speed, const void* v_min, const void* v_max, const char* format, float power)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	if (power != 1.0f)
		IM_ASSERT(v_min != NULL && v_max != NULL); // When using a power curve the drag needs to have known bounds

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const float w = CalcItemWidth();

	const ImVec2 label_size = CalcTextSize(label, NULL, true);
	const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y*2.0f));
	const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id, &frame_bb))
		return false;

	const bool hovered = ItemHoverable(frame_bb, id);

	// Default format string when passing NULL
	// Patch old "%.0f" format string to use "%d", read function comments for more details.
	IM_ASSERT(data_type >= 0 && data_type < ImGuiDataType_COUNT);
	if (format == NULL)
		format = GDataTypeInfo[data_type].PrintFmt;
	else if (data_type == ImGuiDataType_S32 && strcmp(format, "%d") != 0)
		format = PatchFormatStringFloatToInt(format);

	// Tabbing or CTRL-clicking on Drag turns it into an input box
	bool start_text_input = false;
	const bool tab_focus_requested = FocusableItemRegister(window, id);
	if (tab_focus_requested || (hovered && (g.IO.MouseClicked[0] || g.IO.MouseDoubleClicked[0])) || g.NavActivateId == id || (g.NavInputId == id && g.ScalarAsInputTextId != id))
	{
		SetActiveID(id, window);
		SetFocusID(id, window);
		FocusWindow(window);
		g.ActiveIdAllowNavDirFlags = (1 << ImGuiDir_Up) | (1 << ImGuiDir_Down);
		if (tab_focus_requested || g.IO.KeyCtrl || g.IO.MouseDoubleClicked[0] || g.NavInputId == id)
		{
			start_text_input = true;
			g.ScalarAsInputTextId = 0;
		}
	}
	if (start_text_input || (g.ActiveId == id && g.ScalarAsInputTextId == id))
	{
		window->DC.CursorPos = frame_bb.Min;
		FocusableItemUnregister(window);
		return InputScalarAsWidgetReplacement(frame_bb, id, label, data_type, v, format);
	}

	// Actual drag behavior
	const bool value_changed = DragBehavior(id, data_type, v, v_speed, v_min, v_max, format, power, ImGuiDragFlags_None);
	if (value_changed)
		MarkItemEdited(id);

	// Draw frame
	const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : g.HoveredId == id ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
	RenderNavHighlight(frame_bb, id);
	RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, style.FrameRounding);

	// Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
	char value_buf[64];
	const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, v, format);
	RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.5f));

	if (label_size.x > 0.0f)
		RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags);
	return value_changed;
}

bool ui::DragScalarN(const char* label, ImGuiDataType data_type, void* v, int components, float v_speed, const void* v_min, const void* v_max, const char* format, float power)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	bool value_changed = false;
	BeginGroup();
	PushID(label);
	PushMultiItemsWidths(components);
	size_t type_size = GDataTypeInfo[data_type].Size;
	for (int i = 0; i < components; i++)
	{
		PushID(i);
		value_changed |= DragScalar("", data_type, v, v_speed, v_min, v_max, format, power);
		SameLine(0, g.Style.ItemInnerSpacing.x);
		PopID();
		PopItemWidth();
		v = (void*)((char*)v + type_size);
	}
	PopID();

	TextUnformatted(label, FindRenderedTextEnd(label));
	EndGroup();
	return value_changed;
}

bool ui::DragFloat(const char* label, float* v, float v_speed, float v_min, float v_max, const char* format, float power)
{
	return DragScalar(label, ImGuiDataType_Float, v, v_speed, &v_min, &v_max, format, power);
}

bool ui::DragFloat2(const char* label, float v[2], float v_speed, float v_min, float v_max, const char* format, float power)
{
	return DragScalarN(label, ImGuiDataType_Float, v, 2, v_speed, &v_min, &v_max, format, power);
}

bool ui::DragFloat3(const char* label, float v[3], float v_speed, float v_min, float v_max, const char* format, float power)
{
	return DragScalarN(label, ImGuiDataType_Float, v, 3, v_speed, &v_min, &v_max, format, power);
}

bool ui::DragFloat4(const char* label, float v[4], float v_speed, float v_min, float v_max, const char* format, float power)
{
	return DragScalarN(label, ImGuiDataType_Float, v, 4, v_speed, &v_min, &v_max, format, power);
}

bool ui::DragFloatRange2(const char* label, float* v_current_min, float* v_current_max, float v_speed, float v_min, float v_max, const char* format, const char* format_max, float power)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	PushID(label);
	BeginGroup();
	PushMultiItemsWidths(2);

	bool value_changed = DragFloat("##min", v_current_min, v_speed, (v_min >= v_max) ? -FLT_MAX : v_min, (v_min >= v_max) ? *v_current_max : ImMin(v_max, *v_current_max), format, power);
	PopItemWidth();
	SameLine(0, g.Style.ItemInnerSpacing.x);
	value_changed |= DragFloat("##max", v_current_max, v_speed, (v_min >= v_max) ? *v_current_min : ImMax(v_min, *v_current_min), (v_min >= v_max) ? FLT_MAX : v_max, format_max ? format_max : format, power);
	PopItemWidth();
	SameLine(0, g.Style.ItemInnerSpacing.x);

	TextUnformatted(label, FindRenderedTextEnd(label));
	EndGroup();
	PopID();
	return value_changed;
}

// NB: v_speed is float to allow adjusting the drag speed with more precision
bool ui::DragInt(const char* label, int* v, float v_speed, int v_min, int v_max, const char* format)
{
	return DragScalar(label, ImGuiDataType_S32, v, v_speed, &v_min, &v_max, format);
}

bool ui::DragInt2(const char* label, int v[2], float v_speed, int v_min, int v_max, const char* format)
{
	return DragScalarN(label, ImGuiDataType_S32, v, 2, v_speed, &v_min, &v_max, format);
}

bool ui::DragInt3(const char* label, int v[3], float v_speed, int v_min, int v_max, const char* format)
{
	return DragScalarN(label, ImGuiDataType_S32, v, 3, v_speed, &v_min, &v_max, format);
}

bool ui::DragInt4(const char* label, int v[4], float v_speed, int v_min, int v_max, const char* format)
{
	return DragScalarN(label, ImGuiDataType_S32, v, 4, v_speed, &v_min, &v_max, format);
}

bool ui::DragIntRange2(const char* label, int* v_current_min, int* v_current_max, float v_speed, int v_min, int v_max, const char* format, const char* format_max)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	PushID(label);
	BeginGroup();
	PushMultiItemsWidths(2);

	bool value_changed = DragInt("##min", v_current_min, v_speed, (v_min >= v_max) ? INT_MIN : v_min, (v_min >= v_max) ? *v_current_max : ImMin(v_max, *v_current_max), format);
	PopItemWidth();
	SameLine(0, g.Style.ItemInnerSpacing.x);
	value_changed |= DragInt("##max", v_current_max, v_speed, (v_min >= v_max) ? *v_current_min : ImMax(v_min, *v_current_min), (v_min >= v_max) ? INT_MAX : v_max, format_max ? format_max : format);
	PopItemWidth();
	SameLine(0, g.Style.ItemInnerSpacing.x);

	TextUnformatted(label, FindRenderedTextEnd(label));
	EndGroup();
	PopID();

	return value_changed;
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: SliderScalar, SliderFloat, SliderInt, etc.
//-------------------------------------------------------------------------
// - SliderBehaviorT<>() [Internal]
// - SliderBehavior() [Internal]
// - SliderScalar()
// - SliderScalarN()
// - SliderFloat()
// - SliderFloat2()
// - SliderFloat3()
// - SliderFloat4()
// - SliderAngle()
// - SliderInt()
// - SliderInt2()
// - SliderInt3()
// - SliderInt4()
// - VSliderScalar()
// - VSliderFloat()
// - VSliderInt()
//-------------------------------------------------------------------------

bool ImGui_ResetValue(ImGuiDataType data_type, void* v)
{
	switch (data_type)
	{
	case ImGuiDataType_S32:
		*(ImS32*)v = 0;
		return true;
	case ImGuiDataType_U32:
		*(ImU32*)v = 0;
		return true;
	case ImGuiDataType_S64:
		*(ImS64*)v = 0;
		return true;
	case ImGuiDataType_U64:
		*(ImU64*)v = 0;
		return true;
	case ImGuiDataType_Float:
		*(float*)v = 0;
		return true;
	case ImGuiDataType_Double:
		*(double*)v = 0;
		return true;
	case ImGuiDataType_COUNT: break;
	}
	IM_ASSERT(0);
	return false;
}

template<typename TYPE, typename FLOATTYPE>
float ui::SliderCalcRatioFromValueT(ImGuiDataType data_type, TYPE v, TYPE v_min, TYPE v_max, float power, float linear_zero_pos)
{
	if (v_min == v_max)
		return 0.0f;

	const bool is_power = (power != 1.0f) && (data_type == ImGuiDataType_Float || data_type == ImGuiDataType_Double);
	const TYPE v_clamped = (v_min < v_max) ? ImClamp(v, v_min, v_max) : ImClamp(v, v_max, v_min);
	if (is_power)
	{
		if (v_clamped < 0.0f)
		{
			const float f = 1.0f - (float)((v_clamped - v_min) / (ImMin((TYPE)0, v_max) - v_min));
			return (1.0f - ImPow(f, 1.0f / power)) * linear_zero_pos;
		}
		else
		{
			const float f = (float)((v_clamped - ImMax((TYPE)0, v_min)) / (v_max - ImMax((TYPE)0, v_min)));
			return linear_zero_pos + ImPow(f, 1.0f / power) * (1.0f - linear_zero_pos);
		}
	}

	// Linear slider
	return (float)((FLOATTYPE)(v_clamped - v_min) / (FLOATTYPE)(v_max - v_min));
}

// FIXME: Move some of the code into SliderBehavior(). Current responsability is larger than what the equivalent DragBehaviorT<> does, we also do some rendering, etc.
template<typename TYPE, typename SIGNEDTYPE, typename FLOATTYPE>
bool ui::SliderBehaviorT(const ImRect& bb, ImGuiID id, ImGuiDataType data_type, TYPE* v, const TYPE v_min, const TYPE v_max, const char* format, float power, ImGuiSliderFlags flags, ImRect* out_grab_bb, int* percent)
{
	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	*v = ImClamp(*v, v_min, v_max);

	const ImGuiAxis axis = (flags & ImGuiSliderFlags_Vertical) ? ImGuiAxis_Y : ImGuiAxis_X;
	const bool is_decimal = (data_type == ImGuiDataType_Float) || (data_type == ImGuiDataType_Double);
	const bool is_power = (power != 1.0f) && is_decimal;

	const float grab_padding = 2.0f;
	const float slider_sz = (bb.Max[axis] - bb.Min[axis]) - grab_padding * 2.0f;
	float grab_sz = style.GrabMinSize;
	SIGNEDTYPE v_range = (v_min < v_max ? v_max - v_min : v_min - v_max);
	if (!is_decimal && v_range >= 0)                                             // v_range < 0 may happen on integer overflows
		grab_sz = ImMax((float)(slider_sz / (v_range + 1)), style.GrabMinSize);  // For integer sliders: if possible have the grab size represent 1 unit
	grab_sz = ImMin(grab_sz, slider_sz);
	const float slider_usable_sz = slider_sz - grab_sz;
	const float slider_usable_pos_min = bb.Min[axis] + grab_padding + grab_sz * 0.5f;
	const float slider_usable_pos_max = bb.Max[axis] - grab_padding - grab_sz * 0.5f;

	// For power curve sliders that cross over sign boundary we want the curve to be symmetric around 0.0f
	float linear_zero_pos;   // 0.0->1.0f
	if (is_power && v_min * v_max < 0.0f)
	{
		// Different sign
		const FLOATTYPE linear_dist_min_to_0 = ImPow(v_min >= 0 ? (FLOATTYPE)v_min : -(FLOATTYPE)v_min, (FLOATTYPE)1.0f / power);
		const FLOATTYPE linear_dist_max_to_0 = ImPow(v_max >= 0 ? (FLOATTYPE)v_max : -(FLOATTYPE)v_max, (FLOATTYPE)1.0f / power);
		linear_zero_pos = (float)(linear_dist_min_to_0 / (linear_dist_min_to_0 + linear_dist_max_to_0));
	}
	else
	{
		// Same sign
		linear_zero_pos = v_min < 0.0f ? 1.0f : 0.0f;
	}

	// Process interacting with the slider
	bool value_changed = false;
	if (g.ActiveId == id)
	{
		bool set_new_value = false;
		float clicked_t = 0.0f;

		if (g.ActiveIdSource == ImGuiInputSource_Mouse)
		{
			if (!g.IO.MouseDown[0])
			{
				ClearActiveID();
			}
			else
			{
				const float mouse_abs_pos = g.IO.MousePos[axis];
				clicked_t = (slider_usable_sz > 0.0f) ? ImClamp((mouse_abs_pos - slider_usable_pos_min) / slider_usable_sz, 0.0f, 1.0f) : 0.0f;
				if (axis == ImGuiAxis_Y)
					clicked_t = 1.0f - clicked_t;
				set_new_value = true;
			}
		}
		//else if (g.ActiveIdSource == ImGuiInputSource_Nav)
		//{
		//    const ImVec2 delta2 = GetNavInputAmount2d(ImGuiNavDirSourceFlags_Keyboard | ImGuiNavDirSourceFlags_PadDPad, ImGuiInputReadMode_RepeatFast, 0.0f, 0.0f);
		//    float delta = (axis == ImGuiAxis_X) ? delta2.x : -delta2.y;
		//    if (g.NavActivatePressedId == id && !g.ActiveIdIsJustActivated)
		//    {
		//        ClearActiveID();
		//    }
		//    else if (delta != 0.0f)
		//    {
		//        clicked_t = SliderCalcRatioFromValueT<TYPE,FLOATTYPE>(data_type, *v, v_min, v_max, power, linear_zero_pos);
		//        const int decimal_precision = is_decimal ? ImParseFormatPrecision(format, 3) : 0;
		//        if ((decimal_precision > 0) || is_power)
		//        {
		//            delta /= 100.0f;    // Gamepad/keyboard tweak speeds in % of slider bounds
		//            if (IsNavInputDown(ImGuiNavInput_TweakSlow))
		//                delta /= 10.0f;
		//        }
		//        else
		//        {
		//            if ((v_range >= -100.0f && v_range <= 100.0f) || IsNavInputDown(ImGuiNavInput_TweakSlow))
		//                delta = ((delta < 0.0f) ? -1.0f : +1.0f) / (float)v_range; // Gamepad/keyboard tweak speeds in integer steps
		//            else
		//                delta /= 100.0f;
		//        }
		//        if (IsNavInputDown(ImGuiNavInput_TweakFast))
		//            delta *= 10.0f;
		//        set_new_value = true;
		//        if ((clicked_t >= 1.0f && delta > 0.0f) || (clicked_t <= 0.0f && delta < 0.0f)) // This is to avoid applying the saturation when already past the limits
		//            set_new_value = false;
		//        else
		//            clicked_t = ImSaturate(clicked_t + delta);
		//    }
		//}

		if (set_new_value)
		{
			TYPE v_new;
			if (is_power)
			{
				// Account for power curve scale on both sides of the zero
				if (clicked_t < linear_zero_pos)
				{
					// Negative: rescale to the negative range before powering
					float a = 1.0f - (clicked_t / linear_zero_pos);
					a = ImPow(a, power);
					v_new = ImLerp(ImMin(v_max, (TYPE)0), v_min, a);
				}
				else
				{
					// Positive: rescale to the positive range before powering
					float a;
					if (ImFabs(linear_zero_pos - 1.0f) > 1.e-6f)
						a = (clicked_t - linear_zero_pos) / (1.0f - linear_zero_pos);
					else
						a = clicked_t;
					a = ImPow(a, power);
					v_new = ImLerp(ImMax(v_min, (TYPE)0), v_max, a);
				}
			}
			else
			{
				// Linear slider
				if (is_decimal)
				{
					v_new = ImLerp(v_min, v_max, clicked_t);
				}
				else
				{
					// For integer values we want the clicking position to match the grab box so we round above
					// This code is carefully tuned to work with large values (e.g. high ranges of U64) while preserving this property..
					FLOATTYPE v_new_off_f = (v_max - v_min) * clicked_t;
					TYPE v_new_off_floor = (TYPE)(v_new_off_f);
					TYPE v_new_off_round = (TYPE)(v_new_off_f + (FLOATTYPE)0.5);
					if (!is_decimal && v_new_off_floor < v_new_off_round)
						v_new = v_min + v_new_off_round;
					else
						v_new = v_min + v_new_off_floor;
				}
			}

			// Round to user desired precision based on format string
			v_new = RoundScalarWithFormatT<TYPE, SIGNEDTYPE>(format, data_type, v_new);

			// Apply result
			if (*v != v_new)
			{
				*v = v_new;
				value_changed = true;
			}
		}
	}

	*percent = floorf((float)(*v - v_min) / (float)(v_max - v_min) * 100.f);

	// Output grab position so it can be displayed by the caller
	float grab_t = SliderCalcRatioFromValueT<TYPE, FLOATTYPE>(data_type, *v, v_min, v_max, power, linear_zero_pos);
	if (axis == ImGuiAxis_Y)
		grab_t = 1.0f - grab_t;
	const float grab_pos = ImLerp(slider_usable_pos_min, slider_usable_pos_max, grab_t);
	if (axis == ImGuiAxis_X)
		*out_grab_bb = ImRect(grab_pos - grab_sz * 0.5f, bb.Min.y + grab_padding, grab_pos + grab_sz * 0.5f, bb.Max.y - grab_padding);
	else
		*out_grab_bb = ImRect(bb.Min.x + grab_padding, grab_pos - grab_sz * 0.5f, bb.Max.x - grab_padding, grab_pos + grab_sz * 0.5f);

	return value_changed;
}

// For 32-bits and larger types, slider bounds are limited to half the natural type range.
// So e.g. an integer Slider between INT_MAX-10 and INT_MAX will fail, but an integer Slider between INT_MAX/2-10 and INT_MAX/2 will be ok.
// It would be possible to lift that limitation with some work but it doesn't seem to be worth it for sliders.
bool ui::SliderBehavior(const ImRect& bb, ImGuiID id, ImGuiDataType data_type, void* v, const void* v_min, const void* v_max, const char* format, float power, ImGuiSliderFlags flags, ImRect* out_grab_bb, int* percent)
{
	switch (data_type)
	{
	case ImGuiDataType_S32:
		IM_ASSERT(*(const ImS32*)v_min >= IM_S32_MIN / 2 && *(const ImS32*)v_max <= IM_S32_MAX / 2);
		return SliderBehaviorT<ImS32, ImS32, float >(bb, id, data_type, (ImS32*)v, *(const ImS32*)v_min, *(const ImS32*)v_max, format, power, flags, out_grab_bb, percent);
	case ImGuiDataType_U32:
		IM_ASSERT(*(const ImU32*)v_min <= IM_U32_MAX / 2);
		return SliderBehaviorT<ImU32, ImS32, float >(bb, id, data_type, (ImU32*)v, *(const ImU32*)v_min, *(const ImU32*)v_max, format, power, flags, out_grab_bb, percent);
	case ImGuiDataType_S64:
		IM_ASSERT(*(const ImS64*)v_min >= IM_S64_MIN / 2 && *(const ImS64*)v_max <= IM_S64_MAX / 2);
		return SliderBehaviorT<ImS64, ImS64, double>(bb, id, data_type, (ImS64*)v, *(const ImS64*)v_min, *(const ImS64*)v_max, format, power, flags, out_grab_bb, percent);
	case ImGuiDataType_U64:
		IM_ASSERT(*(const ImU64*)v_min <= IM_U64_MAX / 2);
		return SliderBehaviorT<ImU64, ImS64, double>(bb, id, data_type, (ImU64*)v, *(const ImU64*)v_min, *(const ImU64*)v_max, format, power, flags, out_grab_bb, percent);
	case ImGuiDataType_Float:
		IM_ASSERT(*(const float*)v_min >= -FLT_MAX / 2.0f && *(const float*)v_max <= FLT_MAX / 2.0f);
		return SliderBehaviorT<float, float, float >(bb, id, data_type, (float*)v, *(const float*)v_min, *(const float*)v_max, format, power, flags, out_grab_bb, percent);
	case ImGuiDataType_Double:
		IM_ASSERT(*(const double*)v_min >= -DBL_MAX / 2.0f && *(const double*)v_max <= DBL_MAX / 2.0f);
		return SliderBehaviorT<double, double, double>(bb, id, data_type, (double*)v, *(const double*)v_min, *(const double*)v_max, format, power, flags, out_grab_bb, percent);
	case ImGuiDataType_COUNT: break;
	}
	IM_ASSERT(0);
	return false;
}

bool ui::SingleSelect(const char* label, int* current_item, std::vector<const char*> items) {
	*current_item = ImClamp(*current_item, 0, int(items.size() - 1));

	int old_item = *current_item;

	if (ui::BeginCombo(label, items.at(*current_item), 0, items.size())) {
		for (int i = 0; i < items.size(); i++)
			if (ui::Selectable(items.at(i), *current_item == i))
				*current_item = i;

		ui::EndCombo();
	}

	return old_item != *current_item;
}


static auto vector_getter = [](void* vec, int idx, const char** out_text)
{
	auto& vector = *static_cast<std::vector<std::string>*>(vec);
	if (idx < 0 || idx >= static_cast<int>(vector.size())) { return false; }
	*out_text = vector.at(idx).c_str();
	return true;
};
bool ui::ListBoxConfigArray(const char* label, int* currIndex, std::vector<std::string>& values, int height)
{
	return ui::ListBox(label, currIndex, vector_getter,
		static_cast<void*>(&values), values.size(), height);
}

bool ui::ListBoxConfigArraySkeet(const char* label, int* currIndex, std::vector<std::string>& values, int height)
{
	return ui::ListBoxSkeet(label, currIndex, vector_getter, static_cast<void*>(&values), values.size(), height);
}

bool ui::SingleSelect(const char* label, int* current_item, bool(*items_getter)(void* data, int idx, const char** out_text), int items_count) {

	const char* preview_value = NULL;
	if (*current_item >= 0 && *current_item < items_count)
		items_getter(nullptr, *current_item, &preview_value);
	*current_item = ImClamp(*current_item, 0, int(items_count - 1));
	if (!ui::BeginCombo(label, preview_value, 0, items_count))
		return false;

	bool value_changed = false;
	for (int i = 0; i < items_count; i++)
	{

		const bool item_selected = (i == *current_item);
		const char* item_text;
		if (!items_getter(nullptr, i, &item_text))
			item_text = "*Unknown item*";
		if (Selectable(item_text, item_selected))
		{
			value_changed = true;
			*current_item = i;
		}


	}
	ui::EndCombo();
	int old_item = *current_item;
	
	return value_changed;
}

bool ui::SingleSelectOT(const char* label, int* current_item, bool(*items_getter)(void* data, int idx, const char** out_text), int items_count) {

	const char* preview_value = NULL;
	if (*current_item >= 0 && *current_item < items_count)
		items_getter(nullptr, *current_item, &preview_value);
	*current_item = ImClamp(*current_item, 0, int(items_count - 1));
	if (!ui::BeginComboOT(label, preview_value, 0, items_count))
		return false;

	bool value_changed = false;
	for (int i = 0; i < items_count; i++)
	{

		const bool item_selected = (i == *current_item);
		const char* item_text;
		if (!items_getter(nullptr, i, &item_text))
			item_text = "*Unknown item*";
		if (SelectableOT(item_text, item_selected))
		{
			value_changed = true;
			*current_item = i;
		}


	}
	ui::EndComboOT();
	int old_item = *current_item;

	return value_changed;
}

bool ui::SingleSelectSkeet(const char* label, int* current_item, bool(*items_getter)(void* data, int idx, const char** out_text), int items_count) {

	const char* preview_value = NULL;
	if (*current_item >= 0 && *current_item < items_count)
		items_getter(nullptr, *current_item, &preview_value);
	*current_item = ImClamp(*current_item, 0, int(items_count - 1));
	if (!ui::BeginComboSkeet(label, preview_value, 0, items_count))
		return false;

	bool value_changed = false;
	for (int i = 0; i < items_count; i++)
	{

		const bool item_selected = (i == *current_item);
		const char* item_text;
		if (!items_getter(nullptr, i, &item_text))
			item_text = "*Unknown item*";
		if (SelectableSkeet(item_text, item_selected))
		{
			value_changed = true;
			*current_item = i;
		}


	}
	ui::EndComboSkeet();
	int old_item = *current_item;

	return value_changed;
}


#include <string>
bool ui::MultiSelect(const char* label, std::vector<int> data, std::vector<const char*> items) {
	std::vector<int> old_data = data;

	static auto howmuchsel = [](std::vector<int> e) -> int {
		int s = 0;
		for (int i = 0; i < e.size(); i++)
			if (e[i])
				s++;


		return s;
	};
	if (ui::BeginCombo(label, (howmuchsel(data) > 0 ? std::to_string(howmuchsel(data)) + " selected" : "None").c_str(), 0, items.size())) {
		for (int i = 0; i < items.size(); i++)
			ui::Selectable(items.at(i), &(data)[i], ImGuiSelectableFlags_DontClosePopups);
		ui::EndCombo();
	}

	return old_data != data;
}

bool ui::MultiSelect(const char* label, std::unordered_map<int, bool>* data, std::vector<const char*> items) {
	std::unordered_map<int, bool> old_data = *data;

	static auto howmuchsel = [](std::unordered_map<int, bool> e) -> int {
		int s = 0;
		for (int i = 0; i < e.size(); i++)
			if (e[i])
				s++;


		return s;
	};
	if (ui::BeginCombo(label, (howmuchsel(*data) > 0 ? std::to_string(howmuchsel(*data)) + " selected" : "None").c_str(), 0, items.size())) {
		for (int i = 0; i < items.size(); i++)
			ui::Selectable(items.at(i), &(*data)[i], ImGuiSelectableFlags_DontClosePopups);
		ui::EndCombo();
	}

	return old_data != *data;
}

bool ui::SliderScalar(const char* label, ImGuiDataType data_type, void* v, const void* v_min, const void* v_max, const char* format, float power, int remove_from_fmt)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const float w = ImClamp(window->Size.x - 64, 252.f, 297.f);

	const ImVec2 label_size = CalcTextSize(label, NULL, true);
	const ImRect frame_bb(window->DC.CursorPos + ImVec2(window->Size.x - 140, 0), window->DC.CursorPos + ImVec2(window->Size.x - 40, label_size.x > 0 ? label_size.y : 2));
	const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id, &frame_bb))
		return false;

	// Default format string when passing NULL
	// Patch old "%.0f" format string to use "%d", read function comments for more details.
	IM_ASSERT(data_type >= 0 && data_type < ImGuiDataType_COUNT);
	if (format == NULL)
		format = GDataTypeInfo[data_type].PrintFmt;
	else if (data_type == ImGuiDataType_S32 && strcmp(format, "%d") != 0)
		format = PatchFormatStringFloatToInt(format);

	// Tabbing or CTRL-clicking on Slider turns it into an input box
	bool start_text_input = false;
	const bool tab_focus_requested = FocusableItemRegister(window, id);
	const bool hovered = ItemHoverable(frame_bb, id);
	if (tab_focus_requested || (hovered && g.IO.MouseClicked[0]) || g.NavActivateId == id || (g.NavInputId == id && g.ScalarAsInputTextId != id))
	{
		SetActiveID(id, window);
		SetFocusID(id, window);
		FocusWindow(window);
		g.ActiveIdAllowNavDirFlags = (1 << ImGuiDir_Up) | (1 << ImGuiDir_Down);
	}

	if (std::string(label).at(0) != '#' && std::string(label).at(1) != '#')
		window->DrawList->AddText(frame_bb.Min - ImVec2(149 - 41/*shonax*/, 0), ImColor(182 / 255.f, 178 / 255.f, 180 / 255.f, g.Style.Alpha), label);

	ImRect slider_bb = ImRect(frame_bb.Min + ImVec2(0, label_size.x > 0 ? label_size.y - 8 : 8), frame_bb.Max - ImVec2(0, 1));

	if (IsItemHovered())
		window->DrawList->AddRectFilledMultiColor(slider_bb.Min - ImVec2(0, 2), slider_bb.Max + ImVec2(0, 2),
			ImColor(27 / 255.f, 27 / 255.f, 27 / 255.f, g.Style.Alpha),
			ImColor(27 / 255.f, 27 / 255.f, 27 / 255.f, g.Style.Alpha),
			ImColor(57 / 255.f, 57 / 255.f, 57 / 255.f, g.Style.Alpha),
			ImColor(57 / 255.f, 57 / 255.f, 57 / 255.f, g.Style.Alpha));
	else
		window->DrawList->AddRectFilledMultiColor(slider_bb.Min - ImVec2(0, 2), slider_bb.Max + ImVec2(0, 2),
			ImColor(17 / 255.f, 17 / 255.f, 17 / 255.f, g.Style.Alpha),
			ImColor(17 / 255.f, 17 / 255.f, 17 / 255.f, g.Style.Alpha),
			ImColor(47 / 255.f, 47 / 255.f, 47 / 255.f, g.Style.Alpha),
			ImColor(47 / 255.f, 47 / 255.f, 47 / 255.f, g.Style.Alpha));

	int percent = 0;
	ImRect grab_bb;
	const bool value_changed = SliderBehavior(frame_bb, id, data_type, v, v_min, v_max, format, power, ImGuiSliderFlags_None, &grab_bb, &percent);
	if (value_changed)
		MarkItemEdited(id);

	ImColor color1 = ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, ImClamp(0 / 255.f, 0.f, g.Style.Alpha));
	ImColor color2 = ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, ImClamp(0 / 255.f, 0.f, g.Style.Alpha));

	ImColor gradient1 = ImColor(75, 50, 255);
	ImColor gradient2 = ImColor(75, 50, 255);


	window->DrawList->AddRectFilledMultiColor(slider_bb.Min - ImVec2(0, 2), slider_bb.Max + ImVec2(0, 2), color1, color2, color2, color1);
	window->DrawList->AddRectFilledMultiColor(slider_bb.Min - ImVec2(0, 2), ImVec2(slider_bb.Min.x + floorf((float)percent / 100.f * (slider_bb.Max.x - slider_bb.Min.x)), slider_bb.Max.y + 2), gradient1, gradient1, gradient2, gradient2);
	window->DrawList->AddRectFilledMultiColor(slider_bb.Min - ImVec2(0, 2), ImVec2(slider_bb.Min.x + floorf((float)percent / 100.f * (slider_bb.Max.x - slider_bb.Min.x)), slider_bb.Max.y + 2), color1, color2, color2, color1);
	window->DrawList->AddRect(slider_bb.Min - ImVec2(0, 2), slider_bb.Max + ImVec2(0, 2), ImColor(47 / 255.f, 47 / 255.f, 47 / 255.f, g.Style.Alpha));

	window->DrawList->AddRectFilledMultiColor(slider_bb.Min - ImVec2(0, 2), ImVec2(slider_bb.Min.x + floorf((float)percent / 100.f * (slider_bb.Max.x - slider_bb.Min.x)), slider_bb.Max.y + 2),
		ImColor(17 / 255.f, 17 / 255.f, 17 / 255.f, g.Style.Alpha),
		ImColor(17 / 255.f, 17 / 255.f, 17 / 255.f, g.Style.Alpha),
		ImColor(75 / 255.f, 50 / 255.f, 255 / 255.f, g.Style.Alpha),
		ImColor(75 / 255.f, 50 / 255.f, 255 / 255.f, g.Style.Alpha));

	if (data_type == ImGuiDataType_S32)
		*(int*)v -= remove_from_fmt;

	char value_buf[64];
	const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, v, format);
	auto textSize = CalcTextSize(value_buf);

	if (data_type == ImGuiDataType_S32)
		*(int*)v += remove_from_fmt;

	PushFont(GetIO().Fonts->Fonts[0]);
	window->DrawList->AddText(ImVec2(slider_bb.Min.x + 50 - textSize.y / 2, slider_bb.Max.y - textSize.y / 2 - 4), ImColor(182 / 255.f, 178 / 255.f, 180 / 255.f, g.Style.Alpha), value_buf);
	PopFont();

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags);
	return value_changed;
}

// Add multiple sliders on 1 line for compact edition of multiple components
bool ui::SliderScalarN(const char* label, ImGuiDataType data_type, void* v, int components, const void* v_min, const void* v_max, const char* format, float power)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	bool value_changed = false;
	BeginGroup();
	PushID(label);
	PushMultiItemsWidths(components);
	size_t type_size = GDataTypeInfo[data_type].Size;
	for (int i = 0; i < components; i++)
	{
		PushID(i);
		value_changed |= SliderScalar("", data_type, v, v_min, v_max, format, power);
		SameLine(0, g.Style.ItemInnerSpacing.x);
		PopID();
		PopItemWidth();
		v = (void*)((char*)v + type_size);
	}
	PopID();

	TextUnformatted(label, FindRenderedTextEnd(label));
	EndGroup();
	return value_changed;
}

bool ui::SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, float power)
{
	return SliderScalar(label, ImGuiDataType_Float, v, &v_min, &v_max, format, power);
}

bool ui::SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* format, float power)
{
	return SliderScalarN(label, ImGuiDataType_Float, v, 2, &v_min, &v_max, format, power);
}

bool ui::SliderFloat3(const char* label, float v[3], float v_min, float v_max, const char* format, float power)
{
	return SliderScalarN(label, ImGuiDataType_Float, v, 3, &v_min, &v_max, format, power);
}

bool ui::SliderFloat4(const char* label, float v[4], float v_min, float v_max, const char* format, float power)
{
	return SliderScalarN(label, ImGuiDataType_Float, v, 4, &v_min, &v_max, format, power);
}

bool ui::SliderAngle(const char* label, float* v_rad, float v_degrees_min, float v_degrees_max, const char* format)
{
	if (format == NULL)
		format = "%.0f deg";
	float v_deg = (*v_rad) * 360.0f / (2 * IM_PI);
	bool value_changed = SliderFloat(label, &v_deg, v_degrees_min, v_degrees_max, format, 1.0f);
	*v_rad = v_deg * (2 * IM_PI) / 360.0f;
	return value_changed;
}

bool ui::SliderIntSkeet(const char* label, int* v, int v_min, int v_max, const char* format, int remove_from_fmt)
{
	return SliderScalarSkeet(label, ImGuiDataType_S32, v, &v_min, &v_max, format, 1.f, remove_from_fmt);
}


bool ui::SliderInt(const char* label, int* v, int v_min, int v_max, const char* format, int remove_from_fmt)
{
	return SliderScalar(label, ImGuiDataType_S32, v, &v_min, &v_max, format, 1.f, remove_from_fmt);
}

bool ui::SliderInt2(const char* label, int v[2], int v_min, int v_max, const char* format)
{
	return SliderScalarN(label, ImGuiDataType_S32, v, 2, &v_min, &v_max, format);
}

bool ui::SliderInt3(const char* label, int v[3], int v_min, int v_max, const char* format)
{
	return SliderScalarN(label, ImGuiDataType_S32, v, 3, &v_min, &v_max, format);
}

bool ui::SliderInt4(const char* label, int v[4], int v_min, int v_max, const char* format)
{
	return SliderScalarN(label, ImGuiDataType_S32, v, 4, &v_min, &v_max, format);
}

bool ui::VSliderScalar(const char* label, const ImVec2& size, ImGuiDataType data_type, void* v, const void* v_min, const void* v_max, const char* format, float power)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);

	const ImVec2 label_size = CalcTextSize(label, NULL, true);
	const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + size);
	const ImRect bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

	ItemSize(bb, style.FramePadding.y);
	if (!ItemAdd(frame_bb, id))
		return false;

	// Default format string when passing NULL
	// Patch old "%.0f" format string to use "%d", read function comments for more details.
	IM_ASSERT(data_type >= 0 && data_type < ImGuiDataType_COUNT);
	if (format == NULL)
		format = GDataTypeInfo[data_type].PrintFmt;
	else if (data_type == ImGuiDataType_S32 && strcmp(format, "%d") != 0)
		format = PatchFormatStringFloatToInt(format);

	const bool hovered = ItemHoverable(frame_bb, id);
	if ((hovered && g.IO.MouseClicked[0]) || g.NavActivateId == id || g.NavInputId == id)
	{
		SetActiveID(id, window);
		SetFocusID(id, window);
		FocusWindow(window);
		g.ActiveIdAllowNavDirFlags = (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
	}

	// Draw frame
	const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : g.HoveredId == id ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
	RenderNavHighlight(frame_bb, id);
	RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, g.Style.FrameRounding);

	// Slider behavior
	int percent = 0;
	ImRect grab_bb;
	const bool value_changed = SliderBehavior(frame_bb, id, data_type, v, v_min, v_max, format, power, ImGuiSliderFlags_Vertical, &grab_bb, &percent);
	if (value_changed)
		MarkItemEdited(id);

	// Render grab
	window->DrawList->AddRectFilled(grab_bb.Min, grab_bb.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);

	// Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
	// For the vertical slider we allow centered text to overlap the frame padding
	char value_buf[64];
	const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, v, format);
	RenderTextClipped(ImVec2(frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y), frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.0f));
	if (label_size.x > 0.0f)
		RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

	return value_changed;
}

bool ui::VSliderFloat(const char* label, const ImVec2& size, float* v, float v_min, float v_max, const char* format, float power)
{
	return VSliderScalar(label, size, ImGuiDataType_Float, v, &v_min, &v_max, format, power);
}

bool ui::VSliderInt(const char* label, const ImVec2& size, int* v, int v_min, int v_max, const char* format)
{
	return VSliderScalar(label, size, ImGuiDataType_S32, v, &v_min, &v_max, format);
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: InputScalar, InputFloat, InputInt, etc.
//-------------------------------------------------------------------------
// - ImParseFormatFindStart() [Internal]
// - ImParseFormatFindEnd() [Internal]
// - ImParseFormatTrimDecorations() [Internal]
// - ImParseFormatPrecision() [Internal]
// - InputScalarAsWidgetReplacement() [Internal]
// - InputScalar()
// - InputScalarN()
// - InputFloat()
// - InputFloat2()
// - InputFloat3()
// - InputFloat4()
// - InputInt()
// - InputInt2()
// - InputInt3()
// - InputInt4()
// - InputDouble()
//-------------------------------------------------------------------------

// We don't use strchr() because our strings are usually very short and often start with '%'
const char* ImParseFormatFindStart(const char* fmt)
{
	while (char c = fmt[0])
	{
		if (c == '%' && fmt[1] != '%')
			return fmt;
		else if (c == '%')
			fmt++;
		fmt++;
	}
	return fmt;
}

const char* ImParseFormatFindEnd(const char* fmt)
{
	// Printf/scanf types modifiers: I/L/h/j/l/t/w/z. Other uppercase letters qualify as types aka end of the format.
	if (fmt[0] != '%')
		return fmt;
	const unsigned int ignored_uppercase_mask = (1 << ('I' - 'A')) | (1 << ('L' - 'A'));
	const unsigned int ignored_lowercase_mask = (1 << ('h' - 'a')) | (1 << ('j' - 'a')) | (1 << ('l' - 'a')) | (1 << ('t' - 'a')) | (1 << ('w' - 'a')) | (1 << ('z' - 'a'));
	for (char c; (c = *fmt) != 0; fmt++)
	{
		if (c >= 'A' && c <= 'Z' && ((1 << (c - 'A')) & ignored_uppercase_mask) == 0)
			return fmt + 1;
		if (c >= 'a' && c <= 'z' && ((1 << (c - 'a')) & ignored_lowercase_mask) == 0)
			return fmt + 1;
	}
	return fmt;
}

// Extract the format out of a format string with leading or trailing decorations
//  fmt = "blah blah"  -> return fmt
//  fmt = "%.3f"       -> return fmt
//  fmt = "hello %.3f" -> return fmt + 6
//  fmt = "%.3f hello" -> return buf written with "%.3f"
const char* ImParseFormatTrimDecorations(const char* fmt, char* buf, size_t buf_size)
{
	const char* fmt_start = ImParseFormatFindStart(fmt);
	if (fmt_start[0] != '%')
		return fmt;
	const char* fmt_end = ImParseFormatFindEnd(fmt_start);
	if (fmt_end[0] == 0) // If we only have leading decoration, we don't need to copy the data.
		return fmt_start;
	ImStrncpy(buf, fmt_start, ImMin((size_t)(fmt_end - fmt_start) + 1, buf_size));
	return buf;
}

// Parse display precision back from the display format string
// FIXME: This is still used by some navigation code path to infer a minimum tweak step, but we should aim to rework widgets so it isn't needed.
int ImParseFormatPrecision(const char* fmt, int default_precision)
{
	fmt = ImParseFormatFindStart(fmt);
	if (fmt[0] != '%')
		return default_precision;
	fmt++;
	while (*fmt >= '0' && *fmt <= '9')
		fmt++;
	int precision = INT_MAX;
	if (*fmt == '.')
	{
		fmt = ImAtoi<int>(fmt + 1, &precision);
		if (precision < 0 || precision > 99)
			precision = default_precision;
	}
	if (*fmt == 'e' || *fmt == 'E') // Maximum precision with scientific notation
		precision = -1;
	if ((*fmt == 'g' || *fmt == 'G') && precision == INT_MAX)
		precision = -1;
	return (precision == INT_MAX) ? default_precision : precision;
}

// Create text input in place of an active drag/slider (used when doing a CTRL+Click on drag/slider widgets)
// FIXME: Facilitate using this in variety of other situations.
bool ui::InputScalarAsWidgetReplacement(const ImRect& bb, ImGuiID id, const char* label, ImGuiDataType data_type, void* data_ptr, const char* format)
{
	ImGuiContext& g = *GImGui;

	// On the first frame, g.ScalarAsInputTextId == 0, then on subsequent frames it becomes == id.
	// We clear ActiveID on the first frame to allow the InputText() taking it back.
	if (g.ScalarAsInputTextId == 0)
		ClearActiveID();

	char fmt_buf[32];
	char data_buf[32];
	format = ImParseFormatTrimDecorations(format, fmt_buf, IM_ARRAYSIZE(fmt_buf));
	DataTypeFormatString(data_buf, IM_ARRAYSIZE(data_buf), data_type, data_ptr, format);
	ImStrTrimBlanks(data_buf);
	ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll | ((data_type == ImGuiDataType_Float || data_type == ImGuiDataType_Double) ? ImGuiInputTextFlags_CharsScientific : ImGuiInputTextFlags_CharsDecimal);
	bool value_changed = InputTextEx(label, data_buf, IM_ARRAYSIZE(data_buf), bb.GetSize(), flags);
	if (g.ScalarAsInputTextId == 0)
	{
		// First frame we started displaying the InputText widget, we expect it to take the active id.
		IM_ASSERT(g.ActiveId == id);
		g.ScalarAsInputTextId = g.ActiveId;
	}
	if (value_changed)
		return DataTypeApplyOpFromText(data_buf, g.InputTextState.InitialText.Data, data_type, data_ptr, NULL);
	return false;
}

bool ui::InputScalar(const char* label, ImGuiDataType data_type, void* data_ptr, const void* step, const void* step_fast, const char* format, ImGuiInputTextFlags flags)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	IM_ASSERT(data_type >= 0 && data_type < ImGuiDataType_COUNT);
	if (format == NULL)
		format = GDataTypeInfo[data_type].PrintFmt;

	char buf[64];
	DataTypeFormatString(buf, IM_ARRAYSIZE(buf), data_type, data_ptr, format);

	bool value_changed = false;
	if ((flags & (ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsScientific)) == 0)
		flags |= ImGuiInputTextFlags_CharsDecimal;
	flags |= ImGuiInputTextFlags_AutoSelectAll;

	if (step != NULL)
	{
		const float button_size = GetFrameHeight();

		BeginGroup(); // The only purpose of the group here is to allow the caller to query item data e.g. IsItemActive()
		PushID(label);
		PushItemWidth(ImMax(1.0f, CalcItemWidth() - (button_size + style.ItemInnerSpacing.x) * 2));
		if (InputText("", buf, IM_ARRAYSIZE(buf), flags)) // PushId(label) + "" gives us the expected ID from outside point of view
			value_changed = DataTypeApplyOpFromText(buf, g.InputTextState.InitialText.Data, data_type, data_ptr, format);
		PopItemWidth();

		// Step buttons
		ImGuiButtonFlags button_flags = ImGuiButtonFlags_Repeat | ImGuiButtonFlags_DontClosePopups;
		if (flags & ImGuiInputTextFlags_ReadOnly)
			button_flags |= ImGuiButtonFlags_Disabled;
		//SameLine(0, style.ItemInnerSpacing.x);
		//if (ButtonEx("-", ImVec2(button_size, button_size), button_flags))
		//{
		//	DataTypeApplyOp(data_type, '-', data_ptr, data_ptr, g.IO.KeyCtrl && step_fast ? step_fast : step);
		//	value_changed = true;
		//}
		//SameLine(0, style.ItemInnerSpacing.x);
		//if (ButtonEx("+", ImVec2(button_size, button_size), button_flags))
		//{
		//	DataTypeApplyOp(data_type, '+', data_ptr, data_ptr, g.IO.KeyCtrl && step_fast ? step_fast : step);
		//	value_changed = true;
		//}
		//SameLine(0, style.ItemInnerSpacing.x);
		//TextUnformatted(label, FindRenderedTextEnd(label));

		PopID();
		EndGroup();
	}
	else
	{
		if (InputText(label, buf, IM_ARRAYSIZE(buf), flags))
			value_changed = DataTypeApplyOpFromText(buf, g.InputTextState.InitialText.Data, data_type, data_ptr, format);
	}

	return value_changed;
}

bool ui::InputScalarN(const char* label, ImGuiDataType data_type, void* v, int components, const void* step, const void* step_fast, const char* format, ImGuiInputTextFlags flags)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	bool value_changed = false;
	BeginGroup();
	PushID(label);
	PushMultiItemsWidths(components);
	size_t type_size = GDataTypeInfo[data_type].Size;
	for (int i = 0; i < components; i++)
	{
		PushID(i);
		value_changed |= InputScalar("", data_type, v, step, step_fast, format, flags);
		SameLine(0, g.Style.ItemInnerSpacing.x);
		PopID();
		PopItemWidth();
		v = (void*)((char*)v + type_size);
	}
	PopID();

	TextUnformatted(label, FindRenderedTextEnd(label));
	EndGroup();
	return value_changed;
}

bool ui::InputFloat(const char* label, float* v, float step, float step_fast, const char* format, ImGuiInputTextFlags flags)
{
	flags |= ImGuiInputTextFlags_CharsScientific;
	return InputScalar(label, ImGuiDataType_Float, (void*)v, (void*)(step > 0.0f ? &step : NULL), (void*)(step_fast > 0.0f ? &step_fast : NULL), format, flags);
}

bool ui::InputFloat2(const char* label, float v[2], const char* format, ImGuiInputTextFlags flags)
{
	return InputScalarN(label, ImGuiDataType_Float, v, 2, NULL, NULL, format, flags);
}

bool ui::InputFloat3(const char* label, float v[3], const char* format, ImGuiInputTextFlags flags)
{
	return InputScalarN(label, ImGuiDataType_Float, v, 3, NULL, NULL, format, flags);
}

bool ui::InputFloat4(const char* label, float v[4], const char* format, ImGuiInputTextFlags flags)
{
	return InputScalarN(label, ImGuiDataType_Float, v, 4, NULL, NULL, format, flags);
}

// Prefer using "const char* format" directly, which is more flexible and consistent with other API.
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
bool ui::InputFloat(const char* label, float* v, float step, float step_fast, int decimal_precision, ImGuiInputTextFlags flags)
{
	char format[16] = "%f";
	if (decimal_precision >= 0)
		ImFormatString(format, IM_ARRAYSIZE(format), "%%.%df", decimal_precision);
	return InputFloat(label, v, step, step_fast, format, flags);
}

bool ui::InputFloat2(const char* label, float v[2], int decimal_precision, ImGuiInputTextFlags flags)
{
	char format[16] = "%f";
	if (decimal_precision >= 0)
		ImFormatString(format, IM_ARRAYSIZE(format), "%%.%df", decimal_precision);
	return InputScalarN(label, ImGuiDataType_Float, v, 2, NULL, NULL, format, flags);
}

bool ui::InputFloat3(const char* label, float v[3], int decimal_precision, ImGuiInputTextFlags flags)
{
	char format[16] = "%f";
	if (decimal_precision >= 0)
		ImFormatString(format, IM_ARRAYSIZE(format), "%%.%df", decimal_precision);
	return InputScalarN(label, ImGuiDataType_Float, v, 3, NULL, NULL, format, flags);
}

bool ui::InputFloat4(const char* label, float v[4], int decimal_precision, ImGuiInputTextFlags flags)
{
	char format[16] = "%f";
	if (decimal_precision >= 0)
		ImFormatString(format, IM_ARRAYSIZE(format), "%%.%df", decimal_precision);
	return InputScalarN(label, ImGuiDataType_Float, v, 4, NULL, NULL, format, flags);
}
#endif // IMGUI_DISABLE_OBSOLETE_FUNCTIONS

bool ui::InputInt(const char* label, int* v, int step, int step_fast, ImGuiInputTextFlags flags)
{
	// Hexadecimal input provided as a convenience but the flag name is awkward. Typically you'd use InputText() to parse your own data, if you want to handle prefixes.
	const char* format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%08X" : "%d";
	return InputScalar(label, ImGuiDataType_S32, (void*)v, (void*)(step > 0 ? &step : NULL), (void*)(step_fast > 0 ? &step_fast : NULL), format, flags);
}

bool ui::InputInt2(const char* label, int v[2], ImGuiInputTextFlags flags)
{
	return InputScalarN(label, ImGuiDataType_S32, v, 2, NULL, NULL, "%d", flags);
}

bool ui::InputInt3(const char* label, int v[3], ImGuiInputTextFlags flags)
{
	return InputScalarN(label, ImGuiDataType_S32, v, 3, NULL, NULL, "%d", flags);
}

bool ui::InputInt4(const char* label, int v[4], ImGuiInputTextFlags flags)
{
	return InputScalarN(label, ImGuiDataType_S32, v, 4, NULL, NULL, "%d", flags);
}

bool ui::InputDouble(const char* label, double* v, double step, double step_fast, const char* format, ImGuiInputTextFlags flags)
{
	flags |= ImGuiInputTextFlags_CharsScientific;
	return InputScalar(label, ImGuiDataType_Double, (void*)v, (void*)(step > 0.0 ? &step : NULL), (void*)(step_fast > 0.0 ? &step_fast : NULL), format, flags);
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: InputText, InputTextMultiline
//-------------------------------------------------------------------------
// - InputText()
// - InputTextMultiline()
// - InputTextEx() [Internal]
//-------------------------------------------------------------------------

bool ui::InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
	IM_ASSERT(!(flags & ImGuiInputTextFlags_Multiline)); // call InputTextMultiline()
	return InputTextEx(label, buf, (int)buf_size, ImVec2(0, 0), flags, callback, user_data);
}

bool ui::InputTextMultiline(const char* label, char* buf, size_t buf_size, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
	return InputTextEx(label, buf, (int)buf_size, size, flags | ImGuiInputTextFlags_Multiline, callback, user_data);
}

static int InputTextCalcTextLenAndLineCount(const char* text_begin, const char** out_text_end)
{
	int line_count = 0;
	const char* s = text_begin;
	while (char c = *s++) // We are only matching for \n so we can ignore UTF-8 decoding
		if (c == '\n')
			line_count++;
	s--;
	if (s[0] != '\n' && s[0] != '\r')
		line_count++;
	*out_text_end = s;
	return line_count;
}

static ImVec2 InputTextCalcTextSizeW(const ImWchar* text_begin, const ImWchar* text_end, const ImWchar** remaining, ImVec2* out_offset, bool stop_on_new_line)
{
	ImGuiContext& g = *GImGui;
	ImFont* font = g.Font;
	const float line_height = g.FontSize;
	const float scale = line_height / font->FontSize;

	ImVec2 text_size = ImVec2(0, 0);
	float line_width = 0.0f;

	const ImWchar* s = text_begin;
	while (s < text_end)
	{
		unsigned int c = (unsigned int)(*s++);
		if (c == '\n')
		{
			text_size.x = ImMax(text_size.x, line_width);
			text_size.y += line_height;
			line_width = 0.0f;
			if (stop_on_new_line)
				break;
			continue;
		}
		if (c == '\r')
			continue;

		const float char_width = font->GetCharAdvance((ImWchar)c) * scale;
		line_width += char_width;
	}

	if (text_size.x < line_width)
		text_size.x = line_width;

	if (out_offset)
		*out_offset = ImVec2(line_width, text_size.y + line_height);  // offset allow for the possibility of sitting after a trailing \n

	if (line_width > 0 || text_size.y == 0.0f)                        // whereas size.y will ignore the trailing \n
		text_size.y += line_height;

	if (remaining)
		*remaining = s;

	return text_size;
}

// Wrapper for stb_textedit.h to edit text (our wrapper is for: statically sized buffer, single-line, wchar characters. InputText converts between UTF-8 and wchar)
namespace ImGuiStb
{

	static int     STB_TEXTEDIT_STRINGLEN(const STB_TEXTEDIT_STRING* obj) { return obj->CurLenW; }
	static ImWchar STB_TEXTEDIT_GETCHAR(const STB_TEXTEDIT_STRING* obj, int idx) { return obj->TextW[idx]; }
	static float   STB_TEXTEDIT_GETWIDTH(STB_TEXTEDIT_STRING* obj, int line_start_idx, int char_idx) { ImWchar c = obj->TextW[line_start_idx + char_idx]; if (c == '\n') return STB_TEXTEDIT_GETWIDTH_NEWLINE; return GImGui->Font->GetCharAdvance(c) * (GImGui->FontSize / GImGui->Font->FontSize); }
	static int     STB_TEXTEDIT_KEYTOTEXT(int key) { return key >= 0x10000 ? 0 : key; }
	static ImWchar STB_TEXTEDIT_NEWLINE = '\n';
	static void    STB_TEXTEDIT_LAYOUTROW(StbTexteditRow* r, STB_TEXTEDIT_STRING* obj, int line_start_idx)
	{
		const ImWchar* text = obj->TextW.Data;
		const ImWchar* text_remaining = NULL;
		const ImVec2 size = InputTextCalcTextSizeW(text + line_start_idx, text + obj->CurLenW, &text_remaining, NULL, true);
		r->x0 = 0.0f;
		r->x1 = size.x;
		r->baseline_y_delta = size.y;
		r->ymin = 0.0f;
		r->ymax = size.y;
		r->num_chars = (int)(text_remaining - (text + line_start_idx));
	}

	static bool is_separator(unsigned int c) { return ImCharIsBlankW(c) || c == ',' || c == ';' || c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || c == ']' || c == '|'; }
	static int  is_word_boundary_from_right(STB_TEXTEDIT_STRING* obj, int idx) { return idx > 0 ? (is_separator(obj->TextW[idx - 1]) && !is_separator(obj->TextW[idx])) : 1; }
	static int  STB_TEXTEDIT_MOVEWORDLEFT_IMPL(STB_TEXTEDIT_STRING* obj, int idx) { idx--; while (idx >= 0 && !is_word_boundary_from_right(obj, idx)) idx--; return idx < 0 ? 0 : idx; }
#ifdef __APPLE__    // FIXME: Move setting to IO structure
	static int  is_word_boundary_from_left(STB_TEXTEDIT_STRING* obj, int idx) { return idx > 0 ? (!is_separator(obj->TextW[idx - 1]) && is_separator(obj->TextW[idx])) : 1; }
	static int  STB_TEXTEDIT_MOVEWORDRIGHT_IMPL(STB_TEXTEDIT_STRING* obj, int idx) { idx++; int len = obj->CurLenW; while (idx < len && !is_word_boundary_from_left(obj, idx)) idx++; return idx > len ? len : idx; }
#else
	static int  STB_TEXTEDIT_MOVEWORDRIGHT_IMPL(STB_TEXTEDIT_STRING* obj, int idx) { idx++; int len = obj->CurLenW; while (idx < len && !is_word_boundary_from_right(obj, idx)) idx++; return idx > len ? len : idx; }
#endif
#define STB_TEXTEDIT_MOVEWORDLEFT   STB_TEXTEDIT_MOVEWORDLEFT_IMPL    // They need to be #define for stb_textedit.h
#define STB_TEXTEDIT_MOVEWORDRIGHT  STB_TEXTEDIT_MOVEWORDRIGHT_IMPL

	static void STB_TEXTEDIT_DELETECHARS(STB_TEXTEDIT_STRING* obj, int pos, int n)
	{
		ImWchar* dst = obj->TextW.Data + pos;

		// We maintain our buffer length in both UTF-8 and wchar formats
		obj->CurLenA -= ImTextCountUtf8BytesFromStr(dst, dst + n);
		obj->CurLenW -= n;

		// Offset remaining text (FIXME-OPT: Use memmove)
		const ImWchar* src = obj->TextW.Data + pos + n;
		while (ImWchar c = *src++)
			*dst++ = c;
		*dst = '\0';
	}

	static bool STB_TEXTEDIT_INSERTCHARS(STB_TEXTEDIT_STRING* obj, int pos, const ImWchar* new_text, int new_text_len)
	{
		const bool is_resizable = (obj->UserFlags & ImGuiInputTextFlags_CallbackResize) != 0;
		const int text_len = obj->CurLenW;
		IM_ASSERT(pos <= text_len);

		const int new_text_len_utf8 = ImTextCountUtf8BytesFromStr(new_text, new_text + new_text_len);
		if (!is_resizable && (new_text_len_utf8 + obj->CurLenA + 1 > obj->BufCapacityA))
			return false;

		// Grow internal buffer if needed
		if (new_text_len + text_len + 1 > obj->TextW.Size)
		{
			if (!is_resizable)
				return false;
			IM_ASSERT(text_len < obj->TextW.Size);
			obj->TextW.resize(text_len + ImClamp(new_text_len * 4, 32, ImMax(256, new_text_len)) + 1);
		}

		ImWchar* text = obj->TextW.Data;
		if (pos != text_len)
			memmove(text + pos + new_text_len, text + pos, (size_t)(text_len - pos) * sizeof(ImWchar));
		memcpy(text + pos, new_text, (size_t)new_text_len * sizeof(ImWchar));

		obj->CurLenW += new_text_len;
		obj->CurLenA += new_text_len_utf8;
		obj->TextW[obj->CurLenW] = '\0';

		return true;
	}

	// We don't use an enum so we can build even with conflicting symbols (if another user of stb_textedit.h leak their STB_TEXTEDIT_K_* symbols)
#define STB_TEXTEDIT_K_LEFT         0x10000 // keyboard input to move cursor left
#define STB_TEXTEDIT_K_RIGHT        0x10001 // keyboard input to move cursor right
#define STB_TEXTEDIT_K_UP           0x10002 // keyboard input to move cursor up
#define STB_TEXTEDIT_K_DOWN         0x10003 // keyboard input to move cursor down
#define STB_TEXTEDIT_K_LINESTART    0x10004 // keyboard input to move cursor to start of line
#define STB_TEXTEDIT_K_LINEEND      0x10005 // keyboard input to move cursor to end of line
#define STB_TEXTEDIT_K_TEXTSTART    0x10006 // keyboard input to move cursor to start of text
#define STB_TEXTEDIT_K_TEXTEND      0x10007 // keyboard input to move cursor to end of text
#define STB_TEXTEDIT_K_DELETE       0x10008 // keyboard input to delete selection or character under cursor
#define STB_TEXTEDIT_K_BACKSPACE    0x10009 // keyboard input to delete selection or character left of cursor
#define STB_TEXTEDIT_K_UNDO         0x1000A // keyboard input to perform undo
#define STB_TEXTEDIT_K_REDO         0x1000B // keyboard input to perform redo
#define STB_TEXTEDIT_K_WORDLEFT     0x1000C // keyboard input to move cursor left one word
#define STB_TEXTEDIT_K_WORDRIGHT    0x1000D // keyboard input to move cursor right one word
#define STB_TEXTEDIT_K_SHIFT        0x20000

#define STB_TEXTEDIT_IMPLEMENTATION
#include "imstb_textedit.h"

}

void ImGuiInputTextState::OnKeyPressed(int key)
{
	stb_textedit_key(this, &StbState, key);
	CursorFollow = true;
	CursorAnimReset();
}

ImGuiInputTextCallbackData::ImGuiInputTextCallbackData()
{
	memset(this, 0, sizeof(*this));
}

// Public API to manipulate UTF-8 text
// We expose UTF-8 to the user (unlike the STB_TEXTEDIT_* functions which are manipulating wchar)
// FIXME: The existence of this rarely exercised code path is a bit of a nuisance.
void ImGuiInputTextCallbackData::DeleteChars(int pos, int bytes_count)
{
	IM_ASSERT(pos + bytes_count <= BufTextLen);
	char* dst = Buf + pos;
	const char* src = Buf + pos + bytes_count;
	while (char c = *src++)
		*dst++ = c;
	*dst = '\0';

	if (CursorPos + bytes_count >= pos)
		CursorPos -= bytes_count;
	else if (CursorPos >= pos)
		CursorPos = pos;
	SelectionStart = SelectionEnd = CursorPos;
	BufDirty = true;
	BufTextLen -= bytes_count;
}

void ImGuiInputTextCallbackData::InsertChars(int pos, const char* new_text, const char* new_text_end)
{
	const bool is_resizable = (Flags & ImGuiInputTextFlags_CallbackResize) != 0;
	const int new_text_len = new_text_end ? (int)(new_text_end - new_text) : (int)strlen(new_text);
	if (new_text_len + BufTextLen >= BufSize)
	{
		if (!is_resizable)
			return;

		// Contrary to STB_TEXTEDIT_INSERTCHARS() this is working in the UTF8 buffer, hence the midly similar code (until we remove the U16 buffer alltogether!)
		ImGuiContext& g = *GImGui;
		ImGuiInputTextState* edit_state = &g.InputTextState;
		IM_ASSERT(edit_state->ID != 0 && g.ActiveId == edit_state->ID);
		IM_ASSERT(Buf == edit_state->TempBuffer.Data);
		int new_buf_size = BufTextLen + ImClamp(new_text_len * 4, 32, ImMax(256, new_text_len)) + 1;
		edit_state->TempBuffer.reserve(new_buf_size + 1);
		Buf = edit_state->TempBuffer.Data;
		BufSize = edit_state->BufCapacityA = new_buf_size;
	}

	if (BufTextLen != pos)
		memmove(Buf + pos + new_text_len, Buf + pos, (size_t)(BufTextLen - pos));
	memcpy(Buf + pos, new_text, (size_t)new_text_len * sizeof(char));
	Buf[BufTextLen + new_text_len] = '\0';

	if (CursorPos >= pos)
		CursorPos += new_text_len;
	SelectionStart = SelectionEnd = CursorPos;
	BufDirty = true;
	BufTextLen += new_text_len;
}

// Return false to discard a character.
static bool InputTextFilterCharacter(unsigned int* p_char, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
	unsigned int c = *p_char;

	if (c < 128 && c != ' ' && !isprint((int)(c & 0xFF)))
	{
		bool pass = false;
		pass |= (c == '\n' && (flags & ImGuiInputTextFlags_Multiline));
		pass |= (c == '\t' && (flags & ImGuiInputTextFlags_AllowTabInput));
		if (!pass)
			return false;
	}

	if (c >= 0xE000 && c <= 0xF8FF) // Filter private Unicode range. I don't imagine anybody would want to input them. GLFW on OSX seems to send private characters for special keys like arrow keys.
		return false;

	if (flags & (ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsScientific))
	{
		if (flags & ImGuiInputTextFlags_CharsDecimal)
			if (!(c >= '0' && c <= '9') && (c != '.') && (c != '-') && (c != '+') && (c != '*') && (c != '/'))
				return false;

		if (flags & ImGuiInputTextFlags_CharsScientific)
			if (!(c >= '0' && c <= '9') && (c != '.') && (c != '-') && (c != '+') && (c != '*') && (c != '/') && (c != 'e') && (c != 'E'))
				return false;

		if (flags & ImGuiInputTextFlags_CharsHexadecimal)
			if (!(c >= '0' && c <= '9') && !(c >= 'a' && c <= 'f') && !(c >= 'A' && c <= 'F'))
				return false;

		if (flags & ImGuiInputTextFlags_CharsUppercase)
			if (c >= 'a' && c <= 'z')
				*p_char = (c += (unsigned int)('A' - 'a'));

		if (flags & ImGuiInputTextFlags_CharsNoBlank)
			if (ImCharIsBlankW(c))
				return false;
	}

	if (flags & ImGuiInputTextFlags_CallbackCharFilter)
	{
		ImGuiInputTextCallbackData callback_data;
		memset(&callback_data, 0, sizeof(ImGuiInputTextCallbackData));
		callback_data.EventFlag = ImGuiInputTextFlags_CallbackCharFilter;
		callback_data.EventChar = (ImWchar)c;
		callback_data.Flags = flags;
		callback_data.UserData = user_data;
		if (callback(&callback_data) != 0)
			return false;
		*p_char = callback_data.EventChar;
		if (!callback_data.EventChar)
			return false;
	}

	return true;
}

// Edit a string of text
// - buf_size account for the zero-terminator, so a buf_size of 6 can hold "Hello" but not "Hello!".
//   This is so we can easily call InputText() on static arrays using ARRAYSIZE() and to match
//   Note that in std::string world, capacity() would omit 1 byte used by the zero-terminator.
// - When active, hold on a privately held copy of the text (and apply back to 'buf'). So changing 'buf' while the InputText is active has no effect.
// - If you want to use ui::InputText() with std::string, see misc/cpp/imgui_stdlib.h
// (FIXME: Rather messy function partly because we are doing UTF8 > u16 > UTF8 conversions on the go to more easily handle stb_textedit calls. Ideally we should stay in UTF-8 all the time. See https://github.com/nothings/stb/issues/188)
bool ui::InputTextEx(const char* label, char* buf, int buf_size, const ImVec2& size_arg, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* callback_user_data)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	IM_ASSERT(!((flags & ImGuiInputTextFlags_CallbackHistory) && (flags & ImGuiInputTextFlags_Multiline)));        // Can't use both together (they both use up/down keys)
	IM_ASSERT(!((flags & ImGuiInputTextFlags_CallbackCompletion) && (flags & ImGuiInputTextFlags_AllowTabInput))); // Can't use both together (they both use tab key)

	ImGuiContext& g = *GImGui;
	ImGuiIO& io = g.IO;
	const ImGuiStyle& style = g.Style;

	const bool is_multiline = (flags & ImGuiInputTextFlags_Multiline) != 0;
	const bool is_editable = (flags & ImGuiInputTextFlags_ReadOnly) == 0;
	const bool is_password = (flags & ImGuiInputTextFlags_Password) != 0;
	const bool is_undoable = (flags & ImGuiInputTextFlags_NoUndoRedo) == 0;
	const bool is_resizable = (flags & ImGuiInputTextFlags_CallbackResize) != 0;
	if (is_resizable)
		IM_ASSERT(callback != NULL); // Must provide a callback if you set the ImGuiInputTextFlags_CallbackResize flag!

	if (is_multiline) // Open group before calling GetID() because groups tracks id created within their scope,
		BeginGroup();
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);
	ImVec2 size = CalcItemSize(size_arg, window->Size.x - 48, (is_multiline ? GetTextLineHeight() * 8.0f : label_size.y) + style.FramePadding.y*2.0f); // Arbitrary default of 8 lines high for multi-line
	const ImRect frame_bb(window->DC.CursorPos + ImVec2(16, 0), window->DC.CursorPos + size);
	const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? (style.ItemInnerSpacing.x + label_size.x) : 0.0f, 0.0f));

	ImGuiWindow* draw_window = window;
	if (is_multiline)
	{
		if (!ItemAdd(total_bb, id, &frame_bb))
		{
			ItemSize(total_bb, style.FramePadding.y);
			EndGroup();
			return false;
		}
		if (!BeginChildFrame(id, frame_bb.GetSize()))
		{
			EndChildFrame();
			EndGroup();
			return false;
		}
		draw_window = GetCurrentWindow();
		draw_window->DC.NavLayerActiveMaskNext |= draw_window->DC.NavLayerCurrentMask; // This is to ensure that EndChild() will display a navigation highlight
		size.x -= draw_window->ScrollbarSizes.x;
	}
	else
	{
		ItemSize(total_bb, style.FramePadding.y);
		if (!ItemAdd(total_bb, id, &frame_bb))
			return false;
	}
	const bool hovered = ItemHoverable(frame_bb, id);
	if (hovered)
		g.MouseCursor = ImGuiMouseCursor_TextInput;

	// Password pushes a temporary font with only a fallback glyph
	if (is_password)
	{
		const ImFontGlyph* glyph = g.Font->FindGlyph('*');
		ImFont* password_font = &g.InputTextPasswordFont;
		password_font->FontSize = g.Font->FontSize;
		password_font->Scale = g.Font->Scale;
		password_font->DisplayOffset = g.Font->DisplayOffset;
		password_font->Ascent = g.Font->Ascent;
		password_font->Descent = g.Font->Descent;
		password_font->ContainerAtlas = g.Font->ContainerAtlas;
		password_font->FallbackGlyph = glyph;
		password_font->FallbackAdvanceX = glyph->AdvanceX;
		IM_ASSERT(password_font->Glyphs.empty() && password_font->IndexAdvanceX.empty() && password_font->IndexLookup.empty());
		PushFont(password_font);
	}

	// NB: we are only allowed to access 'edit_state' if we are the active widget.
	ImGuiInputTextState& edit_state = g.InputTextState;

	const bool focus_requested = FocusableItemRegister(window, id, (flags & (ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_AllowTabInput)) == 0);    // Using completion callback disable keyboard tabbing
	const bool focus_requested_by_code = focus_requested && (window->FocusIdxAllCounter == window->FocusIdxAllRequestCurrent);
	const bool focus_requested_by_tab = focus_requested && !focus_requested_by_code;

	const bool user_clicked = hovered && io.MouseClicked[0];
	const bool user_scrolled = is_multiline && g.ActiveId == 0 && edit_state.ID == id && g.ActiveIdPreviousFrame == draw_window->GetIDNoKeepAlive("#SCROLLY");
	const bool user_nav_input_start = (g.ActiveId != id) && ((g.NavInputId == id) || (g.NavActivateId == id && g.NavInputSource == ImGuiInputSource_NavKeyboard));

	bool clear_active_id = false;

	bool select_all = (g.ActiveId != id) && ((flags & ImGuiInputTextFlags_AutoSelectAll) != 0 || user_nav_input_start) && (!is_multiline);
	if (focus_requested || user_clicked || user_scrolled || user_nav_input_start)
	{
		if (g.ActiveId != id)
		{
			// Start edition
			// Take a copy of the initial buffer value (both in original UTF-8 format and converted to wchar)
			// From the moment we focused we are ignoring the content of 'buf' (unless we are in read-only mode)
			const int prev_len_w = edit_state.CurLenW;
			const int init_buf_len = (int)strlen(buf);
			edit_state.TextW.resize(buf_size + 1);             // wchar count <= UTF-8 count. we use +1 to make sure that .Data isn't NULL so it doesn't crash.
			edit_state.InitialText.resize(init_buf_len + 1); // UTF-8. we use +1 to make sure that .Data isn't NULL so it doesn't crash.
			memcpy(edit_state.InitialText.Data, buf, init_buf_len + 1);
			const char* buf_end = NULL;
			edit_state.CurLenW = ImTextStrFromUtf8(edit_state.TextW.Data, buf_size, buf, NULL, &buf_end);
			edit_state.CurLenA = (int)(buf_end - buf); // We can't get the result from ImStrncpy() above because it is not UTF-8 aware. Here we'll cut off malformed UTF-8.
			edit_state.CursorAnimReset();

			// Preserve cursor position and undo/redo stack if we come back to same widget
			// FIXME: We should probably compare the whole buffer to be on the safety side. Comparing buf (utf8) and edit_state.Text (wchar).
			const bool recycle_state = (edit_state.ID == id) && (prev_len_w == edit_state.CurLenW);
			if (recycle_state)
			{
				// Recycle existing cursor/selection/undo stack but clamp position
				// Note a single mouse click will override the cursor/position immediately by calling stb_textedit_click handler.
				edit_state.CursorClamp();
			}
			else
			{
				edit_state.ID = id;
				edit_state.ScrollX = 0.0f;
				stb_textedit_initialize_state(&edit_state.StbState, !is_multiline);
				if (!is_multiline && focus_requested_by_code)
					select_all = true;
			}
			if (flags & ImGuiInputTextFlags_AlwaysInsertMode)
				edit_state.StbState.insert_mode = 1;
			if (!is_multiline && (focus_requested_by_tab || (user_clicked && io.KeyCtrl)))
				select_all = true;
		}
		SetActiveID(id, window);
		SetFocusID(id, window);
		FocusWindow(window);
		g.ActiveIdBlockNavInputFlags = (1 << ImGuiNavInput_Cancel);
		if (!is_multiline && !(flags & ImGuiInputTextFlags_CallbackHistory))
			g.ActiveIdAllowNavDirFlags = ((1 << ImGuiDir_Up) | (1 << ImGuiDir_Down));
	}
	else if (io.MouseClicked[0])
	{
		// Release focus when we click outside
		clear_active_id = true;
	}

	bool value_changed = false;
	bool enter_pressed = false;
	int backup_current_text_length = 0;

	if (g.ActiveId == id)
	{
		if (!is_editable && !g.ActiveIdIsJustActivated)
		{
			// When read-only we always use the live data passed to the function
			edit_state.TextW.resize(buf_size + 1);
			const char* buf_end = NULL;
			edit_state.CurLenW = ImTextStrFromUtf8(edit_state.TextW.Data, edit_state.TextW.Size, buf, NULL, &buf_end);
			edit_state.CurLenA = (int)(buf_end - buf);
			edit_state.CursorClamp();
		}

		backup_current_text_length = edit_state.CurLenA;
		edit_state.BufCapacityA = buf_size;
		edit_state.UserFlags = flags;
		edit_state.UserCallback = callback;
		edit_state.UserCallbackData = callback_user_data;

		// Although we are active we don't prevent mouse from hovering other elements unless we are interacting right now with the widget.
		// Down the line we should have a cleaner library-wide concept of Selected vs Active.
		g.ActiveIdAllowOverlap = !io.MouseDown[0];
		g.WantTextInputNextFrame = 1;

		// Edit in progress
		const float mouse_x = (io.MousePos.x - frame_bb.Min.x - style.FramePadding.x) + edit_state.ScrollX;
		const float mouse_y = (is_multiline ? (io.MousePos.y - draw_window->DC.CursorPos.y - style.FramePadding.y) : (g.FontSize*0.5f));

		const bool is_osx = io.ConfigMacOSXBehaviors;
		if (select_all || (hovered && !is_osx && io.MouseDoubleClicked[0]))
		{
			edit_state.SelectAll();
			edit_state.SelectedAllMouseLock = true;
		}
		else if (hovered && is_osx && io.MouseDoubleClicked[0])
		{
			// Double-click select a word only, OS X style (by simulating keystrokes)
			edit_state.OnKeyPressed(STB_TEXTEDIT_K_WORDLEFT);
			edit_state.OnKeyPressed(STB_TEXTEDIT_K_WORDRIGHT | STB_TEXTEDIT_K_SHIFT);
		}
		else if (io.MouseClicked[0] && !edit_state.SelectedAllMouseLock)
		{
			if (hovered)
			{
				stb_textedit_click(&edit_state, &edit_state.StbState, mouse_x, mouse_y);
				edit_state.CursorAnimReset();
			}
		}
		else if (io.MouseDown[0] && !edit_state.SelectedAllMouseLock && (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f))
		{
			stb_textedit_drag(&edit_state, &edit_state.StbState, mouse_x, mouse_y);
			edit_state.CursorAnimReset();
			edit_state.CursorFollow = true;
		}
		if (edit_state.SelectedAllMouseLock && !io.MouseDown[0])
			edit_state.SelectedAllMouseLock = false;

		if (io.InputQueueCharacters.Size > 0)
		{
			// Process text input (before we check for Return because using some IME will effectively send a Return?)
			// We ignore CTRL inputs, but need to allow ALT+CTRL as some keyboards (e.g. German) use AltGR (which _is_ Alt+Ctrl) to input certain characters.
			bool ignore_inputs = (io.KeyCtrl && !io.KeyAlt) || (is_osx && io.KeySuper);
			if (!ignore_inputs && is_editable && !user_nav_input_start)
				for (int n = 0; n < io.InputQueueCharacters.Size; n++)
				{
					// Insert character if they pass filtering
					unsigned int c = (unsigned int)io.InputQueueCharacters[n];
					if (InputTextFilterCharacter(&c, flags, callback, callback_user_data))
						edit_state.OnKeyPressed((int)c);
				}

			// Consume characters
			io.InputQueueCharacters.resize(0);
		}
	}

	bool cancel_edit = false;
	if (g.ActiveId == id && !g.ActiveIdIsJustActivated && !clear_active_id)
	{
		// Handle key-presses
		const int k_mask = (io.KeyShift ? STB_TEXTEDIT_K_SHIFT : 0);
		const bool is_osx = io.ConfigMacOSXBehaviors;
		const bool is_shortcut_key = (is_osx ? (io.KeySuper && !io.KeyCtrl) : (io.KeyCtrl && !io.KeySuper)) && !io.KeyAlt && !io.KeyShift; // OS X style: Shortcuts using Cmd/Super instead of Ctrl
		const bool is_osx_shift_shortcut = is_osx && io.KeySuper && io.KeyShift && !io.KeyCtrl && !io.KeyAlt;
		const bool is_wordmove_key_down = is_osx ? io.KeyAlt : io.KeyCtrl;                     // OS X style: Text editing cursor movement using Alt instead of Ctrl
		const bool is_startend_key_down = is_osx && io.KeySuper && !io.KeyCtrl && !io.KeyAlt;  // OS X style: Line/Text Start and End using Cmd+Arrows instead of Home/End
		const bool is_ctrl_key_only = io.KeyCtrl && !io.KeyShift && !io.KeyAlt && !io.KeySuper;
		const bool is_shift_key_only = io.KeyShift && !io.KeyCtrl && !io.KeyAlt && !io.KeySuper;

		const bool is_cut = ((is_shortcut_key && IsKeyPressedMap(ImGuiKey_X)) || (is_shift_key_only && IsKeyPressedMap(ImGuiKey_Delete))) && is_editable && !is_password && (!is_multiline || edit_state.HasSelection());
		const bool is_copy = ((is_shortcut_key && IsKeyPressedMap(ImGuiKey_C)) || (is_ctrl_key_only  && IsKeyPressedMap(ImGuiKey_Insert))) && !is_password && (!is_multiline || edit_state.HasSelection());
		const bool is_paste = ((is_shortcut_key && IsKeyPressedMap(ImGuiKey_V)) || (is_shift_key_only && IsKeyPressedMap(ImGuiKey_Insert))) && is_editable;
		const bool is_undo = ((is_shortcut_key && IsKeyPressedMap(ImGuiKey_Z)) && is_editable && is_undoable);
		const bool is_redo = ((is_shortcut_key && IsKeyPressedMap(ImGuiKey_Y)) || (is_osx_shift_shortcut && IsKeyPressedMap(ImGuiKey_Z))) && is_editable && is_undoable;

		if (IsKeyPressedMap(ImGuiKey_LeftArrow)) { edit_state.OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_LINESTART : is_wordmove_key_down ? STB_TEXTEDIT_K_WORDLEFT : STB_TEXTEDIT_K_LEFT) | k_mask); }
		else if (IsKeyPressedMap(ImGuiKey_RightArrow)) { edit_state.OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_LINEEND : is_wordmove_key_down ? STB_TEXTEDIT_K_WORDRIGHT : STB_TEXTEDIT_K_RIGHT) | k_mask); }
		else if (IsKeyPressedMap(ImGuiKey_UpArrow) && is_multiline) { if (io.KeyCtrl) SetWindowScrollY(draw_window, ImMax(draw_window->Scroll.y - g.FontSize, 0.0f)); else edit_state.OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_TEXTSTART : STB_TEXTEDIT_K_UP) | k_mask); }
		else if (IsKeyPressedMap(ImGuiKey_DownArrow) && is_multiline) { if (io.KeyCtrl) SetWindowScrollY(draw_window, ImMin(draw_window->Scroll.y + g.FontSize, GetScrollMaxY())); else edit_state.OnKeyPressed((is_startend_key_down ? STB_TEXTEDIT_K_TEXTEND : STB_TEXTEDIT_K_DOWN) | k_mask); }
		else if (IsKeyPressedMap(ImGuiKey_Home)) { edit_state.OnKeyPressed(io.KeyCtrl ? STB_TEXTEDIT_K_TEXTSTART | k_mask : STB_TEXTEDIT_K_LINESTART | k_mask); }
		else if (IsKeyPressedMap(ImGuiKey_End)) { edit_state.OnKeyPressed(io.KeyCtrl ? STB_TEXTEDIT_K_TEXTEND | k_mask : STB_TEXTEDIT_K_LINEEND | k_mask); }
		else if (IsKeyPressedMap(ImGuiKey_Delete) && is_editable) { edit_state.OnKeyPressed(STB_TEXTEDIT_K_DELETE | k_mask); }
		else if (IsKeyPressedMap(ImGuiKey_Backspace) && is_editable)
		{
			if (!edit_state.HasSelection())
			{
				if (is_wordmove_key_down) edit_state.OnKeyPressed(STB_TEXTEDIT_K_WORDLEFT | STB_TEXTEDIT_K_SHIFT);
				else if (is_osx && io.KeySuper && !io.KeyAlt && !io.KeyCtrl) edit_state.OnKeyPressed(STB_TEXTEDIT_K_LINESTART | STB_TEXTEDIT_K_SHIFT);
			}
			edit_state.OnKeyPressed(STB_TEXTEDIT_K_BACKSPACE | k_mask);
		}
		else if (IsKeyPressedMap(ImGuiKey_Enter))
		{
			bool ctrl_enter_for_new_line = (flags & ImGuiInputTextFlags_CtrlEnterForNewLine) != 0;
			if (!is_multiline || (ctrl_enter_for_new_line && !io.KeyCtrl) || (!ctrl_enter_for_new_line && io.KeyCtrl))
			{
				enter_pressed = clear_active_id = true;
			}
			else if (is_editable)
			{
				unsigned int c = '\n'; // Insert new line
				if (InputTextFilterCharacter(&c, flags, callback, callback_user_data))
					edit_state.OnKeyPressed((int)c);
			}
		}
		else if ((flags & ImGuiInputTextFlags_AllowTabInput) && IsKeyPressedMap(ImGuiKey_Tab) && !io.KeyCtrl && !io.KeyShift && !io.KeyAlt && is_editable)
		{
			unsigned int c = '\t'; // Insert TAB
			if (InputTextFilterCharacter(&c, flags, callback, callback_user_data))
				edit_state.OnKeyPressed((int)c);
		}
		else if (IsKeyPressedMap(ImGuiKey_Escape))
		{
			clear_active_id = cancel_edit = true;
		}
		else if (is_undo || is_redo)
		{
			edit_state.OnKeyPressed(is_undo ? STB_TEXTEDIT_K_UNDO : STB_TEXTEDIT_K_REDO);
			edit_state.ClearSelection();
		}
		else if (is_shortcut_key && IsKeyPressedMap(ImGuiKey_A))
		{
			edit_state.SelectAll();
			edit_state.CursorFollow = true;
		}
		else if (is_cut || is_copy)
		{
			// Cut, Copy
			if (io.SetClipboardTextFn)
			{
				const int ib = edit_state.HasSelection() ? ImMin(edit_state.StbState.select_start, edit_state.StbState.select_end) : 0;
				const int ie = edit_state.HasSelection() ? ImMax(edit_state.StbState.select_start, edit_state.StbState.select_end) : edit_state.CurLenW;
				edit_state.TempBuffer.resize((ie - ib) * 4 + 1);
				ImTextStrToUtf8(edit_state.TempBuffer.Data, edit_state.TempBuffer.Size, edit_state.TextW.Data + ib, edit_state.TextW.Data + ie);
				SetClipboardText(edit_state.TempBuffer.Data);
			}
			if (is_cut)
			{
				if (!edit_state.HasSelection())
					edit_state.SelectAll();
				edit_state.CursorFollow = true;
				stb_textedit_cut(&edit_state, &edit_state.StbState);
			}
		}
		else if (is_paste)
		{
			if (const char* clipboard = GetClipboardText())
			{
				// Filter pasted buffer
				const int clipboard_len = (int)strlen(clipboard);
				ImWchar* clipboard_filtered = (ImWchar*)MemAlloc((clipboard_len + 1) * sizeof(ImWchar));
				int clipboard_filtered_len = 0;
				for (const char* s = clipboard; *s; )
				{
					unsigned int c;
					s += ImTextCharFromUtf8(&c, s, NULL);
					if (c == 0)
						break;
					if (c >= 0x10000 || !InputTextFilterCharacter(&c, flags, callback, callback_user_data))
						continue;
					clipboard_filtered[clipboard_filtered_len++] = (ImWchar)c;
				}
				clipboard_filtered[clipboard_filtered_len] = 0;
				if (clipboard_filtered_len > 0) // If everything was filtered, ignore the pasting operation
				{
					stb_textedit_paste(&edit_state, &edit_state.StbState, clipboard_filtered, clipboard_filtered_len);
					edit_state.CursorFollow = true;
				}
				MemFree(clipboard_filtered);
			}
		}
	}

	if (g.ActiveId == id)
	{
		const char* apply_new_text = NULL;
		int apply_new_text_length = 0;
		if (cancel_edit)
		{
			// Restore initial value. Only return true if restoring to the initial value changes the current buffer contents.
			if (is_editable && strcmp(buf, edit_state.InitialText.Data) != 0)
			{
				apply_new_text = edit_state.InitialText.Data;
				apply_new_text_length = edit_state.InitialText.Size - 1;
			}
		}

		// When using 'ImGuiInputTextFlags_EnterReturnsTrue' as a special case we reapply the live buffer back to the input buffer before clearing ActiveId, even though strictly speaking it wasn't modified on this frame.
		// If we didn't do that, code like InputInt() with ImGuiInputTextFlags_EnterReturnsTrue would fail. Also this allows the user to use InputText() with ImGuiInputTextFlags_EnterReturnsTrue without maintaining any user-side storage.
		bool apply_edit_back_to_user_buffer = !cancel_edit || (enter_pressed && (flags & ImGuiInputTextFlags_EnterReturnsTrue) != 0);
		if (apply_edit_back_to_user_buffer)
		{
			// Apply new value immediately - copy modified buffer back
			// Note that as soon as the input box is active, the in-widget value gets priority over any underlying modification of the input buffer
			// FIXME: We actually always render 'buf' when calling DrawList->AddText, making the comment above incorrect.
			// FIXME-OPT: CPU waste to do this every time the widget is active, should mark dirty state from the stb_textedit callbacks.
			if (is_editable)
			{
				edit_state.TempBuffer.resize(edit_state.TextW.Size * 4 + 1);
				ImTextStrToUtf8(edit_state.TempBuffer.Data, edit_state.TempBuffer.Size, edit_state.TextW.Data, NULL);
			}

			// User callback
			if ((flags & (ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackAlways)) != 0)
			{
				IM_ASSERT(callback != NULL);

				// The reason we specify the usage semantic (Completion/History) is that Completion needs to disable keyboard TABBING at the moment.
				ImGuiInputTextFlags event_flag = 0;
				ImGuiKey event_key = ImGuiKey_COUNT;
				if ((flags & ImGuiInputTextFlags_CallbackCompletion) != 0 && IsKeyPressedMap(ImGuiKey_Tab))
				{
					event_flag = ImGuiInputTextFlags_CallbackCompletion;
					event_key = ImGuiKey_Tab;
				}
				else if ((flags & ImGuiInputTextFlags_CallbackHistory) != 0 && IsKeyPressedMap(ImGuiKey_UpArrow))
				{
					event_flag = ImGuiInputTextFlags_CallbackHistory;
					event_key = ImGuiKey_UpArrow;
				}
				else if ((flags & ImGuiInputTextFlags_CallbackHistory) != 0 && IsKeyPressedMap(ImGuiKey_DownArrow))
				{
					event_flag = ImGuiInputTextFlags_CallbackHistory;
					event_key = ImGuiKey_DownArrow;
				}
				else if (flags & ImGuiInputTextFlags_CallbackAlways)
					event_flag = ImGuiInputTextFlags_CallbackAlways;

				if (event_flag)
				{
					ImGuiInputTextCallbackData callback_data;
					memset(&callback_data, 0, sizeof(ImGuiInputTextCallbackData));
					callback_data.EventFlag = event_flag;
					callback_data.Flags = flags;
					callback_data.UserData = callback_user_data;

					callback_data.EventKey = event_key;
					callback_data.Buf = edit_state.TempBuffer.Data;
					callback_data.BufTextLen = edit_state.CurLenA;
					callback_data.BufSize = edit_state.BufCapacityA;
					callback_data.BufDirty = false;

					// We have to convert from wchar-positions to UTF-8-positions, which can be pretty slow (an incentive to ditch the ImWchar buffer, see https://github.com/nothings/stb/issues/188)
					ImWchar* text = edit_state.TextW.Data;
					const int utf8_cursor_pos = callback_data.CursorPos = ImTextCountUtf8BytesFromStr(text, text + edit_state.StbState.cursor);
					const int utf8_selection_start = callback_data.SelectionStart = ImTextCountUtf8BytesFromStr(text, text + edit_state.StbState.select_start);
					const int utf8_selection_end = callback_data.SelectionEnd = ImTextCountUtf8BytesFromStr(text, text + edit_state.StbState.select_end);

					// Call user code
					callback(&callback_data);

					// Read back what user may have modified
					IM_ASSERT(callback_data.Buf == edit_state.TempBuffer.Data);  // Invalid to modify those fields
					IM_ASSERT(callback_data.BufSize == edit_state.BufCapacityA);
					IM_ASSERT(callback_data.Flags == flags);
					if (callback_data.CursorPos != utf8_cursor_pos) { edit_state.StbState.cursor = ImTextCountCharsFromUtf8(callback_data.Buf, callback_data.Buf + callback_data.CursorPos); edit_state.CursorFollow = true; }
					if (callback_data.SelectionStart != utf8_selection_start) { edit_state.StbState.select_start = ImTextCountCharsFromUtf8(callback_data.Buf, callback_data.Buf + callback_data.SelectionStart); }
					if (callback_data.SelectionEnd != utf8_selection_end) { edit_state.StbState.select_end = ImTextCountCharsFromUtf8(callback_data.Buf, callback_data.Buf + callback_data.SelectionEnd); }
					if (callback_data.BufDirty)
					{
						IM_ASSERT(callback_data.BufTextLen == (int)strlen(callback_data.Buf)); // You need to maintain BufTextLen if you change the text!
						if (callback_data.BufTextLen > backup_current_text_length && is_resizable)
							edit_state.TextW.resize(edit_state.TextW.Size + (callback_data.BufTextLen - backup_current_text_length));
						edit_state.CurLenW = ImTextStrFromUtf8(edit_state.TextW.Data, edit_state.TextW.Size, callback_data.Buf, NULL);
						edit_state.CurLenA = callback_data.BufTextLen;  // Assume correct length and valid UTF-8 from user, saves us an extra strlen()
						edit_state.CursorAnimReset();
					}
				}
			}

			// Will copy result string if modified
			if (is_editable && strcmp(edit_state.TempBuffer.Data, buf) != 0)
			{
				apply_new_text = edit_state.TempBuffer.Data;
				apply_new_text_length = edit_state.CurLenA;
			}
		}

		// Copy result to user buffer
		if (apply_new_text)
		{
			IM_ASSERT(apply_new_text_length >= 0);
			if (backup_current_text_length != apply_new_text_length && is_resizable)
			{
				ImGuiInputTextCallbackData callback_data;
				callback_data.EventFlag = ImGuiInputTextFlags_CallbackResize;
				callback_data.Flags = flags;
				callback_data.Buf = buf;
				callback_data.BufTextLen = apply_new_text_length;
				callback_data.BufSize = ImMax(buf_size, apply_new_text_length + 1);
				callback_data.UserData = callback_user_data;
				callback(&callback_data);
				buf = callback_data.Buf;
				buf_size = callback_data.BufSize;
				apply_new_text_length = ImMin(callback_data.BufTextLen, buf_size - 1);
				IM_ASSERT(apply_new_text_length <= buf_size);
			}

			// If the underlying buffer resize was denied or not carried to the next frame, apply_new_text_length+1 may be >= buf_size.
			ImStrncpy(buf, apply_new_text, ImMin(apply_new_text_length + 1, buf_size));
			value_changed = true;
		}

		// Clear temporary user storage
		edit_state.UserFlags = 0;
		edit_state.UserCallback = NULL;
		edit_state.UserCallbackData = NULL;
	}

	// Release active ID at the end of the function (so e.g. pressing Return still does a final application of the value)
	if (clear_active_id && g.ActiveId == id)
		ClearActiveID();

	// Render
	// Select which buffer we are going to display. When ImGuiInputTextFlags_NoLiveEdit is set 'buf' might still be the old value. We set buf to NULL to prevent accidental usage from now on.
	const char* buf_display = (g.ActiveId == id && is_editable) ? edit_state.TempBuffer.Data : buf; buf = NULL;

	// Set upper limit of single-line InputTextEx() at 2 million characters strings. The current pathological worst case is a long line
	// without any carriage return, which would makes ImFont::RenderText() reserve too many vertices and probably crash. Avoid it altogether.
	// Note that we only use this limit on single-line InputText(), so a pathologically large line on a InputTextMultiline() would still crash.
	const int buf_display_max_length = 2 * 1024 * 1024;

	if (!is_multiline)
	{
		window->DrawList->AddRect(frame_bb.Min, frame_bb.Max, ImColor(12, 12, 12, 255));
		window->DrawList->AddRect(frame_bb.Min + ImVec2(1, 1), frame_bb.Max - ImVec2(1, 1), ImColor(50, 50, 50, 255));
		window->DrawList->AddRect(frame_bb.Min + ImVec2(2, 2), frame_bb.Max - ImVec2(2, 2), ImColor(12, 12, 12, 255));
		window->DrawList->AddRectFilled(frame_bb.Min + ImVec2(3, 3), frame_bb.Max - ImVec2(3, 3), ImColor(25, 25, 25, 255));
	}

	const ImVec4 clip_rect(frame_bb.Min.x, frame_bb.Min.y, frame_bb.Min.x + size.x, frame_bb.Min.y + size.y); // Not using frame_bb.Max because we have adjusted size
	ImVec2 render_pos = is_multiline ? draw_window->DC.CursorPos : frame_bb.Min + style.FramePadding;
	ImVec2 text_size(0.f, 0.f);
	const bool is_currently_scrolling = (edit_state.ID == id && is_multiline && g.ActiveId == draw_window->GetIDNoKeepAlive("#SCROLLY"));
	if (g.ActiveId == id || is_currently_scrolling)
	{
		edit_state.CursorAnim += io.DeltaTime;

		// This is going to be messy. We need to:
		// - Display the text (this alone can be more easily clipped)
		// - Handle scrolling, highlight selection, display cursor (those all requires some form of 1d->2d cursor position calculation)
		// - Measure text height (for scrollbar)
		// We are attempting to do most of that in **one main pass** to minimize the computation cost (non-negligible for large amount of text) + 2nd pass for selection rendering (we could merge them by an extra refactoring effort)
		// FIXME: This should occur on buf_display but we'd need to maintain cursor/select_start/select_end for UTF-8.
		const ImWchar* text_begin = edit_state.TextW.Data;
		ImVec2 cursor_offset, select_start_offset;

		{
			// Count lines + find lines numbers straddling 'cursor' and 'select_start' position.
			const ImWchar* searches_input_ptr[2];
			searches_input_ptr[0] = text_begin + edit_state.StbState.cursor;
			searches_input_ptr[1] = NULL;
			int searches_remaining = 1;
			int searches_result_line_number[2] = { -1, -999 };
			if (edit_state.StbState.select_start != edit_state.StbState.select_end)
			{
				searches_input_ptr[1] = text_begin + ImMin(edit_state.StbState.select_start, edit_state.StbState.select_end);
				searches_result_line_number[1] = -1;
				searches_remaining++;
			}

			// Iterate all lines to find our line numbers
			// In multi-line mode, we never exit the loop until all lines are counted, so add one extra to the searches_remaining counter.
			searches_remaining += is_multiline ? 1 : 0;
			int line_count = 0;
			//for (const ImWchar* s = text_begin; (s = (const ImWchar*)wcschr((const wchar_t*)s, (wchar_t)'\n')) != NULL; s++)  // FIXME-OPT: Could use this when wchar_t are 16-bits
			for (const ImWchar* s = text_begin; *s != 0; s++)
				if (*s == '\n')
				{
					line_count++;
					if (searches_result_line_number[0] == -1 && s >= searches_input_ptr[0]) { searches_result_line_number[0] = line_count; if (--searches_remaining <= 0) break; }
					if (searches_result_line_number[1] == -1 && s >= searches_input_ptr[1]) { searches_result_line_number[1] = line_count; if (--searches_remaining <= 0) break; }
				}
			line_count++;
			if (searches_result_line_number[0] == -1) searches_result_line_number[0] = line_count;
			if (searches_result_line_number[1] == -1) searches_result_line_number[1] = line_count;

			// Calculate 2d position by finding the beginning of the line and measuring distance
			cursor_offset.x = InputTextCalcTextSizeW(ImStrbolW(searches_input_ptr[0], text_begin), searches_input_ptr[0]).x;
			cursor_offset.y = searches_result_line_number[0] * g.FontSize;
			if (searches_result_line_number[1] >= 0)
			{
				select_start_offset.x = InputTextCalcTextSizeW(ImStrbolW(searches_input_ptr[1], text_begin), searches_input_ptr[1]).x;
				select_start_offset.y = searches_result_line_number[1] * g.FontSize;
			}

			// Store text height (note that we haven't calculated text width at all, see GitHub issues #383, #1224)
			if (is_multiline)
				text_size = ImVec2(size.x, line_count * g.FontSize);
		}

		// Scroll
		if (edit_state.CursorFollow)
		{
			// Horizontal scroll in chunks of quarter width
			if (!(flags & ImGuiInputTextFlags_NoHorizontalScroll))
			{
				const float scroll_increment_x = size.x * 0.25f;
				if (cursor_offset.x < edit_state.ScrollX)
					edit_state.ScrollX = (float)(int)ImMax(0.0f, cursor_offset.x - scroll_increment_x);
				else if (cursor_offset.x - size.x >= edit_state.ScrollX)
					edit_state.ScrollX = (float)(int)(cursor_offset.x - size.x + scroll_increment_x);
			}
			else
			{
				edit_state.ScrollX = 0.0f;
			}

			// Vertical scroll
			if (is_multiline)
			{
				float scroll_y = draw_window->Scroll.y;
				if (cursor_offset.y - g.FontSize < scroll_y)
					scroll_y = ImMax(0.0f, cursor_offset.y - g.FontSize);
				else if (cursor_offset.y - size.y >= scroll_y)
					scroll_y = cursor_offset.y - size.y;
				draw_window->DC.CursorPos.y += (draw_window->Scroll.y - scroll_y);   // To avoid a frame of lag
				draw_window->Scroll.y = scroll_y;
				render_pos.y = draw_window->DC.CursorPos.y;
			}
		}
		edit_state.CursorFollow = false;
		const ImVec2 render_scroll = ImVec2(edit_state.ScrollX, 0.0f);

		// Draw selection
		if (edit_state.StbState.select_start != edit_state.StbState.select_end)
		{
			const ImWchar* text_selected_begin = text_begin + ImMin(edit_state.StbState.select_start, edit_state.StbState.select_end);
			const ImWchar* text_selected_end = text_begin + ImMax(edit_state.StbState.select_start, edit_state.StbState.select_end);

			float bg_offy_up = is_multiline ? 0.0f : -1.0f;    // FIXME: those offsets should be part of the style? they don't play so well with multi-line selection.
			float bg_offy_dn = is_multiline ? 0.0f : 2.0f;
			ImU32 bg_color = ImColor(178, 5, 64, 255);
			ImVec2 rect_pos = render_pos + select_start_offset - render_scroll;
			for (const ImWchar* p = text_selected_begin; p < text_selected_end; )
			{
				if (rect_pos.y > clip_rect.w + g.FontSize)
					break;
				if (rect_pos.y < clip_rect.y)
				{
					//p = (const ImWchar*)wmemchr((const wchar_t*)p, '\n', text_selected_end - p);  // FIXME-OPT: Could use this when wchar_t are 16-bits
					//p = p ? p + 1 : text_selected_end;
					while (p < text_selected_end)
						if (*p++ == '\n')
							break;
				}
				else
				{
					ImVec2 rect_size = InputTextCalcTextSizeW(p, text_selected_end, &p, NULL, true);
					if (rect_size.x <= 0.0f) rect_size.x = (float)(int)(g.Font->GetCharAdvance((ImWchar)' ') * 0.50f); // So we can see selected empty lines
					ImRect rect(rect_pos + ImVec2(0.0f, bg_offy_up - g.FontSize), rect_pos + ImVec2(rect_size.x, bg_offy_dn));
					rect.ClipWith(clip_rect);
					if (rect.Overlaps(clip_rect))
						draw_window->DrawList->AddRectFilled(rect.Min, rect.Max, bg_color);
				}
				rect_pos.x = render_pos.x - render_scroll.x;
				rect_pos.y += g.FontSize;
			}
		}

		const int buf_display_len = edit_state.CurLenA;
		if (is_multiline || buf_display_len < buf_display_max_length)
			draw_window->DrawList->AddText(g.Font, g.FontSize, render_pos - render_scroll, GetColorU32(ImGuiCol_Text), buf_display, buf_display + buf_display_len, 0.0f, is_multiline ? NULL : &clip_rect);

		// Draw blinking cursor
		bool cursor_is_visible = (!g.IO.ConfigInputTextCursorBlink) || (g.InputTextState.CursorAnim <= 0.0f) || ImFmod(g.InputTextState.CursorAnim, 1.20f) <= 0.80f;
		ImVec2 cursor_screen_pos = render_pos + cursor_offset - render_scroll;
		ImRect cursor_screen_rect(cursor_screen_pos.x, cursor_screen_pos.y - g.FontSize + 0.5f, cursor_screen_pos.x + 1.0f, cursor_screen_pos.y - 1.5f);
		if (cursor_is_visible && cursor_screen_rect.Overlaps(clip_rect))
			draw_window->DrawList->AddLine(cursor_screen_rect.Min, cursor_screen_rect.GetBL(), GetColorU32(ImGuiCol_Text));

		// Notify OS of text input position for advanced IME (-1 x offset so that Windows IME can cover our cursor. Bit of an extra nicety.)
		if (is_editable)
			g.PlatformImePos = ImVec2(cursor_screen_pos.x - 1, cursor_screen_pos.y - g.FontSize);
	}
	else
	{
		// Render text only
		const char* buf_end = NULL;
		if (is_multiline)
			text_size = ImVec2(size.x, InputTextCalcTextLenAndLineCount(buf_display, &buf_end) * g.FontSize); // We don't need width
		else
			buf_end = buf_display + strlen(buf_display);
		if (is_multiline || (buf_end - buf_display) < buf_display_max_length)
			draw_window->DrawList->AddText(g.Font, g.FontSize, render_pos, GetColorU32(ImGuiCol_Text), buf_display, buf_end, 0.0f, is_multiline ? NULL : &clip_rect);
	}

	if (is_multiline)
	{
		Dummy(text_size + ImVec2(0.0f, g.FontSize)); // Always add room to scroll an extra line
		EndChildFrame();
		EndGroup();
	}

	if (is_password)
		PopFont();

	// Log as text
	if (g.LogEnabled && !is_password)
		LogRenderedText(&render_pos, buf_display, NULL);

	if (label_size.x > 0 && strlen(buf_display) == 0)
		RenderText(ImVec2(frame_bb.Min.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

	if (value_changed)
		MarkItemEdited(id);

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags);
	if ((flags & ImGuiInputTextFlags_EnterReturnsTrue) != 0)
		return enter_pressed;
	else
		return value_changed;
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: ColorEdit, ColorPicker, ColorButton, etc.
//-------------------------------------------------------------------------
// - ColorEdit3()
// - ColorEdit4()
// - ColorPicker3()
// - RenderColorRectWithAlphaCheckerboard() [Internal]
// - ColorPicker4()
// - ColorButton()
// - SetColorEditOptions()
// - ColorTooltip() [Internal]
// - ColorEditOptionsPopup() [Internal]
// - ColorPickerOptionsPopup() [Internal]
//-------------------------------------------------------------------------

bool ui::ColorEdit3(const char* label, float col[3], ImGuiColorEditFlags flags)
{
	return ColorEdit4(label, col, 10, flags | ImGuiColorEditFlags_NoAlpha);
}

// Edit colors components (each component in 0.0f..1.0f range).
// See enum ImGuiColorEditFlags_ for available options. e.g. Only access 3 floats if ImGuiColorEditFlags_NoAlpha flag is set.
// With typical options: Left-click on colored square to open color picker. Right-click to open option menu. CTRL-Click over input fields to edit them and TAB to go to next item.
bool ui::ColorEdit4(const char* label, float col[4], int max, ImGuiColorEditFlags flags)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	SameLine(window->Size.x - max);
	flags |= ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_AlphaBar;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const float square_sz = GetFrameHeight();
	const float w_extra = (flags & ImGuiColorEditFlags_NoSmallPreview) ? 0.0f : (square_sz + style.ItemInnerSpacing.x);
	const float w_items_all = CalcItemWidth() - w_extra;
	const char* label_display_end = FindRenderedTextEnd(label);

	BeginGroup();
	PushID(label);

	// If we're not showing any slider there's no point in doing any HSV conversions
	const ImGuiColorEditFlags flags_untouched = flags;
	if (flags & ImGuiColorEditFlags_NoInputs)
		flags = (flags & (~ImGuiColorEditFlags__InputsMask)) | ImGuiColorEditFlags_RGB | ImGuiColorEditFlags_NoOptions;

	// Context menu: display and modify options (before defaults are applied)
	if (!(flags & ImGuiColorEditFlags_NoOptions))
		ColorEditOptionsPopup(col, flags);

	// Read stored options
	if (!(flags & ImGuiColorEditFlags__InputsMask))
		flags |= (g.ColorEditOptions & ImGuiColorEditFlags__InputsMask);
	if (!(flags & ImGuiColorEditFlags__DataTypeMask))
		flags |= (g.ColorEditOptions & ImGuiColorEditFlags__DataTypeMask);
	if (!(flags & ImGuiColorEditFlags__PickerMask))
		flags |= (g.ColorEditOptions & ImGuiColorEditFlags__PickerMask);
	flags |= (g.ColorEditOptions & ~(ImGuiColorEditFlags__InputsMask | ImGuiColorEditFlags__DataTypeMask | ImGuiColorEditFlags__PickerMask));

	const bool alpha = (flags & ImGuiColorEditFlags_NoAlpha) == 0;
	const bool hdr = (flags & ImGuiColorEditFlags_HDR) != 0;
	const int components = alpha ? 4 : 3;

	// Convert to the formats we need
	float f[4] = { col[0], col[1], col[2], alpha ? col[3] : 1.0f };
	if (flags & ImGuiColorEditFlags_HSV)
		ColorConvertRGBtoHSV(f[0], f[1], f[2], f[0], f[1], f[2]);
	int i[4] = { IM_F32_TO_INT8_UNBOUND(f[0]), IM_F32_TO_INT8_UNBOUND(f[1]), IM_F32_TO_INT8_UNBOUND(f[2]), IM_F32_TO_INT8_UNBOUND(f[3]) };

	bool value_changed = false;
	bool value_changed_as_float = false;

	if ((flags & (ImGuiColorEditFlags_RGB | ImGuiColorEditFlags_HSV)) != 0 && (flags & ImGuiColorEditFlags_NoInputs) == 0)
	{
		// RGB/HSV 0..255 Sliders
		const float w_item_one = ImMax(1.0f, (float)(int)((w_items_all - (style.ItemInnerSpacing.x) * (components - 1)) / (float)components));
		const float w_item_last = ImMax(1.0f, (float)(int)(w_items_all - (w_item_one + style.ItemInnerSpacing.x) * (components - 1)));

		const bool hide_prefix = (w_item_one <= CalcTextSize((flags & ImGuiColorEditFlags_Float) ? "M:0.000" : "M:000").x);
		const char* ids[4] = { "##X", "##Y", "##Z", "##W" };
		const char* fmt_table_int[3][4] =
		{
			{   "%3d",   "%3d",   "%3d",   "%3d" }, // Short display
			{ "R:%3d", "G:%3d", "B:%3d", "A:%3d" }, // Long display for RGBA
			{ "H:%3d", "S:%3d", "V:%3d", "A:%3d" }  // Long display for HSVA
		};
		const char* fmt_table_float[3][4] =
		{
			{   "%0.3f",   "%0.3f",   "%0.3f",   "%0.3f" }, // Short display
			{ "R:%0.3f", "G:%0.3f", "B:%0.3f", "A:%0.3f" }, // Long display for RGBA
			{ "H:%0.3f", "S:%0.3f", "V:%0.3f", "A:%0.3f" }  // Long display for HSVA
		};
		const int fmt_idx = hide_prefix ? 0 : (flags & ImGuiColorEditFlags_HSV) ? 2 : 1;

		PushItemWidth(w_item_one);
		for (int n = 0; n < components; n++)
		{
			if (n > 0)
				SameLine(0, style.ItemInnerSpacing.x);
			if (n + 1 == components)
				PushItemWidth(w_item_last);
			if (flags & ImGuiColorEditFlags_Float)
			{
				value_changed |= DragFloat(ids[n], &f[n], 1.0f / 255.0f, 0.0f, hdr ? 0.0f : 1.0f, fmt_table_float[fmt_idx][n]);
				value_changed_as_float |= value_changed;
			}
			else
			{
				value_changed |= DragInt(ids[n], &i[n], 1.0f, 0, hdr ? 0 : 255, fmt_table_int[fmt_idx][n]);
			}
			if (!(flags & ImGuiColorEditFlags_NoOptions))
				OpenPopupOnItemClick("context");
		}
		PopItemWidth();
		PopItemWidth();
	}
	else if ((flags & ImGuiColorEditFlags_HEX) != 0 && (flags & ImGuiColorEditFlags_NoInputs) == 0)
	{
		// RGB Hexadecimal Input
		char buf[64];
		if (alpha)
			ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X%02X", ImClamp(i[0], 0, 255), ImClamp(i[1], 0, 255), ImClamp(i[2], 0, 255), ImClamp(i[3], 0, 255));
		else
			ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X", ImClamp(i[0], 0, 255), ImClamp(i[1], 0, 255), ImClamp(i[2], 0, 255));
		PushItemWidth(w_items_all);
		if (InputText("##Text", buf, IM_ARRAYSIZE(buf), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase))
		{
			value_changed = true;
			char* p = buf;
			while (*p == '#' || ImCharIsBlankA(*p))
				p++;
			i[0] = i[1] = i[2] = i[3] = 0;
			if (alpha)
				sscanf(p, "%02X%02X%02X%02X", (unsigned int*)&i[0], (unsigned int*)&i[1], (unsigned int*)&i[2], (unsigned int*)&i[3]); // Treat at unsigned (%X is unsigned)
			else
				sscanf(p, "%02X%02X%02X", (unsigned int*)&i[0], (unsigned int*)&i[1], (unsigned int*)&i[2]);
		}
		if (!(flags & ImGuiColorEditFlags_NoOptions))
			OpenPopupOnItemClick("context");
		PopItemWidth();
	}

	ImGuiWindow* picker_active_window = NULL;
	if (!(flags & ImGuiColorEditFlags_NoSmallPreview))
	{
		if (!(flags & ImGuiColorEditFlags_NoInputs))
			SameLine(0, style.ItemInnerSpacing.x);

		ImVec4 col_v4(col[0], col[1], col[2], alpha ? col[3] : 1.0f);
		ImVec4 old_col_v4 = col_v4;
		if (ColorButton("##ColorButton", col_v4, flags))
		{
			if (!(flags & ImGuiColorEditFlags_NoPicker))
			{
				// Store current color and open a picker
				g.ColorPickerRef = col_v4;
				OpenPopup("picker");
				SetNextWindowPos(window->DC.LastItemRect.GetBL() + ImVec2(-1, style.ItemSpacing.y));
			}
		}
		if (!(flags & ImGuiColorEditFlags_NoOptions))
			OpenPopupOnItemClick("context");

		if (old_col_v4.x != col_v4.x ||
			old_col_v4.y != col_v4.y ||
			old_col_v4.z != col_v4.z ||
			old_col_v4.w != col_v4.w) {
			col[0] = col_v4.x;
			col[1] = col_v4.y;
			col[2] = col_v4.z;
			col[3] = col_v4.w;
		}

		SetNextWindowSize(ImVec2(260, 210));
		if (BeginPopup("picker", ImGuiWindowFlags_NoMove))
		{
			picker_active_window = g.CurrentWindow;
			if (label != label_display_end)
			{
				TextUnformatted(label, label_display_end);
				Spacing();
			}
			ImGuiColorEditFlags picker_flags_to_forward = ImGuiColorEditFlags__DataTypeMask | ImGuiColorEditFlags__PickerMask | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_AlphaBar;
			ImGuiColorEditFlags picker_flags = (flags_untouched & picker_flags_to_forward) | ImGuiColorEditFlags__InputsMask | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreviewHalf;
			PushItemWidth(square_sz * 12.0f); // Use 256 + bar sizes?
			value_changed |= ColorPicker4("##picker", col, picker_flags, &g.ColorPickerRef.x);
			PopItemWidth();
			EndPopup();
		}
	}

	if (label != label_display_end && !(flags & ImGuiColorEditFlags_NoLabel))
	{
		SameLine(0, style.ItemInnerSpacing.x);
		TextUnformatted(label, label_display_end);
	}

	// Convert back
	if (picker_active_window == NULL)
	{
		if (!value_changed_as_float)
			for (int n = 0; n < 4; n++)
				f[n] = i[n] / 255.0f;
		if (flags & ImGuiColorEditFlags_HSV)
			ColorConvertHSVtoRGB(f[0], f[1], f[2], f[0], f[1], f[2]);
		if (value_changed)
		{
			col[0] = f[0];
			col[1] = f[1];
			col[2] = f[2];
			if (alpha)
				col[3] = f[3];
		}
	}

	PopID();
	EndGroup();

	// Drag and Drop Target
	// NB: The flag test is merely an optional micro-optimization, BeginDragDropTarget() does the same test.
	if ((window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_HoveredRect) && !(flags & ImGuiColorEditFlags_NoDragDrop) && BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
		{
			memcpy((float*)col, payload->Data, sizeof(float) * 3); // Preserve alpha if any //-V512
			value_changed = true;
		}
		if (const ImGuiPayload* payload = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
		{
			memcpy((float*)col, payload->Data, sizeof(float) * components);
			value_changed = true;
		}
		EndDragDropTarget();
	}

	// When picker is being actively used, use its active id so IsItemActive() will function on ColorEdit4().
	if (picker_active_window && g.ActiveId != 0 && g.ActiveIdWindow == picker_active_window)
		window->DC.LastItemId = g.ActiveId;

	if (value_changed)
		MarkItemEdited(window->DC.LastItemId);

	return value_changed;
}



bool ui::ColorEdit4(const char* label, Color* col, int max, ImGuiColorEditFlags flags)
{
	ImVec4 vecColor1 = ImVec4{ col->get_red() / 255.f, col->get_green() / 255.f, col->get_blue() / 255.f, col->get_alpha() / 255.f };
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	SameLine(window->Size.x - max);
	flags |= ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_AlphaBar;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const float square_sz = GetFrameHeight();
	const float w_extra = (flags & ImGuiColorEditFlags_NoSmallPreview) ? 0.0f : (square_sz + style.ItemInnerSpacing.x);
	const float w_items_all = CalcItemWidth() - w_extra;
	const char* label_display_end = FindRenderedTextEnd(label);

	BeginGroup();
	PushID(label);

	// If we're not showing any slider there's no point in doing any HSV conversions
	const ImGuiColorEditFlags flags_untouched = flags;
	if (flags & ImGuiColorEditFlags_NoInputs)
		flags = (flags & (~ImGuiColorEditFlags__InputsMask)) | ImGuiColorEditFlags_RGB | ImGuiColorEditFlags_NoOptions;

	// Context menu: display and modify options (before defaults are applied)
	if (!(flags & ImGuiColorEditFlags_NoOptions))
		ColorEditOptionsPopup(&vecColor1.x, flags);

	// Read stored options
	if (!(flags & ImGuiColorEditFlags__InputsMask))
		flags |= (g.ColorEditOptions & ImGuiColorEditFlags__InputsMask);
	if (!(flags & ImGuiColorEditFlags__DataTypeMask))
		flags |= (g.ColorEditOptions & ImGuiColorEditFlags__DataTypeMask);
	if (!(flags & ImGuiColorEditFlags__PickerMask))
		flags |= (g.ColorEditOptions & ImGuiColorEditFlags__PickerMask);
	flags |= (g.ColorEditOptions & ~(ImGuiColorEditFlags__InputsMask | ImGuiColorEditFlags__DataTypeMask | ImGuiColorEditFlags__PickerMask));

	const bool alpha = (flags & ImGuiColorEditFlags_NoAlpha) == 0;
	const bool hdr = (flags & ImGuiColorEditFlags_HDR) != 0;
	const int components = alpha ? 4 : 3;

	// Convert to the formats we need
	float f[4] = { vecColor1.x, vecColor1.y, vecColor1.z, alpha ? vecColor1.w : 1.0f };
	if (flags & ImGuiColorEditFlags_HSV)
		ColorConvertRGBtoHSV(f[0], f[1], f[2], f[0], f[1], f[2]);
	int i[4] = { IM_F32_TO_INT8_UNBOUND(f[0]), IM_F32_TO_INT8_UNBOUND(f[1]), IM_F32_TO_INT8_UNBOUND(f[2]), IM_F32_TO_INT8_UNBOUND(f[3]) };

	bool value_changed = false;
	bool value_changed_as_float = false;

	if ((flags & (ImGuiColorEditFlags_RGB | ImGuiColorEditFlags_HSV)) != 0 && (flags & ImGuiColorEditFlags_NoInputs) == 0)
	{
		// RGB/HSV 0..255 Sliders
		const float w_item_one = ImMax(1.0f, (float)(int)((w_items_all - (style.ItemInnerSpacing.x) * (components - 1)) / (float)components));
		const float w_item_last = ImMax(1.0f, (float)(int)(w_items_all - (w_item_one + style.ItemInnerSpacing.x) * (components - 1)));

		const bool hide_prefix = (w_item_one <= CalcTextSize((flags & ImGuiColorEditFlags_Float) ? "M:0.000" : "M:000").x);
		const char* ids[4] = { "##X", "##Y", "##Z", "##W" };
		const char* fmt_table_int[3][4] =
		{
			{   "%3d",   "%3d",   "%3d",   "%3d" }, // Short display
			{ "R:%3d", "G:%3d", "B:%3d", "A:%3d" }, // Long display for RGBA
			{ "H:%3d", "S:%3d", "V:%3d", "A:%3d" }  // Long display for HSVA
		};
		const char* fmt_table_float[3][4] =
		{
			{   "%0.3f",   "%0.3f",   "%0.3f",   "%0.3f" }, // Short display
			{ "R:%0.3f", "G:%0.3f", "B:%0.3f", "A:%0.3f" }, // Long display for RGBA
			{ "H:%0.3f", "S:%0.3f", "V:%0.3f", "A:%0.3f" }  // Long display for HSVA
		};
		const int fmt_idx = hide_prefix ? 0 : (flags & ImGuiColorEditFlags_HSV) ? 2 : 1;

		PushItemWidth(w_item_one);
		for (int n = 0; n < components; n++)
		{
			if (n > 0)
				SameLine(0, style.ItemInnerSpacing.x);
			if (n + 1 == components)
				PushItemWidth(w_item_last);
			if (flags & ImGuiColorEditFlags_Float)
			{
				value_changed |= DragFloat(ids[n], &f[n], 1.0f / 255.0f, 0.0f, hdr ? 0.0f : 1.0f, fmt_table_float[fmt_idx][n]);
				value_changed_as_float |= value_changed;
			}
			else
			{
				value_changed |= DragInt(ids[n], &i[n], 1.0f, 0, hdr ? 0 : 255, fmt_table_int[fmt_idx][n]);
			}
			if (!(flags & ImGuiColorEditFlags_NoOptions))
				OpenPopupOnItemClick("context");
		}
		PopItemWidth();
		PopItemWidth();
	}
	else if ((flags & ImGuiColorEditFlags_HEX) != 0 && (flags & ImGuiColorEditFlags_NoInputs) == 0)
	{
		// RGB Hexadecimal Input
		char buf[64];
		if (alpha)
			ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X%02X", ImClamp(i[0], 0, 255), ImClamp(i[1], 0, 255), ImClamp(i[2], 0, 255), ImClamp(i[3], 0, 255));
		else
			ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X", ImClamp(i[0], 0, 255), ImClamp(i[1], 0, 255), ImClamp(i[2], 0, 255));
		PushItemWidth(w_items_all);
		if (InputText("##Text", buf, IM_ARRAYSIZE(buf), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase))
		{
			value_changed = true;
			char* p = buf;
			while (*p == '#' || ImCharIsBlankA(*p))
				p++;
			i[0] = i[1] = i[2] = i[3] = 0;
			if (alpha)
				sscanf(p, "%02X%02X%02X%02X", (unsigned int*)&i[0], (unsigned int*)&i[1], (unsigned int*)&i[2], (unsigned int*)&i[3]); // Treat at unsigned (%X is unsigned)
			else
				sscanf(p, "%02X%02X%02X", (unsigned int*)&i[0], (unsigned int*)&i[1], (unsigned int*)&i[2]);
		}
		if (!(flags & ImGuiColorEditFlags_NoOptions))
			OpenPopupOnItemClick("context");
		PopItemWidth();
	}

	ImGuiWindow* picker_active_window = NULL;
	if (!(flags & ImGuiColorEditFlags_NoSmallPreview))
	{
		if (!(flags & ImGuiColorEditFlags_NoInputs))
			SameLine(0, style.ItemInnerSpacing.x);

		ImVec4 col_v4 = vecColor1;
		ImVec4 old_col_v4 = col_v4;
		if (ColorButton("##ColorButton", col_v4, flags))
		{
			if (!(flags & ImGuiColorEditFlags_NoPicker))
			{
				// Store current color and open a picker
				g.ColorPickerRef = col_v4;
				OpenPopup("picker");
				SetNextWindowPos(window->DC.LastItemRect.GetBL() + ImVec2(-1, style.ItemSpacing.y));
			}
		}
		if (!(flags & ImGuiColorEditFlags_NoOptions))
			OpenPopupOnItemClick("context");

		if (old_col_v4.x != col_v4.x ||
			old_col_v4.y != col_v4.y ||
			old_col_v4.z != col_v4.z ||
			old_col_v4.w != col_v4.w) {
			col->set(col_v4.x * 255, col_v4.y * 255, col_v4.z * 255, col_v4.w * 255);
		}

		SetNextWindowSize(ImVec2(260, 210));
		if (BeginPopup("picker", ImGuiWindowFlags_NoMove))
		{
			picker_active_window = g.CurrentWindow;
			if (label != label_display_end)
			{
				TextUnformatted(label, label_display_end);
				Spacing();
			}
			ImGuiColorEditFlags picker_flags_to_forward = ImGuiColorEditFlags__DataTypeMask | ImGuiColorEditFlags__PickerMask | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_AlphaBar;
			ImGuiColorEditFlags picker_flags = (flags_untouched & picker_flags_to_forward) | ImGuiColorEditFlags__InputsMask | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreviewHalf;
			PushItemWidth(square_sz * 12.0f); // Use 256 + bar sizes?
			value_changed |= ColorPicker4("##picker", &vecColor1.x, picker_flags, &g.ColorPickerRef.x);
			PopItemWidth();
			EndPopup();
		}
	}

	if (label != label_display_end && !(flags & ImGuiColorEditFlags_NoLabel))
	{
		SameLine(0, style.ItemInnerSpacing.x);
		TextUnformatted(label, label_display_end);
	}

	// Convert back
	if (picker_active_window == NULL)
	{
		if (!value_changed_as_float)
			for (int n = 0; n < 4; n++)
				f[n] = i[n] / 255.0f;
		if (flags & ImGuiColorEditFlags_HSV)
			ColorConvertHSVtoRGB(f[0], f[1], f[2], f[0], f[1], f[2]);
		if (value_changed)
			col->set(f[0] * 255, f[1] * 255, f[2] * 255, f[3] * 255);
		
	}

	PopID();
	EndGroup();

	// Drag and Drop Target
	// NB: The flag test is merely an optional micro-optimization, BeginDragDropTarget() does the same test.
	if ((window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_HoveredRect) && !(flags & ImGuiColorEditFlags_NoDragDrop) && BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
		{
			memcpy((float*)col, payload->Data, sizeof(float) * 3); // Preserve alpha if any //-V512
			value_changed = true;
		}
		if (const ImGuiPayload* payload = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
		{
			memcpy((float*)col, payload->Data, sizeof(float) * components);
			value_changed = true;
		}
		EndDragDropTarget();
	}

	// When picker is being actively used, use its active id so IsItemActive() will function on ColorEdit4().
	if (picker_active_window && g.ActiveId != 0 && g.ActiveIdWindow == picker_active_window)
		window->DC.LastItemId = g.ActiveId;

	if (value_changed)
		MarkItemEdited(window->DC.LastItemId);

	return value_changed;
}






bool ui::ColorButtonSkeet(const char* desc_id, ImVec4& col, ImGuiColorEditFlags flags, ImVec2 size)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiID id = window->GetID(desc_id);
	float default_size = GetFrameHeight();
	if (size.x == 0.0f)
		size.x = default_size;
	if (size.y == 0.0f)
		size.y = default_size / 2;
	const ImRect bb(window->DC.CursorPos + ImVec2(0, 2), window->DC.CursorPos + size + ImVec2(0, 2));
	ItemSize(bb, (size.y >= default_size) ? g.Style.FramePadding.y : 0.0f);
	if (!ItemAdd(bb, id))
		return false;

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held);

	if (flags & ImGuiColorEditFlags_NoAlpha)
		flags &= ~(ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaPreviewHalf);

	ImVec4 col_without_alpha(col.x, col.y, col.z, 1.0f);
	float grid_step = ImMin(size.x, size.y) / 2.99f;
	float rounding = ImMin(g.Style.FrameRounding, grid_step * 0.5f);
	ImRect bb_inner = bb;
	float off = -0.75f; // The border (using Col_FrameBg) tends to look off when color is near-opaque and rounding is enabled. This offset seemed like a good middle ground to reduce those artifacts.
	bb_inner.Expand(off);
	if ((flags & ImGuiColorEditFlags_AlphaPreviewHalf) && col.w < 1.0f)
	{
		float mid_x = (float)(int)((bb_inner.Min.x + bb_inner.Max.x) * 0.5f + 0.5f);
		RenderColorRectWithAlphaCheckerboard(ImVec2(bb_inner.Min.x + grid_step, bb_inner.Min.y), bb_inner.Max, GetColorU32(col), grid_step, ImVec2(-grid_step + off, off), rounding, ImDrawCornerFlags_TopRight | ImDrawCornerFlags_BotRight);
		window->DrawList->AddRectFilled(bb_inner.Min, ImVec2(mid_x, bb_inner.Max.y), GetColorU32(col_without_alpha), rounding, ImDrawCornerFlags_TopLeft | ImDrawCornerFlags_BotLeft);
	}
	else
	{
		// Because GetColorU32() multiplies by the global style Alpha and we don't want to display a checkerboard if the source code had no alpha
		ImVec4 col_source = (flags & ImGuiColorEditFlags_AlphaPreview) ? col : col_without_alpha;
		if (col_source.w < 1.0f)
			RenderColorRectWithAlphaCheckerboard(bb_inner.Min, bb_inner.Max, GetColorU32(col_source), grid_step, ImVec2(off, off), rounding);
		else
			window->DrawList->AddRectFilled(bb_inner.Min, bb_inner.Max, GetColorU32(col_source), rounding, ImDrawCornerFlags_All);

		window->DrawList->AddRectFilledMultiColor(bb_inner.Min, bb_inner.Max,
			ImColor(0.f, 0.f, 0.f, 0.f), ImColor(0.f, 0.f, 0.f, 0.f),
			ImColor(0.f, 0.f, 0.f, 0.5f), ImColor(0.f, 0.f, 0.f, 0.5f));
	}

	// Drag and Drop Source
	// NB: The ActiveId test is merely an optional micro-optimization, BeginDragDropSource() does the same test.
	if (g.ActiveId == id && !(flags & ImGuiColorEditFlags_NoDragDrop) && BeginDragDropSource())
	{
		if (flags & ImGuiColorEditFlags_NoAlpha)
			SetDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F, &col, sizeof(float) * 3, ImGuiCond_Once);
		else
			SetDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F, &col, sizeof(float) * 4, ImGuiCond_Once);
		ColorButtonSkeet(desc_id, col, flags);
		SameLine();
		TextUnformatted("Color");
		EndDragDropSource();
	}

	// Tooltip
	if (!(flags & ImGuiColorEditFlags_NoTooltip) && hovered)
		ColorTooltip(desc_id, &col.x, flags & (ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaPreviewHalf));

	if (pressed)
		MarkItemEdited(id);

	bool popup_open = IsPopupOpen(id);
	if (GetIO().MouseClicked[1] && hovered) {
		if (!popup_open)
			OpenPopupEx(id);
	}

	if (popup_open) {
		SetNextWindowSize(ImVec2(80, CalcMaxPopupHeightFromItemCount(2)));

		char name[16];
		ImFormatString(name, IM_ARRAYSIZE(name), "##Combo_%02d", g.BeginPopupStack.Size); // Recycle windows based on depth

		// Peak into expected window size so we can position it
		if (ImGuiWindow* popup_window = FindWindowByName(name))
			if (popup_window->WasActive)
			{
				ImVec2 size_expected = CalcWindowExpectedSize(popup_window);
				ImRect r_outer = GetWindowAllowedExtentRect(popup_window);
				ImVec2 pos = FindBestWindowPosForPopupEx(bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, bb, ImGuiPopupPositionPolicy_ComboBox);
				SetNextWindowPos(pos);
			}

		// Horizontally align ourselves with the framed text
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
		PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(g.Style.FramePadding.x, g.Style.WindowPadding.y));
		bool ret = BeginSkeet(name, NULL, window_flags);
		PopStyleVar();

		

		EndPopupSkeet();
	}

	return pressed;
}






bool ui::ColorEdit4Skeet(const char* label, Color* col, ImGuiColorEditFlags flags)
{
	ImVec4 vecColor1 = ImVec4{ col->get_red() / 255.f, col->get_green() / 255.f, col->get_blue() / 255.f, col->get_alpha() / 255.f };
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	SameLine(window->Size.x - 28);
	flags |= ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_AlphaBar;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const float square_sz = GetFrameHeight();
	const float w_extra = (flags & ImGuiColorEditFlags_NoSmallPreview) ? 0.0f : (square_sz + style.ItemInnerSpacing.x);
	const float w_items_all = CalcItemWidth() - w_extra;
	const char* label_display_end = FindRenderedTextEnd(label);

	BeginGroup();
	PushID(label);

	// If we're not showing any slider there's no point in doing any HSV conversions
	const ImGuiColorEditFlags flags_untouched = flags;
	if (flags & ImGuiColorEditFlags_NoInputs)
		flags = (flags & (~ImGuiColorEditFlags__InputsMask)) | ImGuiColorEditFlags_RGB | ImGuiColorEditFlags_NoOptions;

	// Context menu: display and modify options (before defaults are applied)
	if (!(flags & ImGuiColorEditFlags_NoOptions))
		ColorEditOptionsPopup(&vecColor1.x, flags);

	// Read stored options
	if (!(flags & ImGuiColorEditFlags__InputsMask))
		flags |= (g.ColorEditOptions & ImGuiColorEditFlags__InputsMask);
	if (!(flags & ImGuiColorEditFlags__DataTypeMask))
		flags |= (g.ColorEditOptions & ImGuiColorEditFlags__DataTypeMask);
	if (!(flags & ImGuiColorEditFlags__PickerMask))
		flags |= (g.ColorEditOptions & ImGuiColorEditFlags__PickerMask);
	flags |= (g.ColorEditOptions & ~(ImGuiColorEditFlags__InputsMask | ImGuiColorEditFlags__DataTypeMask | ImGuiColorEditFlags__PickerMask));

	const bool alpha = (flags & ImGuiColorEditFlags_NoAlpha) == 0;
	const bool hdr = (flags & ImGuiColorEditFlags_HDR) != 0;
	const int components = alpha ? 4 : 3;

	// Convert to the formats we need
	float f[4] = { vecColor1.x, vecColor1.y, vecColor1.z, alpha ? vecColor1.w : 1.0f };
	if (flags & ImGuiColorEditFlags_HSV)
		ColorConvertRGBtoHSV(f[0], f[1], f[2], f[0], f[1], f[2]);
	int i[4] = { IM_F32_TO_INT8_UNBOUND(f[0]), IM_F32_TO_INT8_UNBOUND(f[1]), IM_F32_TO_INT8_UNBOUND(f[2]), IM_F32_TO_INT8_UNBOUND(f[3]) };

	bool value_changed = false;
	bool value_changed_as_float = false;

	if ((flags & (ImGuiColorEditFlags_RGB | ImGuiColorEditFlags_HSV)) != 0 && (flags & ImGuiColorEditFlags_NoInputs) == 0)
	{
		// RGB/HSV 0..255 Sliders
		const float w_item_one = ImMax(1.0f, (float)(int)((w_items_all - (style.ItemInnerSpacing.x) * (components - 1)) / (float)components));
		const float w_item_last = ImMax(1.0f, (float)(int)(w_items_all - (w_item_one + style.ItemInnerSpacing.x) * (components - 1)));

		const bool hide_prefix = (w_item_one <= CalcTextSize((flags & ImGuiColorEditFlags_Float) ? "M:0.000" : "M:000").x);
		const char* ids[4] = { "##X", "##Y", "##Z", "##W" };
		const char* fmt_table_int[3][4] =
		{
			{   "%3d",   "%3d",   "%3d",   "%3d" }, // Short display
			{ "R:%3d", "G:%3d", "B:%3d", "A:%3d" }, // Long display for RGBA
			{ "H:%3d", "S:%3d", "V:%3d", "A:%3d" }  // Long display for HSVA
		};
		const char* fmt_table_float[3][4] =
		{
			{   "%0.3f",   "%0.3f",   "%0.3f",   "%0.3f" }, // Short display
			{ "R:%0.3f", "G:%0.3f", "B:%0.3f", "A:%0.3f" }, // Long display for RGBA
			{ "H:%0.3f", "S:%0.3f", "V:%0.3f", "A:%0.3f" }  // Long display for HSVA
		};
		const int fmt_idx = hide_prefix ? 0 : (flags & ImGuiColorEditFlags_HSV) ? 2 : 1;

		PushItemWidth(w_item_one);
		for (int n = 0; n < components; n++)
		{
			if (n > 0)
				SameLine(0, style.ItemInnerSpacing.x);
			if (n + 1 == components)
				PushItemWidth(w_item_last);
			if (flags & ImGuiColorEditFlags_Float)
			{
				value_changed |= DragFloat(ids[n], &f[n], 1.0f / 255.0f, 0.0f, hdr ? 0.0f : 1.0f, fmt_table_float[fmt_idx][n]);
				value_changed_as_float |= value_changed;
			}
			else
			{
				value_changed |= DragInt(ids[n], &i[n], 1.0f, 0, hdr ? 0 : 255, fmt_table_int[fmt_idx][n]);
			}
			if (!(flags & ImGuiColorEditFlags_NoOptions))
				OpenPopupOnItemClick("context");
		}
		PopItemWidth();
		PopItemWidth();
	}
	else if ((flags & ImGuiColorEditFlags_HEX) != 0 && (flags & ImGuiColorEditFlags_NoInputs) == 0)
	{
		// RGB Hexadecimal Input
		char buf[64];
		if (alpha)
			ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X%02X", ImClamp(i[0], 0, 255), ImClamp(i[1], 0, 255), ImClamp(i[2], 0, 255), ImClamp(i[3], 0, 255));
		else
			ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X", ImClamp(i[0], 0, 255), ImClamp(i[1], 0, 255), ImClamp(i[2], 0, 255));
		PushItemWidth(w_items_all);
		if (InputText("##Text", buf, IM_ARRAYSIZE(buf), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase))
		{
			value_changed = true;
			char* p = buf;
			while (*p == '#' || ImCharIsBlankA(*p))
				p++;
			i[0] = i[1] = i[2] = i[3] = 0;
			if (alpha)
				sscanf(p, "%02X%02X%02X%02X", (unsigned int*)&i[0], (unsigned int*)&i[1], (unsigned int*)&i[2], (unsigned int*)&i[3]); // Treat at unsigned (%X is unsigned)
			else
				sscanf(p, "%02X%02X%02X", (unsigned int*)&i[0], (unsigned int*)&i[1], (unsigned int*)&i[2]);
		}
		if (!(flags & ImGuiColorEditFlags_NoOptions))
			OpenPopupOnItemClick("context");
		PopItemWidth();
	}

	ImGuiWindow* picker_active_window = NULL;
	if (!(flags & ImGuiColorEditFlags_NoSmallPreview))
	{
		if (!(flags & ImGuiColorEditFlags_NoInputs))
			SameLine(0, style.ItemInnerSpacing.x);

		ImVec4 col_v4 = vecColor1;
		ImVec4 old_col_v4 = col_v4;
		if (ColorButtonSkeet("##ColorButton", col_v4, flags))
		{
			if (!(flags & ImGuiColorEditFlags_NoPicker))
			{
				// Store current color and open a picker
				g.ColorPickerRef = col_v4;
				OpenPopup("picker");
				SetNextWindowPos(window->DC.LastItemRect.GetBL() + ImVec2(-1, style.ItemSpacing.y));
			}
		}
		if (!(flags & ImGuiColorEditFlags_NoOptions))
			OpenPopupOnItemClick("context");

		if (old_col_v4.x != col_v4.x ||
			old_col_v4.y != col_v4.y ||
			old_col_v4.z != col_v4.z ||
			old_col_v4.w != col_v4.w) {
			col->set(col_v4.x * 255, col_v4.y * 255, col_v4.z * 255, col_v4.w * 255);
		}

		SetNextWindowSize(ImVec2(224 - 10.7, 230 - 38.1));
		if (BeginPopupSkeet("picker", ImGuiWindowFlags_NoMove))
		{
			picker_active_window = g.CurrentWindow;
			if (label != label_display_end)
			{
				TextUnformatted(label, label_display_end);
				Spacing();
			}
			ImGuiColorEditFlags picker_flags_to_forward = ImGuiColorEditFlags__DataTypeMask | ImGuiColorEditFlags__PickerMask | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_AlphaBar;
			ImGuiColorEditFlags picker_flags = (flags_untouched & picker_flags_to_forward) | ImGuiColorEditFlags__InputsMask | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreviewHalf;
			PushItemWidth(square_sz * 12.0f); // Use 256 + bar sizes?
			value_changed |= ColorPicker4Skeet("##picker", &vecColor1.x, picker_flags, &g.ColorPickerRef.x);
			PopItemWidth();
			EndPopupSkeet();
		}
	}

	if (label != label_display_end && !(flags & ImGuiColorEditFlags_NoLabel))
	{
		SameLine(0, style.ItemInnerSpacing.x);
		TextUnformatted(label, label_display_end);
	}

	// Convert back
	if (picker_active_window == NULL)
	{
		if (!value_changed_as_float)
			for (int n = 0; n < 4; n++)
				f[n] = i[n] / 255.0f;
		if (flags & ImGuiColorEditFlags_HSV)
			ColorConvertHSVtoRGB(f[0], f[1], f[2], f[0], f[1], f[2]);
		if (value_changed)
			col->set(f[0] * 255, f[1] * 255, f[2] * 255, f[3] * 255);
		
	}

	PopID();
	EndGroup();

	// Drag and Drop Target
	// NB: The flag test is merely an optional micro-optimization, BeginDragDropTarget() does the same test.
	if ((window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_HoveredRect) && !(flags & ImGuiColorEditFlags_NoDragDrop) && BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
		{
			memcpy((float*)col, payload->Data, sizeof(float) * 3); // Preserve alpha if any //-V512
			value_changed = true;
		}
		if (const ImGuiPayload* payload = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
		{
			memcpy((float*)col, payload->Data, sizeof(float) * components);
			value_changed = true;
		}
		EndDragDropTarget();
	}

	// When picker is being actively used, use its active id so IsItemActive() will function on ColorEdit4().
	if (picker_active_window && g.ActiveId != 0 && g.ActiveIdWindow == picker_active_window)
		window->DC.LastItemId = g.ActiveId;

	if (value_changed)
		MarkItemEdited(window->DC.LastItemId);

	if (value_changed)
		col->set(f[0] * 255, f[1] * 255, f[2] * 255, f[3] * 255);

	return value_changed;
}



bool ui::ColorPicker4Skeet(const char* label, float col[4], ImGuiColorEditFlags flags, const float* ref_col)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = GetCurrentWindow();
	ImDrawList* draw_list = window->DrawList;

	ImGuiStyle& style = g.Style;
	ImGuiIO& io = g.IO;

	flags |= ImGuiColorEditFlags_NoSidePreview;

	PushID(label);
	BeginGroup();

	if (!(flags & ImGuiColorEditFlags_NoSidePreview))
		flags |= ImGuiColorEditFlags_NoSmallPreview;

	// Context menu: display and store options.
	if (!(flags & ImGuiColorEditFlags_NoOptions))
		ColorPickerOptionsPopup(col, flags);

	// Read stored options
	if (!(flags & ImGuiColorEditFlags__PickerMask))
		flags |= ((g.ColorEditOptions & ImGuiColorEditFlags__PickerMask) ? g.ColorEditOptions : ImGuiColorEditFlags__OptionsDefault) & ImGuiColorEditFlags__PickerMask;
	IM_ASSERT(ImIsPowerOfTwo((int)(flags & ImGuiColorEditFlags__PickerMask))); // Check that only 1 is selected
	if (!(flags & ImGuiColorEditFlags_NoOptions))
		flags |= (g.ColorEditOptions & ImGuiColorEditFlags_AlphaBar);

	window->DC.CursorPos += ImVec2(4, 0);

	// Setup
	int components = (flags & ImGuiColorEditFlags_NoAlpha) ? 3 : 4;
	bool alpha_bar = (flags & ImGuiColorEditFlags_AlphaBar) && !(flags & ImGuiColorEditFlags_NoAlpha);
	ImVec2 picker_pos = window->DC.CursorPos;
	float square_sz = GetFrameHeight();
	float bars_width = square_sz; // Arbitrary smallish width of Hue/Alpha picking bars
	float sv_picker_size = ImMax(bars_width * 1, CalcItemWidth() - (alpha_bar ? 2 : 1) * (bars_width + style.ItemInnerSpacing.x)); // Saturation/Value picking box
	float bar0_pos_x = picker_pos.x + sv_picker_size + style.ItemInnerSpacing.x;
	float bar1_pos_x = bar0_pos_x + bars_width + style.ItemInnerSpacing.x;
	float bars_triangles_half_sz = (float)(int)(bars_width * 0.20f);

	float backup_initial_col[4];
	memcpy(backup_initial_col, col, components * sizeof(float));

	float wheel_thickness = sv_picker_size * 0.08f;
	float wheel_r_outer = sv_picker_size * 0.50f;
	float wheel_r_inner = wheel_r_outer - wheel_thickness;
	ImVec2 wheel_center(picker_pos.x + (sv_picker_size + bars_width) * 0.5f, picker_pos.y + sv_picker_size * 0.5f);

	// Note: the triangle is displayed rotated with triangle_pa pointing to Hue, but most coordinates stays unrotated for logic.
	float triangle_r = wheel_r_inner - (int)(sv_picker_size * 0.027f);
	ImVec2 triangle_pa = ImVec2(triangle_r, 0.0f); // Hue point.
	ImVec2 triangle_pb = ImVec2(triangle_r * -0.5f, triangle_r * -0.866025f); // Black point.
	ImVec2 triangle_pc = ImVec2(triangle_r * -0.5f, triangle_r * +0.866025f); // White point.

	float H, S, V;
	ColorConvertRGBtoHSV(col[0], col[1], col[2], H, S, V);

	bool value_changed = false, value_changed_h = false, value_changed_sv = false;

	PushItemFlag(ImGuiItemFlags_NoNav, true);
	if (flags & ImGuiColorEditFlags_PickerHueWheel)
	{
		// Hue wheel + SV triangle logic
		InvisibleButton("hsv", ImVec2(sv_picker_size + style.ItemInnerSpacing.x + bars_width, sv_picker_size));
		if (IsItemActive())
		{
			ImVec2 initial_off = g.IO.MouseClickedPos[0] - wheel_center;
			ImVec2 current_off = g.IO.MousePos - wheel_center;
			float initial_dist2 = ImLengthSqr(initial_off);
			if (initial_dist2 >= (wheel_r_inner - 1) * (wheel_r_inner - 1) && initial_dist2 <= (wheel_r_outer + 1) * (wheel_r_outer + 1))
			{
				// Interactive with Hue wheel
				H = ImAtan2(current_off.y, current_off.x) / IM_PI * 0.5f;
				if (H < 0.0f)
					H += 1.0f;
				value_changed = value_changed_h = true;
			}
			float cos_hue_angle = ImCos(-H * 2.0f * IM_PI);
			float sin_hue_angle = ImSin(-H * 2.0f * IM_PI);
			if (ImTriangleContainsPoint(triangle_pa, triangle_pb, triangle_pc, ImRotate(initial_off, cos_hue_angle, sin_hue_angle)))
			{
				// Interacting with SV triangle
				ImVec2 current_off_unrotated = ImRotate(current_off, cos_hue_angle, sin_hue_angle);
				if (!ImTriangleContainsPoint(triangle_pa, triangle_pb, triangle_pc, current_off_unrotated))
					current_off_unrotated = ImTriangleClosestPoint(triangle_pa, triangle_pb, triangle_pc, current_off_unrotated);
				float uu, vv, ww;
				ImTriangleBarycentricCoords(triangle_pa, triangle_pb, triangle_pc, current_off_unrotated, uu, vv, ww);
				V = ImClamp(1.0f - vv, 0.0001f, 1.0f);
				S = ImClamp(uu / V, 0.0001f, 1.0f);
				value_changed = value_changed_sv = true;
			}
		}
		if (!(flags & ImGuiColorEditFlags_NoOptions))
			OpenPopupOnItemClick("context");
	}
	else if (flags & ImGuiColorEditFlags_PickerHueBar)
	{
		// SV rectangle logic
		InvisibleButton("sv", ImVec2(sv_picker_size, sv_picker_size));
		if (IsItemActive())
		{
			S = ImSaturate((io.MousePos.x - picker_pos.x) / (sv_picker_size - 1));
			V = 1.0f - ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size - 1));
			value_changed = value_changed_sv = true;
		}
		if (!(flags & ImGuiColorEditFlags_NoOptions))
			OpenPopupOnItemClick("context");

		// Hue bar logic
		SetCursorScreenPos(ImVec2(bar0_pos_x, picker_pos.y));
		InvisibleButton("hue", ImVec2(bars_width, sv_picker_size));
		if (IsItemActive())
		{
			H = ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size - 1));
			value_changed = value_changed_h = true;
		}
	}

	// Alpha bar logic
	if (alpha_bar)
	{
		SetCursorScreenPos(ImVec2(bar1_pos_x, picker_pos.y));
		InvisibleButton("alpha", ImVec2(bars_width, sv_picker_size));
		if (IsItemActive())
		{
			col[3] = 1.0f - ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size - 1));
			value_changed = true;
		}
	}
	PopItemFlag(); // ImGuiItemFlags_NoNav

	// Convert back color to RGB
	if (value_changed_h || value_changed_sv)
		ColorConvertHSVtoRGB(H >= 1.0f ? H - 10 * 1e-6f : H, S > 0.0f ? S : 10 * 1e-6f, V > 0.0f ? V : 1e-6f, col[0], col[1], col[2]);


	ImVec4 hue_color_f(1, 1, 1, 1); ColorConvertHSVtoRGB(H, 1, 1, hue_color_f.x, hue_color_f.y, hue_color_f.z);
	ImU32 hue_color32 = ColorConvertFloat4ToU32(hue_color_f);
	ImU32 col32_no_alpha = ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], 1.0f));

	const ImU32 hue_colors[6 + 1] = { IM_COL32(255,0,0,255), IM_COL32(255,255,0,255), IM_COL32(0,255,0,255), IM_COL32(0,255,255,255), IM_COL32(0,0,255,255), IM_COL32(255,0,255,255), IM_COL32(255,0,0,255) };
	ImVec2 sv_cursor_pos;

	if (flags & ImGuiColorEditFlags_PickerHueWheel)
	{
		// Render Hue Wheel
		const float aeps = 1.5f / wheel_r_outer; // Half a pixel arc length in radians (2pi cancels out).
		const int segment_per_arc = ImMax(4, (int)wheel_r_outer / 12);
		for (int n = 0; n < 6; n++)
		{
			const float a0 = (n) / 6.0f * 2.0f * IM_PI - aeps;
			const float a1 = (n + 1.0f) / 6.0f * 2.0f * IM_PI + aeps;
			const int vert_start_idx = draw_list->VtxBuffer.Size;
			draw_list->PathArcTo(wheel_center, (wheel_r_inner + wheel_r_outer) * 0.5f, a0, a1, segment_per_arc);
			draw_list->PathStroke(IM_COL32_WHITE, false, wheel_thickness);
			const int vert_end_idx = draw_list->VtxBuffer.Size;

			// Paint colors over existing vertices
			ImVec2 gradient_p0(wheel_center.x + ImCos(a0) * wheel_r_inner, wheel_center.y + ImSin(a0) * wheel_r_inner);
			ImVec2 gradient_p1(wheel_center.x + ImCos(a1) * wheel_r_inner, wheel_center.y + ImSin(a1) * wheel_r_inner);
			ShadeVertsLinearColorGradientKeepAlpha(draw_list, vert_start_idx, vert_end_idx, gradient_p0, gradient_p1, hue_colors[n], hue_colors[n + 1]);
		}

		// Render Cursor + preview on Hue Wheel
		float cos_hue_angle = ImCos(H * 2.0f * IM_PI);
		float sin_hue_angle = ImSin(H * 2.0f * IM_PI);
		ImVec2 hue_cursor_pos(wheel_center.x + cos_hue_angle * (wheel_r_inner + wheel_r_outer) * 0.5f, wheel_center.y + sin_hue_angle * (wheel_r_inner + wheel_r_outer) * 0.5f);
		float hue_cursor_rad = value_changed_h ? wheel_thickness * 0.65f : wheel_thickness * 0.55f;
		int hue_cursor_segments = ImClamp((int)(hue_cursor_rad / 1.4f), 9, 32);
		draw_list->AddCircleFilled(hue_cursor_pos, hue_cursor_rad, hue_color32, hue_cursor_segments);
		draw_list->AddCircle(hue_cursor_pos, hue_cursor_rad + 1, IM_COL32(128, 128, 128, 255), hue_cursor_segments);
		draw_list->AddCircle(hue_cursor_pos, hue_cursor_rad, IM_COL32_WHITE, hue_cursor_segments);

		// Render SV triangle (rotated according to hue)
		ImVec2 tra = wheel_center + ImRotate(triangle_pa, cos_hue_angle, sin_hue_angle);
		ImVec2 trb = wheel_center + ImRotate(triangle_pb, cos_hue_angle, sin_hue_angle);
		ImVec2 trc = wheel_center + ImRotate(triangle_pc, cos_hue_angle, sin_hue_angle);
		ImVec2 uv_white = GetFontTexUvWhitePixel();
		draw_list->PrimReserve(6, 6);
		draw_list->PrimVtx(tra, uv_white, hue_color32);
		draw_list->PrimVtx(trb, uv_white, hue_color32);
		draw_list->PrimVtx(trc, uv_white, IM_COL32_WHITE);
		draw_list->PrimVtx(tra, uv_white, IM_COL32_BLACK_TRANS);
		draw_list->PrimVtx(trb, uv_white, IM_COL32_BLACK);
		draw_list->PrimVtx(trc, uv_white, IM_COL32_BLACK_TRANS);
		draw_list->AddTriangle(tra, trb, trc, IM_COL32(128, 128, 128, 255), 1.5f);
		sv_cursor_pos = ImLerp(ImLerp(trc, tra, ImSaturate(S)), trb, ImSaturate(1 - V));
	}
	else if (flags & ImGuiColorEditFlags_PickerHueBar)
	{
		// Render SV Square
		draw_list->AddRectFilledMultiColor(picker_pos, picker_pos + ImVec2(sv_picker_size, sv_picker_size), IM_COL32_WHITE, hue_color32, hue_color32, IM_COL32_WHITE);
		draw_list->AddRectFilledMultiColor(picker_pos, picker_pos + ImVec2(sv_picker_size, sv_picker_size), IM_COL32_BLACK_TRANS, IM_COL32_BLACK_TRANS, IM_COL32_BLACK, IM_COL32_BLACK);
		RenderFrameBorder(picker_pos, picker_pos + ImVec2(sv_picker_size, sv_picker_size), 0.0f);
		sv_cursor_pos.x = ImClamp((float)(int)(picker_pos.x + ImSaturate(S) * sv_picker_size + 0.5f), picker_pos.x + 2, picker_pos.x + sv_picker_size - 2); // Sneakily prevent the circle to stick out too much
		sv_cursor_pos.y = ImClamp((float)(int)(picker_pos.y + ImSaturate(1 - V) * sv_picker_size + 0.5f), picker_pos.y + 2, picker_pos.y + sv_picker_size - 2);

		// Render Hue Bar
		for (int i = 0; i < 6; ++i)
			draw_list->AddRectFilledMultiColor(ImVec2(bar0_pos_x, picker_pos.y + i * (sv_picker_size / 6)), ImVec2(bar0_pos_x + bars_width, picker_pos.y + (i + 1) * (sv_picker_size / 6)), hue_colors[i], hue_colors[i], hue_colors[i + 1], hue_colors[i + 1]);
		float bar0_line_y = (float)(int)(picker_pos.y + H * sv_picker_size + 0.5f);
		RenderFrameBorder(ImVec2(bar0_pos_x, picker_pos.y), ImVec2(bar0_pos_x + bars_width, picker_pos.y + sv_picker_size), 0.0f);
		RenderArrowsForVerticalBar(draw_list, ImVec2(bar0_pos_x - 1, bar0_line_y), ImVec2(bars_triangles_half_sz + 1, bars_triangles_half_sz), bars_width + 2.0f);
	}

	// Render cursor/preview circle (clamp S/V within 0..1 range because floating points colors may lead HSV values to be out of range)
	draw_list->AddRect(sv_cursor_pos - ImVec2(2, 2), sv_cursor_pos + ImVec2(2, 2), ImColor(0.f, 0.f, 0.f, 1.f));

	// Render alpha bar
	if (alpha_bar)
	{
		float alpha = ImSaturate(col[3]);
		ImRect bar1_bb(bar1_pos_x, picker_pos.y, bar1_pos_x + bars_width, picker_pos.y + sv_picker_size);
		RenderColorRectWithAlphaCheckerboard(bar1_bb.Min, bar1_bb.Max, IM_COL32(0, 0, 0, 0), bar1_bb.GetWidth() / 2.0f, ImVec2(0.0f, 0.0f));
		draw_list->AddRectFilledMultiColor(bar1_bb.Min, bar1_bb.Max, col32_no_alpha, col32_no_alpha, col32_no_alpha & ~IM_COL32_A_MASK, col32_no_alpha & ~IM_COL32_A_MASK);
		float bar1_line_y = (float)(int)(picker_pos.y + (1.0f - alpha) * sv_picker_size + 0.5f);
		RenderFrameBorder(bar1_bb.Min, bar1_bb.Max, 0.0f);
		RenderArrowsForVerticalBar(draw_list, ImVec2(bar1_pos_x - 1, bar1_line_y), ImVec2(bars_triangles_half_sz + 1, bars_triangles_half_sz), bars_width + 2.0f);
	}

	EndGroup();

	if (value_changed && memcmp(backup_initial_col, col, components * sizeof(float)) == 0)
		value_changed = false;
	if (value_changed)
		MarkItemEdited(window->DC.LastItemId);

	PopID();

	return value_changed;
}









bool ui::ColorEdit4Shonax(const char* label, Color* col1, ImGuiColorEditFlags flags, int max)
{

	float col4[4] = { col1->get_red() / 255.f, col1->get_green() / 255.f, col1->get_blue() / 255.f, col1->get_alpha() / 255.f };
	if (ui::ColorEdit4(label, col4, max, flags))
	{
		col1->set(col4[0] * 255, col4[1] * 255, col4[2] * 255, col4[3] * 255);
		return true;
	}
	return false;
}


bool ui::ColorPicker3(const char* label, float col[3], ImGuiColorEditFlags flags)
{
	float col4[4] = { col[0], col[1], col[2], 1.0f };
	if (!ColorPicker4(label, col4, flags | ImGuiColorEditFlags_NoAlpha))
		return false;
	col[0] = col4[0]; col[1] = col4[1]; col[2] = col4[2];
	return true;
}

static inline ImU32 ImAlphaBlendColor(ImU32 col_a, ImU32 col_b)
{
	float t = ((col_b >> IM_COL32_A_SHIFT) & 0xFF) / 255.f;
	int r = ImLerp((int)(col_a >> IM_COL32_R_SHIFT) & 0xFF, (int)(col_b >> IM_COL32_R_SHIFT) & 0xFF, t);
	int g = ImLerp((int)(col_a >> IM_COL32_G_SHIFT) & 0xFF, (int)(col_b >> IM_COL32_G_SHIFT) & 0xFF, t);
	int b = ImLerp((int)(col_a >> IM_COL32_B_SHIFT) & 0xFF, (int)(col_b >> IM_COL32_B_SHIFT) & 0xFF, t);
	return IM_COL32(r, g, b, 0xFF);
}

// Helper for ColorPicker4()
// NB: This is rather brittle and will show artifact when rounding this enabled if rounded corners overlap multiple cells. Caller currently responsible for avoiding that.
// I spent a non reasonable amount of time trying to getting this right for ColorButton with rounding+anti-aliasing+ImGuiColorEditFlags_HalfAlphaPreview flag + various grid sizes and offsets, and eventually gave up... probably more reasonable to disable rounding alltogether.
void ui::RenderColorRectWithAlphaCheckerboard(ImVec2 p_min, ImVec2 p_max, ImU32 col, float grid_step, ImVec2 grid_off, float rounding, int rounding_corners_flags)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (((col & IM_COL32_A_MASK) >> IM_COL32_A_SHIFT) < 0xFF)
	{
		ImU32 col_bg1 = GetColorU32(ImAlphaBlendColor(IM_COL32(204, 204, 204, 255), col));
		ImU32 col_bg2 = GetColorU32(ImAlphaBlendColor(IM_COL32(128, 128, 128, 255), col));
		window->DrawList->AddRectFilled(p_min, p_max, col_bg1, rounding, rounding_corners_flags);

		int yi = 0;
		for (float y = p_min.y + grid_off.y; y < p_max.y; y += grid_step, yi++)
		{
			float y1 = ImClamp(y, p_min.y, p_max.y), y2 = ImMin(y + grid_step, p_max.y);
			if (y2 <= y1)
				continue;
			for (float x = p_min.x + grid_off.x + (yi & 1) * grid_step; x < p_max.x; x += grid_step * 2.0f)
			{
				float x1 = ImClamp(x, p_min.x, p_max.x), x2 = ImMin(x + grid_step, p_max.x);
				if (x2 <= x1)
					continue;
				int rounding_corners_flags_cell = 0;
				if (y1 <= p_min.y) { if (x1 <= p_min.x) rounding_corners_flags_cell |= ImDrawCornerFlags_TopLeft; if (x2 >= p_max.x) rounding_corners_flags_cell |= ImDrawCornerFlags_TopRight; }
				if (y2 >= p_max.y) { if (x1 <= p_min.x) rounding_corners_flags_cell |= ImDrawCornerFlags_BotLeft; if (x2 >= p_max.x) rounding_corners_flags_cell |= ImDrawCornerFlags_BotRight; }
				rounding_corners_flags_cell &= rounding_corners_flags;
				window->DrawList->AddRectFilled(ImVec2(x1, y1), ImVec2(x2, y2), col_bg2, rounding_corners_flags_cell ? rounding : 0.0f, rounding_corners_flags_cell);
			}
		}
	}
	else
	{
		window->DrawList->AddRectFilled(p_min, p_max, col, rounding, rounding_corners_flags);
	}
}


// Note: ColorPicker4() only accesses 3 floats if ImGuiColorEditFlags_NoAlpha flag is set.
// FIXME: we adjust the big color square height based on item width, which may cause a flickering feedback loop (if automatic height makes a vertical scrollbar appears, affecting automatic width..)
bool ui::ColorPicker4(const char* label, float col[4], ImGuiColorEditFlags flags, const float* ref_col)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = GetCurrentWindow();
	ImDrawList* draw_list = window->DrawList;

	ImGuiStyle& style = g.Style;
	ImGuiIO& io = g.IO;

	flags |= ImGuiColorEditFlags_NoSidePreview;

	PushID(label);
	BeginGroup();

	if (!(flags & ImGuiColorEditFlags_NoSidePreview))
		flags |= ImGuiColorEditFlags_NoSmallPreview;

	// Context menu: display and store options.
	if (!(flags & ImGuiColorEditFlags_NoOptions))
		ColorPickerOptionsPopup(col, flags);

	// Read stored options
	if (!(flags & ImGuiColorEditFlags__PickerMask))
		flags |= ((g.ColorEditOptions & ImGuiColorEditFlags__PickerMask) ? g.ColorEditOptions : ImGuiColorEditFlags__OptionsDefault) & ImGuiColorEditFlags__PickerMask;
	IM_ASSERT(ImIsPowerOfTwo((int)(flags & ImGuiColorEditFlags__PickerMask))); // Check that only 1 is selected
	if (!(flags & ImGuiColorEditFlags_NoOptions))
		flags |= (g.ColorEditOptions & ImGuiColorEditFlags_AlphaBar);

	window->DC.CursorPos += ImVec2(4, 0);

	// Setup
	int components = (flags & ImGuiColorEditFlags_NoAlpha) ? 3 : 4;
	bool alpha_bar = (flags & ImGuiColorEditFlags_AlphaBar) && !(flags & ImGuiColorEditFlags_NoAlpha);
	ImVec2 picker_pos = window->DC.CursorPos;
	float square_sz = GetFrameHeight();
	float bars_width = square_sz; // Arbitrary smallish width of Hue/Alpha picking bars
	float sv_picker_size = ImMax(bars_width * 1, CalcItemWidth() - (alpha_bar ? 2 : 1) * (bars_width + style.ItemInnerSpacing.x)); // Saturation/Value picking box
	float bar0_pos_x = picker_pos.x + sv_picker_size + style.ItemInnerSpacing.x;
	float bar1_pos_x = bar0_pos_x + bars_width + style.ItemInnerSpacing.x;
	float bars_triangles_half_sz = (float)(int)(bars_width * 0.20f);

	float backup_initial_col[4];
	memcpy(backup_initial_col, col, components * sizeof(float));

	float wheel_thickness = sv_picker_size * 0.08f;
	float wheel_r_outer = sv_picker_size * 0.50f;
	float wheel_r_inner = wheel_r_outer - wheel_thickness;
	ImVec2 wheel_center(picker_pos.x + (sv_picker_size + bars_width)*0.5f, picker_pos.y + sv_picker_size * 0.5f);

	// Note: the triangle is displayed rotated with triangle_pa pointing to Hue, but most coordinates stays unrotated for logic.
	float triangle_r = wheel_r_inner - (int)(sv_picker_size * 0.027f);
	ImVec2 triangle_pa = ImVec2(triangle_r, 0.0f); // Hue point.
	ImVec2 triangle_pb = ImVec2(triangle_r * -0.5f, triangle_r * -0.866025f); // Black point.
	ImVec2 triangle_pc = ImVec2(triangle_r * -0.5f, triangle_r * +0.866025f); // White point.

	float H, S, V;
	ColorConvertRGBtoHSV(col[0], col[1], col[2], H, S, V);

	bool value_changed = false, value_changed_h = false, value_changed_sv = false;

	PushItemFlag(ImGuiItemFlags_NoNav, true);
	if (flags & ImGuiColorEditFlags_PickerHueWheel)
	{
		// Hue wheel + SV triangle logic
		InvisibleButton("hsv", ImVec2(sv_picker_size + style.ItemInnerSpacing.x + bars_width, sv_picker_size));
		if (IsItemActive())
		{
			ImVec2 initial_off = g.IO.MouseClickedPos[0] - wheel_center;
			ImVec2 current_off = g.IO.MousePos - wheel_center;
			float initial_dist2 = ImLengthSqr(initial_off);
			if (initial_dist2 >= (wheel_r_inner - 1)*(wheel_r_inner - 1) && initial_dist2 <= (wheel_r_outer + 1)*(wheel_r_outer + 1))
			{
				// Interactive with Hue wheel
				H = ImAtan2(current_off.y, current_off.x) / IM_PI * 0.5f;
				if (H < 0.0f)
					H += 1.0f;
				value_changed = value_changed_h = true;
			}
			float cos_hue_angle = ImCos(-H * 2.0f * IM_PI);
			float sin_hue_angle = ImSin(-H * 2.0f * IM_PI);
			if (ImTriangleContainsPoint(triangle_pa, triangle_pb, triangle_pc, ImRotate(initial_off, cos_hue_angle, sin_hue_angle)))
			{
				// Interacting with SV triangle
				ImVec2 current_off_unrotated = ImRotate(current_off, cos_hue_angle, sin_hue_angle);
				if (!ImTriangleContainsPoint(triangle_pa, triangle_pb, triangle_pc, current_off_unrotated))
					current_off_unrotated = ImTriangleClosestPoint(triangle_pa, triangle_pb, triangle_pc, current_off_unrotated);
				float uu, vv, ww;
				ImTriangleBarycentricCoords(triangle_pa, triangle_pb, triangle_pc, current_off_unrotated, uu, vv, ww);
				V = ImClamp(1.0f - vv, 0.0001f, 1.0f);
				S = ImClamp(uu / V, 0.0001f, 1.0f);
				value_changed = value_changed_sv = true;
			}
		}
		if (!(flags & ImGuiColorEditFlags_NoOptions))
			OpenPopupOnItemClick("context");
	}
	else if (flags & ImGuiColorEditFlags_PickerHueBar)
	{
		// SV rectangle logic
		InvisibleButton("sv", ImVec2(sv_picker_size, sv_picker_size));
		if (IsItemActive())
		{
			S = ImSaturate((io.MousePos.x - picker_pos.x) / (sv_picker_size - 1));
			V = 1.0f - ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size - 1));
			value_changed = value_changed_sv = true;
		}
		if (!(flags & ImGuiColorEditFlags_NoOptions))
			OpenPopupOnItemClick("context");

		// Hue bar logic
		SetCursorScreenPos(ImVec2(bar0_pos_x, picker_pos.y));
		InvisibleButton("hue", ImVec2(bars_width, sv_picker_size));
		if (IsItemActive())
		{
			H = ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size - 1));
			value_changed = value_changed_h = true;
		}
	}

	// Alpha bar logic
	if (alpha_bar)
	{
		SetCursorScreenPos(ImVec2(bar1_pos_x, picker_pos.y));
		InvisibleButton("alpha", ImVec2(bars_width, sv_picker_size));
		if (IsItemActive())
		{
			col[3] = 1.0f - ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size - 1));
			value_changed = true;
		}
	}
	PopItemFlag(); // ImGuiItemFlags_NoNav

	// Convert back color to RGB
	if (value_changed_h || value_changed_sv)
		ColorConvertHSVtoRGB(H >= 1.0f ? H - 10 * 1e-6f : H, S > 0.0f ? S : 10 * 1e-6f, V > 0.0f ? V : 1e-6f, col[0], col[1], col[2]);

	// R,G,B and H,S,V slider color editor
	//bool value_changed_fix_hue_wrap = false;
	//if ((flags & ImGuiColorEditFlags_NoInputs) == 0)
	//{
	//    PushItemWidth((alpha_bar ? bar1_pos_x : bar0_pos_x) + bars_width - picker_pos.x);
	//    ImGuiColorEditFlags sub_flags_to_forward = ImGuiColorEditFlags__DataTypeMask | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaPreviewHalf;
	//    ImGuiColorEditFlags sub_flags = (flags & sub_flags_to_forward) | ImGuiColorEditFlags_NoPicker;
	//    if (flags & ImGuiColorEditFlags_RGB || (flags & ImGuiColorEditFlags__InputsMask) == 0)
	//        if (ColorEdit4("##rgb", col, sub_flags | ImGuiColorEditFlags_RGB))
	//        {
	//            // FIXME: Hackily differenciating using the DragInt (ActiveId != 0 && !ActiveIdAllowOverlap) vs. using the InputText or DropTarget.
	//            // For the later we don't want to run the hue-wrap canceling code. If you are well versed in HSV picker please provide your input! (See #2050)
	//            value_changed_fix_hue_wrap = (g.ActiveId != 0 && !g.ActiveIdAllowOverlap);
	//            value_changed = true;
	//        }
	//    if (flags & ImGuiColorEditFlags_HSV || (flags & ImGuiColorEditFlags__InputsMask) == 0)
	//        value_changed |= ColorEdit4("##hsv", col, sub_flags | ImGuiColorEditFlags_HSV);
	//    if (flags & ImGuiColorEditFlags_HEX || (flags & ImGuiColorEditFlags__InputsMask) == 0)
	//        value_changed |= ColorEdit4("##hex", col, sub_flags | ImGuiColorEditFlags_HEX);
	//    PopItemWidth();
	//}

	// Try to cancel hue wrap (after ColorEdit4 call), if any
	/*if (value_changed_fix_hue_wrap)
	{
		float new_H, new_S, new_V;
		ColorConvertRGBtoHSV(col[0], col[1], col[2], new_H, new_S, new_V);
		if (new_H <= 0 && H > 0)
		{
			if (new_V <= 0 && V != new_V)
				ColorConvertHSVtoRGB(H, S, new_V <= 0 ? V * 0.5f : new_V, col[0], col[1], col[2]);
			else if (new_S <= 0)
				ColorConvertHSVtoRGB(H, new_S <= 0 ? S * 0.5f : new_S, new_V, col[0], col[1], col[2]);
		}
	}*/

	ImVec4 hue_color_f(1, 1, 1, 1); ColorConvertHSVtoRGB(H, 1, 1, hue_color_f.x, hue_color_f.y, hue_color_f.z);
	ImU32 hue_color32 = ColorConvertFloat4ToU32(hue_color_f);
	ImU32 col32_no_alpha = ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], 1.0f));

	const ImU32 hue_colors[6 + 1] = { IM_COL32(255,0,0,255), IM_COL32(255,255,0,255), IM_COL32(0,255,0,255), IM_COL32(0,255,255,255), IM_COL32(0,0,255,255), IM_COL32(255,0,255,255), IM_COL32(255,0,0,255) };
	ImVec2 sv_cursor_pos;

	if (flags & ImGuiColorEditFlags_PickerHueWheel)
	{
		// Render Hue Wheel
		const float aeps = 1.5f / wheel_r_outer; // Half a pixel arc length in radians (2pi cancels out).
		const int segment_per_arc = ImMax(4, (int)wheel_r_outer / 12);
		for (int n = 0; n < 6; n++)
		{
			const float a0 = (n) / 6.0f * 2.0f * IM_PI - aeps;
			const float a1 = (n + 1.0f) / 6.0f * 2.0f * IM_PI + aeps;
			const int vert_start_idx = draw_list->VtxBuffer.Size;
			draw_list->PathArcTo(wheel_center, (wheel_r_inner + wheel_r_outer)*0.5f, a0, a1, segment_per_arc);
			draw_list->PathStroke(IM_COL32_WHITE, false, wheel_thickness);
			const int vert_end_idx = draw_list->VtxBuffer.Size;

			// Paint colors over existing vertices
			ImVec2 gradient_p0(wheel_center.x + ImCos(a0) * wheel_r_inner, wheel_center.y + ImSin(a0) * wheel_r_inner);
			ImVec2 gradient_p1(wheel_center.x + ImCos(a1) * wheel_r_inner, wheel_center.y + ImSin(a1) * wheel_r_inner);
			ShadeVertsLinearColorGradientKeepAlpha(draw_list, vert_start_idx, vert_end_idx, gradient_p0, gradient_p1, hue_colors[n], hue_colors[n + 1]);
		}

		// Render Cursor + preview on Hue Wheel
		float cos_hue_angle = ImCos(H * 2.0f * IM_PI);
		float sin_hue_angle = ImSin(H * 2.0f * IM_PI);
		ImVec2 hue_cursor_pos(wheel_center.x + cos_hue_angle * (wheel_r_inner + wheel_r_outer)*0.5f, wheel_center.y + sin_hue_angle * (wheel_r_inner + wheel_r_outer)*0.5f);
		float hue_cursor_rad = value_changed_h ? wheel_thickness * 0.65f : wheel_thickness * 0.55f;
		int hue_cursor_segments = ImClamp((int)(hue_cursor_rad / 1.4f), 9, 32);
		draw_list->AddCircleFilled(hue_cursor_pos, hue_cursor_rad, hue_color32, hue_cursor_segments);
		draw_list->AddCircle(hue_cursor_pos, hue_cursor_rad + 1, IM_COL32(128, 128, 128, 255), hue_cursor_segments);
		draw_list->AddCircle(hue_cursor_pos, hue_cursor_rad, IM_COL32_WHITE, hue_cursor_segments);

		// Render SV triangle (rotated according to hue)
		ImVec2 tra = wheel_center + ImRotate(triangle_pa, cos_hue_angle, sin_hue_angle);
		ImVec2 trb = wheel_center + ImRotate(triangle_pb, cos_hue_angle, sin_hue_angle);
		ImVec2 trc = wheel_center + ImRotate(triangle_pc, cos_hue_angle, sin_hue_angle);
		ImVec2 uv_white = GetFontTexUvWhitePixel();
		draw_list->PrimReserve(6, 6);
		draw_list->PrimVtx(tra, uv_white, hue_color32);
		draw_list->PrimVtx(trb, uv_white, hue_color32);
		draw_list->PrimVtx(trc, uv_white, IM_COL32_WHITE);
		draw_list->PrimVtx(tra, uv_white, IM_COL32_BLACK_TRANS);
		draw_list->PrimVtx(trb, uv_white, IM_COL32_BLACK);
		draw_list->PrimVtx(trc, uv_white, IM_COL32_BLACK_TRANS);
		draw_list->AddTriangle(tra, trb, trc, IM_COL32(128, 128, 128, 255), 1.5f);
		sv_cursor_pos = ImLerp(ImLerp(trc, tra, ImSaturate(S)), trb, ImSaturate(1 - V));
	}
	else if (flags & ImGuiColorEditFlags_PickerHueBar)
	{
		// Render SV Square
		draw_list->AddRectFilledMultiColor(picker_pos, picker_pos + ImVec2(sv_picker_size, sv_picker_size), IM_COL32_WHITE, hue_color32, hue_color32, IM_COL32_WHITE);
		draw_list->AddRectFilledMultiColor(picker_pos, picker_pos + ImVec2(sv_picker_size, sv_picker_size), IM_COL32_BLACK_TRANS, IM_COL32_BLACK_TRANS, IM_COL32_BLACK, IM_COL32_BLACK);
		RenderFrameBorder(picker_pos, picker_pos + ImVec2(sv_picker_size, sv_picker_size), 0.0f);
		sv_cursor_pos.x = ImClamp((float)(int)(picker_pos.x + ImSaturate(S)     * sv_picker_size + 0.5f), picker_pos.x + 2, picker_pos.x + sv_picker_size - 2); // Sneakily prevent the circle to stick out too much
		sv_cursor_pos.y = ImClamp((float)(int)(picker_pos.y + ImSaturate(1 - V) * sv_picker_size + 0.5f), picker_pos.y + 2, picker_pos.y + sv_picker_size - 2);

		// Render Hue Bar
		for (int i = 0; i < 6; ++i)
			draw_list->AddRectFilledMultiColor(ImVec2(bar0_pos_x, picker_pos.y + i * (sv_picker_size / 6)), ImVec2(bar0_pos_x + bars_width, picker_pos.y + (i + 1) * (sv_picker_size / 6)), hue_colors[i], hue_colors[i], hue_colors[i + 1], hue_colors[i + 1]);
		float bar0_line_y = (float)(int)(picker_pos.y + H * sv_picker_size + 0.5f);
		RenderFrameBorder(ImVec2(bar0_pos_x, picker_pos.y), ImVec2(bar0_pos_x + bars_width, picker_pos.y + sv_picker_size), 0.0f);
		RenderArrowsForVerticalBar(draw_list, ImVec2(bar0_pos_x - 1, bar0_line_y), ImVec2(bars_triangles_half_sz + 1, bars_triangles_half_sz), bars_width + 2.0f);
	}

	// Render cursor/preview circle (clamp S/V within 0..1 range because floating points colors may lead HSV values to be out of range)
	draw_list->AddRect(sv_cursor_pos - ImVec2(2, 2), sv_cursor_pos + ImVec2(2, 2), ImColor(0.f, 0.f, 0.f, 1.f));

	// Render alpha bar
	if (alpha_bar)
	{
		float alpha = ImSaturate(col[3]);
		ImRect bar1_bb(bar1_pos_x, picker_pos.y, bar1_pos_x + bars_width, picker_pos.y + sv_picker_size);
		RenderColorRectWithAlphaCheckerboard(bar1_bb.Min, bar1_bb.Max, IM_COL32(0, 0, 0, 0), bar1_bb.GetWidth() / 2.0f, ImVec2(0.0f, 0.0f));
		draw_list->AddRectFilledMultiColor(bar1_bb.Min, bar1_bb.Max, col32_no_alpha, col32_no_alpha, col32_no_alpha & ~IM_COL32_A_MASK, col32_no_alpha & ~IM_COL32_A_MASK);
		float bar1_line_y = (float)(int)(picker_pos.y + (1.0f - alpha) * sv_picker_size + 0.5f);
		RenderFrameBorder(bar1_bb.Min, bar1_bb.Max, 0.0f);
		RenderArrowsForVerticalBar(draw_list, ImVec2(bar1_pos_x - 1, bar1_line_y), ImVec2(bars_triangles_half_sz + 1, bars_triangles_half_sz), bars_width + 2.0f);
	}

	EndGroup();

	if (value_changed && memcmp(backup_initial_col, col, components * sizeof(float)) == 0)
		value_changed = false;
	if (value_changed)
		MarkItemEdited(window->DC.LastItemId);

	PopID();

	return value_changed;
}


// A little colored square. Return true when clicked.
// FIXME: May want to display/ignore the alpha component in the color display? Yet show it in the tooltip.
// 'desc_id' is not called 'label' because we don't display it next to the button, but only in the tooltip.
bool ui::ColorButton(const char* desc_id, ImVec4& col, ImGuiColorEditFlags flags, ImVec2 size)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiID id = window->GetID(desc_id);
	float default_size = GetFrameHeight();
	if (size.x == 0.0f)
		size.x = default_size;
	if (size.y == 0.0f)
		size.y = default_size / 2;
	const ImRect bb(window->DC.CursorPos + ImVec2(0, 2), window->DC.CursorPos + size + ImVec2(0, 2));
	ItemSize(bb, (size.y >= default_size) ? g.Style.FramePadding.y : 0.0f);
	if (!ItemAdd(bb, id))
		return false;

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held);

	if (flags & ImGuiColorEditFlags_NoAlpha)
		flags &= ~(ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaPreviewHalf);

	ImVec4 col_without_alpha(col.x, col.y, col.z, 1.0f);
	float grid_step = ImMin(size.x, size.y) / 2.99f;
	float rounding = ImMin(g.Style.FrameRounding, grid_step * 0.5f);
	ImRect bb_inner = bb;
	float off = -0.75f; // The border (using Col_FrameBg) tends to look off when color is near-opaque and rounding is enabled. This offset seemed like a good middle ground to reduce those artifacts.
	bb_inner.Expand(off);
	if ((flags & ImGuiColorEditFlags_AlphaPreviewHalf) && col.w < 1.0f)
	{
		float mid_x = (float)(int)((bb_inner.Min.x + bb_inner.Max.x) * 0.5f + 0.5f);
		RenderColorRectWithAlphaCheckerboard(ImVec2(bb_inner.Min.x + grid_step, bb_inner.Min.y), bb_inner.Max, GetColorU32(col), grid_step, ImVec2(-grid_step + off, off), rounding, ImDrawCornerFlags_TopRight | ImDrawCornerFlags_BotRight);
		window->DrawList->AddRectFilled(bb_inner.Min, ImVec2(mid_x, bb_inner.Max.y), GetColorU32(col_without_alpha), rounding, ImDrawCornerFlags_TopLeft | ImDrawCornerFlags_BotLeft);
	}
	else
	{
		// Because GetColorU32() multiplies by the global style Alpha and we don't want to display a checkerboard if the source code had no alpha
		ImVec4 col_source = (flags & ImGuiColorEditFlags_AlphaPreview) ? col : col_without_alpha;
		if (col_source.w < 1.0f)
			RenderColorRectWithAlphaCheckerboard(bb_inner.Min, bb_inner.Max, GetColorU32(col_source), grid_step, ImVec2(off, off), rounding);
		else
			window->DrawList->AddRectFilled(bb_inner.Min, bb_inner.Max, GetColorU32(col_source), rounding, ImDrawCornerFlags_All);

		window->DrawList->AddRectFilledMultiColor(bb_inner.Min, bb_inner.Max,
			ImColor(0.f, 0.f, 0.f, 0.f), ImColor(0.f, 0.f, 0.f, 0.f),
			ImColor(0.f, 0.f, 0.f, 0.5f), ImColor(0.f, 0.f, 0.f, 0.5f));
	}

	// Drag and Drop Source
	// NB: The ActiveId test is merely an optional micro-optimization, BeginDragDropSource() does the same test.
	if (g.ActiveId == id && !(flags & ImGuiColorEditFlags_NoDragDrop) && BeginDragDropSource())
	{
		if (flags & ImGuiColorEditFlags_NoAlpha)
			SetDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F, &col, sizeof(float) * 3, ImGuiCond_Once);
		else
			SetDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F, &col, sizeof(float) * 4, ImGuiCond_Once);
		ColorButton(desc_id, col, flags);
		SameLine();
		TextUnformatted("Color");
		EndDragDropSource();
	}

	// Tooltip
	if (!(flags & ImGuiColorEditFlags_NoTooltip) && hovered)
		ColorTooltip(desc_id, &col.x, flags & (ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaPreviewHalf));

	if (pressed)
		MarkItemEdited(id);

	bool popup_open = IsPopupOpen(id);
	if (GetIO().MouseClicked[1] && hovered) {
		if (!popup_open)
			OpenPopupEx(id);
	}

	if (popup_open) {
		SetNextWindowSize(ImVec2(80, CalcMaxPopupHeightFromItemCount(2)));

		char name[16];
		ImFormatString(name, IM_ARRAYSIZE(name), "##Combo_%02d", g.BeginPopupStack.Size); // Recycle windows based on depth

		// Peak into expected window size so we can position it
		if (ImGuiWindow* popup_window = FindWindowByName(name))
			if (popup_window->WasActive)
			{
				ImVec2 size_expected = CalcWindowExpectedSize(popup_window);
				ImRect r_outer = GetWindowAllowedExtentRect(popup_window);
				ImVec2 pos = FindBestWindowPosForPopupEx(bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, bb, ImGuiPopupPositionPolicy_ComboBox);
				SetNextWindowPos(pos);
			}

		// Horizontally align ourselves with the framed text
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
		PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(g.Style.FramePadding.x, g.Style.WindowPadding.y));
		bool ret = Begin(name, NULL, window_flags);
		PopStyleVar();

		/*if (Selectable("Copy"))
			clip::set_text(
				std::to_string((int)(col.x * 255.f)) + "," +
				std::to_string((int)(col.y * 255.f)) + "," +
				std::to_string((int)(col.z * 255.f)) + "," +
				std::to_string((int)(col.w * 255.f)));

		if (Selectable("Paste")) {
			static auto split = [](std::string str, const char* del) -> std::vector<std::string>
			{
				char* pTempStr = _strdup(str.c_str());
				char* pWord = strtok(pTempStr, del);
				std::vector<std::string> dest;

				while (pWord != NULL)
				{
					dest.push_back(pWord);
					pWord = strtok(NULL, del);
				}

				free(pTempStr);

				return dest;
			};

			std::string colt = "255,255,255,255";

			if (clip::get_text(colt)) {
				std::vector<std::string> cols = split(colt, ",");
				if (cols.size() == 4) {
					col.x = std::stoi(cols.at(0)) / 255.f;
					col.y = std::stoi(cols.at(1)) / 255.f;
					col.z = std::stoi(cols.at(2)) / 255.f;
					col.w = std::stoi(cols.at(3)) / 255.f;
				}
			}
		}*/

		EndPopup();
	}

	return pressed;
}

void ui::SetColorEditOptions(ImGuiColorEditFlags flags)
{
	ImGuiContext& g = *GImGui;
	if ((flags & ImGuiColorEditFlags__InputsMask) == 0)
		flags |= ImGuiColorEditFlags__OptionsDefault & ImGuiColorEditFlags__InputsMask;
	if ((flags & ImGuiColorEditFlags__DataTypeMask) == 0)
		flags |= ImGuiColorEditFlags__OptionsDefault & ImGuiColorEditFlags__DataTypeMask;
	if ((flags & ImGuiColorEditFlags__PickerMask) == 0)
		flags |= ImGuiColorEditFlags__OptionsDefault & ImGuiColorEditFlags__PickerMask;
	IM_ASSERT(ImIsPowerOfTwo((int)(flags & ImGuiColorEditFlags__InputsMask)));   // Check only 1 option is selected
	IM_ASSERT(ImIsPowerOfTwo((int)(flags & ImGuiColorEditFlags__DataTypeMask))); // Check only 1 option is selected
	IM_ASSERT(ImIsPowerOfTwo((int)(flags & ImGuiColorEditFlags__PickerMask)));   // Check only 1 option is selected
	g.ColorEditOptions = flags;
}

// Note: only access 3 floats if ImGuiColorEditFlags_NoAlpha flag is set.
void ui::ColorTooltip(const char* text, const float* col, ImGuiColorEditFlags flags)
{
	ImGuiContext& g = *GImGui;

	int cr = IM_F32_TO_INT8_SAT(col[0]), cg = IM_F32_TO_INT8_SAT(col[1]), cb = IM_F32_TO_INT8_SAT(col[2]), ca = (flags & ImGuiColorEditFlags_NoAlpha) ? 255 : IM_F32_TO_INT8_SAT(col[3]);
	BeginTooltipEx(0, true);

	const char* text_end = text ? FindRenderedTextEnd(text, NULL) : text;
	if (text_end > text)
	{
		TextUnformatted(text, text_end);
		Separator();
	}

	ImVec2 sz(g.FontSize * 3 + g.Style.FramePadding.y * 2, g.FontSize * 3 + g.Style.FramePadding.y * 2);
	//ColorButton("##preview", ImVec4(col[0], col[1], col[2], col[3]), (flags & (ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaPreviewHalf)) | ImGuiColorEditFlags_NoTooltip, sz);
	SameLine();
	if (flags & ImGuiColorEditFlags_NoAlpha)
		Text("#%02X%02X%02X\nR: %d, G: %d, B: %d\n(%.3f, %.3f, %.3f)", cr, cg, cb, cr, cg, cb, col[0], col[1], col[2]);
	else
		Text("#%02X%02X%02X%02X\nR:%d, G:%d, B:%d, A:%d\n(%.3f, %.3f, %.3f, %.3f)", cr, cg, cb, ca, cr, cg, cb, ca, col[0], col[1], col[2], col[3]);
	EndTooltip();
}

void ui::ColorEditOptionsPopup(const float* col, ImGuiColorEditFlags flags)
{
	bool allow_opt_inputs = !(flags & ImGuiColorEditFlags__InputsMask);
	bool allow_opt_datatype = !(flags & ImGuiColorEditFlags__DataTypeMask);
	if ((!allow_opt_inputs && !allow_opt_datatype) || !BeginPopup("context"))
		return;
	ImGuiContext& g = *GImGui;
	ImGuiColorEditFlags opts = g.ColorEditOptions;
	if (allow_opt_inputs)
	{
		if (RadioButton("RGB", (opts & ImGuiColorEditFlags_RGB) != 0)) opts = (opts & ~ImGuiColorEditFlags__InputsMask) | ImGuiColorEditFlags_RGB;
		if (RadioButton("HSV", (opts & ImGuiColorEditFlags_HSV) != 0)) opts = (opts & ~ImGuiColorEditFlags__InputsMask) | ImGuiColorEditFlags_HSV;
		if (RadioButton("HEX", (opts & ImGuiColorEditFlags_HEX) != 0)) opts = (opts & ~ImGuiColorEditFlags__InputsMask) | ImGuiColorEditFlags_HEX;
	}
	if (allow_opt_datatype)
	{
		if (allow_opt_inputs) Separator();
		if (RadioButton("0..255", (opts & ImGuiColorEditFlags_Uint8) != 0)) opts = (opts & ~ImGuiColorEditFlags__DataTypeMask) | ImGuiColorEditFlags_Uint8;
		if (RadioButton("0.00..1.00", (opts & ImGuiColorEditFlags_Float) != 0)) opts = (opts & ~ImGuiColorEditFlags__DataTypeMask) | ImGuiColorEditFlags_Float;
	}

	if (allow_opt_inputs || allow_opt_datatype)
		Separator();
	if (Button("Copy as..", ImVec2(-1, 0)))
		OpenPopup("Copy");
	if (BeginPopup("Copy"))
	{
		int cr = IM_F32_TO_INT8_SAT(col[0]), cg = IM_F32_TO_INT8_SAT(col[1]), cb = IM_F32_TO_INT8_SAT(col[2]), ca = (flags & ImGuiColorEditFlags_NoAlpha) ? 255 : IM_F32_TO_INT8_SAT(col[3]);
		char buf[64];
		ImFormatString(buf, IM_ARRAYSIZE(buf), "(%.3ff, %.3ff, %.3ff, %.3ff)", col[0], col[1], col[2], (flags & ImGuiColorEditFlags_NoAlpha) ? 1.0f : col[3]);
		if (Selectable(buf))
			SetClipboardText(buf);
		ImFormatString(buf, IM_ARRAYSIZE(buf), "(%d,%d,%d,%d)", cr, cg, cb, ca);
		if (Selectable(buf))
			SetClipboardText(buf);
		if (flags & ImGuiColorEditFlags_NoAlpha)
			ImFormatString(buf, IM_ARRAYSIZE(buf), "0x%02X%02X%02X", cr, cg, cb);
		else
			ImFormatString(buf, IM_ARRAYSIZE(buf), "0x%02X%02X%02X%02X", cr, cg, cb, ca);
		if (Selectable(buf))
			SetClipboardText(buf);
		EndPopup();
	}

	g.ColorEditOptions = opts;
	EndPopup();
}

void ui::ColorPickerOptionsPopup(const float* ref_col, ImGuiColorEditFlags flags)
{
	bool allow_opt_picker = !(flags & ImGuiColorEditFlags__PickerMask);
	bool allow_opt_alpha_bar = !(flags & ImGuiColorEditFlags_NoAlpha) && !(flags & ImGuiColorEditFlags_AlphaBar);
	if ((!allow_opt_picker && !allow_opt_alpha_bar) || !BeginPopup("context"))
		return;
	ImGuiContext& g = *GImGui;
	if (allow_opt_picker)
	{
		ImVec2 picker_size(g.FontSize * 8, ImMax(g.FontSize * 8 - (GetFrameHeight() + g.Style.ItemInnerSpacing.x), 1.0f)); // FIXME: Picker size copied from main picker function
		PushItemWidth(picker_size.x);
		for (int picker_type = 0; picker_type < 2; picker_type++)
		{
			// Draw small/thumbnail version of each picker type (over an invisible button for selection)
			if (picker_type > 0) Separator();
			PushID(picker_type);
			ImGuiColorEditFlags picker_flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoSidePreview | (flags & ImGuiColorEditFlags_NoAlpha);
			if (picker_type == 0) picker_flags |= ImGuiColorEditFlags_PickerHueBar;
			if (picker_type == 1) picker_flags |= ImGuiColorEditFlags_PickerHueWheel;
			ImVec2 backup_pos = GetCursorScreenPos();
			if (Selectable("##selectable", false, 0, picker_size)) // By default, Selectable() is closing popup
				g.ColorEditOptions = (g.ColorEditOptions & ~ImGuiColorEditFlags__PickerMask) | (picker_flags & ImGuiColorEditFlags__PickerMask);
			SetCursorScreenPos(backup_pos);
			ImVec4 dummy_ref_col;
			memcpy(&dummy_ref_col, ref_col, sizeof(float) * ((picker_flags & ImGuiColorEditFlags_NoAlpha) ? 3 : 4));
			ColorPicker4("##dummypicker", &dummy_ref_col.x, picker_flags);
			PopID();
		}
		PopItemWidth();
	}
	if (allow_opt_alpha_bar)
	{
		if (allow_opt_picker) Separator();
		CheckboxFlags("Alpha Bar", (unsigned int*)&g.ColorEditOptions, ImGuiColorEditFlags_AlphaBar);
	}
	EndPopup();
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: TreeNode, CollapsingHeader, etc.
//-------------------------------------------------------------------------
// - TreeNode()
// - TreeNodeV()
// - TreeNodeEx()
// - TreeNodeExV()
// - TreeNodeBehavior() [Internal]
// - TreePush()
// - TreePop()
// - TreeAdvanceToLabelPos()
// - GetTreeNodeToLabelSpacing()
// - SetNextTreeNodeOpen()
// - CollapsingHeader()
//-------------------------------------------------------------------------

bool ui::TreeNode(const char* str_id, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	bool is_open = TreeNodeExV(str_id, 0, fmt, args);
	va_end(args);
	return is_open;
}

bool ui::TreeNode(const void* ptr_id, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	bool is_open = TreeNodeExV(ptr_id, 0, fmt, args);
	va_end(args);
	return is_open;
}

bool ui::TreeNode(const char* label)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;
	return TreeNodeBehavior(window->GetID(label), 0, label, NULL);
}

bool ui::TreeNodeV(const char* str_id, const char* fmt, va_list args)
{
	return TreeNodeExV(str_id, 0, fmt, args);
}

bool ui::TreeNodeV(const void* ptr_id, const char* fmt, va_list args)
{
	return TreeNodeExV(ptr_id, 0, fmt, args);
}

bool ui::TreeNodeEx(const char* label, ImGuiTreeNodeFlags flags)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	return TreeNodeBehavior(window->GetID(label), flags, label, NULL);
}

bool ui::TreeNodeEx(const char* str_id, ImGuiTreeNodeFlags flags, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	bool is_open = TreeNodeExV(str_id, flags, fmt, args);
	va_end(args);
	return is_open;
}

bool ui::TreeNodeEx(const void* ptr_id, ImGuiTreeNodeFlags flags, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	bool is_open = TreeNodeExV(ptr_id, flags, fmt, args);
	va_end(args);
	return is_open;
}

bool ui::TreeNodeExV(const char* str_id, ImGuiTreeNodeFlags flags, const char* fmt, va_list args)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const char* label_end = g.TempBuffer + ImFormatStringV(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), fmt, args);
	return TreeNodeBehavior(window->GetID(str_id), flags, g.TempBuffer, label_end);
}

bool ui::TreeNodeExV(const void* ptr_id, ImGuiTreeNodeFlags flags, const char* fmt, va_list args)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const char* label_end = g.TempBuffer + ImFormatStringV(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), fmt, args);
	return TreeNodeBehavior(window->GetID(ptr_id), flags, g.TempBuffer, label_end);
}

bool ui::TreeNodeBehaviorIsOpen(ImGuiID id, ImGuiTreeNodeFlags flags)
{
	if (flags & ImGuiTreeNodeFlags_Leaf)
		return true;

	// We only write to the tree storage if the user clicks (or explicitly use SetNextTreeNode*** functions)
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	ImGuiStorage* storage = window->DC.StateStorage;

	bool is_open;
	if (g.NextTreeNodeOpenCond != 0)
	{
		if (g.NextTreeNodeOpenCond & ImGuiCond_Always)
		{
			is_open = g.NextTreeNodeOpenVal;
			storage->SetInt(id, is_open);
		}
		else
		{
			// We treat ImGuiCond_Once and ImGuiCond_FirstUseEver the same because tree node state are not saved persistently.
			const int stored_value = storage->GetInt(id, -1);
			if (stored_value == -1)
			{
				is_open = g.NextTreeNodeOpenVal;
				storage->SetInt(id, is_open);
			}
			else
			{
				is_open = stored_value != 0;
			}
		}
		g.NextTreeNodeOpenCond = 0;
	}
	else
	{
		is_open = storage->GetInt(id, (flags & ImGuiTreeNodeFlags_DefaultOpen) ? 1 : 0) != 0;
	}

	// When logging is enabled, we automatically expand tree nodes (but *NOT* collapsing headers.. seems like sensible behavior).
	// NB- If we are above max depth we still allow manually opened nodes to be logged.
	if (g.LogEnabled && !(flags & ImGuiTreeNodeFlags_NoAutoOpenOnLog) && window->DC.TreeDepth < g.LogAutoExpandMaxDepth)
		is_open = true;

	return is_open;
}

bool ui::TreeNodeBehavior(ImGuiID id, ImGuiTreeNodeFlags flags, const char* label, const char* label_end)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const bool display_frame = (flags & ImGuiTreeNodeFlags_Framed) != 0;
	const ImVec2 padding = (display_frame || (flags & ImGuiTreeNodeFlags_FramePadding)) ? style.FramePadding : ImVec2(style.FramePadding.x, 0.0f);

	if (!label_end)
		label_end = FindRenderedTextEnd(label);
	const ImVec2 label_size = CalcTextSize(label, label_end, false);

	// We vertically grow up to current line height up the typical widget height.
	const float text_base_offset_y = ImMax(padding.y, window->DC.CurrentLineTextBaseOffset); // Latch before ItemSize changes it
	const float frame_height = ImMax(ImMin(window->DC.CurrentLineSize.y, g.FontSize + style.FramePadding.y * 2), label_size.y + padding.y * 2);
	ImRect frame_bb = ImRect(window->DC.CursorPos, ImVec2(window->Pos.x + GetContentRegionMax().x, window->DC.CursorPos.y + frame_height));
	if (display_frame)
	{
		// Framed header expand a little outside the default padding
		frame_bb.Min.x -= (float)(int)(window->WindowPadding.x*0.5f) - 1;
		frame_bb.Max.x += (float)(int)(window->WindowPadding.x*0.5f) - 1;
	}

	const float text_offset_x = (g.FontSize + (display_frame ? padding.x * 3 : padding.x * 2));   // Collapser arrow width + Spacing
	const float text_width = g.FontSize + (label_size.x > 0.0f ? label_size.x + padding.x * 2 : 0.0f);   // Include collapser
	ItemSize(ImVec2(text_width, frame_height), text_base_offset_y);

	// For regular tree nodes, we arbitrary allow to click past 2 worth of ItemSpacing
	// (Ideally we'd want to add a flag for the user to specify if we want the hit test to be done up to the right side of the content or not)
	const ImRect interact_bb = display_frame ? frame_bb : ImRect(frame_bb.Min.x, frame_bb.Min.y, frame_bb.Min.x + text_width + style.ItemSpacing.x * 2, frame_bb.Max.y);
	bool is_open = TreeNodeBehaviorIsOpen(id, flags);
	bool is_leaf = (flags & ImGuiTreeNodeFlags_Leaf) != 0;

	// Store a flag for the current depth to tell if we will allow closing this node when navigating one of its child.
	// For this purpose we essentially compare if g.NavIdIsAlive went from 0 to 1 between TreeNode() and TreePop().
	// This is currently only support 32 level deep and we are fine with (1 << Depth) overflowing into a zero.
	if (is_open && !g.NavIdIsAlive && (flags & ImGuiTreeNodeFlags_NavLeftJumpsBackHere) && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
		window->DC.TreeDepthMayJumpToParentOnPop |= (1 << window->DC.TreeDepth);

	bool item_add = ItemAdd(interact_bb, id);
	window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_HasDisplayRect;
	window->DC.LastItemDisplayRect = frame_bb;

	if (!item_add)
	{
		if (is_open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
			TreePushRawID(id);
		IMGUI_TEST_ENGINE_ITEM_INFO(window->DC.LastItemId, label, window->DC.ItemFlags | (is_leaf ? 0 : ImGuiItemStatusFlags_Openable) | (is_open ? ImGuiItemStatusFlags_Opened : 0));
		return is_open;
	}

	// Flags that affects opening behavior:
	// - 0 (default) .................... single-click anywhere to open
	// - OpenOnDoubleClick .............. double-click anywhere to open
	// - OpenOnArrow .................... single-click on arrow to open
	// - OpenOnDoubleClick|OpenOnArrow .. single-click on arrow or double-click anywhere to open
	ImGuiButtonFlags button_flags = ImGuiButtonFlags_NoKeyModifiers;
	if (flags & ImGuiTreeNodeFlags_AllowItemOverlap)
		button_flags |= ImGuiButtonFlags_AllowItemOverlap;
	if (flags & ImGuiTreeNodeFlags_OpenOnDoubleClick)
		button_flags |= ImGuiButtonFlags_PressedOnDoubleClick | ((flags & ImGuiTreeNodeFlags_OpenOnArrow) ? ImGuiButtonFlags_PressedOnClickRelease : 0);
	if (!is_leaf)
		button_flags |= ImGuiButtonFlags_PressedOnDragDropHold;

	bool selected = (flags & ImGuiTreeNodeFlags_Selected) != 0;
	bool hovered, held;
	bool pressed = ButtonBehavior(interact_bb, id, &hovered, &held, button_flags);
	bool toggled = false;
	if (!is_leaf)
	{
		if (pressed)
		{
			toggled = !(flags & (ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) || (g.NavActivateId == id);
			if (flags & ImGuiTreeNodeFlags_OpenOnArrow)
				toggled |= IsMouseHoveringRect(interact_bb.Min, ImVec2(interact_bb.Min.x + text_offset_x, interact_bb.Max.y)) && (!g.NavDisableMouseHover);
			if (flags & ImGuiTreeNodeFlags_OpenOnDoubleClick)
				toggled |= g.IO.MouseDoubleClicked[0];
			if (g.DragDropActive && is_open) // When using Drag and Drop "hold to open" we keep the node highlighted after opening, but never close it again.
				toggled = false;
		}

		if (g.NavId == id && g.NavMoveRequest && g.NavMoveDir == ImGuiDir_Left && is_open)
		{
			toggled = true;
			NavMoveRequestCancel();
		}
		if (g.NavId == id && g.NavMoveRequest && g.NavMoveDir == ImGuiDir_Right && !is_open) // If there's something upcoming on the line we may want to give it the priority?
		{
			toggled = true;
			NavMoveRequestCancel();
		}

		if (toggled)
		{
			is_open = !is_open;
			window->DC.StateStorage->SetInt(id, is_open);
		}
	}
	if (flags & ImGuiTreeNodeFlags_AllowItemOverlap)
		SetItemAllowOverlap();

	// Render
	const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
	const ImVec2 text_pos = frame_bb.Min + ImVec2(text_offset_x, text_base_offset_y);
	ImGuiNavHighlightFlags nav_highlight_flags = ImGuiNavHighlightFlags_TypeThin;
	if (display_frame)
	{
		// Framed type
		RenderFrame(frame_bb.Min, frame_bb.Max, col, true, style.FrameRounding);
		RenderNavHighlight(frame_bb, id, nav_highlight_flags);
		RenderArrow(frame_bb.Min + ImVec2(padding.x, text_base_offset_y), is_open ? ImGuiDir_Down : ImGuiDir_Right, 1.0f);
		if (g.LogEnabled)
		{
			// NB: '##' is normally used to hide text (as a library-wide feature), so we need to specify the text range to make sure the ## aren't stripped out here.
			const char log_prefix[] = "\n##";
			const char log_suffix[] = "##";
			LogRenderedText(&text_pos, log_prefix, log_prefix + 3);
			RenderTextClipped(text_pos, frame_bb.Max, label, label_end, &label_size);
			LogRenderedText(&text_pos, log_suffix + 1, log_suffix + 3);
		}
		else
		{
			RenderTextClipped(text_pos, frame_bb.Max, label, label_end, &label_size);
		}
	}
	else
	{
		// Unframed typed for tree nodes
		if (hovered || selected)
		{
			RenderFrame(frame_bb.Min, frame_bb.Max, col, false);
			RenderNavHighlight(frame_bb, id, nav_highlight_flags);
		}

		if (flags & ImGuiTreeNodeFlags_Bullet)
			RenderBullet(frame_bb.Min + ImVec2(text_offset_x * 0.5f, g.FontSize*0.50f + text_base_offset_y));
		else if (!is_leaf)
			RenderArrow(frame_bb.Min + ImVec2(padding.x, g.FontSize*0.15f + text_base_offset_y), is_open ? ImGuiDir_Down : ImGuiDir_Right, 0.70f);
		if (g.LogEnabled)
			LogRenderedText(&text_pos, ">");
		RenderText(text_pos, label, label_end, false);
	}

	if (is_open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
		TreePushRawID(id);
	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags | (is_leaf ? 0 : ImGuiItemStatusFlags_Openable) | (is_open ? ImGuiItemStatusFlags_Opened : 0));
	return is_open;
}

void ui::TreePush(const char* str_id)
{
	ImGuiWindow* window = GetCurrentWindow();
	Indent();
	window->DC.TreeDepth++;
	PushID(str_id ? str_id : "#TreePush");
}

void ui::TreePush(const void* ptr_id)
{
	ImGuiWindow* window = GetCurrentWindow();
	Indent();
	window->DC.TreeDepth++;
	PushID(ptr_id ? ptr_id : (const void*)"#TreePush");
}

void ui::TreePushRawID(ImGuiID id)
{
	ImGuiWindow* window = GetCurrentWindow();
	Indent();
	window->DC.TreeDepth++;
	window->IDStack.push_back(id);
}

void ui::TreePop()
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	Unindent();

	window->DC.TreeDepth--;
	if (g.NavMoveDir == ImGuiDir_Left && g.NavWindow == window && NavMoveRequestButNoResultYet())
		if (g.NavIdIsAlive && (window->DC.TreeDepthMayJumpToParentOnPop & (1 << window->DC.TreeDepth)))
		{
			SetNavID(window->IDStack.back(), g.NavLayer);
			NavMoveRequestCancel();
		}
	window->DC.TreeDepthMayJumpToParentOnPop &= (1 << window->DC.TreeDepth) - 1;

	IM_ASSERT(window->IDStack.Size > 1); // There should always be 1 element in the IDStack (pushed during window creation). If this triggers you called TreePop/PopID too much.
	PopID();
}

void ui::TreeAdvanceToLabelPos()
{
	ImGuiContext& g = *GImGui;
	g.CurrentWindow->DC.CursorPos.x += GetTreeNodeToLabelSpacing();
}

// Horizontal distance preceding label when using TreeNode() or Bullet()
float ui::GetTreeNodeToLabelSpacing()
{
	ImGuiContext& g = *GImGui;
	return g.FontSize + (g.Style.FramePadding.x * 2.0f);
}

void ui::SetNextTreeNodeOpen(bool is_open, ImGuiCond cond)
{
	ImGuiContext& g = *GImGui;
	if (g.CurrentWindow->SkipItems)
		return;
	g.NextTreeNodeOpenVal = is_open;
	g.NextTreeNodeOpenCond = cond ? cond : ImGuiCond_Always;
}

// CollapsingHeader returns true when opened but do not indent nor push into the ID stack (because of the ImGuiTreeNodeFlags_NoTreePushOnOpen flag).
// This is basically the same as calling TreeNodeEx(label, ImGuiTreeNodeFlags_CollapsingHeader). You can remove the _NoTreePushOnOpen flag if you want behavior closer to normal TreeNode().
bool ui::CollapsingHeader(const char* label, ImGuiTreeNodeFlags flags)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	return TreeNodeBehavior(window->GetID(label), flags | ImGuiTreeNodeFlags_CollapsingHeader, label);
}

bool ui::CollapsingHeader(const char* label, bool* p_open, ImGuiTreeNodeFlags flags)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	if (p_open && !*p_open)
		return false;

	ImGuiID id = window->GetID(label);
	bool is_open = TreeNodeBehavior(id, flags | ImGuiTreeNodeFlags_CollapsingHeader | (p_open ? ImGuiTreeNodeFlags_AllowItemOverlap : 0), label);
	if (p_open)
	{
		// Create a small overlapping close button // FIXME: We can evolve this into user accessible helpers to add extra buttons on title bars, headers, etc.
		ImGuiContext& g = *GImGui;
		ImGuiItemHoveredDataBackup last_item_backup;
		float button_radius = g.FontSize * 0.5f;
		ImVec2 button_center = ImVec2(ImMin(window->DC.LastItemRect.Max.x, window->ClipRect.Max.x) - g.Style.FramePadding.x - button_radius, window->DC.LastItemRect.GetCenter().y);
		if (CloseButton(window->GetID((void*)((intptr_t)id + 1)), button_center, button_radius))
			*p_open = false;
		last_item_backup.Restore();
	}

	return is_open;
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: Selectable
//-------------------------------------------------------------------------
// - Selectable()
//-------------------------------------------------------------------------

// Tip: pass a non-visible label (e.g. "##dummy") then you can use the space to draw other text or image.
// But you need to make sure the ID is unique, e.g. enclose calls in PushID/PopID or use ##unique_id.
bool ui::Selectable(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size_arg)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.ColumnsSet) // FIXME-OPT: Avoid if vertically clipped.
		PopClipRect();

	ImGuiID id = window->GetID(label);
	ImVec2 label_size = CalcTextSize(label, NULL, true);
	ImVec2 size(size_arg.x != 0.0f ? size_arg.x : label_size.x, size_arg.y != 0.0f ? size_arg.y : label_size.y + 2);
	ImVec2 pos = window->DC.CursorPos;
	pos.y += window->DC.CurrentLineTextBaseOffset;
	ImRect bb_inner(pos, pos + size);
	ItemSize(bb_inner);

	// Fill horizontal space.
	ImVec2 window_padding = window->WindowPadding;
	float max_x = (flags & ImGuiSelectableFlags_SpanAllColumns) ? GetWindowContentRegionMax().x : GetContentRegionMax().x;
	float w_draw = ImMax(label_size.x, window->Pos.x + max_x - window_padding.x - pos.x);
	ImVec2 size_draw((size_arg.x != 0 && !(flags & ImGuiSelectableFlags_DrawFillAvailWidth)) ? size_arg.x : w_draw, size_arg.y != 0.0f ? size_arg.y : size.y);
	ImRect bb(pos, pos + size_draw);
	if (size_arg.x == 0.0f || (flags & ImGuiSelectableFlags_DrawFillAvailWidth))
		bb.Max.x += window_padding.x;

	// Selectables are tightly packed together, we extend the box to cover spacing between selectable.
	float spacing_L = (float)(int)(style.ItemSpacing.x * 0.5f);
	float spacing_U = (float)(int)(style.ItemSpacing.y * 0.5f);
	float spacing_R = style.ItemSpacing.x - spacing_L;
	float spacing_D = style.ItemSpacing.y - spacing_U;
	bb.Min.x -= spacing_L;
	bb.Min.y -= spacing_U;
	bb.Max.x += spacing_R;
	bb.Max.y += spacing_D;
	if (!ItemAdd(bb, id))
	{
		if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.ColumnsSet)
			PushColumnClipRect();
		return false;
	}

	// We use NoHoldingActiveID on menus so user can click and _hold_ on a menu then drag to browse child entries
	ImGuiButtonFlags button_flags = 0;
	if (flags & ImGuiSelectableFlags_NoHoldingActiveID) button_flags |= ImGuiButtonFlags_NoHoldingActiveID;
	if (flags & ImGuiSelectableFlags_PressedOnClick) button_flags |= ImGuiButtonFlags_PressedOnClick;
	if (flags & ImGuiSelectableFlags_PressedOnRelease) button_flags |= ImGuiButtonFlags_PressedOnRelease;
	if (flags & ImGuiSelectableFlags_Disabled) button_flags |= ImGuiButtonFlags_Disabled;
	if (flags & ImGuiSelectableFlags_AllowDoubleClick) button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick;
	if (flags & ImGuiSelectableFlags_Disabled)
		selected = false;

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, button_flags);
	// Hovering selectable with mouse updates NavId accordingly so navigation can be resumed with gamepad/keyboard (this doesn't happen on most widgets)
	if (pressed || hovered)
		if (!g.NavDisableMouseHover && g.NavWindow == window && g.NavLayer == window->DC.NavLayerCurrent)
		{
			g.NavDisableHighlight = true;
			SetNavID(id, window->DC.NavLayerCurrent);
		}
	if (pressed)
		MarkItemEdited(id);

	if (hovered) {
		window->DrawList->AddRectFilled(bb.Min, bb.Max, ImColor(75 / 255.f, 50 / 255.f, 255 / 255.f, g.Style.Alpha));
		PushFont(GetIO().Fonts->Fonts[1]);
	}

	window->DrawList->PushClipRect(bb.Min, bb.Max, true);
	window->DrawList->AddText(bb.Min + ImVec2(8, size.y / 2 - label_size.y / 2 + 2), selected ? ImColor(255, 255, 255) : ImColor(160 / 255.f, 160 / 255.f, 160 / 255.f, g.Style.Alpha), label);
	window->DrawList->PopClipRect();

	if (hovered)
		PopFont();

	if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.ColumnsSet)
	{
		PushColumnClipRect();
		bb.Max.x -= (GetContentRegionMax().x - max_x);
	}

	// Automatically close popups
	if (pressed && (window->Flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiSelectableFlags_DontClosePopups) && !(window->DC.ItemFlags & ImGuiItemFlags_SelectableDontClosePopup))
		CloseCurrentPopup();
	return pressed;
}

bool ui::Selectable(const char* label, bool* p_selected, ImGuiSelectableFlags flags, const ImVec2& size_arg)
{
	if (Selectable(label, *p_selected, flags, size_arg))
	{
		*p_selected = !*p_selected;
		return true;
	}
	return false;
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: ListBox
//-------------------------------------------------------------------------
// - ListBox()
// - ListBoxHeader()
// - ListBoxFooter()
//-------------------------------------------------------------------------

// FIXME: In principle this function should be called BeginListBox(). We should rename it after re-evaluating if we want to keep the same signature.
// Helper to calculate the size of a listbox and display a label on the right.
// Tip: To have a list filling the entire window width, PushItemWidth(-1) and pass an non-visible label e.g. "##empty"
bool ui::ListBoxHeader(const char* label, const ImVec2& size_arg)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	const ImGuiStyle& style = GetStyle();
	const ImGuiID id = GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	// Size default to hold ~7 items. Fractional number of items helps seeing that we can scroll down/up without looking at scrollbar.
	ImVec2 size = CalcItemSize(size_arg, window->Size.x - 64, GetTextLineHeightWithSpacing() * 7.4f + style.ItemSpacing.y);
	ImVec2 frame_size = ImVec2(size.x, ImMax(size.y, label_size.y));
	ImRect frame_bb(window->DC.CursorPos + ImVec2(16, 0), window->DC.CursorPos + frame_size);
	ImRect bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
	window->DC.LastItemRect = bb; // Forward storage for ListBoxFooter.. dodgy.

	if (!IsRectVisible(bb.Min, bb.Max))
	{
		ItemSize(bb.GetSize(), style.FramePadding.y);
		ItemAdd(bb, 0, &frame_bb);
		return false;
	}

	BeginGroup();
	if (label_size.x > 0)
		RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

	BeginChildFrame(id, frame_bb.GetSize());
	return true;
}


bool ui::BeginChildFrameSkeet(const char* label, const ImVec2& size, ImGuiWindowFlags extra_flags)
{
	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	PushStyleColor(ImGuiCol_ChildBg, style.Colors[ImGuiCol_FrameBg]);
	PushStyleVar(ImGuiStyleVar_ChildRounding, style.FrameRounding);
	PushStyleVar(ImGuiStyleVar_ChildBorderSize, style.FrameBorderSize);
	PushStyleVar(ImGuiStyleVar_WindowPadding, style.FramePadding);
	bool ret = BeginChildSkeet(label, size, true, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding | extra_flags | ImGuiWindowFlags_ChildFrame);
	PopStyleVar(3);
	PopStyleColor();
	return ret;
}




bool ui::ListBoxHeaderSkeet(const char* label, const ImVec2& size_arg)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	const ImGuiStyle& style = GetStyle();
	const ImGuiID id = GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	// Size default to hold ~7 items. Fractional number of items helps seeing that we can scroll down/up without looking at scrollbar.
	ImVec2 size = CalcItemSize(size_arg, CalcItemWidth(), GetTextLineHeightWithSpacing() * 7.4f + style.ItemSpacing.y);
	ImVec2 frame_size = ImVec2(size.x, ImMax(size.y, label_size.y));
	ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + frame_size);
	ImRect bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
	window->DC.LastItemRect = bb;

	BeginGroup();
	if (label_size.x > 0)
		RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

	BeginChildFrame(id, frame_bb.GetSize());
	return true;
	/*
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	const ImGuiStyle& style = GetStyle();
	const ImGuiID id = GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	// Size default to hold ~7 items. Fractional number of items helps seeing that we can scroll down/up without looking at scrollbar.
	ImVec2 size = CalcItemSize(size_arg, window->Size.x - 64, GetTextLineHeightWithSpacing() * 7.4f + style.ItemSpacing.y);
	ImVec2 frame_size = ImVec2(size.x, ImMax(size.y, label_size.y));
	ImRect frame_bb(window->DC.CursorPos + ImVec2(16, 0), window->DC.CursorPos + frame_size);
	ImRect bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
	window->DC.LastItemRect = bb; // Forward storage for ListBoxFooter.. dodgy.

	if (!IsRectVisible(bb.Min, bb.Max))
	{
		ItemSize(bb.GetSize(), style.FramePadding.y);
		ItemAdd(bb, 0, &frame_bb);
		return false;
	}

	BeginGroup();
	if (label_size.x > 0)
		RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

	BeginChildFrameSkeet(label, frame_bb.GetSize(), 0);
	return true;*/
}

// FIXME: In principle this function should be called EndListBox(). We should rename it after re-evaluating if we want to keep the same signature.
bool ui::ListBoxHeader(const char* label, int items_count, int height_in_items)
{
	// Size default to hold ~7.25 items.
	// We add +25% worth of item height to allow the user to see at a glance if there are more items up/down, without looking at the scrollbar.
	// We don't add this extra bit if items_count <= height_in_items. It is slightly dodgy, because it means a dynamic list of items will make the widget resize occasionally when it crosses that size.
	// I am expecting that someone will come and complain about this behavior in a remote future, then we can advise on a better solution.
	if (height_in_items < 0)
		height_in_items = ImMin(items_count, 7);
	const ImGuiStyle& style = GetStyle();
	float height_in_items_f = (height_in_items < items_count) ? (height_in_items + 0.25f) : (height_in_items + 0.00f);

	// We include ItemSpacing.y so that a list sized for the exact number of items doesn't make a scrollbar appears. We could also enforce that by passing a flag to BeginChild().
	ImVec2 size;
	size.x = 0.0f;
	size.y = GetTextLineHeightWithSpacing() * height_in_items_f + style.FramePadding.y * 2.0f;
	return ListBoxHeader(label, size);
}

bool ui::ListBoxHeaderSkeet(const char* label, int items_count, int height_in_items)
{
	// Size default to hold ~7.25 items.
	// We add +25% worth of item height to allow the user to see at a glance if there are more items up/down, without looking at the scrollbar.
	// We don't add this extra bit if items_count <= height_in_items. It is slightly dodgy, because it means a dynamic list of items will make the widget resize occasionally when it crosses that size.
	// I am expecting that someone will come and complain about this behavior in a remote future, then we can advise on a better solution.
	if (height_in_items < 0)
		height_in_items = ImMin(items_count, 7);
	const ImGuiStyle& style = GetStyle();
	float height_in_items_f = (height_in_items < items_count) ? (height_in_items + 0.25f) : (height_in_items + 0.00f);

	// We include ItemSpacing.y so that a list sized for the exact number of items doesn't make a scrollbar appears. We could also enforce that by passing a flag to BeginChild().
	ImVec2 size;
	size.x = 0.0f;
	size.y = GetTextLineHeightWithSpacing() * height_in_items_f + style.FramePadding.y * 2.0f;
	return ListBoxHeaderSkeet(label, size);
}

// FIXME: In principle this function should be called EndListBox(). We should rename it after re-evaluating if we want to keep the same signature.
void ui::ListBoxFooter()
{
	ImGuiWindow* parent_window = GetCurrentWindow()->ParentWindow;
	const ImRect bb = parent_window->DC.LastItemRect;
	const ImGuiStyle& style = GetStyle();

	EndChildFrame();

	// Redeclare item size so that it includes the label (we have stored the full size in LastItemRect)
	// We call SameLine() to restore DC.CurrentLine* data
	SameLine();
	parent_window->DC.CursorPos = bb.Min;
	ItemSize(bb, style.FramePadding.y);
	EndGroup();
}
void ui::ListBoxFooterSkeet()
{
	ImGuiWindow* parent_window = GetCurrentWindow()->ParentWindow;
	const ImRect bb = parent_window->DC.LastItemRect;
	const ImGuiStyle& style = GetStyle();

	EndChildFrameSkeet();

	// Redeclare item size so that it includes the label (we have stored the full size in LastItemRect)
	// We call SameLine() to restore DC.CurrentLine* data
	SameLine();
	parent_window->DC.CursorPos = bb.Min;
	ItemSize(bb, style.FramePadding.y);
	EndGroup();
}
bool ui::ListBox(const char* label, int* current_item, const char* const items[], int items_count, int height_items)
{
	const bool value_changed = ListBox(label, current_item, Items_ArrayGetter, (void*)items, items_count, height_items);
	return value_changed;
}

bool ui::ListBox(const char* label, int* current_item, bool(*items_getter)(void*, int, const char**), void* data, int items_count, int height_in_items)
{
	if (!ListBoxHeader(label, items_count, height_in_items))
		return false;

	// Assume all items have even height (= 1 line of text). If you need items of different or variable sizes you can create a custom version of ListBox() in your code without using the clipper.
	ImGuiContext& g = *GImGui;
	bool value_changed = false;
	ImGuiListClipper clipper(items_count, GetTextLineHeightWithSpacing()); // We know exactly our line height here so we pass it as a minor optimization, but generally you don't need to.
	while (clipper.Step())
		for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
		{
			const bool item_selected = (i == *current_item);
			const char* item_text;
			if (!items_getter(data, i, &item_text))
				item_text = "*Unknown item*";

			PushID(i);
			if (Selectable(item_text, item_selected))
			{
				*current_item = i;
				value_changed = true;
			}
			if (item_selected)
				SetItemDefaultFocus();
			PopID();
		}
	ListBoxFooter();
	if (value_changed)
		MarkItemEdited(g.CurrentWindow->DC.LastItemId);

	return value_changed;
}

bool ui::ListBoxSkeet(const char* label, int* current_item, bool(*items_getter)(void*, int, const char**), void* data, int items_count, int height_in_items)
{
	
	
	if (!ListBoxHeaderSkeet(label, items_count, height_in_items))
		return false;

	// Assume all items have even height (= 1 line of text). If you need items of different or variable sizes you can create a custom version of ListBox() in your code without using the clipper.
	ImGuiContext& g = *GImGui;
	bool value_changed = false;
	ImGuiListClipper clipper(items_count, GetTextLineHeightWithSpacing()); // We know exactly our line height here so we pass it as a minor optimization, but generally you don't need to.
	while (clipper.Step())
		for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
		{
			const bool item_selected = (i == *current_item);
			const char* item_text;
			if (!items_getter(data, i, &item_text))
				item_text = "*Unknown item*";

			PushID(i);
			if (SelectableSkeet(item_text, item_selected))
			{
				*current_item = i;
				value_changed = true;
			}
			if (item_selected)
				SetItemDefaultFocus();
			PopID();
		}
	ListBoxFooterSkeet();
	if (value_changed)
		MarkItemEdited(g.CurrentWindow->DC.LastItemId);

	return value_changed;
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: PlotLines, PlotHistogram
//-------------------------------------------------------------------------
// - PlotEx() [Internal]
// - PlotLines()
// - PlotHistogram()
//-------------------------------------------------------------------------

void ui::PlotEx(ImGuiPlotType plot_type, const char* label, float(*values_getter)(void* data, int idx), void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 frame_size)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);

	const ImVec2 label_size = CalcTextSize(label, NULL, true);
	if (frame_size.x == 0.0f)
		frame_size.x = CalcItemWidth();
	if (frame_size.y == 0.0f)
		frame_size.y = label_size.y + (style.FramePadding.y * 2);

	const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + frame_size);
	const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
	const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0));
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, 0, &frame_bb))
		return;
	const bool hovered = ItemHoverable(frame_bb, id);

	// Determine scale from values if not specified
	if (scale_min == FLT_MAX || scale_max == FLT_MAX)
	{
		float v_min = FLT_MAX;
		float v_max = -FLT_MAX;
		for (int i = 0; i < values_count; i++)
		{
			const float v = values_getter(data, i);
			v_min = ImMin(v_min, v);
			v_max = ImMax(v_max, v);
		}
		if (scale_min == FLT_MAX)
			scale_min = v_min;
		if (scale_max == FLT_MAX)
			scale_max = v_max;
	}

	RenderFrame(frame_bb.Min, frame_bb.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

	if (values_count > 0)
	{
		int res_w = ImMin((int)frame_size.x, values_count) + ((plot_type == ImGuiPlotType_Lines) ? -1 : 0);
		int item_count = values_count + ((plot_type == ImGuiPlotType_Lines) ? -1 : 0);

		// Tooltip on hover
		int v_hovered = -1;
		if (hovered && inner_bb.Contains(g.IO.MousePos))
		{
			const float t = ImClamp((g.IO.MousePos.x - inner_bb.Min.x) / (inner_bb.Max.x - inner_bb.Min.x), 0.0f, 0.9999f);
			const int v_idx = (int)(t * item_count);
			IM_ASSERT(v_idx >= 0 && v_idx < values_count);

			const float v0 = values_getter(data, (v_idx + values_offset) % values_count);
			const float v1 = values_getter(data, (v_idx + 1 + values_offset) % values_count);
			if (plot_type == ImGuiPlotType_Lines)
				SetTooltip("%d: %8.4g\n%d: %8.4g", v_idx, v0, v_idx + 1, v1);
			else if (plot_type == ImGuiPlotType_Histogram)
				SetTooltip("%d: %8.4g", v_idx, v0);
			v_hovered = v_idx;
		}

		const float t_step = 1.0f / (float)res_w;
		const float inv_scale = (scale_min == scale_max) ? 0.0f : (1.0f / (scale_max - scale_min));

		float v0 = values_getter(data, (0 + values_offset) % values_count);
		float t0 = 0.0f;
		ImVec2 tp0 = ImVec2(t0, 1.0f - ImSaturate((v0 - scale_min) * inv_scale));                       // Point in the normalized space of our target rectangle
		float histogram_zero_line_t = (scale_min * scale_max < 0.0f) ? (-scale_min * inv_scale) : (scale_min < 0.0f ? 0.0f : 1.0f);   // Where does the zero line stands

		const ImU32 col_base = GetColorU32((plot_type == ImGuiPlotType_Lines) ? ImGuiCol_PlotLines : ImGuiCol_PlotHistogram);
		const ImU32 col_hovered = GetColorU32((plot_type == ImGuiPlotType_Lines) ? ImGuiCol_PlotLinesHovered : ImGuiCol_PlotHistogramHovered);

		for (int n = 0; n < res_w; n++)
		{
			const float t1 = t0 + t_step;
			const int v1_idx = (int)(t0 * item_count + 0.5f);
			IM_ASSERT(v1_idx >= 0 && v1_idx < values_count);
			const float v1 = values_getter(data, (v1_idx + values_offset + 1) % values_count);
			const ImVec2 tp1 = ImVec2(t1, 1.0f - ImSaturate((v1 - scale_min) * inv_scale));

			// NB: Draw calls are merged together by the DrawList system. Still, we should render our batch are lower level to save a bit of CPU.
			ImVec2 pos0 = ImLerp(inner_bb.Min, inner_bb.Max, tp0);
			ImVec2 pos1 = ImLerp(inner_bb.Min, inner_bb.Max, (plot_type == ImGuiPlotType_Lines) ? tp1 : ImVec2(tp1.x, histogram_zero_line_t));
			if (plot_type == ImGuiPlotType_Lines)
			{
				window->DrawList->AddLine(pos0, pos1, v_hovered == v1_idx ? col_hovered : col_base);
			}
			else if (plot_type == ImGuiPlotType_Histogram)
			{
				if (pos1.x >= pos0.x + 2.0f)
					pos1.x -= 1.0f;
				window->DrawList->AddRectFilled(pos0, pos1, v_hovered == v1_idx ? col_hovered : col_base);
			}

			t0 = t1;
			tp0 = tp1;
		}
	}

	// Text overlay
	if (overlay_text)
		RenderTextClipped(ImVec2(frame_bb.Min.x, frame_bb.Min.y + style.FramePadding.y), frame_bb.Max, overlay_text, NULL, NULL, ImVec2(0.5f, 0.0f));

	if (label_size.x > 0.0f)
		RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, inner_bb.Min.y), label);
}

struct ImGuiPlotArrayGetterData
{
	const float* Values;
	int Stride;

	ImGuiPlotArrayGetterData(const float* values, int stride) { Values = values; Stride = stride; }
};

static float Plot_ArrayGetter(void* data, int idx)
{
	ImGuiPlotArrayGetterData* plot_data = (ImGuiPlotArrayGetterData*)data;
	const float v = *(const float*)(const void*)((const unsigned char*)plot_data->Values + (size_t)idx * plot_data->Stride);
	return v;
}

void ui::PlotLines(const char* label, const float* values, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size, int stride)
{
	ImGuiPlotArrayGetterData data(values, stride);
	PlotEx(ImGuiPlotType_Lines, label, &Plot_ArrayGetter, (void*)&data, values_count, values_offset, overlay_text, scale_min, scale_max, graph_size);
}

void ui::PlotLines(const char* label, float(*values_getter)(void* data, int idx), void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size)
{
	PlotEx(ImGuiPlotType_Lines, label, values_getter, data, values_count, values_offset, overlay_text, scale_min, scale_max, graph_size);
}

void ui::PlotHistogram(const char* label, const float* values, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size, int stride)
{
	ImGuiPlotArrayGetterData data(values, stride);
	PlotEx(ImGuiPlotType_Histogram, label, &Plot_ArrayGetter, (void*)&data, values_count, values_offset, overlay_text, scale_min, scale_max, graph_size);
}

void ui::PlotHistogram(const char* label, float(*values_getter)(void* data, int idx), void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size)
{
	PlotEx(ImGuiPlotType_Histogram, label, values_getter, data, values_count, values_offset, overlay_text, scale_min, scale_max, graph_size);
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: Value helpers
// Those is not very useful, legacy API.
//-------------------------------------------------------------------------
// - Value()
//-------------------------------------------------------------------------

void ui::Value(const char* prefix, bool b)
{
	Text("%s: %s", prefix, (b ? "true" : "false"));
}

void ui::Value(const char* prefix, int v)
{
	Text("%s: %d", prefix, v);
}

void ui::Value(const char* prefix, unsigned int v)
{
	Text("%s: %d", prefix, v);
}

void ui::Value(const char* prefix, float v, const char* float_format)
{
	if (float_format)
	{
		char fmt[64];
		ImFormatString(fmt, IM_ARRAYSIZE(fmt), "%%s: %s", float_format);
		Text(fmt, prefix, v);
	}
	else
	{
		Text("%s: %.3f", prefix, v);
	}
}

//-------------------------------------------------------------------------
// [SECTION] MenuItem, BeginMenu, EndMenu, etc.
//-------------------------------------------------------------------------
// - ImGuiMenuColumns [Internal]
// - BeginMainMenuBar()
// - EndMainMenuBar()
// - BeginMenuBar()
// - EndMenuBar()
// - BeginMenu()
// - EndMenu()
// - MenuItem()
//-------------------------------------------------------------------------

// Helpers for internal use
ImGuiMenuColumns::ImGuiMenuColumns()
{
	Count = 0;
	Spacing = Width = NextWidth = 0.0f;
	memset(Pos, 0, sizeof(Pos));
	memset(NextWidths, 0, sizeof(NextWidths));
}

void ImGuiMenuColumns::Update(int count, float spacing, bool clear)
{
	IM_ASSERT(Count <= IM_ARRAYSIZE(Pos));
	Count = count;
	Width = NextWidth = 0.0f;
	Spacing = spacing;
	if (clear) memset(NextWidths, 0, sizeof(NextWidths));
	for (int i = 0; i < Count; i++)
	{
		if (i > 0 && NextWidths[i] > 0.0f)
			Width += Spacing;
		Pos[i] = (float)(int)Width;
		Width += NextWidths[i];
		NextWidths[i] = 0.0f;
	}
}

float ImGuiMenuColumns::DeclColumns(float w0, float w1, float w2) // not using va_arg because they promote float to double
{
	NextWidth = 0.0f;
	NextWidths[0] = ImMax(NextWidths[0], w0);
	NextWidths[1] = ImMax(NextWidths[1], w1);
	NextWidths[2] = ImMax(NextWidths[2], w2);
	for (int i = 0; i < 3; i++)
		NextWidth += NextWidths[i] + ((i > 0 && NextWidths[i] > 0.0f) ? Spacing : 0.0f);
	return ImMax(Width, NextWidth);
}

float ImGuiMenuColumns::CalcExtraSpace(float avail_w)
{
	return ImMax(0.0f, avail_w - Width);
}

// For the main menu bar, which cannot be moved, we honor g.Style.DisplaySafeAreaPadding to ensure text can be visible on a TV set.
bool ui::BeginMainMenuBar()
{
	ImGuiContext& g = *GImGui;
	g.NextWindowData.MenuBarOffsetMinVal = ImVec2(g.Style.DisplaySafeAreaPadding.x, ImMax(g.Style.DisplaySafeAreaPadding.y - g.Style.FramePadding.y, 0.0f));
	SetNextWindowPos(ImVec2(0.0f, 0.0f));
	SetNextWindowSize(ImVec2(g.IO.DisplaySize.x, g.NextWindowData.MenuBarOffsetMinVal.y + g.FontBaseSize + g.Style.FramePadding.y));
	PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, 0));
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
	bool is_open = Begin("##MainMenuBar", NULL, window_flags) && BeginMenuBar();
	PopStyleVar(2);
	g.NextWindowData.MenuBarOffsetMinVal = ImVec2(0.0f, 0.0f);
	if (!is_open)
	{
		End();
		return false;
	}
	return true; //-V1020
}

void ui::EndMainMenuBar()
{
	EndMenuBar();

	// When the user has left the menu layer (typically: closed menus through activation of an item), we restore focus to the previous window
	ImGuiContext& g = *GImGui;
	if (g.CurrentWindow == g.NavWindow && g.NavLayer == 0)
		FocusPreviousWindowIgnoringOne(g.NavWindow);

	End();
}

bool ui::BeginMenuBar()
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;
	if (!(window->Flags & ImGuiWindowFlags_MenuBar))
		return false;

	IM_ASSERT(!window->DC.MenuBarAppending);
	BeginGroup(); // Backup position on layer 0
	PushID("##menubar");

	// We don't clip with current window clipping rectangle as it is already set to the area below. However we clip with window full rect.
	// We remove 1 worth of rounding to Max.x to that text in long menus and small windows don't tend to display over the lower-right rounded area, which looks particularly glitchy.
	ImRect bar_rect = window->MenuBarRect();
	ImRect clip_rect(ImFloor(bar_rect.Min.x + 0.5f), ImFloor(bar_rect.Min.y + window->WindowBorderSize + 0.5f), ImFloor(ImMax(bar_rect.Min.x, bar_rect.Max.x - window->WindowRounding) + 0.5f), ImFloor(bar_rect.Max.y + 0.5f));
	clip_rect.ClipWith(window->OuterRectClipped);
	PushClipRect(clip_rect.Min, clip_rect.Max, false);

	window->DC.CursorPos = ImVec2(bar_rect.Min.x + window->DC.MenuBarOffset.x, bar_rect.Min.y + window->DC.MenuBarOffset.y);
	window->DC.LayoutType = ImGuiLayoutType_Horizontal;
	window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
	window->DC.NavLayerCurrentMask = (1 << ImGuiNavLayer_Menu);
	window->DC.MenuBarAppending = true;
	AlignTextToFramePadding();
	return true;
}

void ui::EndMenuBar()
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;
	ImGuiContext& g = *GImGui;

	// Nav: When a move request within one of our child menu failed, capture the request to navigate among our siblings.
	if (NavMoveRequestButNoResultYet() && (g.NavMoveDir == ImGuiDir_Left || g.NavMoveDir == ImGuiDir_Right) && (g.NavWindow->Flags & ImGuiWindowFlags_ChildMenu))
	{
		ImGuiWindow* nav_earliest_child = g.NavWindow;
		while (nav_earliest_child->ParentWindow && (nav_earliest_child->ParentWindow->Flags & ImGuiWindowFlags_ChildMenu))
			nav_earliest_child = nav_earliest_child->ParentWindow;
		if (nav_earliest_child->ParentWindow == window && nav_earliest_child->DC.ParentLayoutType == ImGuiLayoutType_Horizontal && g.NavMoveRequestForward == ImGuiNavForward_None)
		{
			// To do so we claim focus back, restore NavId and then process the movement request for yet another frame.
			// This involve a one-frame delay which isn't very problematic in this situation. We could remove it by scoring in advance for multiple window (probably not worth the hassle/cost)
			IM_ASSERT(window->DC.NavLayerActiveMaskNext & 0x02); // Sanity check
			FocusWindow(window);
			SetNavIDWithRectRel(window->NavLastIds[1], 1, window->NavRectRel[1]);
			g.NavLayer = ImGuiNavLayer_Menu;
			g.NavDisableHighlight = true; // Hide highlight for the current frame so we don't see the intermediary selection.
			g.NavMoveRequestForward = ImGuiNavForward_ForwardQueued;
			NavMoveRequestCancel();
		}
	}

	IM_ASSERT(window->Flags & ImGuiWindowFlags_MenuBar);
	IM_ASSERT(window->DC.MenuBarAppending);
	PopClipRect();
	PopID();
	window->DC.MenuBarOffset.x = window->DC.CursorPos.x - window->MenuBarRect().Min.x; // Save horizontal position so next append can reuse it. This is kinda equivalent to a per-layer CursorPos.
	window->DC.GroupStack.back().AdvanceCursor = false;
	EndGroup(); // Restore position on layer 0
	window->DC.LayoutType = ImGuiLayoutType_Vertical;
	window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
	window->DC.NavLayerCurrentMask = (1 << ImGuiNavLayer_Main);
	window->DC.MenuBarAppending = false;
}

bool ui::BeginMenu(const char* label, bool enabled)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);

	ImVec2 label_size = CalcTextSize(label, NULL, true);

	bool pressed;
	bool menu_is_open = IsPopupOpen(id);
	bool menuset_is_open = !(window->Flags & ImGuiWindowFlags_Popup) && (g.OpenPopupStack.Size > g.BeginPopupStack.Size && g.OpenPopupStack[g.BeginPopupStack.Size].OpenParentId == window->IDStack.back());
	ImGuiWindow* backed_nav_window = g.NavWindow;
	if (menuset_is_open)
		g.NavWindow = window;  // Odd hack to allow hovering across menus of a same menu-set (otherwise we wouldn't be able to hover parent)

	// The reference position stored in popup_pos will be used by Begin() to find a suitable position for the child menu,
	// However the final position is going to be different! It is choosen by FindBestWindowPosForPopup().
	// e.g. Menus tend to overlap each other horizontally to amplify relative Z-ordering.
	ImVec2 popup_pos, pos = window->DC.CursorPos;
	if (window->DC.LayoutType == ImGuiLayoutType_Horizontal)
	{
		// Menu inside an horizontal menu bar
		// Selectable extend their highlight by half ItemSpacing in each direction.
		// For ChildMenu, the popup position will be overwritten by the call to FindBestWindowPosForPopup() in Begin()
		popup_pos = ImVec2(pos.x - 1.0f - (float)(int)(style.ItemSpacing.x * 0.5f), pos.y - style.FramePadding.y + window->MenuBarHeight());
		window->DC.CursorPos.x += (float)(int)(style.ItemSpacing.x * 0.5f);
		PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing * 2.0f);
		float w = label_size.x;
		pressed = Selectable(label, menu_is_open, ImGuiSelectableFlags_NoHoldingActiveID | ImGuiSelectableFlags_PressedOnClick | ImGuiSelectableFlags_DontClosePopups | (!enabled ? ImGuiSelectableFlags_Disabled : 0), ImVec2(w, 0.0f));
		PopStyleVar();
		window->DC.CursorPos.x += (float)(int)(style.ItemSpacing.x * (-1.0f + 0.5f)); // -1 spacing to compensate the spacing added when Selectable() did a SameLine(). It would also work to call SameLine() ourselves after the PopStyleVar().
	}
	else
	{
		// Menu inside a menu
		popup_pos = ImVec2(pos.x, pos.y - style.WindowPadding.y);
		float w = window->MenuColumns.DeclColumns(label_size.x, 0.0f, (float)(int)(g.FontSize * 1.20f)); // Feedback to next frame
		float extra_w = ImMax(0.0f, GetContentRegionAvail().x - w);
		pressed = Selectable(label, menu_is_open, ImGuiSelectableFlags_NoHoldingActiveID | ImGuiSelectableFlags_PressedOnClick | ImGuiSelectableFlags_DontClosePopups | ImGuiSelectableFlags_DrawFillAvailWidth | (!enabled ? ImGuiSelectableFlags_Disabled : 0), ImVec2(w, 0.0f));
		if (!enabled) PushStyleColor(ImGuiCol_Text, g.Style.Colors[ImGuiCol_TextDisabled]);
		RenderArrow(pos + ImVec2(window->MenuColumns.Pos[2] + extra_w + g.FontSize * 0.30f, 0.0f), ImGuiDir_Right);
		if (!enabled) PopStyleColor();
	}

	const bool hovered = enabled && ItemHoverable(window->DC.LastItemRect, id);
	if (menuset_is_open)
		g.NavWindow = backed_nav_window;

	bool want_open = false, want_close = false;
	if (window->DC.LayoutType == ImGuiLayoutType_Vertical) // (window->Flags & (ImGuiWindowFlags_Popup|ImGuiWindowFlags_ChildMenu))
	{
		// Implement http://bjk5.com/post/44698559168/breaking-down-amazons-mega-dropdown to avoid using timers, so menus feels more reactive.
		bool moving_within_opened_triangle = false;
		if (g.HoveredWindow == window && g.OpenPopupStack.Size > g.BeginPopupStack.Size && g.OpenPopupStack[g.BeginPopupStack.Size].ParentWindow == window && !(window->Flags & ImGuiWindowFlags_MenuBar))
		{
			if (ImGuiWindow* next_window = g.OpenPopupStack[g.BeginPopupStack.Size].Window)
			{
				// FIXME-DPI: Values should be derived from a master "scale" factor.
				ImRect next_window_rect = next_window->Rect();
				ImVec2 ta = g.IO.MousePos - g.IO.MouseDelta;
				ImVec2 tb = (window->Pos.x < next_window->Pos.x) ? next_window_rect.GetTL() : next_window_rect.GetTR();
				ImVec2 tc = (window->Pos.x < next_window->Pos.x) ? next_window_rect.GetBL() : next_window_rect.GetBR();
				float extra = ImClamp(ImFabs(ta.x - tb.x) * 0.30f, 5.0f, 30.0f); // add a bit of extra slack.
				ta.x += (window->Pos.x < next_window->Pos.x) ? -0.5f : +0.5f;    // to avoid numerical issues
				tb.y = ta.y + ImMax((tb.y - extra) - ta.y, -100.0f);             // triangle is maximum 200 high to limit the slope and the bias toward large sub-menus // FIXME: Multiply by fb_scale?
				tc.y = ta.y + ImMin((tc.y + extra) - ta.y, +100.0f);
				moving_within_opened_triangle = ImTriangleContainsPoint(ta, tb, tc, g.IO.MousePos);
				//window->DrawList->PushClipRectFullScreen(); window->DrawList->AddTriangleFilled(ta, tb, tc, moving_within_opened_triangle ? IM_COL32(0,128,0,128) : IM_COL32(128,0,0,128)); window->DrawList->PopClipRect(); // Debug
			}
		}

		want_close = (menu_is_open && !hovered && g.HoveredWindow == window && g.HoveredIdPreviousFrame != 0 && g.HoveredIdPreviousFrame != id && !moving_within_opened_triangle);
		want_open = (!menu_is_open && hovered && !moving_within_opened_triangle) || (!menu_is_open && hovered && pressed);

		if (g.NavActivateId == id)
		{
			want_close = menu_is_open;
			want_open = !menu_is_open;
		}
		if (g.NavId == id && g.NavMoveRequest && g.NavMoveDir == ImGuiDir_Right) // Nav-Right to open
		{
			want_open = true;
			NavMoveRequestCancel();
		}
	}
	else
	{
		// Menu bar
		if (menu_is_open && pressed && menuset_is_open) // Click an open menu again to close it
		{
			want_close = true;
			want_open = menu_is_open = false;
		}
		else if (pressed || (hovered && menuset_is_open && !menu_is_open)) // First click to open, then hover to open others
		{
			want_open = true;
		}
		else if (g.NavId == id && g.NavMoveRequest && g.NavMoveDir == ImGuiDir_Down) // Nav-Down to open
		{
			want_open = true;
			NavMoveRequestCancel();
		}
	}

	if (!enabled) // explicitly close if an open menu becomes disabled, facilitate users code a lot in pattern such as 'if (BeginMenu("options", has_object)) { ..use object.. }'
		want_close = true;
	if (want_close && IsPopupOpen(id))
		ClosePopupToLevel(g.BeginPopupStack.Size, true);

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags | ImGuiItemStatusFlags_Openable | (menu_is_open ? ImGuiItemStatusFlags_Opened : 0));

	if (!menu_is_open && want_open && g.OpenPopupStack.Size > g.BeginPopupStack.Size)
	{
		// Don't recycle same menu level in the same frame, first close the other menu and yield for a frame.
		OpenPopup(label);
		return false;
	}

	menu_is_open |= want_open;
	if (want_open)
		OpenPopup(label);

	if (menu_is_open)
	{
		// Sub-menus are ChildWindow so that mouse can be hovering across them (otherwise top-most popup menu would steal focus and not allow hovering on parent menu)
		SetNextWindowPos(popup_pos, ImGuiCond_Always);
		ImGuiWindowFlags flags = ImGuiWindowFlags_ChildMenu | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavFocus;
		if (window->Flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_ChildMenu))
			flags |= ImGuiWindowFlags_ChildWindow;
		menu_is_open = BeginPopupEx(id, flags); // menu_is_open can be 'false' when the popup is completely clipped (e.g. zero size display)
	}

	return menu_is_open;
}

void ui::EndMenu()
{
	// Nav: When a left move request _within our child menu_ failed, close ourselves (the _parent_ menu).
	// A menu doesn't close itself because EndMenuBar() wants the catch the last Left<>Right inputs.
	// However, it means that with the current code, a BeginMenu() from outside another menu or a menu-bar won't be closable with the Left direction.
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	if (g.NavWindow && g.NavWindow->ParentWindow == window && g.NavMoveDir == ImGuiDir_Left && NavMoveRequestButNoResultYet() && window->DC.LayoutType == ImGuiLayoutType_Vertical)
	{
		ClosePopupToLevel(g.BeginPopupStack.Size, true);
		NavMoveRequestCancel();
	}

	EndPopup();
}

bool ui::MenuItem(const char* label, const char* shortcut, bool selected, bool enabled)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	ImGuiStyle& style = g.Style;
	ImVec2 pos = window->DC.CursorPos;
	ImVec2 label_size = CalcTextSize(label, NULL, true);

	ImGuiSelectableFlags flags = ImGuiSelectableFlags_PressedOnRelease | (enabled ? 0 : ImGuiSelectableFlags_Disabled);
	bool pressed;
	if (window->DC.LayoutType == ImGuiLayoutType_Horizontal)
	{
		// Mimic the exact layout spacing of BeginMenu() to allow MenuItem() inside a menu bar, which is a little misleading but may be useful
		// Note that in this situation we render neither the shortcut neither the selected tick mark
		float w = label_size.x;
		window->DC.CursorPos.x += (float)(int)(style.ItemSpacing.x * 0.5f);
		PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing * 2.0f);
		pressed = Selectable(label, false, flags, ImVec2(w, 0.0f));
		PopStyleVar();
		window->DC.CursorPos.x += (float)(int)(style.ItemSpacing.x * (-1.0f + 0.5f)); // -1 spacing to compensate the spacing added when Selectable() did a SameLine(). It would also work to call SameLine() ourselves after the PopStyleVar().
	}
	else
	{
		ImVec2 shortcut_size = shortcut ? CalcTextSize(shortcut, NULL) : ImVec2(0.0f, 0.0f);
		float w = window->MenuColumns.DeclColumns(label_size.x, shortcut_size.x, (float)(int)(g.FontSize * 1.20f)); // Feedback for next frame
		float extra_w = ImMax(0.0f, GetContentRegionAvail().x - w);
		pressed = Selectable(label, false, flags | ImGuiSelectableFlags_DrawFillAvailWidth, ImVec2(w, 0.0f));
		if (shortcut_size.x > 0.0f)
		{
			PushStyleColor(ImGuiCol_Text, g.Style.Colors[ImGuiCol_TextDisabled]);
			RenderText(pos + ImVec2(window->MenuColumns.Pos[1] + extra_w, 0.0f), shortcut, NULL, false);
			PopStyleColor();
		}
		if (selected)
			RenderCheckMark(pos + ImVec2(window->MenuColumns.Pos[2] + extra_w + g.FontSize * 0.40f, g.FontSize * 0.134f * 0.5f), GetColorU32(enabled ? ImGuiCol_Text : ImGuiCol_TextDisabled), g.FontSize  * 0.866f);
	}

	IMGUI_TEST_ENGINE_ITEM_INFO(window->DC.LastItemId, label, window->DC.ItemFlags | ImGuiItemStatusFlags_Checkable | (selected ? ImGuiItemStatusFlags_Checked : 0));
	return pressed;
}

bool ui::MenuItem(const char* label, const char* shortcut, bool* p_selected, bool enabled)
{
	if (MenuItem(label, shortcut, p_selected ? *p_selected : false, enabled))
	{
		if (p_selected)
			*p_selected = !*p_selected;
		return true;
	}
	return false;
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: BeginTabBar, EndTabBar, etc.
//-------------------------------------------------------------------------
// [BETA API] API may evolve! This code has been extracted out of the Docking branch,
// and some of the construct which are not used in Master may be left here to facilitate merging.
//-------------------------------------------------------------------------
// - BeginTabBar()
// - BeginTabBarEx() [Internal]
// - EndTabBar()
// - TabBarLayout() [Internal]
// - TabBarCalcTabID() [Internal]
// - TabBarCalcMaxTabWidth() [Internal]
// - TabBarFindTabById() [Internal]
// - TabBarRemoveTab() [Internal]
// - TabBarCloseTab() [Internal]
// - TabBarScrollClamp()v
// - TabBarScrollToTab() [Internal]
// - TabBarQueueChangeTabOrder() [Internal]
// - TabBarScrollingButtons() [Internal]
// - TabBarTabListPopupButton() [Internal]
//-------------------------------------------------------------------------

namespace ui
{
	static void				RenderOuterBorders(ImGuiWindow* window);
	static void             TabBarLayout(ImGuiTabBar* tab_bar);
	static ImU32            TabBarCalcTabID(ImGuiTabBar* tab_bar, const char* label);
	static float            TabBarCalcMaxTabWidth();
	static float            TabBarScrollClamp(ImGuiTabBar* tab_bar, float scrolling);
	static void             TabBarScrollToTab(ImGuiTabBar* tab_bar, ImGuiTabItem* tab);
	static ImGuiTabItem*    TabBarScrollingButtons(ImGuiTabBar* tab_bar);
	static ImGuiTabItem*    TabBarTabListPopupButton(ImGuiTabBar* tab_bar);
}

ImGuiTabBar::ImGuiTabBar()
{
	ID = 0;
	SelectedTabId = NextSelectedTabId = VisibleTabId = 0;
	CurrFrameVisible = PrevFrameVisible = -1;
	ContentsHeight = 0.0f;
	OffsetMax = OffsetNextTab = 0.0f;
	ScrollingAnim = ScrollingTarget = 0.0f;
	Flags = ImGuiTabBarFlags_None;
	ReorderRequestTabId = 0;
	ReorderRequestDir = 0;
	WantLayout = VisibleTabWasSubmitted = false;
	LastTabItemIdx = -1;
}

static int IMGUI_CDECL TabItemComparerByVisibleOffset(const void* lhs, const void* rhs)
{
	const ImGuiTabItem* a = (const ImGuiTabItem*)lhs;
	const ImGuiTabItem* b = (const ImGuiTabItem*)rhs;
	return (int)(a->Offset - b->Offset);
}

static int IMGUI_CDECL TabBarSortItemComparer(const void* lhs, const void* rhs)
{
	const ImGuiTabBarSortItem* a = (const ImGuiTabBarSortItem*)lhs;
	const ImGuiTabBarSortItem* b = (const ImGuiTabBarSortItem*)rhs;
	if (int d = (int)(b->Width - a->Width))
		return d;
	return (b->Index - a->Index);
}

bool    ui::BeginTabBar(const char* str_id, ImGuiTabBarFlags flags)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	if (window->SkipItems)
		return false;

	ImGuiID id = window->GetID(str_id);
	ImGuiTabBar* tab_bar = g.TabBars.GetOrAddByKey(id);
	ImRect tab_bar_bb = ImRect(window->DC.CursorPos.x, window->DC.CursorPos.y, window->InnerClipRect.Max.x, window->DC.CursorPos.y + g.FontSize + g.Style.FramePadding.y * 2);
	tab_bar->ID = id;
	return BeginTabBarEx(tab_bar, tab_bar_bb, flags | ImGuiTabBarFlags_IsFocused);
}

bool    ui::BeginTabBarEx(ImGuiTabBar* tab_bar, const ImRect& tab_bar_bb, ImGuiTabBarFlags flags)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	if (window->SkipItems)
		return false;

	if ((flags & ImGuiTabBarFlags_DockNode) == 0)
		window->IDStack.push_back(tab_bar->ID);

	g.CurrentTabBar.push_back(tab_bar);
	if (tab_bar->CurrFrameVisible == g.FrameCount)
	{
		//IMGUI_DEBUG_LOG("BeginTabBarEx already called this frame\n", g.FrameCount);
		IM_ASSERT(0);
		return true;
	}

	// When toggling back from ordered to manually-reorderable, shuffle tabs to enforce the last visible order.
	// Otherwise, the most recently inserted tabs would move at the end of visible list which can be a little too confusing or magic for the user.
	if ((flags & ImGuiTabBarFlags_Reorderable) && !(tab_bar->Flags & ImGuiTabBarFlags_Reorderable) && tab_bar->Tabs.Size > 1 && tab_bar->PrevFrameVisible != -1)
		ImQsort(tab_bar->Tabs.Data, tab_bar->Tabs.Size, sizeof(ImGuiTabItem), TabItemComparerByVisibleOffset);

	// Flags
	if ((flags & ImGuiTabBarFlags_FittingPolicyMask_) == 0)
		flags |= ImGuiTabBarFlags_FittingPolicyDefault_;

	tab_bar->Flags = flags;
	tab_bar->BarRect = tab_bar_bb;
	tab_bar->WantLayout = true; // Layout will be done on the first call to ItemTab()
	tab_bar->PrevFrameVisible = tab_bar->CurrFrameVisible;
	tab_bar->CurrFrameVisible = g.FrameCount;
	tab_bar->FramePadding = g.Style.FramePadding;

	// Layout
	ItemSize(ImVec2(tab_bar->OffsetMax, tab_bar->BarRect.GetHeight()));
	window->DC.CursorPos.x = tab_bar->BarRect.Min.x;

	return true;
}

void    ui::EndTabBar()
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	if (window->SkipItems)
		return;

	IM_ASSERT(!g.CurrentTabBar.empty());      // Mismatched BeginTabBar/EndTabBar
	ImGuiTabBar* tab_bar = g.CurrentTabBar.back();
	if (tab_bar->WantLayout)
		TabBarLayout(tab_bar);

	// Restore the last visible height if no tab is visible, this reduce vertical flicker/movement when a tabs gets removed without calling SetTabItemClosed().
	const bool tab_bar_appearing = (tab_bar->PrevFrameVisible + 1 < g.FrameCount);
	if (tab_bar->VisibleTabWasSubmitted || tab_bar->VisibleTabId == 0 || tab_bar_appearing)
		tab_bar->ContentsHeight = ImMax(window->DC.CursorPos.y - tab_bar->BarRect.Max.y, 0.0f);
	else
		window->DC.CursorPos.y = tab_bar->BarRect.Max.y + tab_bar->ContentsHeight;

	if ((tab_bar->Flags & ImGuiTabBarFlags_DockNode) == 0)
		PopID();
	g.CurrentTabBar.pop_back();
}

// This is called only once a frame before by the first call to ItemTab()
// The reason we're not calling it in BeginTabBar() is to leave a chance to the user to call the SetTabItemClosed() functions.
static void ui::TabBarLayout(ImGuiTabBar* tab_bar)
{
	ImGuiContext& g = *GImGui;
	tab_bar->WantLayout = false;

	// Garbage collect
	int tab_dst_n = 0;
	for (int tab_src_n = 0; tab_src_n < tab_bar->Tabs.Size; tab_src_n++)
	{
		ImGuiTabItem* tab = &tab_bar->Tabs[tab_src_n];
		if (tab->LastFrameVisible < tab_bar->PrevFrameVisible)
		{
			if (tab->ID == tab_bar->SelectedTabId)
				tab_bar->SelectedTabId = 0;
			continue;
		}
		if (tab_dst_n != tab_src_n)
			tab_bar->Tabs[tab_dst_n] = tab_bar->Tabs[tab_src_n];
		tab_dst_n++;
	}
	if (tab_bar->Tabs.Size != tab_dst_n)
		tab_bar->Tabs.resize(tab_dst_n);

	// Setup next selected tab
	ImGuiID scroll_track_selected_tab_id = 0;
	if (tab_bar->NextSelectedTabId)
	{
		tab_bar->SelectedTabId = tab_bar->NextSelectedTabId;
		tab_bar->NextSelectedTabId = 0;
		scroll_track_selected_tab_id = tab_bar->SelectedTabId;
	}

	// Process order change request (we could probably process it when requested but it's just saner to do it in a single spot).
	if (tab_bar->ReorderRequestTabId != 0)
	{
		if (ImGuiTabItem* tab1 = TabBarFindTabByID(tab_bar, tab_bar->ReorderRequestTabId))
		{
			//IM_ASSERT(tab_bar->Flags & ImGuiTabBarFlags_Reorderable); // <- this may happen when using debug tools
			int tab2_order = tab_bar->GetTabOrder(tab1) + tab_bar->ReorderRequestDir;
			if (tab2_order >= 0 && tab2_order < tab_bar->Tabs.Size)
			{
				ImGuiTabItem* tab2 = &tab_bar->Tabs[tab2_order];
				ImGuiTabItem item_tmp = *tab1;
				*tab1 = *tab2;
				*tab2 = item_tmp;
				if (tab2->ID == tab_bar->SelectedTabId)
					scroll_track_selected_tab_id = tab2->ID;
				tab1 = tab2 = NULL;
			}
			if (tab_bar->Flags & ImGuiTabBarFlags_SaveSettings)
				MarkIniSettingsDirty();
		}
		tab_bar->ReorderRequestTabId = 0;
	}

	// Tab List Popup (will alter tab_bar->BarRect and therefore the available width!)
	const bool tab_list_popup_button = (tab_bar->Flags & ImGuiTabBarFlags_TabListPopupButton) != 0;
	if (tab_list_popup_button)
		if (ImGuiTabItem* tab_to_select = TabBarTabListPopupButton(tab_bar)) // NB: Will alter BarRect.Max.x!
			scroll_track_selected_tab_id = tab_bar->SelectedTabId = tab_to_select->ID;

	ImVector<ImGuiTabBarSortItem>& width_sort_buffer = g.TabSortByWidthBuffer;
	width_sort_buffer.resize(tab_bar->Tabs.Size);

	// Compute ideal widths
	float width_total_contents = 0.0f;
	ImGuiTabItem* most_recently_selected_tab = NULL;
	bool found_selected_tab_id = false;
	for (int tab_n = 0; tab_n < tab_bar->Tabs.Size; tab_n++)
	{
		ImGuiTabItem* tab = &tab_bar->Tabs[tab_n];
		IM_ASSERT(tab->LastFrameVisible >= tab_bar->PrevFrameVisible);

		if (most_recently_selected_tab == NULL || most_recently_selected_tab->LastFrameSelected < tab->LastFrameSelected)
			most_recently_selected_tab = tab;
		if (tab->ID == tab_bar->SelectedTabId)
			found_selected_tab_id = true;

		// Refresh tab width immediately, otherwise changes of style e.g. style.FramePadding.x would noticeably lag in the tab bar.
		// Additionally, when using TabBarAddTab() to manipulate tab bar order we occasionally insert new tabs that don't have a width yet,
		// and we cannot wait for the next BeginTabItem() call. We cannot compute this width within TabBarAddTab() because font size depends on the active window.
		const char* tab_name = tab_bar->GetTabName(tab);
		tab->WidthContents = TabItemCalcSize(tab_name, (tab->Flags & ImGuiTabItemFlags_NoCloseButton) ? false : true).x;

		width_total_contents += (tab_n > 0 ? g.Style.ItemInnerSpacing.x : 0.0f) + tab->WidthContents;

		// Store data so we can build an array sorted by width if we need to shrink tabs down
		width_sort_buffer[tab_n].Index = tab_n;
		width_sort_buffer[tab_n].Width = tab->WidthContents;
	}

	// Compute width
	const float width_avail = tab_bar->BarRect.GetWidth();
	float width_excess = (width_avail < width_total_contents) ? (width_total_contents - width_avail) : 0.0f;
	if (width_excess > 0.0f && (tab_bar->Flags & ImGuiTabBarFlags_FittingPolicyResizeDown))
	{
		// If we don't have enough room, resize down the largest tabs first
		if (tab_bar->Tabs.Size > 1)
			ImQsort(width_sort_buffer.Data, (size_t)width_sort_buffer.Size, sizeof(ImGuiTabBarSortItem), TabBarSortItemComparer);
		int tab_count_same_width = 1;
		while (width_excess > 0.0f && tab_count_same_width < tab_bar->Tabs.Size)
		{
			while (tab_count_same_width < tab_bar->Tabs.Size && width_sort_buffer[0].Width == width_sort_buffer[tab_count_same_width].Width)
				tab_count_same_width++;
			float width_to_remove_per_tab_max = (tab_count_same_width < tab_bar->Tabs.Size) ? (width_sort_buffer[0].Width - width_sort_buffer[tab_count_same_width].Width) : (width_sort_buffer[0].Width - 1.0f);
			float width_to_remove_per_tab = ImMin(width_excess / tab_count_same_width, width_to_remove_per_tab_max);
			for (int tab_n = 0; tab_n < tab_count_same_width; tab_n++)
				width_sort_buffer[tab_n].Width -= width_to_remove_per_tab;
			width_excess -= width_to_remove_per_tab * tab_count_same_width;
		}
		for (int tab_n = 0; tab_n < tab_bar->Tabs.Size; tab_n++)
			tab_bar->Tabs[width_sort_buffer[tab_n].Index].Width = (float)(int)width_sort_buffer[tab_n].Width;
	}
	else
	{
		const float tab_max_width = TabBarCalcMaxTabWidth();
		for (int tab_n = 0; tab_n < tab_bar->Tabs.Size; tab_n++)
		{
			ImGuiTabItem* tab = &tab_bar->Tabs[tab_n];
			tab->Width = ImMin(tab->WidthContents, tab_max_width);
		}
	}

	// Layout all active tabs
	float offset_x = 0.0f;
	for (int tab_n = 0; tab_n < tab_bar->Tabs.Size; tab_n++)
	{
		ImGuiTabItem* tab = &tab_bar->Tabs[tab_n];
		tab->Offset = offset_x;
		if (scroll_track_selected_tab_id == 0 && g.NavJustMovedToId == tab->ID)
			scroll_track_selected_tab_id = tab->ID;
		offset_x += tab->Width + g.Style.ItemInnerSpacing.x;
	}
	tab_bar->OffsetMax = ImMax(offset_x - g.Style.ItemInnerSpacing.x, 0.0f);
	tab_bar->OffsetNextTab = 0.0f;

	// Horizontal scrolling buttons
	const bool scrolling_buttons = (tab_bar->OffsetMax > tab_bar->BarRect.GetWidth() && tab_bar->Tabs.Size > 1) && !(tab_bar->Flags & ImGuiTabBarFlags_NoTabListScrollingButtons) && (tab_bar->Flags & ImGuiTabBarFlags_FittingPolicyScroll);
	if (scrolling_buttons)
		if (ImGuiTabItem* tab_to_select = TabBarScrollingButtons(tab_bar)) // NB: Will alter BarRect.Max.x!
			scroll_track_selected_tab_id = tab_bar->SelectedTabId = tab_to_select->ID;

	// If we have lost the selected tab, select the next most recently active one
	if (found_selected_tab_id == false)
		tab_bar->SelectedTabId = 0;
	if (tab_bar->SelectedTabId == 0 && tab_bar->NextSelectedTabId == 0 && most_recently_selected_tab != NULL)
		scroll_track_selected_tab_id = tab_bar->SelectedTabId = most_recently_selected_tab->ID;

	// Lock in visible tab
	tab_bar->VisibleTabId = tab_bar->SelectedTabId;
	tab_bar->VisibleTabWasSubmitted = false;

	// Update scrolling
	if (scroll_track_selected_tab_id)
		if (ImGuiTabItem* scroll_track_selected_tab = TabBarFindTabByID(tab_bar, scroll_track_selected_tab_id))
			TabBarScrollToTab(tab_bar, scroll_track_selected_tab);
	tab_bar->ScrollingAnim = TabBarScrollClamp(tab_bar, tab_bar->ScrollingAnim);
	tab_bar->ScrollingTarget = TabBarScrollClamp(tab_bar, tab_bar->ScrollingTarget);
	const float scrolling_speed = (tab_bar->PrevFrameVisible + 1 < g.FrameCount) ? FLT_MAX : (g.IO.DeltaTime * g.FontSize * 70.0f);
	if (tab_bar->ScrollingAnim != tab_bar->ScrollingTarget)
		tab_bar->ScrollingAnim = ImLinearSweep(tab_bar->ScrollingAnim, tab_bar->ScrollingTarget, scrolling_speed);

	// Clear name buffers
	if ((tab_bar->Flags & ImGuiTabBarFlags_DockNode) == 0)
		tab_bar->TabsNames.Buf.resize(0);
}

// Dockables uses Name/ID in the global namespace. Non-dockable items use the ID stack.
static ImU32   ui::TabBarCalcTabID(ImGuiTabBar* tab_bar, const char* label)
{
	if (tab_bar->Flags & ImGuiTabBarFlags_DockNode)
	{
		ImGuiID id = ImHashStr(label, 0);
		KeepAliveID(id);
		return id;
	}
	else
	{
		ImGuiWindow* window = GImGui->CurrentWindow;
		return window->GetID(label);
	}
}

static float ui::TabBarCalcMaxTabWidth()
{
	ImGuiContext& g = *GImGui;
	return g.FontSize * 20.0f;
}

ImGuiTabItem* ui::TabBarFindTabByID(ImGuiTabBar* tab_bar, ImGuiID tab_id)
{
	if (tab_id != 0)
		for (int n = 0; n < tab_bar->Tabs.Size; n++)
			if (tab_bar->Tabs[n].ID == tab_id)
				return &tab_bar->Tabs[n];
	return NULL;
}

// The *TabId fields be already set by the docking system _before_ the actual TabItem was created, so we clear them regardless.
void ui::TabBarRemoveTab(ImGuiTabBar* tab_bar, ImGuiID tab_id)
{
	if (ImGuiTabItem* tab = TabBarFindTabByID(tab_bar, tab_id))
		tab_bar->Tabs.erase(tab);
	if (tab_bar->VisibleTabId == tab_id) { tab_bar->VisibleTabId = 0; }
	if (tab_bar->SelectedTabId == tab_id) { tab_bar->SelectedTabId = 0; }
	if (tab_bar->NextSelectedTabId == tab_id) { tab_bar->NextSelectedTabId = 0; }
}

// Called on manual closure attempt
void ui::TabBarCloseTab(ImGuiTabBar* tab_bar, ImGuiTabItem* tab)
{
	if ((tab_bar->VisibleTabId == tab->ID) && !(tab->Flags & ImGuiTabItemFlags_UnsavedDocument))
	{
		// This will remove a frame of lag for selecting another tab on closure.
		// However we don't run it in the case where the 'Unsaved' flag is set, so user gets a chance to fully undo the closure
		tab->LastFrameVisible = -1;
		tab_bar->SelectedTabId = tab_bar->NextSelectedTabId = 0;
	}
	else if ((tab_bar->VisibleTabId != tab->ID) && (tab->Flags & ImGuiTabItemFlags_UnsavedDocument))
	{
		// Actually select before expecting closure
		tab_bar->NextSelectedTabId = tab->ID;
	}
}

static float ui::TabBarScrollClamp(ImGuiTabBar* tab_bar, float scrolling)
{
	scrolling = ImMin(scrolling, tab_bar->OffsetMax - tab_bar->BarRect.GetWidth());
	return ImMax(scrolling, 0.0f);
}

static void ui::TabBarScrollToTab(ImGuiTabBar* tab_bar, ImGuiTabItem* tab)
{
	ImGuiContext& g = *GImGui;
	float margin = g.FontSize * 1.0f; // When to scroll to make Tab N+1 visible always make a bit of N visible to suggest more scrolling area (since we don't have a scrollbar)
	int order = tab_bar->GetTabOrder(tab);
	float tab_x1 = tab->Offset + (order > 0 ? -margin : 0.0f);
	float tab_x2 = tab->Offset + tab->Width + (order + 1 < tab_bar->Tabs.Size ? margin : 1.0f);
	if (tab_bar->ScrollingTarget > tab_x1)
		tab_bar->ScrollingTarget = tab_x1;
	if (tab_bar->ScrollingTarget + tab_bar->BarRect.GetWidth() < tab_x2)
		tab_bar->ScrollingTarget = tab_x2 - tab_bar->BarRect.GetWidth();
}

void ui::TabBarQueueChangeTabOrder(ImGuiTabBar* tab_bar, const ImGuiTabItem* tab, int dir)
{
	IM_ASSERT(dir == -1 || dir == +1);
	IM_ASSERT(tab_bar->ReorderRequestTabId == 0);
	tab_bar->ReorderRequestTabId = tab->ID;
	tab_bar->ReorderRequestDir = dir;
}

static ImGuiTabItem* ui::TabBarScrollingButtons(ImGuiTabBar* tab_bar)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;

	const ImVec2 arrow_button_size(g.FontSize - 2.0f, g.FontSize + g.Style.FramePadding.y * 2.0f);
	const float scrolling_buttons_width = arrow_button_size.x * 2.0f;

	const ImVec2 backup_cursor_pos = window->DC.CursorPos;
	//window->DrawList->AddRect(ImVec2(tab_bar->BarRect.Max.x - scrolling_buttons_width, tab_bar->BarRect.Min.y), ImVec2(tab_bar->BarRect.Max.x, tab_bar->BarRect.Max.y), IM_COL32(255,0,0,255));

	const ImRect avail_bar_rect = tab_bar->BarRect;
	bool want_clip_rect = !avail_bar_rect.Contains(ImRect(window->DC.CursorPos, window->DC.CursorPos + ImVec2(scrolling_buttons_width, 0.0f)));
	if (want_clip_rect)
		PushClipRect(tab_bar->BarRect.Min, tab_bar->BarRect.Max + ImVec2(g.Style.ItemInnerSpacing.x, 0.0f), true);

	ImGuiTabItem* tab_to_select = NULL;

	int select_dir = 0;
	ImVec4 arrow_col = g.Style.Colors[ImGuiCol_Text];
	arrow_col.w *= 0.5f;

	PushStyleColor(ImGuiCol_Text, arrow_col);
	PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	const float backup_repeat_delay = g.IO.KeyRepeatDelay;
	const float backup_repeat_rate = g.IO.KeyRepeatRate;
	g.IO.KeyRepeatDelay = 0.250f;
	g.IO.KeyRepeatRate = 0.200f;
	window->DC.CursorPos = ImVec2(tab_bar->BarRect.Max.x - scrolling_buttons_width, tab_bar->BarRect.Min.y);
	if (ArrowButtonEx("##<", ImGuiDir_Left, arrow_button_size, ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_Repeat))
		select_dir = -1;
	window->DC.CursorPos = ImVec2(tab_bar->BarRect.Max.x - scrolling_buttons_width + arrow_button_size.x, tab_bar->BarRect.Min.y);
	if (ArrowButtonEx("##>", ImGuiDir_Right, arrow_button_size, ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_Repeat))
		select_dir = +1;
	PopStyleColor(2);
	g.IO.KeyRepeatRate = backup_repeat_rate;
	g.IO.KeyRepeatDelay = backup_repeat_delay;

	if (want_clip_rect)
		PopClipRect();

	if (select_dir != 0)
		if (ImGuiTabItem* tab_item = TabBarFindTabByID(tab_bar, tab_bar->SelectedTabId))
		{
			int selected_order = tab_bar->GetTabOrder(tab_item);
			int target_order = selected_order + select_dir;
			tab_to_select = &tab_bar->Tabs[(target_order >= 0 && target_order < tab_bar->Tabs.Size) ? target_order : selected_order]; // If we are at the end of the list, still scroll to make our tab visible
		}
	window->DC.CursorPos = backup_cursor_pos;
	tab_bar->BarRect.Max.x -= scrolling_buttons_width + 1.0f;

	return tab_to_select;
}

static ImGuiTabItem* ui::TabBarTabListPopupButton(ImGuiTabBar* tab_bar)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;

	// We use g.Style.FramePadding.y to match the square ArrowButton size
	const float tab_list_popup_button_width = g.FontSize + g.Style.FramePadding.y;
	const ImVec2 backup_cursor_pos = window->DC.CursorPos;
	window->DC.CursorPos = ImVec2(tab_bar->BarRect.Min.x - g.Style.FramePadding.y, tab_bar->BarRect.Min.y);
	tab_bar->BarRect.Min.x += tab_list_popup_button_width;

	ImVec4 arrow_col = g.Style.Colors[ImGuiCol_Text];
	arrow_col.w *= 0.5f;
	PushStyleColor(ImGuiCol_Text, arrow_col);
	PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	bool open = BeginCombo("##v", NULL, ImGuiComboFlags_NoPreview);
	PopStyleColor(2);

	ImGuiTabItem* tab_to_select = NULL;
	if (open)
	{
		for (int tab_n = 0; tab_n < tab_bar->Tabs.Size; tab_n++)
		{
			ImGuiTabItem* tab = &tab_bar->Tabs[tab_n];
			const char* tab_name = tab_bar->GetTabName(tab);
			if (Selectable(tab_name, tab_bar->SelectedTabId == tab->ID))
				tab_to_select = tab;
		}
		EndCombo();
	}

	window->DC.CursorPos = backup_cursor_pos;
	return tab_to_select;
}

//-------------------------------------------------------------------------
// [SECTION] Widgets: BeginTabItem, EndTabItem, etc.
//-------------------------------------------------------------------------
// [BETA API] API may evolve! This code has been extracted out of the Docking branch,
// and some of the construct which are not used in Master may be left here to facilitate merging.
//-------------------------------------------------------------------------
// - BeginTabItem()
// - EndTabItem()
// - TabItemEx() [Internal]
// - SetTabItemClosed()
// - TabItemCalcSize() [Internal]
// - TabItemBackground() [Internal]
// - TabItemLabelAndCloseButton() [Internal]
//-------------------------------------------------------------------------

bool    ui::BeginTabItem(const char* label, bool* p_open, ImGuiTabItemFlags flags)
{
	ImGuiContext& g = *GImGui;
	if (g.CurrentWindow->SkipItems)
		return false;

	IM_ASSERT(g.CurrentTabBar.Size > 0 && "Needs to be called between BeginTabBar() and EndTabBar()!");
	ImGuiTabBar* tab_bar = g.CurrentTabBar.back();
	bool ret = TabItemEx(tab_bar, label, p_open, flags);
	if (ret && !(flags & ImGuiTabItemFlags_NoPushId))
	{
		ImGuiTabItem* tab = &tab_bar->Tabs[tab_bar->LastTabItemIdx];
		g.CurrentWindow->IDStack.push_back(tab->ID);    // We already hashed 'label' so push into the ID stack directly instead of doing another hash through PushID(label)
	}
	return ret;
}

void    ui::EndTabItem()
{
	ImGuiContext& g = *GImGui;
	if (g.CurrentWindow->SkipItems)
		return;

	IM_ASSERT(g.CurrentTabBar.Size > 0 && "Needs to be called between BeginTabBar() and EndTabBar()!");
	ImGuiTabBar* tab_bar = g.CurrentTabBar.back();
	IM_ASSERT(tab_bar->LastTabItemIdx >= 0 && "Needs to be called between BeginTabItem() and EndTabItem()");
	ImGuiTabItem* tab = &tab_bar->Tabs[tab_bar->LastTabItemIdx];
	if (!(tab->Flags & ImGuiTabItemFlags_NoPushId))
		g.CurrentWindow->IDStack.pop_back();
}

bool    ui::TabItemEx(ImGuiTabBar* tab_bar, const char* label, bool* p_open, ImGuiTabItemFlags flags)
{
	// Layout whole tab bar if not already done
	if (tab_bar->WantLayout)
		TabBarLayout(tab_bar);

	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	if (window->SkipItems)
		return false;

	const ImGuiStyle& style = g.Style;
	const ImGuiID id = TabBarCalcTabID(tab_bar, label);

	// If the user called us with *p_open == false, we early out and don't render. We make a dummy call to ItemAdd() so that attempts to use a contextual popup menu with an implicit ID won't use an older ID.
	if (p_open && !*p_open)
	{
		PushItemFlag(ImGuiItemFlags_NoNav | ImGuiItemFlags_NoNavDefaultFocus, true);
		ItemAdd(ImRect(), id);
		PopItemFlag();
		return false;
	}

	// Calculate tab contents size
	ImVec2 size = TabItemCalcSize(label, p_open != NULL);

	// Acquire tab data
	ImGuiTabItem* tab = TabBarFindTabByID(tab_bar, id);
	bool tab_is_new = false;
	if (tab == NULL)
	{
		tab_bar->Tabs.push_back(ImGuiTabItem());
		tab = &tab_bar->Tabs.back();
		tab->ID = id;
		tab->Width = size.x;
		tab_is_new = true;
	}
	tab_bar->LastTabItemIdx = (short)tab_bar->Tabs.index_from_ptr(tab);
	tab->WidthContents = size.x;

	if (p_open == NULL)
		flags |= ImGuiTabItemFlags_NoCloseButton;

	const bool tab_bar_appearing = (tab_bar->PrevFrameVisible + 1 < g.FrameCount);
	const bool tab_bar_focused = (tab_bar->Flags & ImGuiTabBarFlags_IsFocused) != 0;
	const bool tab_appearing = (tab->LastFrameVisible + 1 < g.FrameCount);
	tab->LastFrameVisible = g.FrameCount;
	tab->Flags = flags;

	// Append name with zero-terminator
	tab->NameOffset = tab_bar->TabsNames.size();
	tab_bar->TabsNames.append(label, label + strlen(label) + 1);

	// If we are not reorderable, always reset offset based on submission order.
	// (We already handled layout and sizing using the previous known order, but sizing is not affected by order!)
	if (!tab_appearing && !(tab_bar->Flags & ImGuiTabBarFlags_Reorderable))
	{
		tab->Offset = tab_bar->OffsetNextTab;
		tab_bar->OffsetNextTab += tab->Width + g.Style.ItemInnerSpacing.x;
	}

	// Update selected tab
	if (tab_appearing && (tab_bar->Flags & ImGuiTabBarFlags_AutoSelectNewTabs) && tab_bar->NextSelectedTabId == 0)
		if (!tab_bar_appearing || tab_bar->SelectedTabId == 0)
			tab_bar->NextSelectedTabId = id;  // New tabs gets activated

	// Lock visibility
	bool tab_contents_visible = (tab_bar->VisibleTabId == id);
	if (tab_contents_visible)
		tab_bar->VisibleTabWasSubmitted = true;

	// On the very first frame of a tab bar we let first tab contents be visible to minimize appearing glitches
	if (!tab_contents_visible && tab_bar->SelectedTabId == 0 && tab_bar_appearing)
		if (tab_bar->Tabs.Size == 1 && !(tab_bar->Flags & ImGuiTabBarFlags_AutoSelectNewTabs))
			tab_contents_visible = true;

	if (tab_appearing && !(tab_bar_appearing && !tab_is_new))
	{
		PushItemFlag(ImGuiItemFlags_NoNav | ImGuiItemFlags_NoNavDefaultFocus, true);
		ItemAdd(ImRect(), id);
		PopItemFlag();
		return tab_contents_visible;
	}

	if (tab_bar->SelectedTabId == id)
		tab->LastFrameSelected = g.FrameCount;

	// Backup current layout position
	const ImVec2 backup_main_cursor_pos = window->DC.CursorPos;

	// Layout
	size.x = tab->Width;
	window->DC.CursorPos = tab_bar->BarRect.Min + ImVec2((float)(int)tab->Offset - tab_bar->ScrollingAnim, 0.0f);
	ImVec2 pos = window->DC.CursorPos;
	ImRect bb(pos, pos + size);

	// We don't have CPU clipping primitives to clip the CloseButton (until it becomes a texture), so need to add an extra draw call (temporary in the case of vertical animation)
	bool want_clip_rect = (bb.Min.x < tab_bar->BarRect.Min.x) || (bb.Max.x >= tab_bar->BarRect.Max.x);
	if (want_clip_rect)
		PushClipRect(ImVec2(ImMax(bb.Min.x, tab_bar->BarRect.Min.x), bb.Min.y - 1), ImVec2(tab_bar->BarRect.Max.x, bb.Max.y), true);

	ItemSize(bb, style.FramePadding.y);
	if (!ItemAdd(bb, id))
	{
		if (want_clip_rect)
			PopClipRect();
		window->DC.CursorPos = backup_main_cursor_pos;
		return tab_contents_visible;
	}

	// Click to Select a tab
	ImGuiButtonFlags button_flags = (ImGuiButtonFlags_PressedOnClick | ImGuiButtonFlags_AllowItemOverlap);
	if (g.DragDropActive)
		button_flags |= ImGuiButtonFlags_PressedOnDragDropHold;
	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, button_flags);
	hovered |= (g.HoveredId == id);
	if (pressed || ((flags & ImGuiTabItemFlags_SetSelected) && !tab_contents_visible)) // SetSelected can only be passed on explicit tab bar
		tab_bar->NextSelectedTabId = id;

	// Allow the close button to overlap unless we are dragging (in which case we don't want any overlapping tabs to be hovered)
	if (!held)
		SetItemAllowOverlap();

	// Drag and drop: re-order tabs
	if (held && !tab_appearing && IsMouseDragging(0))
	{
		if (!g.DragDropActive && (tab_bar->Flags & ImGuiTabBarFlags_Reorderable))
		{
			// While moving a tab it will jump on the other side of the mouse, so we also test for MouseDelta.x
			if (g.IO.MouseDelta.x < 0.0f && g.IO.MousePos.x < bb.Min.x)
			{
				if (tab_bar->Flags & ImGuiTabBarFlags_Reorderable)
					TabBarQueueChangeTabOrder(tab_bar, tab, -1);
			}
			else if (g.IO.MouseDelta.x > 0.0f && g.IO.MousePos.x > bb.Max.x)
			{
				if (tab_bar->Flags & ImGuiTabBarFlags_Reorderable)
					TabBarQueueChangeTabOrder(tab_bar, tab, +1);
			}
		}
	}

#if 0
	if (hovered && g.HoveredIdNotActiveTimer > 0.50f && bb.GetWidth() < tab->WidthContents)
	{
		// Enlarge tab display when hovering
		bb.Max.x = bb.Min.x + (float)(int)ImLerp(bb.GetWidth(), tab->WidthContents, ImSaturate((g.HoveredIdNotActiveTimer - 0.40f) * 6.0f));
		display_draw_list = GetOverlayDrawList(window);
		TabItemBackground(display_draw_list, bb, flags, GetColorU32(ImGuiCol_TitleBgActive));
	}
#endif

	// Render tab shape
	ImDrawList* display_draw_list = window->DrawList;
	const ImU32 tab_col = GetColorU32((held || hovered) ? ImGuiCol_TabHovered : tab_contents_visible ? (tab_bar_focused ? ImGuiCol_TabActive : ImGuiCol_TabUnfocusedActive) : (tab_bar_focused ? ImGuiCol_Tab : ImGuiCol_TabUnfocused));
	TabItemBackground(display_draw_list, bb, flags, tab_col);
	RenderNavHighlight(bb, id);

	// Select with right mouse button. This is so the common idiom for context menu automatically highlight the current widget.
	const bool hovered_unblocked = IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
	if (hovered_unblocked && (IsMouseClicked(1) || IsMouseReleased(1)))
		tab_bar->NextSelectedTabId = id;

	if (tab_bar->Flags & ImGuiTabBarFlags_NoCloseWithMiddleMouseButton)
		flags |= ImGuiTabItemFlags_NoCloseWithMiddleMouseButton;

	// Render tab label, process close button
	const ImGuiID close_button_id = p_open ? window->GetID((void*)((intptr_t)id + 1)) : 0;
	bool just_closed = TabItemLabelAndCloseButton(display_draw_list, bb, flags, tab_bar->FramePadding, label, id, close_button_id);
	if (just_closed && p_open != NULL)
	{
		*p_open = false;
		TabBarCloseTab(tab_bar, tab);
	}

	// Restore main window position so user can draw there
	if (want_clip_rect)
		PopClipRect();
	window->DC.CursorPos = backup_main_cursor_pos;

	// Tooltip (FIXME: Won't work over the close button because ItemOverlap systems messes up with HoveredIdTimer)
	if (g.HoveredId == id && !held && g.HoveredIdNotActiveTimer > 0.50f)
		if (!(tab_bar->Flags & ImGuiTabBarFlags_NoTooltip))
			SetTooltip("%.*s", (int)(FindRenderedTextEnd(label) - label), label);

	return tab_contents_visible;
}

// [Public] This is call is 100% optional but it allows to remove some one-frame glitches when a tab has been unexpectedly removed.
// To use it to need to call the function SetTabItemClosed() after BeginTabBar() and before any call to BeginTabItem()
void    ui::SetTabItemClosed(const char* label)
{
	ImGuiContext& g = *GImGui;
	bool is_within_manual_tab_bar = (g.CurrentTabBar.Size > 0) && !(g.CurrentTabBar.back()->Flags & ImGuiTabBarFlags_DockNode);
	if (is_within_manual_tab_bar)
	{
		ImGuiTabBar* tab_bar = g.CurrentTabBar.back();
		IM_ASSERT(tab_bar->WantLayout);         // Needs to be called AFTER BeginTabBar() and BEFORE the first call to BeginTabItem()
		ImGuiID tab_id = TabBarCalcTabID(tab_bar, label);
		TabBarRemoveTab(tab_bar, tab_id);
	}
}

ImVec2 ui::TabItemCalcSize(const char* label, bool has_close_button)
{
	ImGuiContext& g = *GImGui;
	ImVec2 label_size = CalcTextSize(label, NULL, true);
	ImVec2 size = ImVec2(label_size.x + g.Style.FramePadding.x, label_size.y + g.Style.FramePadding.y * 2.0f);
	if (has_close_button)
		size.x += g.Style.FramePadding.x + (g.Style.ItemInnerSpacing.x + g.FontSize); // We use Y intentionally to fit the close button circle.
	else
		size.x += g.Style.FramePadding.x + 1.0f;
	return ImVec2(ImMin(size.x, TabBarCalcMaxTabWidth()), size.y);
}

void ui::TabItemBackground(ImDrawList* draw_list, const ImRect& bb, ImGuiTabItemFlags flags, ImU32 col)
{
	// While rendering tabs, we trim 1 pixel off the top of our bounding box so they can fit within a regular frame height while looking "detached" from it.
	ImGuiContext& g = *GImGui;
	const float width = bb.GetWidth();
	IM_UNUSED(flags);
	IM_ASSERT(width > 0.0f);
	const float rounding = ImMax(0.0f, ImMin(g.Style.TabRounding, width * 0.5f - 1.0f));
	const float y1 = bb.Min.y + 1.0f;
	const float y2 = bb.Max.y - 1.0f;
	draw_list->PathLineTo(ImVec2(bb.Min.x, y2));
	draw_list->PathArcToFast(ImVec2(bb.Min.x + rounding, y1 + rounding), rounding, 6, 9);
	draw_list->PathArcToFast(ImVec2(bb.Max.x - rounding, y1 + rounding), rounding, 9, 12);
	draw_list->PathLineTo(ImVec2(bb.Max.x, y2));
	draw_list->PathFillConvex(col);
	if (g.Style.TabBorderSize > 0.0f)
	{
		draw_list->PathLineTo(ImVec2(bb.Min.x + 0.5f, y2));
		draw_list->PathArcToFast(ImVec2(bb.Min.x + rounding + 0.5f, y1 + rounding + 0.5f), rounding, 6, 9);
		draw_list->PathArcToFast(ImVec2(bb.Max.x - rounding - 0.5f, y1 + rounding + 0.5f), rounding, 9, 12);
		draw_list->PathLineTo(ImVec2(bb.Max.x - 0.5f, y2));
		draw_list->PathStroke(GetColorU32(ImGuiCol_Border), false, g.Style.TabBorderSize);
	}
}

// Render text label (with custom clipping) + Unsaved Document marker + Close Button logic
// We tend to lock style.FramePadding for a given tab-bar, hence the 'frame_padding' parameter.
bool ui::TabItemLabelAndCloseButton(ImDrawList* draw_list, const ImRect& bb, ImGuiTabItemFlags flags, ImVec2 frame_padding, const char* label, ImGuiID tab_id, ImGuiID close_button_id)
{
	ImGuiContext& g = *GImGui;
	ImVec2 label_size = CalcTextSize(label, NULL, true);
	if (bb.GetWidth() <= 1.0f)
		return false;

	// Render text label (with clipping + alpha gradient) + unsaved marker
	const char* TAB_UNSAVED_MARKER = "*";
	ImRect text_pixel_clip_bb(bb.Min.x + frame_padding.x, bb.Min.y + frame_padding.y, bb.Max.x - frame_padding.x, bb.Max.y);
	if (flags & ImGuiTabItemFlags_UnsavedDocument)
	{
		text_pixel_clip_bb.Max.x -= CalcTextSize(TAB_UNSAVED_MARKER, NULL, false).x;
		ImVec2 unsaved_marker_pos(ImMin(bb.Min.x + frame_padding.x + label_size.x + 2, text_pixel_clip_bb.Max.x), bb.Min.y + frame_padding.y + (float)(int)(-g.FontSize * 0.25f));
		RenderTextClippedEx(draw_list, unsaved_marker_pos, bb.Max - frame_padding, TAB_UNSAVED_MARKER, NULL, NULL);
	}
	ImRect text_ellipsis_clip_bb = text_pixel_clip_bb;

	// Close Button
	// We are relying on a subtle and confusing distinction between 'hovered' and 'g.HoveredId' which happens because we are using ImGuiButtonFlags_AllowOverlapMode + SetItemAllowOverlap()
	//  'hovered' will be true when hovering the Tab but NOT when hovering the close button
	//  'g.HoveredId==id' will be true when hovering the Tab including when hovering the close button
	//  'g.ActiveId==close_button_id' will be true when we are holding on the close button, in which case both hovered booleans are false
	bool close_button_pressed = false;
	bool close_button_visible = false;
	if (close_button_id != 0)
		if (g.HoveredId == tab_id || g.HoveredId == close_button_id || g.ActiveId == close_button_id)
			close_button_visible = true;
	if (close_button_visible)
	{
		ImGuiItemHoveredDataBackup last_item_backup;
		const float close_button_sz = g.FontSize * 0.5f;
		if (CloseButton(close_button_id, ImVec2(bb.Max.x - frame_padding.x - close_button_sz, bb.Min.y + frame_padding.y + close_button_sz), close_button_sz))
			close_button_pressed = true;
		last_item_backup.Restore();

		// Close with middle mouse button
		if (!(flags & ImGuiTabItemFlags_NoCloseWithMiddleMouseButton) && IsMouseClicked(2))
			close_button_pressed = true;

		text_pixel_clip_bb.Max.x -= close_button_sz * 2.0f;
	}

	// Label with ellipsis
	// FIXME: This should be extracted into a helper but the use of text_pixel_clip_bb and !close_button_visible makes it tricky to abstract at the moment
	const char* label_display_end = FindRenderedTextEnd(label);
	if (label_size.x > text_ellipsis_clip_bb.GetWidth())
	{
		const int ellipsis_dot_count = 3;
		const float ellipsis_width = (1.0f + 1.0f) * ellipsis_dot_count - 1.0f;
		const char* label_end = NULL;
		float label_size_clipped_x = g.Font->CalcTextSizeA(g.FontSize, text_ellipsis_clip_bb.GetWidth() - ellipsis_width + 1.0f, 0.0f, label, label_display_end, &label_end).x;
		if (label_end == label && label_end < label_display_end)    // Always display at least 1 character if there's no room for character + ellipsis
		{
			label_end = label + ImTextCountUtf8BytesFromChar(label, label_display_end);
			label_size_clipped_x = g.Font->CalcTextSizeA(g.FontSize, FLT_MAX, 0.0f, label, label_end).x;
		}
		while (label_end > label && ImCharIsBlankA(label_end[-1])) // Trim trailing space
		{
			label_end--;
			label_size_clipped_x -= g.Font->CalcTextSizeA(g.FontSize, FLT_MAX, 0.0f, label_end, label_end + 1).x; // Ascii blanks are always 1 byte
		}
		RenderTextClippedEx(draw_list, text_pixel_clip_bb.Min, text_pixel_clip_bb.Max, label, label_end, &label_size, ImVec2(0.0f, 0.0f));

		const float ellipsis_x = text_pixel_clip_bb.Min.x + label_size_clipped_x + 1.0f;
		if (!close_button_visible && ellipsis_x + ellipsis_width <= bb.Max.x)
			RenderPixelEllipsis(draw_list, ImVec2(ellipsis_x, text_pixel_clip_bb.Min.y), ellipsis_dot_count, GetColorU32(ImGuiCol_Text));
	}
	else
	{
		RenderTextClippedEx(draw_list, text_pixel_clip_bb.Min, text_pixel_clip_bb.Max, label, label_display_end, &label_size, ImVec2(0.0f, 0.0f));
	}

	return close_button_pressed;
}

struct animation {
	bool clicked;
	bool reverse;
	float size;
	float mult;
};
//skeet
bool ui::CheckboxSkeet(const char* label, bool* v)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	const ImVec2 pos = window->DC.CursorPos;
	const ImRect total_bb(pos, pos + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, label_size.y));
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id))
		return false;

	bool hovered, held;
	bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
	if (pressed)
	{
		*v = !(*v);
		MarkItemEdited(id);
	}
	
	if (*v)
		window->DrawList->AddRectFilled(pos + ImVec2(2, 2), pos + ImVec2(10, 10), ImColor(g_cfg.misc.ui_color.r() / 255.f, g_cfg.misc.ui_color.g() / 255.f, g_cfg.misc.ui_color.b() / 255.f, 1.f));
	else {
		if (hovered)
			window->DrawList->AddRectFilled(pos + ImVec2(2, 2), pos + ImVec2(10, 10), ImColor(75 / 255.f, 75 / 255.f, 75 / 255.f, g.Style.Alpha));
		else
			window->DrawList->AddRectFilled(pos + ImVec2(2, 2), pos + ImVec2(10, 10), ImColor(65 / 255.f, 65 / 255.f, 65 / 255.f, g.Style.Alpha));
	}

	window->DrawList->AddRectFilledMultiColor(pos + ImVec2(2, 2), pos + ImVec2(10, 10), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, 0 / 255.f), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, 0 / 255.f), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, ImClamp(120.f / 255.f, 0.f, g.Style.Alpha)), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, ImClamp(120.f / 255.f, 0.f, g.Style.Alpha)));

	window->DrawList->AddRect(pos + ImVec2(2, 2), pos + ImVec2(10, 10), ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));

	window->DrawList->AddText(pos + ImVec2(15, -1), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, g.Style.Alpha), label);
	window->DrawList->AddText(pos + ImVec2(14, -2), ImColor(205 / 255.f, 205 / 255.f, 205 / 255.f, g.Style.Alpha), label);

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));

	return pressed;
	/*
	static std::map<ImGuiID, animation> circle_anim;
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	const ImVec2 pos = window->DC.CursorPos;
	const ImRect total_bb(pos, pos + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, label_size.y));
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id))
		return false;

	bool hovered, held;
	bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
	if (pressed)
	{
		*v = !(*v);
		MarkItemEdited(id);
	}
	//uncomm
	const float square_sz = GetFrameHeight();
	float radius = square_sz * 0.82f;
	auto it_circle = circle_anim.find(id);
	if (it_circle == circle_anim.end())
	{
		circle_anim.insert({ id, {false, false, 0.f, 0.f} });
		it_circle = circle_anim.find(id);
	}

	if (pressed && *v)
		it_circle->second.clicked = true;
	else if (pressed && !(*v) && it_circle->second.clicked)
	{
		it_circle->second.mult = 0.f;
		it_circle->second.size = 0.f;

		it_circle->second.reverse = false;
		it_circle->second.clicked = false;
	}

	if (it_circle->second.clicked)
	{
		if (!it_circle->second.reverse)
		{
			it_circle->second.size = ImClamp(it_circle->second.size + 3.f * GetIO().DeltaTime, 0.f, 1.f);
			it_circle->second.mult = ImClamp(it_circle->second.mult + 6.f * GetIO().DeltaTime, 0.f, 1.f);

			if (it_circle->second.mult >= 0.99f)
				it_circle->second.reverse = true;
		}
		else
		{
			it_circle->second.size = ImClamp(it_circle->second.size + 3.f * GetIO().DeltaTime, 0.f, 1.f);
			it_circle->second.mult = ImClamp(it_circle->second.mult - 6.f * GetIO().DeltaTime, 0.f, 1.f);

			if (it_circle->second.mult <= 0.01f)
			{
				it_circle->second.mult = 0.f;
				it_circle->second.size = 0.f;

				it_circle->second.reverse = false;
				it_circle->second.clicked = false;
			}

		}
	}
	float deltatime = 1.5f * ui::GetIO().DeltaTime;
	const ImVec4 hover_act = ImVec4(150 / 255.f, 150 / 255.f, 150 / 255.f, 0.f);
	const ImVec4 hover_dis = ImVec4(49 / 255.f, 156 / 255.f, 255 / 255.f, g.Style.Alpha);
	static std::map<ImGuiID, ImVec4> hover_color;
	auto it_hcolor = hover_color.find(id);
	if (it_hcolor == hover_color.end())
	{
		hover_color.insert({ id, hover_act });
		it_hcolor = hover_color.find(id);
	}
	if (hovered || g.ActiveId == id)
	{
		ImVec4 to = hover_dis;
		if (it_hcolor->second.x != to.x)
		{
			if (it_hcolor->second.x < to.x)
				it_hcolor->second.x = ImMin(it_hcolor->second.x + deltatime, to.x);
			else if (it_hcolor->second.x > to.x)
				it_hcolor->second.x = ImMax(to.x, it_hcolor->second.x - deltatime);
		}

		if (it_hcolor->second.y != to.y)
		{
			if (it_hcolor->second.y < to.y)
				it_hcolor->second.y = ImMin(it_hcolor->second.y + deltatime, to.y);
			else if (it_hcolor->second.y > to.y)
				it_hcolor->second.y = ImMax(to.y, it_hcolor->second.y - deltatime);
		}

		if (it_hcolor->second.z != to.z)
		{
			if (it_hcolor->second.z < to.z)
				it_hcolor->second.z = ImMin(it_hcolor->second.z + deltatime, to.z);
			else if (it_hcolor->second.z > to.z)
				it_hcolor->second.z = ImMax(to.z, it_hcolor->second.z - deltatime);
		}

		if (it_hcolor->second.w != to.w)
		{
			if (it_hcolor->second.w < to.w)
				it_hcolor->second.w = ImMin(it_hcolor->second.w + deltatime, to.w);
			else if (it_hcolor->second.w > to.w)
				it_hcolor->second.w = ImMax(to.w, it_hcolor->second.w - deltatime);
		}
	}
	else
	{
		ImVec4 to = hover_act;
		if (it_hcolor->second.x != to.x)
		{
			if (it_hcolor->second.x < to.x)
				it_hcolor->second.x = ImMin(it_hcolor->second.x + deltatime, to.x);
			else if (it_hcolor->second.x > to.x)
				it_hcolor->second.x = ImMax(to.x, it_hcolor->second.x - deltatime);
		}

		if (it_hcolor->second.y != to.y)
		{
			if (it_hcolor->second.y < to.y)
				it_hcolor->second.y = ImMin(it_hcolor->second.y + deltatime, to.y);
			else if (it_hcolor->second.y > to.y)
				it_hcolor->second.y = ImMax(to.y, it_hcolor->second.y - deltatime);
		}

		if (it_hcolor->second.z != to.z)
		{
			if (it_hcolor->second.z < to.z)
				it_hcolor->second.z = ImMin(it_hcolor->second.z + deltatime, to.z);
			else if (it_hcolor->second.z > to.z)
				it_hcolor->second.z = ImMax(to.z, it_hcolor->second.z - deltatime);
		}

		if (it_hcolor->second.w != to.w)
		{
			if (it_hcolor->second.w < to.w)
				it_hcolor->second.w = ImMin(it_hcolor->second.w + deltatime, to.w);
			else if (it_hcolor->second.w > to.w)
				it_hcolor->second.w = ImMax(to.w, it_hcolor->second.w - deltatime);
		}
	}

	if (*v)
	{
		//RenderFrame(total_bb.Min, total_bb.Max, skeet_menu.get_accent_color()), true, style.FrameRounding);
	}

	//ImU32 circle_col = GetColorU32(ImGuiCol_CheckMark, it_circle->second.mult * 0.7f);
	ImU32 circle_col = GetColorU32withApha(skeet_menu.get_accent_color().Value, it_circle->second.mult * 0.7f);


	if (it_circle->second.mult > 0.01f)
		window->DrawList->AddCircleFilled(pos + ImVec2(6, 6), radius * it_circle->second.size, circle_col, 30);



	if (*v)
		window->DrawList->AddRectFilled(pos + ImVec2(2, 2), pos + ImVec2(10, 10), skeet_menu.get_accent_color());
	else {
		if (hovered)
		{
			//cb_bb.Min + ImVec2(1, 1), cb_bb.Max - ImVec2(1, 1)
			window->DrawList->AddRectFilled(pos + ImVec2(2, 2), pos + ImVec2(10, 10), ImColor(75 / 255.f, 75 / 255.f, 75 / 255.f, g.Style.Alpha));
			
		}
			
		else
			window->DrawList->AddRectFilled(pos + ImVec2(2, 2), pos + ImVec2(10, 10), ImColor(65 / 255.f, 65 / 255.f, 65 / 255.f, g.Style.Alpha));
	}
	window->DrawList->AddRect(pos + ImVec2(2, 2) + ImVec2(2, 2), pos + ImVec2(10, 10) - ImVec2(2, 2), ImColor(255 / 255.f, 255 / 255.f, 255 / 255.f, it_hcolor->second.w));
	window->DrawList->AddRectFilledMultiColor(pos + ImVec2(2, 2), pos + ImVec2(10, 10), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, 0 / 255.f), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, 0 / 255.f), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, ImClamp(120.f / 255.f, 0.f, g.Style.Alpha)), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, ImClamp(120.f / 255.f, 0.f, g.Style.Alpha)));

	window->DrawList->AddRect(pos + ImVec2(2, 2), pos + ImVec2(10, 10), ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));
	window->DrawList->AddText(pos + ImVec2(14, -1), ImColor(205 / 255.f, 205 / 255.f, 205 / 255.f, g.Style.Alpha), label);

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));

	return pressed;*/
}


bool ui::SingleSelectSkeet(const char* label, int* current_item, std::vector<const char*> items) {
	*current_item = ImClamp(*current_item, 0, int(items.size() - 1));

	int old_item = *current_item;

	if (ui::BeginComboSkeet(label, items.at(*current_item), 0, items.size())) {
		for (int i = 0; i < items.size(); i++)
			if (ui::SelectableSkeet(items.at(i), *current_item == i))
				*current_item = i;

		ui::EndComboSkeet();
	}

	return old_item != *current_item;
}


bool ui::BeginComboSkeet(const char* label, const char* preview_value, ImGuiComboFlags flags, int items)
{
	/*
	// Always consume the SetNextWindowSizeConstraint() call in our early return paths
	ImGuiContext& g = *GImGui;
	ImGuiCond backup_next_window_size_constraint = g.NextWindowData.SizeConstraintCond;
	g.NextWindowData.SizeConstraintCond = 0;

	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) != (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)); // Can't use both flags together

	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);

	const float arrow_size = (flags & ImGuiComboFlags_NoArrowButton) ? 0.0f : GetFrameHeight();
	const ImVec2 label_size = CalcTextSize(label, NULL, true);
	//window->Size.x
	//const float w = ImClamp(window->Size.x , 155.f, 200.f + 16.f);
	const float w = window->Size.x - style.FramePadding.x - 20;
	const ImRect frame_bb(window->DC.CursorPos + ImVec2(16, 0), window->DC.CursorPos + ImVec2(w, label_size.x > 0 ? label_size.y + 25 : 25));
	const ImRect total_bb(frame_bb.Min, frame_bb.Max);
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id, &frame_bb))
		return false;

	bool hovered, held;
	bool pressed = ButtonBehavior(frame_bb, id, &hovered, &held);
	bool popup_open = IsPopupOpen(id);

	const ImRect value_bb(frame_bb.Min, frame_bb.Max - ImVec2(arrow_size, 0.0f));
	ImRect cb_bb = ImRect(frame_bb.Min + ImVec2(0, label_size.x > 0 ? label_size.y + 5 : 5), frame_bb.Max);



	//ImRect cb_bb = ImRect(frame_bb.Min + ImVec2(120, 0), frame_bb.Max);
	const ImVec4 hover_act = ImVec4(150 / 255.f, 150 / 255.f, 150 / 255.f, 0.f);
	const ImVec4 hover_dis = ImVec4(49 / 255.f, 156 / 255.f, 255 / 255.f, g.Style.Alpha);
	float deltatime = 1.5f * ui::GetIO().DeltaTime;

	static std::map<ImGuiID, ImVec4> hover_color;
	auto it_hcolor = hover_color.find(id);
	if (it_hcolor == hover_color.end())
	{
		hover_color.insert({ id, hover_act });
		it_hcolor = hover_color.find(id);
	}
	if (hovered || g.ActiveId == id || popup_open)
	{
		ImVec4 to = hover_dis;
		if (it_hcolor->second.x != to.x)
		{
			if (it_hcolor->second.x < to.x)
				it_hcolor->second.x = ImMin(it_hcolor->second.x + deltatime, to.x);
			else if (it_hcolor->second.x > to.x)
				it_hcolor->second.x = ImMax(to.x, it_hcolor->second.x - deltatime);
		}

		if (it_hcolor->second.y != to.y)
		{
			if (it_hcolor->second.y < to.y)
				it_hcolor->second.y = ImMin(it_hcolor->second.y + deltatime, to.y);
			else if (it_hcolor->second.y > to.y)
				it_hcolor->second.y = ImMax(to.y, it_hcolor->second.y - deltatime);
		}

		if (it_hcolor->second.z != to.z)
		{
			if (it_hcolor->second.z < to.z)
				it_hcolor->second.z = ImMin(it_hcolor->second.z + deltatime, to.z);
			else if (it_hcolor->second.z > to.z)
				it_hcolor->second.z = ImMax(to.z, it_hcolor->second.z - deltatime);
		}

		if (it_hcolor->second.w != to.w)
		{
			if (it_hcolor->second.w < to.w)
				it_hcolor->second.w = ImMin(it_hcolor->second.w + deltatime, to.w);
			else if (it_hcolor->second.w > to.w)
				it_hcolor->second.w = ImMax(to.w, it_hcolor->second.w - deltatime);
		}
	}
	else
	{
		ImVec4 to = hover_act;
		if (it_hcolor->second.x != to.x)
		{
			if (it_hcolor->second.x < to.x)
				it_hcolor->second.x = ImMin(it_hcolor->second.x + deltatime, to.x);
			else if (it_hcolor->second.x > to.x)
				it_hcolor->second.x = ImMax(to.x, it_hcolor->second.x - deltatime);
		}

		if (it_hcolor->second.y != to.y)
		{
			if (it_hcolor->second.y < to.y)
				it_hcolor->second.y = ImMin(it_hcolor->second.y + deltatime, to.y);
			else if (it_hcolor->second.y > to.y)
				it_hcolor->second.y = ImMax(to.y, it_hcolor->second.y - deltatime);
		}

		if (it_hcolor->second.z != to.z)
		{
			if (it_hcolor->second.z < to.z)
				it_hcolor->second.z = ImMin(it_hcolor->second.z + deltatime, to.z);
			else if (it_hcolor->second.z > to.z)
				it_hcolor->second.z = ImMax(to.z, it_hcolor->second.z - deltatime);
		}

		if (it_hcolor->second.w != to.w)
		{
			if (it_hcolor->second.w < to.w)
				it_hcolor->second.w = ImMin(it_hcolor->second.w + deltatime, to.w);
			else if (it_hcolor->second.w > to.w)
				it_hcolor->second.w = ImMax(to.w, it_hcolor->second.w - deltatime);
		}
	}







	if (!hovered && !popup_open)
		window->DrawList->AddRectFilledMultiColor(cb_bb.Min, cb_bb.Max,
			ImColor(26 / 255.f, 26 / 255.f, 26 / 255.f, g.Style.Alpha),
			ImColor(26 / 255.f, 26 / 255.f, 26 / 255.f, g.Style.Alpha),
			ImColor(36 / 255.f, 36 / 255.f, 36 / 255.f, g.Style.Alpha),
			ImColor(36 / 255.f, 36 / 255.f, 36 / 255.f, g.Style.Alpha));
	else
		window->DrawList->AddRectFilledMultiColor(cb_bb.Min, cb_bb.Max,
			ImColor(36 / 255.f, 36 / 255.f, 36 / 255.f,  g.Style.Alpha),
			ImColor(36 / 255.f, 36 / 255.f, 36 / 255.f,  g.Style.Alpha),
			ImColor(46 / 255.f, 46 / 255.f, 46 / 255.f, g.Style.Alpha),
			ImColor(46 / 255.f, 46 / 255.f, 46 / 255.f, g.Style.Alpha));

	window->DrawList->AddRect(cb_bb.Min, cb_bb.Max, ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));
	window->DrawList->AddRect(cb_bb.Min + ImVec2(1, 1), cb_bb.Max - ImVec2(1, 1), ImColor(255 / 255.f, 255 / 255.f, 255 / 255.f, it_hcolor->second.w));

	if (label_size.x > 0)
		window->DrawList->AddText(frame_bb.Min + ImVec2(0, 2), ImColor(205 / 255.f, 205 / 255.f, 205 / 255.f, g.Style.Alpha), label);

	window->DrawList->PushClipRect(cb_bb.Min, cb_bb.Max - ImVec2(20, 0), true);
	window->DrawList->AddText(cb_bb.Min + ImVec2(8, 4), ImColor(205 / 255.f, 205 / 255.f, 205 / 255.f, g.Style.Alpha), preview_value);
	window->DrawList->PopClipRect();

	if (!popup_open)
		RenderArrow(cb_bb.Max - ImVec2(16, 12), ImGuiDir_Down, 0.5f);
	else
		RenderArrow(cb_bb.Max - ImVec2(16, 12), ImGuiDir_Up, 0.5f);

	if ((pressed || g.NavActivateId == id) && !popup_open)
	{
		if (window->DC.NavLayerCurrent == 0)
			window->NavLastIds[0] = id;
		OpenPopupEx(id);
		popup_open = true;
	}

	if (!popup_open)
		return false;

	SetNextWindowSize(ImVec2(w - 16, CalcMaxPopupHeightFromItemCount(items)));

	char name[16];
	ImFormatString(name, IM_ARRAYSIZE(name), "##Combo_%02d", g.BeginPopupStack.Size); // Recycle windows based on depth

	// Peak into expected window size so we can position it
	if (ImGuiWindow* popup_window = FindWindowByName(name))
		if (popup_window->WasActive)
		{
			ImVec2 size_expected = CalcWindowExpectedSize(popup_window);
			if (flags & ImGuiComboFlags_PopupAlignLeft)
				popup_window->AutoPosLastDirection = ImGuiDir_Left;
			ImRect r_outer = GetWindowAllowedExtentRect(popup_window);
			ImVec2 pos = FindBestWindowPosForPopupEx(frame_bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, frame_bb, ImGuiPopupPositionPolicy_ComboBox);
			SetNextWindowPos(pos);
		}

	// Horizontally align ourselves with the framed text
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
	PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(style.FramePadding.x, style.WindowPadding.y));
	bool ret = BeginSkeet(name, NULL, window_flags);
	PopStyleVar();
	if (!ret)
	{
		EndPopupSkeet();
		IM_ASSERT(0);   // This should never happen as we tested for IsPopupOpen() above
		return false;
	}
	return true;*/
	// Always consume the SetNextWindowSizeConstraint() call in our early return paths
ImGuiContext& g = *GImGui;
ImGuiCond backup_next_window_size_constraint = g.NextWindowData.SizeConstraintCond;
g.NextWindowData.SizeConstraintCond = 0;

ImGuiWindow* window = GetCurrentWindow();
if (window->SkipItems)
return false;

IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) != (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)); // Can't use both flags together

const ImGuiStyle& style = g.Style;
const ImGuiID id = window->GetID(label);

const float arrow_size = (flags & ImGuiComboFlags_NoArrowButton) ? 0.0f : GetFrameHeight();
const ImVec2 label_size = CalcTextSize(label, NULL, true);
const float w = ImClamp(window->Size.x - 48, 155.f, 200.f + 16.f);
const ImRect frame_bb(window->DC.CursorPos + ImVec2(16, 0), window->DC.CursorPos + ImVec2(w, label_size.x > 0 ? label_size.y + 25 : 25));
const ImRect total_bb(frame_bb.Min, frame_bb.Max);
ItemSize(total_bb, style.FramePadding.y);
if (!ItemAdd(total_bb, id, &frame_bb))
return false;

bool hovered, held;
bool pressed = ButtonBehavior(frame_bb, id, &hovered, &held);
bool popup_open = IsPopupOpen(id);

const ImRect value_bb(frame_bb.Min, frame_bb.Max - ImVec2(arrow_size, 0.0f));
ImRect cb_bb = ImRect(frame_bb.Min + ImVec2(0, label_size.x > 0 ? label_size.y + 5 : 5), frame_bb.Max);

if (!hovered && !popup_open)
window->DrawList->AddRectFilledMultiColor(cb_bb.Min, cb_bb.Max,
	ImColor(31 / 255.f, 31 / 255.f, 31 / 255.f, g.Style.Alpha),
	ImColor(31 / 255.f, 31 / 255.f, 31 / 255.f, g.Style.Alpha),
	ImColor(36 / 255.f, 36 / 255.f, 36 / 255.f, g.Style.Alpha),
	ImColor(36 / 255.f, 36 / 255.f, 36 / 255.f, g.Style.Alpha));
else
window->DrawList->AddRectFilledMultiColor(cb_bb.Min, cb_bb.Max,
	ImColor(36 / 255.f, 36 / 255.f, 36 / 255.f, g.Style.Alpha),
	ImColor(36 / 255.f, 36 / 255.f, 36 / 255.f, g.Style.Alpha),
	ImColor(46 / 255.f, 46 / 255.f, 46 / 255.f, g.Style.Alpha),
	ImColor(46 / 255.f, 46 / 255.f, 46 / 255.f, g.Style.Alpha));

window->DrawList->AddRect(cb_bb.Min, cb_bb.Max, ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));

if (label_size.x > 0) {
	window->DrawList->AddText(frame_bb.Min + ImVec2(0, 2), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, g.Style.Alpha), label);
	window->DrawList->AddText(frame_bb.Min + ImVec2(-1, 1), ImColor(205 / 255.f, 205 / 255.f, 205 / 255.f, g.Style.Alpha), label);
}

window->DrawList->PushClipRect(cb_bb.Min, cb_bb.Max - ImVec2(20, 0), true);
window->DrawList->AddText(cb_bb.Min + ImVec2(8, 4), ImColor(155 / 255.f, 155 / 255.f, 155 / 255.f, g.Style.Alpha), preview_value);
window->DrawList->PopClipRect();

if (!popup_open)
RenderArrow(cb_bb.Max - ImVec2(16, 12), ImGuiDir_Down, 0.5f);
else
RenderArrow(cb_bb.Max - ImVec2(16, 12), ImGuiDir_Up, 0.5f);

if ((pressed || g.NavActivateId == id) && !popup_open)
{
	if (window->DC.NavLayerCurrent == 0)
		window->NavLastIds[0] = id;
	OpenPopupEx(id);
	popup_open = true;
}

if (!popup_open)
return false;

SetNextWindowSize(ImVec2(w - 16, CalcMaxPopupHeightFromItemCount(items) + 2 * items - 6));

char name[16];
ImFormatString(name, IM_ARRAYSIZE(name), "##Combo_%02d", g.BeginPopupStack.Size); // Recycle windows based on depth

// Peak into expected window size so we can position it
if (ImGuiWindow* popup_window = FindWindowByName(name))
if (popup_window->WasActive)
{
	ImVec2 size_expected = CalcWindowExpectedSize(popup_window);
	if (flags & ImGuiComboFlags_PopupAlignLeft)
		popup_window->AutoPosLastDirection = ImGuiDir_Left;
	ImRect r_outer = GetWindowAllowedExtentRect(popup_window);
	ImVec2 pos = FindBestWindowPosForPopupEx(frame_bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, frame_bb, ImGuiPopupPositionPolicy_ComboBox);
	SetNextWindowPos(pos);
}

// Horizontally align ourselves with the framed text
ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(style.FramePadding.x, style.WindowPadding.y));
bool ret = BeginSkeet(name, NULL, window_flags);
PopStyleVar();
if (!ret)
{
	EndPopup();
	IM_ASSERT(0);   // This should never happen as we tested for IsPopupOpen() above
	return false;
}
return true;
}


bool ui::SelectableOT(const char* label, bool* p_selected, ImGuiSelectableFlags flags, const ImVec2& size_arg)
{
	if (SelectableOT(label, *p_selected, flags, size_arg))
	{
		*p_selected = !*p_selected;
		return true;
	}
	return false;
}

bool ui::SelectableSkeet(const char* label, bool* p_selected, ImGuiSelectableFlags flags, const ImVec2& size_arg)
{
	if (SelectableSkeet(label, *p_selected, flags, size_arg))
	{
		*p_selected = !*p_selected;
		return true;
	}
	return false;
}
bool ui::SelectableSkeet(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size_arg)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.ColumnsSet) // FIXME-OPT: Avoid if vertically clipped.
		PopClipRect();

	ImGuiID id = window->GetID(label);
	ImVec2 label_size = CalcTextSize(label, NULL, true);
	ImVec2 size(size_arg.x != 0.0f ? size_arg.x : label_size.x, size_arg.y != 0.0f ? size_arg.y : label_size.y + 2);
	ImVec2 pos = window->DC.CursorPos;
	pos.y += window->DC.CurrentLineTextBaseOffset;
	ImRect bb_inner(pos, pos + size);
	ItemSize(bb_inner);

	// Fill horizontal space.
	ImVec2 window_padding = window->WindowPadding;
	float max_x = (flags & ImGuiSelectableFlags_SpanAllColumns) ? GetWindowContentRegionMax().x : GetContentRegionMax().x;
	float w_draw = ImMax(label_size.x, window->Pos.x + max_x - window_padding.x - pos.x);
	ImVec2 size_draw((size_arg.x != 0 && !(flags & ImGuiSelectableFlags_DrawFillAvailWidth)) ? size_arg.x : w_draw, size_arg.y != 0.0f ? size_arg.y : size.y);
	ImRect bb(pos, pos + size_draw);
	if (size_arg.x == 0.0f || (flags & ImGuiSelectableFlags_DrawFillAvailWidth))
		bb.Max.x += window_padding.x;

	// Selectables are tightly packed together, we extend the box to cover spacing between selectable.
	float spacing_L = (float)(int)(style.ItemSpacing.x * 0.5f);
	float spacing_U = (float)(int)(style.ItemSpacing.y * 0.5f);
	float spacing_R = style.ItemSpacing.x - spacing_L;
	float spacing_D = style.ItemSpacing.y - spacing_U;
	bb.Min.x -= spacing_L;
	bb.Min.y -= spacing_U;
	bb.Max.x += spacing_R;
	bb.Max.y += spacing_D;
	if (!ItemAdd(bb, id))
	{
		if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.ColumnsSet)
			PushColumnClipRect();
		return false;
	}

	// We use NoHoldingActiveID on menus so user can click and _hold_ on a menu then drag to browse child entries
	ImGuiButtonFlags button_flags = 0;
	if (flags & ImGuiSelectableFlags_NoHoldingActiveID) button_flags |= ImGuiButtonFlags_NoHoldingActiveID;
	if (flags & ImGuiSelectableFlags_PressedOnClick) button_flags |= ImGuiButtonFlags_PressedOnClick;
	if (flags & ImGuiSelectableFlags_PressedOnRelease) button_flags |= ImGuiButtonFlags_PressedOnRelease;
	if (flags & ImGuiSelectableFlags_Disabled) button_flags |= ImGuiButtonFlags_Disabled;
	if (flags & ImGuiSelectableFlags_AllowDoubleClick) button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick;
	if (flags & ImGuiSelectableFlags_Disabled)
		selected = false;

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, button_flags);
	// Hovering selectable with mouse updates NavId accordingly so navigation can be resumed with gamepad/keyboard (this doesn't happen on most widgets)
	if (pressed || hovered)
		if (!g.NavDisableMouseHover && g.NavWindow == window && g.NavLayer == window->DC.NavLayerCurrent)
		{
			g.NavDisableHighlight = true;
			SetNavID(id, window->DC.NavLayerCurrent);
		}
	if (pressed)
		MarkItemEdited(id);




	const ImVec4 hover_act = ImVec4(150 / 255.f, 150 / 255.f, 150 / 255.f, 0.f);
	const ImVec4 hover_dis = ImVec4(49 / 255.f, 156 / 255.f, 255 / 255.f, g.Style.Alpha);
	float deltatime = 1.5f * ui::GetIO().DeltaTime;

	static std::map<ImGuiID, ImVec4> hover_color;
	auto it_hcolor = hover_color.find(id);
	if (it_hcolor == hover_color.end())
	{
		hover_color.insert({ id, hover_act });
		it_hcolor = hover_color.find(id);
	}
	if (hovered || g.ActiveId == id)
	{
		ImVec4 to = hover_dis;
		if (it_hcolor->second.x != to.x)
		{
			if (it_hcolor->second.x < to.x)
				it_hcolor->second.x = ImMin(it_hcolor->second.x + deltatime, to.x);
			else if (it_hcolor->second.x > to.x)
				it_hcolor->second.x = ImMax(to.x, it_hcolor->second.x - deltatime);
		}

		if (it_hcolor->second.y != to.y)
		{
			if (it_hcolor->second.y < to.y)
				it_hcolor->second.y = ImMin(it_hcolor->second.y + deltatime, to.y);
			else if (it_hcolor->second.y > to.y)
				it_hcolor->second.y = ImMax(to.y, it_hcolor->second.y - deltatime);
		}

		if (it_hcolor->second.z != to.z)
		{
			if (it_hcolor->second.z < to.z)
				it_hcolor->second.z = ImMin(it_hcolor->second.z + deltatime, to.z);
			else if (it_hcolor->second.z > to.z)
				it_hcolor->second.z = ImMax(to.z, it_hcolor->second.z - deltatime);
		}

		if (it_hcolor->second.w != to.w)
		{
			if (it_hcolor->second.w < to.w)
				it_hcolor->second.w = ImMin(it_hcolor->second.w + deltatime, to.w);
			else if (it_hcolor->second.w > to.w)
				it_hcolor->second.w = ImMax(to.w, it_hcolor->second.w - deltatime);
		}
	}
	else
	{
		ImVec4 to = hover_act;
		if (it_hcolor->second.x != to.x)
		{
			if (it_hcolor->second.x < to.x)
				it_hcolor->second.x = ImMin(it_hcolor->second.x + deltatime, to.x);
			else if (it_hcolor->second.x > to.x)
				it_hcolor->second.x = ImMax(to.x, it_hcolor->second.x - deltatime);
		}

		if (it_hcolor->second.y != to.y)
		{
			if (it_hcolor->second.y < to.y)
				it_hcolor->second.y = ImMin(it_hcolor->second.y + deltatime, to.y);
			else if (it_hcolor->second.y > to.y)
				it_hcolor->second.y = ImMax(to.y, it_hcolor->second.y - deltatime);
		}

		if (it_hcolor->second.z != to.z)
		{
			if (it_hcolor->second.z < to.z)
				it_hcolor->second.z = ImMin(it_hcolor->second.z + deltatime, to.z);
			else if (it_hcolor->second.z > to.z)
				it_hcolor->second.z = ImMax(to.z, it_hcolor->second.z - deltatime);
		}

		if (it_hcolor->second.w != to.w)
		{
			if (it_hcolor->second.w < to.w)
				it_hcolor->second.w = ImMin(it_hcolor->second.w + deltatime, to.w);
			else if (it_hcolor->second.w > to.w)
				it_hcolor->second.w = ImMax(to.w, it_hcolor->second.w - deltatime);
		}
	}




	




	if (hovered) {
		window->DrawList->AddRectFilled(bb.Min, bb.Max, ImColor(26 / 255.f, 26 / 255.f, 26 / 255.f, g.Style.Alpha));
		//PushFont(GetIO().Fonts->Fonts[1]);
	}






	window->DrawList->PushClipRect(bb.Min, bb.Max, true);


	if (selected && !hovered)
	{
		window->DrawList->AddRectFilled(bb.Min, bb.Max, ImColor(26 / 255.f, 26 / 255.f, 26 / 255.f, g.Style.Alpha));
	}
	window->DrawList->AddRectFilled(bb.Min, bb.Max, ImColor(26 / 255.f, 26 / 255.f, 26 / 255.f, it_hcolor->second.w));
	window->DrawList->AddRect(bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), ImColor(255 / 255.f, 255 / 255.f, 255 / 255.f, it_hcolor->second.w));

	window->DrawList->AddText(bb.Min + ImVec2(8, size.y / 2 - label_size.y / 2 + 2), selected ? skeet_menu.get_accent_color() : ImColor(205 / 255.f, 205 / 255.f, 205 / 255.f, g.Style.Alpha), label);
	window->DrawList->PopClipRect();

	//if (hovered)
	//	PopFont();

	if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.ColumnsSet)
	{
		PushColumnClipRect();
		bb.Max.x -= (GetContentRegionMax().x - max_x);
	}

	// Automatically close popups
	if (pressed && (window->Flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiSelectableFlags_DontClosePopups) && !(window->DC.ItemFlags & ImGuiItemFlags_SelectableDontClosePopup))
		CloseCurrentPopup();
	return pressed;
}


bool ui::SliderScalarSkeet(const char* label, ImGuiDataType data_type, void* v, const void* v_min, const void* v_max, const char* format, float power, int remove_from_fmt)
{
	/*
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);
	const float w = window->Size.x - style.FramePadding.x - 60;
	const ImRect frame_bb(window->DC.CursorPos + ImVec2(16, 0), window->DC.CursorPos + ImVec2(w, label_size.x > 0 ? label_size.y + 10 : 10) + ImVec2(16, 0));
	const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id, &frame_bb))
		return false;
	
	// Default format string when passing NULL
	// Patch old "%.0f" format string to use "%d", read function comments for more details.
	IM_ASSERT(data_type >= 0 && data_type < ImGuiDataType_COUNT);
	if (format == NULL)
		format = GDataTypeInfo[data_type].PrintFmt;
	else if (data_type == ImGuiDataType_S32 && strcmp(format, "%d") != 0)
		format = PatchFormatStringFloatToInt(format);

	// Tabbing or CTRL-clicking on Slider turns it into an input box
	bool start_text_input = false;
	const bool tab_focus_requested = FocusableItemRegister(window, id);
	const bool hovered = ItemHoverable(frame_bb, id);
	if (tab_focus_requested || (hovered && g.IO.MouseClicked[0]) || g.NavActivateId == id || (g.NavInputId == id && g.ScalarAsInputTextId != id))
	{
		SetActiveID(id, window);
		SetFocusID(id, window);
		FocusWindow(window);
		g.ActiveIdAllowNavDirFlags = (1 << ImGuiDir_Up) | (1 << ImGuiDir_Down);
	}

	if (std::string(label).at(0) != '#' && std::string(label).at(1) != '#')
		window->DrawList->AddText(frame_bb.Min, ImColor(205 / 255.f, 205 / 255.f, 205 / 255.f, g.Style.Alpha), label);

	ImRect slider_bb = ImRect(frame_bb.Min + ImVec2(0, label_size.x > 0 ? label_size.y + 3 : 3), frame_bb.Max);

	if (IsItemHovered())
		window->DrawList->AddRectFilled(slider_bb.Min, slider_bb.Max,
			ImColor(65 / 255.f, 65 / 255.f, 65 / 255.f, g.Style.Alpha));
	else
		window->DrawList->AddRectFilled(slider_bb.Min, slider_bb.Max,
			ImColor(55 / 255.f, 55 / 255.f, 55 / 255.f, g.Style.Alpha));

	int percent = 0;
	ImRect grab_bb;
	const bool value_changed = SliderBehavior(frame_bb, id, data_type, v, v_min, v_max, format, power, ImGuiSliderFlags_None, &grab_bb, &percent);
	if (value_changed)
		MarkItemEdited(id);



	 //static float size = 5.f;
	static float min_size = 5.f;
	static float max_size = 7.f;

	//static float old_value = 0.f;
	//static float active_value = 0.f;

	static std::map<ImGuiID, float> size;
	static std::map<ImGuiID, float> old_value;
	static std::map<ImGuiID, float> active_value;
	auto it_size = size.find(id);
	auto it_old_value = old_value.find(id);
	auto it_active_value = active_value.find(id);

	if (it_size == size.end())
	{
		size.insert({ id, 5.f });
		it_size = size.find(id);
	}
	if (it_old_value == old_value.end())
	{
		old_value.insert({ id, 0 });
		it_old_value = old_value.find(id);
	}
	if (it_active_value == active_value.end())
	{
		active_value.insert({ id, floorf((float)percent / 100.f * (slider_bb.Max.x - slider_bb.Min.x)) });
		it_active_value = active_value.find(id);
	}

	it_active_value->second = floorf((float)percent / 100.f * (slider_bb.Max.x - slider_bb.Min.x));

	if (it_old_value->second != it_active_value->second)
	{
		if (it_old_value->second < it_active_value->second)
			it_old_value->second += (it_active_value->second - it_old_value->second) / 10 + 0.3f;
		if (it_old_value->second > it_active_value->second)
			it_old_value->second -= (it_old_value->second - it_active_value->second) / 10 + 0.3f;
	}

	if (g.ActiveId == id || IsItemHovered())
	{
		if (it_size->second < max_size)
		{
			it_size->second += 0.1f;
			std::clamp(it_size->second, min_size, max_size);
		}
	}
	else
	{
		if (it_size->second > min_size)
		{
			it_size->second -= 0.1f;
			std::clamp(it_size->second, min_size, max_size);
		}
	}



	float deltatimez = 1.5f * ui::GetIO().DeltaTime;
	const ImVec4 hover_actz = ImVec4(150 / 255.f, 150 / 255.f, 150 / 255.f, 0.f);
	const ImVec4 hover_disz = ImVec4(49 / 255.f, 156 / 255.f, 255 / 255.f, g.Style.Alpha);
	static std::map<ImGuiID, ImVec4> hover_colorz;
	auto it_hcolorz = hover_colorz.find(id);
	if (it_hcolorz == hover_colorz.end())
	{
		hover_colorz.insert({ id, hover_actz });
		it_hcolorz = hover_colorz.find(id);
	}
	if (hovered || g.ActiveId == id)
	{
		ImVec4 to = hover_disz;
		if (it_hcolorz->second.x != to.x)
		{
			if (it_hcolorz->second.x < to.x)
				it_hcolorz->second.x = ImMin(it_hcolorz->second.x + deltatimez, to.x);
			else if (it_hcolorz->second.x > to.x)
				it_hcolorz->second.x = ImMax(to.x, it_hcolorz->second.x - deltatimez);
		}

		if (it_hcolorz->second.y != to.y)
		{
			if (it_hcolorz->second.y < to.y)
				it_hcolorz->second.y = ImMin(it_hcolorz->second.y + deltatimez, to.y);
			else if (it_hcolorz->second.y > to.y)
				it_hcolorz->second.y = ImMax(to.y, it_hcolorz->second.y - deltatimez);
		}

		if (it_hcolorz->second.z != to.z)
		{
			if (it_hcolorz->second.z < to.z)
				it_hcolorz->second.z = ImMin(it_hcolorz->second.z + deltatimez, to.z);
			else if (it_hcolorz->second.z > to.z)
				it_hcolorz->second.z = ImMax(to.z, it_hcolorz->second.z - deltatimez);
		}

		if (it_hcolorz->second.w != to.w)
		{
			if (it_hcolorz->second.w < to.w)
				it_hcolorz->second.w = ImMin(it_hcolorz->second.w + deltatimez, to.w);
			else if (it_hcolorz->second.w > to.w)
				it_hcolorz->second.w = ImMax(to.w, it_hcolorz->second.w - deltatimez);
		}
	}
	else
	{
		ImVec4 to = hover_actz;
		if (it_hcolorz->second.x != to.x)
		{
			if (it_hcolorz->second.x < to.x)
				it_hcolorz->second.x = ImMin(it_hcolorz->second.x + deltatimez, to.x);
			else if (it_hcolorz->second.x > to.x)
				it_hcolorz->second.x = ImMax(to.x, it_hcolorz->second.x - deltatimez);
		}

		if (it_hcolorz->second.y != to.y)
		{
			if (it_hcolorz->second.y < to.y)
				it_hcolorz->second.y = ImMin(it_hcolorz->second.y + deltatimez, to.y);
			else if (it_hcolorz->second.y > to.y)
				it_hcolorz->second.y = ImMax(to.y, it_hcolorz->second.y - deltatimez);
		}

		if (it_hcolorz->second.z != to.z)
		{
			if (it_hcolorz->second.z < to.z)
				it_hcolorz->second.z = ImMin(it_hcolorz->second.z + deltatimez, to.z);
			else if (it_hcolorz->second.z > to.z)
				it_hcolorz->second.z = ImMax(to.z, it_hcolorz->second.z - deltatimez);
		}

		if (it_hcolorz->second.w != to.w)
		{
			if (it_hcolorz->second.w < to.w)
				it_hcolorz->second.w = ImMin(it_hcolorz->second.w + deltatimez, to.w);
			else if (it_hcolorz->second.w > to.w)
				it_hcolorz->second.w = ImMax(to.w, it_hcolorz->second.w - deltatimez);
		}
	}


	window->DrawList->AddRectFilledMultiColor(slider_bb.Min, slider_bb.Max, ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, ImClamp(120.f / 255.f, 0.f, g.Style.Alpha)), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, ImClamp(120.f / 255.f, 0.f, g.Style.Alpha)), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, ImClamp(0 / 255.f, 0.f, g.Style.Alpha)), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, ImClamp(0 / 255.f, 0.f, g.Style.Alpha)));
	window->DrawList->AddRectFilled(slider_bb.Min, ImVec2(slider_bb.Min.x + it_old_value->second, slider_bb.Max.y), skeet_menu.get_accent_color());
	window->DrawList->AddRectFilledMultiColor(slider_bb.Min, ImVec2(slider_bb.Min.x + it_old_value->second, slider_bb.Max.y), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, 0 / 255.f), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, 0 / 255.f), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, ImClamp(120.f / 255.f, 0.f, g.Style.Alpha)), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, ImClamp(120.f / 255.f, 0.f, g.Style.Alpha)));
	window->DrawList->AddRect(slider_bb.Min, slider_bb.Max, ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));
	window->DrawList->AddRect(slider_bb.Min + ImVec2(1, 1), slider_bb.Max - ImVec2(1, 1), ImColor(255 / 255.f, 255 / 255.f, 255 / 255.f, it_hcolorz->second.w));


	if (data_type == ImGuiDataType_S32)
		*(int*)v -= remove_from_fmt;

	char value_buf[64];
	const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, v, format);
	auto textSize = CalcTextSize(value_buf);

	if (data_type == ImGuiDataType_S32)
		*(int*)v += remove_from_fmt;



	const ImVec4 hover_act = ImVec4(150 / 255.f, 150 / 255.f, 150 / 255.f, 0.3f);
	const ImVec4 hover_dis = ImVec4(34 / 255.f, 149 / 255.f, 255 / 255.f, g.Style.Alpha);
	float deltatime = 1.5f * ui::GetIO().DeltaTime;
	static std::map<ImGuiID, ImVec4> hover_color;
	auto it_hcolor = hover_color.find(id);
	if (it_hcolor == hover_color.end())
	{
		hover_color.insert({ id, hover_act });
		it_hcolor = hover_color.find(id);
	}
	if (IsItemHovered() || g.NavActivateId == id)
	{
		ImVec4 to = hover_dis;
		if (it_hcolor->second.x != to.x)
		{
			if (it_hcolor->second.x < to.x)
				it_hcolor->second.x = ImMin(it_hcolor->second.x + deltatime, to.x);
			else if (it_hcolor->second.x > to.x)
				it_hcolor->second.x = ImMax(to.x, it_hcolor->second.x - deltatime);
		}

		if (it_hcolor->second.y != to.y)
		{
			if (it_hcolor->second.y < to.y)
				it_hcolor->second.y = ImMin(it_hcolor->second.y + deltatime, to.y);
			else if (it_hcolor->second.y > to.y)
				it_hcolor->second.y = ImMax(to.y, it_hcolor->second.y - deltatime);
		}

		if (it_hcolor->second.z != to.z)
		{
			if (it_hcolor->second.z < to.z)
				it_hcolor->second.z = ImMin(it_hcolor->second.z + deltatime, to.z);
			else if (it_hcolor->second.z > to.z)
				it_hcolor->second.z = ImMax(to.z, it_hcolor->second.z - deltatime);
		}

		if (it_hcolor->second.w != to.w)
		{
			if (it_hcolor->second.w < to.w)
				it_hcolor->second.w = ImMin(it_hcolor->second.w + deltatime, to.w);
			else if (it_hcolor->second.w > to.w)
				it_hcolor->second.w = ImMax(to.w, it_hcolor->second.w - deltatime);
		}
	}
	else
	{
		ImVec4 to = hover_act;
		if (it_hcolor->second.x != to.x)
		{
			if (it_hcolor->second.x < to.x)
				it_hcolor->second.x = ImMin(it_hcolor->second.x + deltatime, to.x);
			else if (it_hcolor->second.x > to.x)
				it_hcolor->second.x = ImMax(to.x, it_hcolor->second.x - deltatime);
		}

		if (it_hcolor->second.y != to.y)
		{
			if (it_hcolor->second.y < to.y)
				it_hcolor->second.y = ImMin(it_hcolor->second.y + deltatime, to.y);
			else if (it_hcolor->second.y > to.y)
				it_hcolor->second.y = ImMax(to.y, it_hcolor->second.y - deltatime);
		}

		if (it_hcolor->second.z != to.z)
		{
			if (it_hcolor->second.z < to.z)
				it_hcolor->second.z = ImMin(it_hcolor->second.z + deltatime, to.z);
			else if (it_hcolor->second.z > to.z)
				it_hcolor->second.z = ImMax(to.z, it_hcolor->second.z - deltatime);
		}

		if (it_hcolor->second.w != to.w)
		{
			if (it_hcolor->second.w < to.w)
				it_hcolor->second.w = ImMin(it_hcolor->second.w + deltatime, to.w);
			else if (it_hcolor->second.w > to.w)
				it_hcolor->second.w = ImMax(to.w, it_hcolor->second.w - deltatime);
		}
	}

	//if (IsItemHovered())
	{
		ui::PushFont(skeet_menu.f2);
		//window->DrawList->AddText(ImVec2(slider_bb.Max.x + 10, slider_bb.Min.y - CalcTextSize(value_buf).y / 2), ImColor(205 / 255.f, 205 / 255.f, 205 / 255.f, g.Style.Alpha), value_buf);
		window->DrawList->AddText(ImVec2(slider_bb.Max + ImVec2(5, -10)), ImColor(205 / 255.f, 205 / 255.f, 205 / 255.f, it_hcolorz->second.w), value_buf);
		PopFont();
	}


	//// Render grab
	//window->DrawList->AddRectFilled(grab_bb.Min, grab_bb.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);

	//// Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
	

	//if (label_size.x > 0.0f)
	//    RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags);
	return value_changed;*/
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
	return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const float w = ImClamp(window->Size.x - 64, 155.f, 200.f);

	const ImVec2 label_size = CalcTextSize(label, NULL, true);
	const ImRect frame_bb(window->DC.CursorPos + ImVec2(16, 0), window->DC.CursorPos + ImVec2(w, label_size.x > 0 ? label_size.y + 10 : 10) + ImVec2(16, 0));
	const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id, &frame_bb))
	return false;

	// Default format string when passing NULL
	// Patch old "%.0f" format string to use "%d", read function comments for more details.
	IM_ASSERT(data_type >= 0 && data_type < ImGuiDataType_COUNT);
	if (format == NULL)
	format = GDataTypeInfo[data_type].PrintFmt;
	else if (data_type == ImGuiDataType_S32 && strcmp(format, "%d") != 0)
	format = PatchFormatStringFloatToInt(format);

	// Tabbing or CTRL-clicking on Slider turns it into an input box
	bool start_text_input = false;
	const bool tab_focus_requested = FocusableItemRegister(window, id);
	const bool hovered = ItemHoverable(frame_bb, id);
	if (tab_focus_requested || (hovered && g.IO.MouseClicked[0]) || g.NavActivateId == id || (g.NavInputId == id && g.ScalarAsInputTextId != id))
	{
		SetActiveID(id, window);
		SetFocusID(id, window);
		FocusWindow(window);
		g.ActiveIdAllowNavDirFlags = (1 << ImGuiDir_Up) | (1 << ImGuiDir_Down);
	}

	if (std::string(label).at(0) != '#' && std::string(label).at(1) != '#') {
		window->DrawList->AddText(frame_bb.Min, ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, g.Style.Alpha), label);
		window->DrawList->AddText(frame_bb.Min - ImVec2(1, 1), ImColor(205 / 255.f, 205 / 255.f, 205 / 255.f, g.Style.Alpha), label);
	}


	ImRect slider_bb = ImRect(frame_bb.Min + ImVec2(0, label_size.x > 0 ? label_size.y + 3 : 3), frame_bb.Max);

	static bool ispressed = false;

	if (IsItemHovered()) {
		window->DrawList->AddRectFilled(slider_bb.Min, slider_bb.Max,
			ImColor(65 / 255.f, 65 / 255.f, 65 / 255.f, g.Style.Alpha));
		if (GetAsyncKeyState(VK_LEFT) && v != v_min && !ispressed) {
			*(int*)v -= 1;
			ispressed = true;
		}
		else if (GetAsyncKeyState(VK_RIGHT) && v != v_max && !ispressed) {
			*(int*)v += 1;
			ispressed = true;
		}

		if (!GetAsyncKeyState(VK_LEFT) && !GetAsyncKeyState(VK_RIGHT))
			ispressed = false;
	}
	else
	window->DrawList->AddRectFilled(slider_bb.Min, slider_bb.Max,
		ImColor(55 / 255.f, 55 / 255.f, 55 / 255.f, g.Style.Alpha));

	int percent = 0;
	ImRect grab_bb;
	const bool value_changed = SliderBehavior(frame_bb, id, data_type, v, v_min, v_max, format, power, ImGuiSliderFlags_None, &grab_bb, &percent);
	if (value_changed)
	MarkItemEdited(id);

	window->DrawList->AddRectFilledMultiColor(slider_bb.Min, slider_bb.Max, ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, ImClamp(120.f / 255.f, 0.f, g.Style.Alpha)), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, ImClamp(120.f / 255.f, 0.f, g.Style.Alpha)), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, ImClamp(0 / 255.f, 0.f, g.Style.Alpha)), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, ImClamp(0 / 255.f, 0.f, g.Style.Alpha)));
	window->DrawList->AddRectFilled(slider_bb.Min, ImVec2(slider_bb.Min.x + floorf((float)percent / 100.f * (slider_bb.Max.x - slider_bb.Min.x)), slider_bb.Max.y), ImColor (g_cfg.misc.ui_color.r() / 255.f, g_cfg.misc.ui_color.g() / 255.f, g_cfg.misc.ui_color.b() / 255.f, 255.f));

	window->DrawList->AddRectFilledMultiColor(slider_bb.Min, ImVec2(slider_bb.Min.x + floorf((float)percent / 100.f * (slider_bb.Max.x - slider_bb.Min.x)), slider_bb.Max.y), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, 0 / 255.f), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, 0 / 255.f), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, ImClamp(120.f / 255.f, 0.f, g.Style.Alpha)), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, ImClamp(120.f / 255.f, 0.f, g.Style.Alpha)));
	window->DrawList->AddRect(slider_bb.Min, slider_bb.Max, ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));

	if (data_type == ImGuiDataType_S32)
	*(int*)v -= remove_from_fmt;

	char value_buf[64];
	const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, v, format);
	auto textSize = CalcTextSize(value_buf);

	if (data_type == ImGuiDataType_S32)
	*(int*)v += remove_from_fmt;


	PushFont(skeet_menu.verdanabold);
	window->DrawList->AddText(ImVec2(slider_bb.Min.x + floorf((float)percent / 100.f * (slider_bb.Max.x - slider_bb.Min.x)) - textSize.x / 2 - 1, slider_bb.Max.y - textSize.y / 2 - 2), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, g.Style.Alpha / 1.5), value_buf);
	window->DrawList->AddText(ImVec2(slider_bb.Min.x + floorf((float)percent / 100.f * (slider_bb.Max.x - slider_bb.Min.x)) - textSize.x / 2 + 1, slider_bb.Max.y - textSize.y / 2), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, g.Style.Alpha / 1.5), value_buf);
	window->DrawList->AddText(ImVec2(slider_bb.Min.x + floorf((float)percent / 100.f * (slider_bb.Max.x - slider_bb.Min.x)) - textSize.x / 2 + 1, slider_bb.Max.y - textSize.y / 2 - 2), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, g.Style.Alpha / 1.5), value_buf);
	window->DrawList->AddText(ImVec2(slider_bb.Min.x + floorf((float)percent / 100.f * (slider_bb.Max.x - slider_bb.Min.x)) - textSize.x / 2 - 1, slider_bb.Max.y - textSize.y / 2), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, g.Style.Alpha / 1.5), value_buf);
	window->DrawList->AddText(ImVec2(slider_bb.Min.x + floorf((float)percent / 100.f * (slider_bb.Max.x - slider_bb.Min.x)) - textSize.x / 2, slider_bb.Max.y - textSize.y / 2 - 1), ImColor(205 / 255.f, 205 / 255.f, 205 / 255.f, g.Style.Alpha), value_buf);
	PopFont();

//// Render grab
//window->DrawList->AddRectFilled(grab_bb.Min, grab_bb.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);

//// Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
/*
RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f,0.5f));*/

//if (label_size.x > 0.0f)
//    RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags);
return value_changed;
}

//////////////////////////////////////////////////////////////////////
bool ui::SliderFloatSkeet(const char* label, float* v, float v_min, float v_max, const char* format, float power)
{
	return SliderScalarSkeet(label, ImGuiDataType_Float, v, &v_min, &v_max, format, power, 0);
}



bool ui::ButtonExSkeet(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	ImVec2 pos = window->DC.CursorPos + ImVec2(16, 0);
	if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrentLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
		pos.y += window->DC.CurrentLineTextBaseOffset - style.FramePadding.y;
	ImVec2 size = CalcItemSize(size_arg, ImClamp(window->Size.x - 64, 50.f, 200.f), 25);

	const ImRect bb(pos, pos + size);
	ItemSize(size, style.FramePadding.y);
	if (!ItemAdd(bb, id))
		return false;

	if (window->DC.ItemFlags & ImGuiItemFlags_ButtonRepeat)
		flags |= ImGuiButtonFlags_Repeat;
	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);
	if (pressed)
		MarkItemEdited(id);


	if (held)
		window->DrawList->AddRectFilledMultiColor(bb.Min, bb.Max,
			ImColor(26 / 255.f, 26 / 255.f, 26 / 255.f, g.Style.Alpha),
			ImColor(26 / 255.f, 26 / 255.f, 26 / 255.f, g.Style.Alpha),
			ImColor(34 / 255.f, 34 / 255.f, 34 / 255.f, g.Style.Alpha),
			ImColor(34 / 255.f, 34 / 255.f, 34 / 255.f, g.Style.Alpha));
	else
		window->DrawList->AddRectFilledMultiColor(bb.Min, bb.Max,
			ImColor(34 / 255.f, 34 / 255.f, 34 / 255.f, g.Style.Alpha),
			ImColor(34 / 255.f, 34 / 255.f, 34 / 255.f, g.Style.Alpha),
			ImColor(26 / 255.f, 26 / 255.f, 26 / 255.f, g.Style.Alpha),
			ImColor(26 / 255.f, 26 / 255.f, 26 / 255.f, g.Style.Alpha));

	window->DrawList->AddRect(bb.Min, bb.Max, ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));
	window->DrawList->AddRect(bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), ImColor(50 / 255.f, 50 / 255.f, 50 / 255.f, g.Style.Alpha));

	//PushFont(GetIO().Fonts->Fonts[1]);
	window->DrawList->PushClipRect(bb.Min, bb.Max, true);
	window->DrawList->AddText(bb.Min + ImVec2((bb.Max.x - bb.Min.x) / 2 - label_size.x / 2, (bb.Max.y - bb.Min.y) / 2 - label_size.y / 2), ImColor(205 / 255.f, 205 / 255.f, 205 / 255.f, g.Style.Alpha), label);
	window->DrawList->PopClipRect();
	//PopFont();

	// Automatically close popups
	//if (pressed && !(flags & ImGuiButtonFlags_DontClosePopups) && (window->Flags & ImGuiWindowFlags_Popup))
	//    CloseCurrentPopup();

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.LastItemStatusFlags);
	return pressed;
}


bool ui::ButtonSkeet(const char* label, const ImVec2& size_arg)
{
	return ButtonExSkeet(label, size_arg, 0);
}


bool ui::ListBoxHeader(int fix, const char* label, const ImVec2& size_arg)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	const ImGuiStyle& style = GetStyle();
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	// Size default to hold ~7 items. Fractional number of items helps seeing that we can scroll down/up without looking at scrollbar.
	ImVec2 size = CalcItemSize(size_arg, window->Size.x - 64, GetTextLineHeightWithSpacing() * 7.4f + style.ItemSpacing.y);
	ImVec2 frame_size = ImVec2(size.x, ImMax(size.y, label_size.y));
	ImRect frame_bb(window->DC.CursorPos + ImVec2(16, 0), window->DC.CursorPos + frame_size);
	ImRect bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
	window->DC.LastItemRect = bb; // Forward storage for ListBoxFooter.. dodgy.

	if (!IsRectVisible(bb.Min, bb.Max))
	{
		ItemSize(bb.GetSize(), style.FramePadding.y);
		ItemAdd(bb, fix, &frame_bb);
		return false;
	}

	BeginGroup();
	if (label_size.x > 0)
		RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

	BeginChildFrame(fix, frame_bb.GetSize());
	return true;
}

// FIXME: In principle this function should be called EndListBox(). We should rename it after re-evaluating if we want to keep the same signature.
bool ui::ListBoxHeader(int fix, const char* label, int items_count, int height_in_items)
{
	// Size default to hold ~7.25 items.
	// We add +25% worth of item height to allow the user to see at a glance if there are more items up/down, without looking at the scrollbar.
	// We don't add this extra bit if items_count <= height_in_items. It is slightly dodgy, because it means a dynamic list of items will make the widget resize occasionally when it crosses that size.
	// I am expecting that someone will come and complain about this behavior in a remote future, then we can advise on a better solution.
	if (height_in_items < 0)
		height_in_items = ImMin(items_count, 7);
	const ImGuiStyle& style = GetStyle();
	float height_in_items_f = (height_in_items < items_count) ? (height_in_items + 0.25f) : (height_in_items + 0.00f);

	// We include ItemSpacing.y so that a list sized for the exact number of items doesn't make a scrollbar appears. We could also enforce that by passing a flag to BeginChild().
	ImVec2 size;
	size.x = 0.0f;
	size.y = GetTextLineHeightWithSpacing() * height_in_items_f + style.FramePadding.y * 2.0f;
	return ListBoxHeader(fix, label, size);
}

bool ui::ListBoxSkeet(int fix, const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data, int items_count, int height_in_items)
{
	if (!ListBoxHeader(fix, label, items_count, height_in_items))
		return false;

	// Assume all items have even height (= 1 line of text). If you need items of different or variable sizes you can create a custom version of ListBox() in your code without using the clipper.
	ImGuiContext& g = *GImGui;
	bool value_changed = false;
	ImGuiListClipper clipper(items_count, GetTextLineHeightWithSpacing()); // We know exactly our line height here so we pass it as a minor optimization, but generally you don't need to.
	while (clipper.Step())
		for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
		{
			const bool item_selected = (i == *current_item);
			const char* item_text;
			if (!items_getter(data, i, &item_text))
				item_text = "*Unknown item*";

			PushID(i);
			if (SelectableSkeet(item_text, item_selected))
			{
				*current_item = i;
				value_changed = true;
			}
			if (item_selected)
				SetItemDefaultFocus();
			PopID();
		}
	ListBoxFooter();
	if (value_changed)
		MarkItemEdited(g.CurrentWindow->DC.LastItemId);

	return value_changed;
}




bool ui::ListBoxSkeet(int fix, const char* szLabel, int* iCurrentItem, std::function<const char* (int)> pLambda, int nItemsCount, int iHeightInItems)
{
	return ListBoxSkeet(fix, szLabel, iCurrentItem, [](void* pData, int nIndex, const char** szOutText)
		{
			*szOutText = (*(std::function<const char* (int)>*)pData)(nIndex);
			return true;
		}, &pLambda, nItemsCount, iHeightInItems);
}


namespace ui
{
	static bool             BeginChildEx(const char* name, ImGuiID id, const ImVec2& size_arg, bool border, ImGuiWindowFlags flags);
	static void UpdateManualResize(ImGuiWindow* window, const ImVec2& size_auto_fit, int* border_held, int resize_grip_count, ImU32 resize_grip_col[4]);
}


static bool ui::BeginChildEx(const char* name, ImGuiID id, const ImVec2& size_arg, bool border, ImGuiWindowFlags flags)
{//koppel

	ImGuiStyle style;
	ImGuiContext& g = *GImGui;
	ImGuiWindow* parent_window = g.CurrentWindow;

	flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_ChildWindow;
	flags |= (parent_window->Flags & ImGuiWindowFlags_NoMove);  // Inherit the NoMove flag

	// Size
	const ImVec2 content_avail = GetContentRegionAvail();
	ImVec2 size = ImFloor(size_arg);
	const int auto_fit_axises = ((size.x == 0.0f) ? (1 << ImGuiAxis_X) : 0x00) | ((size.y == 0.0f) ? (1 << ImGuiAxis_Y) : 0x00);
	if (size.x <= 0.0f)
		size.x = ImMax(content_avail.x + size.x, 4.0f); // Arbitrary minimum child size (0.0f causing too much issues)
	if (size.y <= 0.0f)
		size.y = ImMax(content_avail.y + size.y, 4.0f);
	SetNextWindowSize(size);


	//style.color
	// Build up name. If you need to append to a same child from multiple location in the ID stack, use BeginChild(ImGuiID id) with a stable value.
	char title[256];
	if (name)
		ImFormatString(title, IM_ARRAYSIZE(title), "%s", name);
	else
		ImFormatString(title, IM_ARRAYSIZE(title), "##%08X", id);

	const float backup_border_size = g.Style.ChildBorderSize;
	//if (!border)
	g.Style.ChildBorderSize = 0.0f;
	bool ret = BeginSkeet(title, NULL, flags);
	g.Style.ChildBorderSize = backup_border_size;

	ImGuiWindow* child_window = g.CurrentWindow;
	child_window->ChildId = id;
	child_window->AutoFitChildAxises = auto_fit_axises;

	// Set the cursor to handle case where the user called SetNextWindowPos()+BeginChild() manually.
	// While this is not really documented/defined, it seems that the expected thing to do.
	if (child_window->BeginCount == 1)
		parent_window->DC.CursorPos = child_window->Pos;

	if (!(flags & ImGuiWindowFlags_ChildFrame))
		child_window->DC.CursorPos += ImVec2(16, 16);

	// Process navigation-in immediately so NavInit can run on first frame
	if (g.NavActivateId == id && !(flags & ImGuiWindowFlags_NavFlattened) && (child_window->DC.NavLayerActiveMask != 0 || child_window->DC.NavHasScroll))
	{
		FocusWindow(child_window);
		NavInitWindow(child_window, false);
		SetActiveID(id + 1, child_window); // Steal ActiveId with a dummy id so that key-press won't activate child item
		g.ActiveIdSource = ImGuiInputSource_Nav;
	}
	return ret;
	/*
	ImGuiContext& g = *GImGui;
	ImGuiWindow* parent_window = g.CurrentWindow;

	flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_ChildWindow;
	flags |= (parent_window->Flags & ImGuiWindowFlags_NoMove);  // Inherit the NoMove flag

	// Size
	const ImVec2 content_avail = GetContentRegionAvail();
	ImVec2 size = ImFloor(size_arg);
	const int auto_fit_axises = ((size.x == 0.0f) ? (1 << ImGuiAxis_X) : 0x00) | ((size.y == 0.0f) ? (1 << ImGuiAxis_Y) : 0x00);
	if (size.x <= 0.0f)
		size.x = ImMax(content_avail.x + size.x, 4.0f); // Arbitrary minimum child size (0.0f causing too much issues)
	if (size.y <= 0.0f)
		size.y = ImMax(content_avail.y + size.y, 4.0f);
	SetNextWindowSize(size);

	// Build up name. If you need to append to a same child from multiple location in the ID stack, use BeginChild(ImGuiID id) with a stable value.
	char title[256];
	ImFormatString(title, IM_ARRAYSIZE(title), "%s", name);

	const float backup_border_size = g.Style.ChildBorderSize;
	if (!border)
		g.Style.ChildBorderSize = 0.0f;
	bool ret = BeginSkeet(title, NULL, flags);
	g.Style.ChildBorderSize = backup_border_size;

	ImGuiWindow* child_window = g.CurrentWindow;
	child_window->ChildId = id;
	child_window->AutoFitChildAxises = auto_fit_axises;

	// Set the cursor to handle case where the user called SetNextWindowPos()+BeginChild() manually.
	// While this is not really documented/defined, it seems that the expected thing to do.
	if (child_window->BeginCount == 1)
		parent_window->DC.CursorPos = child_window->Pos;

	if (!(flags & ImGuiWindowFlags_ChildFrame))
		child_window->DC.CursorPos += ImVec2(16, 16);

	// Process navigation-in immediately so NavInit can run on first frame
	if (g.NavActivateId == id && !(flags & ImGuiWindowFlags_NavFlattened) && (child_window->DC.NavLayerActiveMask != 0 || child_window->DC.NavHasScroll))
	{
		FocusWindow(child_window);
		NavInitWindow(child_window, false);
		SetActiveID(id + 1, child_window); // Steal ActiveId with a dummy id so that key-press won't activate child item
		g.ActiveIdSource = ImGuiInputSource_Nav;
	}
	return ret;*/
}


bool ui::BeginChildSkeet(const char* str_id, const ImVec2& size_arg, bool border, ImGuiWindowFlags extra_flags)
{
	ImGuiWindow* window = GetCurrentWindow();
	return BeginChildEx(str_id, window->GetID(str_id), size_arg, border, extra_flags);
}
static void CheckStacksSize(ImGuiWindow* window, bool write)
{
	// NOT checking: DC.ItemWidth, DC.AllowKeyboardFocus, DC.ButtonRepeat, DC.TextWrapPos (per window) to allow user to conveniently push once and not pop (they are cleared on Begin)
	ImGuiContext& g = *GImGui;
	short* p_backup = &window->DC.StackSizesBackup[0];
	{ int current = window->IDStack.Size;       if (write) *p_backup = (short)current; else IM_ASSERT(*p_backup == current && "PushID/PopID or TreeNode/TreePop Mismatch!");   p_backup++; }    // Too few or too many PopID()/TreePop()
	{ int current = window->DC.GroupStack.Size; if (write) *p_backup = (short)current; else IM_ASSERT(*p_backup == current && "BeginGroup/EndGroup Mismatch!");                p_backup++; }    // Too few or too many EndGroup()
	{ int current = g.BeginPopupStack.Size;     if (write) *p_backup = (short)current; else IM_ASSERT(*p_backup == current && "BeginMenu/EndMenu or BeginPopup/EndPopup Mismatch"); p_backup++; }// Too few or too many EndMenu()/EndPopup()
	// For color, style and font stacks there is an incentive to use Push/Begin/Pop/.../End patterns, so we relax our checks a little to allow them.
	{ int current = g.ColorModifiers.Size;      if (write) *p_backup = (short)current; else IM_ASSERT(*p_backup >= current && "PushStyleColor/PopStyleColor Mismatch!");       p_backup++; }    // Too few or too many PopStyleColor()
	{ int current = g.StyleModifiers.Size;      if (write) *p_backup = (short)current; else IM_ASSERT(*p_backup >= current && "PushStyleVar/PopStyleVar Mismatch!");           p_backup++; }    // Too few or too many PopStyleVar()
	{ int current = g.FontStack.Size;           if (write) *p_backup = (short)current; else IM_ASSERT(*p_backup >= current && "PushFont/PopFont Mismatch!");                   p_backup++; }    // Too few or too many PopFont()
	IM_ASSERT(p_backup == window->DC.StackSizesBackup + IM_ARRAYSIZE(window->DC.StackSizesBackup));
}

static void SetCurrentWindow(ImGuiWindow* window)
{
	ImGuiContext& g = *GImGui;
	g.CurrentWindow = window;
	if (window)
		g.FontSize = g.DrawListSharedData.FontSize = window->CalcFontSize();
}

void ui::EndSkeet()
{
	ImGuiContext& g = *GImGui;

	if (g.CurrentWindowStack.Size <= 1 && g.FrameScopePushedImplicitWindow)
	{
		IM_ASSERT(g.CurrentWindowStack.Size > 1 && "Calling End() too many times!");
		return; // FIXME-ERRORHANDLING
	}
	IM_ASSERT(g.CurrentWindowStack.Size > 0);

	ImGuiWindow* window = g.CurrentWindow;

	if (window->DC.ColumnsSet != NULL)
		EndColumns();
	PopClipRect();   // Inner window clip rectangle

	if (window->Flags & ImGuiWindowFlags_ChildWindow && !(window->Flags & ImGuiWindowFlags_ChildFrame)) {
		window->DrawList->AddRectFilledMultiColor(window->Pos + ImVec2(2, 2), window->Pos + ImVec2(window->Size.x - 6, 20),
			ImColor(19 / 255.f, 19 / 255.f, 19 / 255.f, g.Style.Alpha),
			ImColor(19 / 255.f, 19 / 255.f, 19 / 255.f, g.Style.Alpha),
			ImColor(19 / 255.f, 19 / 255.f, 19 / 255.f, 0.f),
			ImColor(19 / 255.f, 19 / 255.f, 19 / 255.f, 0.f));

		window->DrawList->AddRectFilledMultiColor(window->Pos + ImVec2(2, window->Size.y - 22), window->Pos + ImVec2(window->Size.x - 6, window->Size.y - 1),
			ImColor(19 / 255.f, 19 / 255.f, 19 / 255.f, 0.f),
			ImColor(19 / 255.f, 19 / 255.f, 19 / 255.f, 0.f),
			ImColor(19 / 255.f, 19 / 255.f, 19 / 255.f, g.Style.Alpha),
			ImColor(19 / 255.f, 19 / 255.f, 19 / 255.f, g.Style.Alpha));
		ui::PushFont(skeet_menu.verdana);
		ImVec2 textSize = CalcTextSize(window->Name);
		window->DrawList->AddText(window->Pos + ImVec2(13, textSize.y / -2 + 2), ImColor(205 / 255.f, 205 / 255.f, 205 / 255.f, g.Style.Alpha), window->Name);
		ui::PopFont();
	}

	// Stop logging
	if (!(window->Flags & ImGuiWindowFlags_ChildWindow))    // FIXME: add more options for scope of logging
		LogFinish();

	// Pop from window stack
	g.CurrentWindowStack.pop_back();
	if (window->Flags & ImGuiWindowFlags_Popup)
		g.BeginPopupStack.pop_back();
	CheckStacksSize(window, false);
	SetCurrentWindow(g.CurrentWindowStack.empty() ? NULL : g.CurrentWindowStack.back());
}


void ui::EndChildSkeet()
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;

	IM_ASSERT(window->Flags & ImGuiWindowFlags_ChildWindow);   // Mismatched BeginChild()/EndChild() callss

	ItemSize(ImVec2(0, 11));
	if (window->BeginCount > 1)
	{
		EndSkeet();
	}
	else
	{
		ImVec2 sz = window->Size;
		if (window->AutoFitChildAxises & (1 << ImGuiAxis_X)) // Arbitrary minimum zero-ish child size of 4.0f causes less trouble than a 0.0f
			sz.x = ImMax(4.0f, sz.x);
		if (window->AutoFitChildAxises & (1 << ImGuiAxis_Y))
			sz.y = ImMax(4.0f, sz.y);
		EndSkeet();

		ImGuiWindow* parent_window = g.CurrentWindow;
		ImRect bb(parent_window->DC.CursorPos, parent_window->DC.CursorPos + sz);
		ItemSize(sz);
		if ((window->DC.NavLayerActiveMask != 0 || window->DC.NavHasScroll) && !(window->Flags & ImGuiWindowFlags_NavFlattened))
		{
			ItemAdd(bb, window->ChildId);
			RenderNavHighlight(bb, window->ChildId);

			// When browsing a window that has no activable items (scroll only) we keep a highlight on the child
			if (window->DC.NavLayerActiveMask == 0 && window == g.NavWindow)
				RenderNavHighlight(ImRect(bb.Min - ImVec2(2, 2), bb.Max + ImVec2(2, 2)), g.NavId, ImGuiNavHighlightFlags_TypeThin);
		}
		else
		{
			// Not navigable into
			ItemAdd(bb, 0);
		}
	}
}

void ui::PushColor(ImGuiCol idx, ImGuiCol idx2, const ImVec4& col) {

	ImGuiContext& g = *GImGui;
	ImGuiColorMod backup;
	backup.Col = idx;
	backup.BackupValue = g.Style.Colors[idx];
	g.ColorModifiers.push_back(backup);
	g.Style.Colors[idx] = g.Style.Colors[idx2];
}

bool ui::TabButtonExSkeetv2(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags, int* selected, int num, int total)
{
	ImGuiWindow* window = GetCurrentWindow();

	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;

	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	ImVec2 pos = window->DC.CursorPos;

	if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrentLineTextBaseOffset)
		pos.y += window->DC.CurrentLineTextBaseOffset - style.FramePadding.y;

	ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

	const ImRect bb(pos, pos + size);

	ItemSize(bb, style.FramePadding.y);

	if (!ItemAdd(bb, id))
		return false;

	if (window->DC.ItemFlags & ImGuiItemFlags_ButtonRepeat)
		flags |= ImGuiButtonFlags_Repeat;

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

	if (pressed)
		MarkItemEdited(id);

	// =========================
	// - Render
	// =========================

	if (hovered) {

		RenderFrame(bb.Min + ImVec2(2, 3), bb.Max + ImVec2(1, 0), ImColor(56, 54, 58, 0), false, 4.f);
	}
	else {

		RenderFrame(bb.Min, bb.Max, ImColor(56, 54, 58, 0), false, 4.f);
	}


	// Text
	PushColor(ImGuiCol_Text, ImGuiCol_Shonax_Skeet_v2_shadow, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
	RenderTextClipped(bb.Min + style.FramePadding + ImVec2(1, 1), bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);
	PopStyleColor();

	if (hovered) {

		PushColor(ImGuiCol_Text, ImGuiCol_Shonax_Skeet_v2_hovered, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);
		PopStyleColor();
	}
	else {

		PushColor(ImGuiCol_Text, ImGuiCol_Shonax_Skeet_v2_text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);
		PopStyleColor();
	}

	if (pressed)
		*selected = num;

	if (*selected == num)
	{
		PushColor(ImGuiCol_Text, ImGuiCol_Shonax_Skeet_v2_hovered, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);
		PopStyleColor();
	}
		

	return pressed;
}

bool ui::TabButtonSkeetv2(const char* label, int* selected, int num, int total, const ImVec2& size_arg)
{
	return TabButtonExSkeetv2(label, size_arg, 0, selected, num, total);
}

void ui::TabButtonSkeet(const char* label, int* selected, int num, int total) {
	//koppel
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	ImVec2 pos = window->Pos + ImVec2(7, 30) + ImVec2(0, (560 - 50) / 8 * num);
	ImVec2 size = ImVec2(75, 62);

	const ImRect bb(pos, pos + size);
	ItemSize(size, style.FramePadding.y);
	if (!ItemAdd(bb, id))
		return;

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, 0);
	if (pressed)
		MarkItemEdited(id);

	if (*selected != num)
	{
		window->DrawList->AddRectFilled(pos, pos + size + ImVec2(0, 1), ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));
		window->DrawList->AddLine(pos + ImVec2(size.x, -1), pos + ImVec2(size.x, size.y), ImColor(45 / 255.f, 45 / 255.f, 45 / 255.f, g.Style.Alpha));
	}
	else
	{
		//upper
		window->DrawList->AddLine(pos + ImVec2(0, 0), pos + ImVec2(size.x, 0), ImColor(45 / 255.f, 45 / 255.f, 45 / 255.f, g.Style.Alpha));
		//lower
		window->DrawList->AddLine(pos + ImVec2(0, 62), pos + ImVec2(size.x + 1, 62), ImColor(45 / 255.f, 45 / 255.f, 45 / 255.f, g.Style.Alpha));

		// upper background and line
		window->DrawList->AddRectFilled(window->Pos + ImVec2(7, 9), window->Pos + ImVec2(size.x + 7, 30), ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));
		window->DrawList->AddLine(window->Pos + ImVec2(size.x + 7, 9), window->Pos + ImVec2(size.x + 7, 30), ImColor(45 / 255.f, 45 / 255.f, 45 / 255.f, g.Style.Alpha));

		// lower background and line
		//koppelmenu
		if (ui::GetWindowSize().y > 620) {
				window->DrawList->AddRectFilled(window->Pos + ImVec2(7, 16) + ImVec2(0, (690 - 30) / total * 8 - 3), window->Pos + ImVec2(7, 16) + ImVec2(0, (622 - 15) / total * 8 + 1) + ImVec2(75, window->Size.y - 560), ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));
				window->DrawList->AddLine(window->Pos + ImVec2(7, 16) + ImVec2(0, (690 - 30) / total * 8 - 3) + ImVec2(size.x, -1), window->Pos + ImVec2(7, 16) + ImVec2(0, (622 - 15) / total * 8 + 1) + ImVec2(75, window->Size.y - 560), ImColor(45 / 255.f, 45 / 255.f, 45 / 255.f, g.Style.Alpha));
			}
		
		if (ui::GetWindowSize().y < 620) {
			window->DrawList->AddRectFilled(window->Pos + ImVec2(7, 16) + ImVec2(0, (650 - 33) / total * 8 - 2), window->Pos + ImVec2(7, 16) + ImVec2(0, (650 - 16) / total * 8 - 7) + ImVec2(75, window->Size.y - 650), ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));
			window->DrawList->AddLine(window->Pos + ImVec2(7, 16) + ImVec2(0, (650 - 35) / total * 8 - 2) + ImVec2(size.x, -1), window->Pos + ImVec2(7, 14) + ImVec2(0, (650 - 14) / total * 8 - 7) + ImVec2(75, window->Size.y - 652), ImColor(45 / 255.f, 45 / 255.f, 45 / 255.f, g.Style.Alpha));
			}
	}

	if (pressed)
		*selected = num;
	//koppelmenu
	//window->DrawList->AddRectFilled(window->Pos + ImVec2(7, 16) + ImVec2(0, (560 - 16) / total * 8 - 7), window->Pos + ImVec2(7, 16) + ImVec2(0, (560 - 16) / total * 8 - 7) + ImVec2(75, window->Size.y - 560), ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));
	//window->DrawList->AddLine(window->Pos + ImVec2(7, 16) + ImVec2(0, (560 - 16) / total * 8 - 7) + ImVec2(size.x, -1), window->Pos + ImVec2(7, 16) + ImVec2(0, (560 - 16) / total * 8 - 7) + ImVec2(75, window->Size.y - 560), ImColor(45 / 255.f, 45 / 255.f, 45 / 255.f, g.Style.Alpha));


	ImColor textColor = ImColor(100 / 255.f, 100 / 255.f, 100 / 255.f, g.Style.Alpha);
	if (hovered)
		textColor = ImColor(175 / 255.f, 175 / 255.f, 175 / 255.f, g.Style.Alpha);
	if (*selected == num)
		textColor = ImColor(205 / 255.f, 205 / 255.f, 205 / 255.f, g.Style.Alpha);

	if (num == 1) {
		window->DrawList->AddText(ImVec2(pos.x + size.x / 2 - label_size.x / 2 + 1, pos.y + size.y / 2 - label_size.y / 2 - 1), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, g.Style.Alpha / 1.5), label);
		window->DrawList->AddText(ImVec2(pos.x + size.x / 2 - label_size.x / 2 - 1, pos.y + size.y / 2 - label_size.y / 2 - 1), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, g.Style.Alpha / 1.5), label);
		window->DrawList->AddText(ImVec2(pos.x + size.x / 2 - label_size.x / 2 + 1, pos.y + size.y / 2 - label_size.y / 2), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, g.Style.Alpha / 1.5), label);
		window->DrawList->AddText(ImVec2(pos.x + size.x / 2 - label_size.x / 2, pos.y + size.y / 2 - label_size.y / 2 - 1), textColor, label);
	}
	else if (num == 8) {
		window->DrawList->AddText(ImVec2(pos.x + size.x / 2 - label_size.x / 2, pos.y + size.y / 2 - label_size.y / 2 - 1), textColor, label);
	}
	else {
		window->DrawList->AddText(ImVec2(pos.x + size.x / 2 - label_size.x / 2 + 1, pos.y + size.y / 2 - label_size.y / 2), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, g.Style.Alpha / 1.5), label);
		window->DrawList->AddText(ImVec2(pos.x + size.x / 2 - label_size.x / 2, pos.y + size.y / 2 - label_size.y / 2 - 1), textColor, label);
	}
	/*ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	ImVec2 pos = window->Pos + ImVec2(6, 20) + ImVec2(0, (window->Size.y - 24) / total * num - 5);
	ImVec2 size = ImVec2(92, 92 + (window->Size.y - 660) / total);

	const ImRect bb(pos, pos + size);
	ItemSize(size, style.FramePadding.y);
	if (!ItemAdd(bb, id))
		return;

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, 0);
	if (pressed)
		MarkItemEdited(id);

	if (!num)
	{
		window->DrawList->AddRectFilled(
			window->Pos + ImVec2(6, 8),
			pos + ImVec2(size.x, 0),
			ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));
		window->DrawList->AddLine(
			window->Pos + ImVec2(6 + size.x, 8),
			pos + ImVec2(size.x, 0),
			ImColor(45 / 255.f, 45 / 255.f, 45 / 255.f, g.Style.Alpha));
	}

	if (*selected != num)
	{
		window->DrawList->AddRectFilled(
			pos + ImVec2(0, 0),
			pos + ImVec2(size.x, size.y),
			ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));
		window->DrawList->AddLine(
			pos + ImVec2(size.x, 0),
			pos + ImVec2(size.x, size.y),
			ImColor(45 / 255.f, 45 / 255.f, 45 / 255.f, g.Style.Alpha));
	}
	else
	{
		//upper
		window->DrawList->AddLine(
			pos + ImVec2(0, 0),
			pos + ImVec2(size.x, 0),
			ImColor(45 / 255.f, 45 / 255.f, 45 / 255.f, g.Style.Alpha));
		//lower
		window->DrawList->AddLine(
			pos + ImVec2(0, size.y - 2),
			pos + ImVec2(size.x, size.y - 2),
			ImColor(45 / 255.f, 45 / 255.f, 45 / 255.f, g.Style.Alpha));
	}

	if (pressed)
		*selected = num;

	ImColor textColor = ImColor(80 / 255.f, 80 / 255.f, 80 / 255.f, g.Style.Alpha);
	if (hovered)
		textColor = ImColor(140 / 255.f, 140 / 255.f, 140 / 255.f, g.Style.Alpha);
	if (*selected == num)
		textColor = ImColor(205 / 255.f, 205 / 255.f, 205 / 255.f, g.Style.Alpha);

	window->DrawList->AddText(ImVec2(pos.x + size.x / 2 - label_size.x / 2, pos.y + size.y / 2 - label_size.y / 2 - 1), textColor, label);*/
}


static void SetWindowConditionAllowFlags(ImGuiWindow* window, ImGuiCond flags, bool enabled)
{
	window->SetWindowPosAllowFlags = enabled ? (window->SetWindowPosAllowFlags | flags) : (window->SetWindowPosAllowFlags & ~flags);
	window->SetWindowSizeAllowFlags = enabled ? (window->SetWindowSizeAllowFlags | flags) : (window->SetWindowSizeAllowFlags & ~flags);
	window->SetWindowCollapsedAllowFlags = enabled ? (window->SetWindowCollapsedAllowFlags | flags) : (window->SetWindowCollapsedAllowFlags & ~flags);
}


static ImGuiWindow* CreateNewWindow(const char* name, ImVec2 size, ImGuiWindowFlags flags)
{
	ImGuiContext& g = *GImGui;

	// Create window the first time
	ImGuiWindow* window = IM_NEW(ImGuiWindow)(&g, name);
	window->Flags = flags;
	g.WindowsById.SetVoidPtr(window->ID, window);

	// Default/arbitrary window position. Use SetNextWindowPos() with the appropriate condition flag to change the initial position of a window.
	window->Pos = ImVec2(60, 60);

	// User can disable loading and saving of settings. Tooltip and child windows also don't store settings.
	if (!(flags & ImGuiWindowFlags_NoSavedSettings))
		if (ImGuiWindowSettings* settings = ui::FindWindowSettings(window->ID))
		{
			// Retrieve settings from .ini file
			window->SettingsIdx = g.SettingsWindows.index_from_ptr(settings);
			SetWindowConditionAllowFlags(window, ImGuiCond_FirstUseEver, false);
			window->Pos = ImFloor(settings->Pos);
			window->Collapsed = settings->Collapsed;
			if (ImLengthSqr(settings->Size) > 0.00001f)
				size = ImFloor(settings->Size);
		}
	window->Size = window->SizeFull = window->SizeFullAtLastBegin = ImFloor(size);
	window->DC.CursorMaxPos = window->Pos; // So first call to CalcSizeContents() doesn't return crazy values

	if ((flags & ImGuiWindowFlags_AlwaysAutoResize) != 0)
	{
		window->AutoFitFramesX = window->AutoFitFramesY = 2;
		window->AutoFitOnlyGrows = false;
	}
	else
	{
		if (window->Size.x <= 0.0f)
			window->AutoFitFramesX = 2;
		if (window->Size.y <= 0.0f)
			window->AutoFitFramesY = 2;
		window->AutoFitOnlyGrows = (window->AutoFitFramesX > 0) || (window->AutoFitFramesY > 0);
	}

	g.WindowsFocusOrder.push_back(window);
	if (flags & ImGuiWindowFlags_NoBringToFrontOnFocus)
		g.Windows.push_front(window); // Quite slow but rare and only once
	else
		g.Windows.push_back(window);
	return window;
}

static ImVec2 CalcSizeContents(ImGuiWindow* window)
{
	if (window->Collapsed)
		if (window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0)
			return window->SizeContents;
	if (window->Hidden && window->HiddenFramesForResize == 0 && window->HiddenFramesRegular > 0)
		return window->SizeContents;

	ImVec2 sz;
	sz.x = (float)(int)((window->SizeContentsExplicit.x != 0.0f) ? window->SizeContentsExplicit.x : (window->DC.CursorMaxPos.x - window->Pos.x + window->Scroll.x));
	sz.y = (float)(int)((window->SizeContentsExplicit.y != 0.0f) ? window->SizeContentsExplicit.y : (window->DC.CursorMaxPos.y - window->Pos.y + window->Scroll.y));
	return sz + window->WindowPadding;
}

static ImVec2 CalcSizeAfterConstraint(ImGuiWindow* window, ImVec2 new_size)
{
	ImGuiContext& g = *GImGui;
	if (g.NextWindowData.SizeConstraintCond != 0)
	{
		// Using -1,-1 on either X/Y axis to preserve the current size.
		ImRect cr = g.NextWindowData.SizeConstraintRect;
		new_size.x = (cr.Min.x >= 0 && cr.Max.x >= 0) ? ImClamp(new_size.x, cr.Min.x, cr.Max.x) : window->SizeFull.x;
		new_size.y = (cr.Min.y >= 0 && cr.Max.y >= 0) ? ImClamp(new_size.y, cr.Min.y, cr.Max.y) : window->SizeFull.y;
		if (g.NextWindowData.SizeCallback)
		{
			ImGuiSizeCallbackData data;
			data.UserData = g.NextWindowData.SizeCallbackUserData;
			data.Pos = window->Pos;
			data.CurrentSize = window->SizeFull;
			data.DesiredSize = new_size;
			g.NextWindowData.SizeCallback(&data);
			new_size = data.DesiredSize;
		}
	}

	// Minimum size
	if (!(window->Flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_AlwaysAutoResize)))
	{
		new_size = ImMax(new_size, g.Style.WindowMinSize);
		new_size.y = ImMax(new_size.y, window->TitleBarHeight() + window->MenuBarHeight() + ImMax(0.0f, g.Style.WindowRounding - 1.0f)); // Reduce artifacts with very small windows
	}
	return new_size;
}


static ImVec2 CalcSizeAutoFit(ImGuiWindow* window, const ImVec2& size_contents)
{
	ImGuiContext& g = *GImGui;
	ImGuiStyle& style = g.Style;
	if (window->Flags & ImGuiWindowFlags_Tooltip)
	{
		// Tooltip always resize
		return size_contents;
	}
	else
	{
		// Maximum window size is determined by the display size
		const bool is_popup = (window->Flags & ImGuiWindowFlags_Popup) != 0;
		const bool is_menu = (window->Flags & ImGuiWindowFlags_ChildMenu) != 0;
		ImVec2 size_min = style.WindowMinSize;
		if (is_popup || is_menu) // Popups and menus bypass style.WindowMinSize by default, but we give then a non-zero minimum size to facilitate understanding problematic cases (e.g. empty popups)
			size_min = ImMin(size_min, ImVec2(4.0f, 4.0f));
		ImVec2 size_auto_fit = ImClamp(size_contents, size_min, ImMax(size_min, g.IO.DisplaySize - style.DisplaySafeAreaPadding * 2.0f));

		// When the window cannot fit all contents (either because of constraints, either because screen is too small),
		// we are growing the size on the other axis to compensate for expected scrollbar. FIXME: Might turn bigger than ViewportSize-WindowPadding.
		ImVec2 size_auto_fit_after_constraint = CalcSizeAfterConstraint(window, size_auto_fit);
		if (size_auto_fit_after_constraint.x < size_contents.x && !(window->Flags & ImGuiWindowFlags_NoScrollbar) && (window->Flags & ImGuiWindowFlags_HorizontalScrollbar))
			size_auto_fit.y += style.ScrollbarSize;
		if (size_auto_fit_after_constraint.y < size_contents.y && !(window->Flags & ImGuiWindowFlags_NoScrollbar))
			size_auto_fit.x += style.ScrollbarSize;
		return size_auto_fit;
	}
}

static ImVec2 CalcNextScrollFromScrollTargetAndClamp(ImGuiWindow* window, bool snap_on_edges)
{
	ImGuiContext& g = *GImGui;
	ImVec2 scroll = window->Scroll;
	if (window->ScrollTarget.x < FLT_MAX)
	{
		float cr_x = window->ScrollTargetCenterRatio.x;
		scroll.x = window->ScrollTarget.x - cr_x * (window->SizeFull.x - window->ScrollbarSizes.x);
	}
	if (window->ScrollTarget.y < FLT_MAX)
	{
		// 'snap_on_edges' allows for a discontinuity at the edge of scrolling limits to take account of WindowPadding so that scrolling to make the last item visible scroll far enough to see the padding.
		float cr_y = window->ScrollTargetCenterRatio.y;
		float target_y = window->ScrollTarget.y;
		if (snap_on_edges && cr_y <= 0.0f && target_y <= window->WindowPadding.y)
			target_y = 0.0f;
		if (snap_on_edges && cr_y >= 1.0f && target_y >= window->SizeContents.y - window->WindowPadding.y + g.Style.ItemSpacing.y)
			target_y = window->SizeContents.y;
		scroll.y = target_y - (1.0f - cr_y) * (window->TitleBarHeight() + window->MenuBarHeight()) - cr_y * (window->SizeFull.y - window->ScrollbarSizes.y);
	}
	scroll = ImMax(scroll, ImVec2(0.0f, 0.0f));
	if (!window->Collapsed && !window->SkipItems)
	{
		scroll.x = ImMin(scroll.x, ui::GetWindowScrollMaxX(window));
		scroll.y = ImMin(scroll.y, ui::GetWindowScrollMaxY(window));
	}
	return scroll;
}




static ImGuiCol GetWindowBgColorIdxFromFlags(ImGuiWindowFlags flags)
{
	if (flags & (ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_Popup))
		return ImGuiCol_PopupBg;
	if (flags & ImGuiWindowFlags_ChildWindow)
		return ImGuiCol_ChildBg;
	return ImGuiCol_WindowBg;
}

static void CalcResizePosSizeFromAnyCorner(ImGuiWindow* window, const ImVec2& corner_target, const ImVec2& corner_norm, ImVec2* out_pos, ImVec2* out_size)
{
	ImVec2 pos_min = ImLerp(corner_target, window->Pos, corner_norm);                // Expected window upper-left
	ImVec2 pos_max = ImLerp(window->Pos + window->Size, corner_target, corner_norm); // Expected window lower-right
	ImVec2 size_expected = pos_max - pos_min;
	ImVec2 size_constrained = CalcSizeAfterConstraint(window, size_expected);
	*out_pos = pos_min;
	if (corner_norm.x == 0.0f)
		out_pos->x -= (size_constrained.x - size_expected.x);
	if (corner_norm.y == 0.0f)
		out_pos->y -= (size_constrained.y - size_expected.y);
	*out_size = size_constrained;
}

struct ImGuiResizeGripDef
{
	ImVec2  CornerPosN;
	ImVec2  InnerDir;
	int     AngleMin12, AngleMax12;
};

static const ImGuiResizeGripDef resize_grip_def[4] =
{
	{ ImVec2(1,1), ImVec2(-1,-1), 0, 3 }, // Lower right
	{ ImVec2(0,1), ImVec2(+1,-1), 3, 6 }, // Lower left
	{ ImVec2(0,0), ImVec2(+1,+1), 6, 9 }, // Upper left
	{ ImVec2(1,0), ImVec2(-1,+1), 9,12 }, // Upper right
};

static ImRect GetResizeBorderRect(ImGuiWindow* window, int border_n, float perp_padding, float thickness)
{
	ImRect rect = window->Rect();
	if (thickness == 0.0f) rect.Max -= ImVec2(1, 1);
	if (border_n == 0) return ImRect(rect.Min.x + perp_padding, rect.Min.y - thickness, rect.Max.x - perp_padding, rect.Min.y + thickness);      // Top
	if (border_n == 1) return ImRect(rect.Max.x - thickness, rect.Min.y + perp_padding, rect.Max.x + thickness, rect.Max.y - perp_padding);   // Right
	if (border_n == 2) return ImRect(rect.Min.x + perp_padding, rect.Max.y - thickness, rect.Max.x - perp_padding, rect.Max.y + thickness);      // Bottom
	if (border_n == 3) return ImRect(rect.Min.x - thickness, rect.Min.y + perp_padding, rect.Min.x + thickness, rect.Max.y - perp_padding);   // Left
	IM_ASSERT(0);
	return ImRect();
}



static const float WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS = 4.0f;
static const float WINDOWS_RESIZE_FROM_EDGES_FEEDBACK_TIMER = 0.04f;

static void ui::UpdateManualResize(ImGuiWindow* window, const ImVec2& size_auto_fit, int* border_held, int resize_grip_count, ImU32 resize_grip_col[4])
{
	ImGuiContext& g = *GImGui;
	ImGuiWindowFlags flags = window->Flags;
	if ((flags & ImGuiWindowFlags_NoResize) || (flags & ImGuiWindowFlags_AlwaysAutoResize) || window->AutoFitFramesX > 0 || window->AutoFitFramesY > 0)
		return;
	if (window->WasActive == false) // Early out to avoid running this code for e.g. an hidden implicit/fallback Debug window.
		return;

	const int resize_border_count = g.IO.ConfigWindowsResizeFromEdges ? 4 : 0;
	const float grip_draw_size = (float)(int)ImMax(g.FontSize * 1.35f, window->WindowRounding + 1.0f + g.FontSize * 0.2f);
	const float grip_hover_inner_size = (float)(int)(grip_draw_size * 0.75f);
	const float grip_hover_outer_size = g.IO.ConfigWindowsResizeFromEdges ? WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS : 0.0f;

	ImVec2 pos_target(FLT_MAX, FLT_MAX);
	ImVec2 size_target(FLT_MAX, FLT_MAX);

	// Manual resize grips
	PushID("#RESIZE");
	for (int resize_grip_n = 0; resize_grip_n < resize_grip_count; resize_grip_n++)
	{
		const ImGuiResizeGripDef& grip = resize_grip_def[resize_grip_n];
		const ImVec2 corner = ImLerp(window->Pos, window->Pos + window->Size, grip.CornerPosN);

		// Using the FlattenChilds button flag we make the resize button accessible even if we are hovering over a child window
		ImRect resize_rect(corner - grip.InnerDir * grip_hover_outer_size, corner + grip.InnerDir * grip_hover_inner_size);
		if (resize_rect.Min.x > resize_rect.Max.x) ImSwap(resize_rect.Min.x, resize_rect.Max.x);
		if (resize_rect.Min.y > resize_rect.Max.y) ImSwap(resize_rect.Min.y, resize_rect.Max.y);
		bool hovered, held;
		ButtonBehavior(resize_rect, window->GetID((void*)(intptr_t)resize_grip_n), &hovered, &held, ImGuiButtonFlags_FlattenChildren | ImGuiButtonFlags_NoNavFocus);
		//GetOverlayDrawList(window)->AddRect(resize_rect.Min, resize_rect.Max, IM_COL32(255, 255, 0, 255));
		if (hovered || held)
			g.MouseCursor = (resize_grip_n & 1) ? ImGuiMouseCursor_ResizeNESW : ImGuiMouseCursor_ResizeNWSE;

		if (held && g.IO.MouseDoubleClicked[0] && resize_grip_n == 0)
		{
			// Manual auto-fit when double-clicking
			size_target = CalcSizeAfterConstraint(window, size_auto_fit);
			ClearActiveID();
		}
		else if (held)
		{
			// Resize from any of the four corners
			// We don't use an incremental MouseDelta but rather compute an absolute target size based on mouse position
			ImVec2 corner_target = g.IO.MousePos - g.ActiveIdClickOffset + ImLerp(grip.InnerDir * grip_hover_outer_size, grip.InnerDir * -grip_hover_inner_size, grip.CornerPosN); // Corner of the window corresponding to our corner grip
			CalcResizePosSizeFromAnyCorner(window, corner_target, grip.CornerPosN, &pos_target, &size_target);
		}
		if (resize_grip_n == 0 || held || hovered)
			resize_grip_col[resize_grip_n] = GetColorU32(held ? ImGuiCol_ResizeGripActive : hovered ? ImGuiCol_ResizeGripHovered : ImGuiCol_ResizeGrip);
	}
	for (int border_n = 0; border_n < resize_border_count; border_n++)
	{
		bool hovered, held;
		ImRect border_rect = GetResizeBorderRect(window, border_n, grip_hover_inner_size, WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS);
		ButtonBehavior(border_rect, window->GetID((void*)(intptr_t)(border_n + 4)), &hovered, &held, ImGuiButtonFlags_FlattenChildren);
		//GetOverlayDrawList(window)->AddRect(border_rect.Min, border_rect.Max, IM_COL32(255, 255, 0, 255));
		if ((hovered && g.HoveredIdTimer > WINDOWS_RESIZE_FROM_EDGES_FEEDBACK_TIMER) || held)
		{
			g.MouseCursor = (border_n & 1) ? ImGuiMouseCursor_ResizeEW : ImGuiMouseCursor_ResizeNS;
			if (held)
				*border_held = border_n;
		}
		if (held)
		{
			ImVec2 border_target = window->Pos;
			ImVec2 border_posn;
			if (border_n == 0) { border_posn = ImVec2(0, 0); border_target.y = (g.IO.MousePos.y - g.ActiveIdClickOffset.y + WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS); } // Top
			if (border_n == 1) { border_posn = ImVec2(1, 0); border_target.x = (g.IO.MousePos.x - g.ActiveIdClickOffset.x + WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS); } // Right
			if (border_n == 2) { border_posn = ImVec2(0, 1); border_target.y = (g.IO.MousePos.y - g.ActiveIdClickOffset.y + WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS); } // Bottom
			if (border_n == 3) { border_posn = ImVec2(0, 0); border_target.x = (g.IO.MousePos.x - g.ActiveIdClickOffset.x + WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS); } // Left
			CalcResizePosSizeFromAnyCorner(window, border_target, border_posn, &pos_target, &size_target);
		}
	}
	PopID();

	// Navigation resize (keyboard/gamepad)
	if (g.NavWindowingTarget && g.NavWindowingTarget->RootWindow == window)
	{
		ImVec2 nav_resize_delta;
		if (g.NavInputSource == ImGuiInputSource_NavKeyboard && g.IO.KeyShift)
			nav_resize_delta = GetNavInputAmount2d(ImGuiNavDirSourceFlags_Keyboard, ImGuiInputReadMode_Down);
		if (g.NavInputSource == ImGuiInputSource_NavGamepad)
			nav_resize_delta = GetNavInputAmount2d(ImGuiNavDirSourceFlags_PadDPad, ImGuiInputReadMode_Down);
		if (nav_resize_delta.x != 0.0f || nav_resize_delta.y != 0.0f)
		{
			const float NAV_RESIZE_SPEED = 600.0f;
			nav_resize_delta *= ImFloor(NAV_RESIZE_SPEED * g.IO.DeltaTime * ImMin(g.IO.DisplayFramebufferScale.x, g.IO.DisplayFramebufferScale.y));
			g.NavWindowingToggleLayer = false;
			g.NavDisableMouseHover = true;
			resize_grip_col[0] = GetColorU32(ImGuiCol_ResizeGripActive);
			// FIXME-NAV: Should store and accumulate into a separate size buffer to handle sizing constraints properly, right now a constraint will make us stuck.
			size_target = CalcSizeAfterConstraint(window, window->SizeFull + nav_resize_delta);
		}
	}

	// Apply back modified position/size to window
	if (size_target.x != FLT_MAX)
	{
		window->SizeFull = size_target;
		MarkIniSettingsDirty(window);
	}
	if (pos_target.x != FLT_MAX)
	{
		window->Pos = ImFloor(pos_target);
		MarkIniSettingsDirty(window);
	}

	window->Size = window->SizeFull;
}
static ImRect GetViewportRect()
{
	ImGuiContext& g = *GImGui;
	return ImRect(0.0f, 0.0f, g.IO.DisplaySize.x, g.IO.DisplaySize.y);
}

bool ui::BeginSkeet(const char* name, bool* p_open, ImGuiWindowFlags flags)
{
	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	IM_ASSERT(name != NULL && name[0] != '\0');     // Window name required
	IM_ASSERT(g.FrameScopeActive);                  // Forgot to call ImGui::NewFrame()
	IM_ASSERT(g.FrameCountEnded != g.FrameCount);   // Called ImGui::Render() or ImGui::EndFrame() and haven't called ImGui::NewFrame() again yet

	// Find or create
	ImGuiWindow* window = FindWindowByName(name);
	const bool window_just_created = (window == NULL);
	if (window_just_created)
	{
		ImVec2 size_on_first_use = (g.NextWindowData.SizeCond != 0) ? g.NextWindowData.SizeVal : ImVec2(0.0f, 0.0f); // Any condition flag will do since we are creating a new window here.
		window = CreateNewWindow(name, size_on_first_use, flags);
	}

	// Automatically disable manual moving/resizing when NoInputs is set
	if ((flags & ImGuiWindowFlags_NoInputs) == ImGuiWindowFlags_NoInputs)
		flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

	if (flags & ImGuiWindowFlags_NavFlattened)
		IM_ASSERT(flags & ImGuiWindowFlags_ChildWindow);

	const int current_frame = g.FrameCount;
	const bool first_begin_of_the_frame = (window->LastFrameActive != current_frame);

	// Update Flags, LastFrameActive, BeginOrderXXX fields
	if (first_begin_of_the_frame)
		window->Flags = (ImGuiWindowFlags)flags;
	else
		flags = window->Flags;

	// Parent window is latched only on the first call to Begin() of the frame, so further append-calls can be done from a different window stack
	ImGuiWindow* parent_window_in_stack = g.CurrentWindowStack.empty() ? NULL : g.CurrentWindowStack.back();
	ImGuiWindow* parent_window = first_begin_of_the_frame ? ((flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Popup)) ? parent_window_in_stack : NULL) : window->ParentWindow;
	IM_ASSERT(parent_window != NULL || !(flags & ImGuiWindowFlags_ChildWindow));
	window->HasCloseButton = (p_open != NULL);

	// Update the Appearing flag
	bool window_just_activated_by_user = (window->LastFrameActive < current_frame - 1);   // Not using !WasActive because the implicit "Debug" window would always toggle off->on
	const bool window_just_appearing_after_hidden_for_resize = (window->HiddenFramesForResize > 0);
	if (flags & ImGuiWindowFlags_Popup)
	{
		ImGuiPopupRef& popup_ref = g.OpenPopupStack[g.BeginPopupStack.Size];
		window_just_activated_by_user |= (window->PopupId != popup_ref.PopupId); // We recycle popups so treat window as activated if popup id changed
		window_just_activated_by_user |= (window != popup_ref.Window);
	}
	window->Appearing = (window_just_activated_by_user || window_just_appearing_after_hidden_for_resize);
	if (window->Appearing)
		SetWindowConditionAllowFlags(window, ImGuiCond_Appearing, true);

	// Add to stack
	g.CurrentWindowStack.push_back(window);
	SetCurrentWindow(window);
	CheckStacksSize(window, true);
	if (flags & ImGuiWindowFlags_Popup)
	{
		ImGuiPopupRef& popup_ref = g.OpenPopupStack[g.BeginPopupStack.Size];
		popup_ref.Window = window;
		g.BeginPopupStack.push_back(popup_ref);
		window->PopupId = popup_ref.PopupId;
	}

	if (window_just_appearing_after_hidden_for_resize && !(flags & ImGuiWindowFlags_ChildWindow))
		window->NavLastIds[0] = 0;

	// Process SetNextWindow***() calls
	bool window_pos_set_by_api = false;
	bool window_size_x_set_by_api = false, window_size_y_set_by_api = false;
	if (g.NextWindowData.PosCond)
	{
		window_pos_set_by_api = (window->SetWindowPosAllowFlags & g.NextWindowData.PosCond) != 0;
		if (window_pos_set_by_api && ImLengthSqr(g.NextWindowData.PosPivotVal) > 0.00001f)
		{
			// May be processed on the next frame if this is our first frame and we are measuring size
			// FIXME: Look into removing the branch so everything can go through this same code path for consistency.
			window->SetWindowPosVal = g.NextWindowData.PosVal;
			window->SetWindowPosPivot = g.NextWindowData.PosPivotVal;
			window->SetWindowPosAllowFlags &= ~(ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing);
		}
		else
		{
			SetWindowPos(window, g.NextWindowData.PosVal, g.NextWindowData.PosCond);
		}
	}
	if (g.NextWindowData.SizeCond)
	{
		window_size_x_set_by_api = (window->SetWindowSizeAllowFlags & g.NextWindowData.SizeCond) != 0 && (g.NextWindowData.SizeVal.x > 0.0f);
		window_size_y_set_by_api = (window->SetWindowSizeAllowFlags & g.NextWindowData.SizeCond) != 0 && (g.NextWindowData.SizeVal.y > 0.0f);
		SetWindowSize(window, g.NextWindowData.SizeVal, g.NextWindowData.SizeCond);
	}
	if (g.NextWindowData.ContentSizeCond)
	{
		// Adjust passed "client size" to become a "window size"
		window->SizeContentsExplicit = g.NextWindowData.ContentSizeVal;
		if (window->SizeContentsExplicit.y != 0.0f)
			window->SizeContentsExplicit.y += window->TitleBarHeight() + window->MenuBarHeight();
	}
	else if (first_begin_of_the_frame)
	{
		window->SizeContentsExplicit = ImVec2(0.0f, 0.0f);
	}
	if (g.NextWindowData.CollapsedCond)
		SetWindowCollapsed(window, g.NextWindowData.CollapsedVal, g.NextWindowData.CollapsedCond);
	if (g.NextWindowData.FocusCond)
		FocusWindow(window);
	if (window->Appearing)
		SetWindowConditionAllowFlags(window, ImGuiCond_Appearing, false);

	// When reusing window again multiple times a frame, just append content (don't need to setup again)
	if (first_begin_of_the_frame)
	{
		// Initialize
		const bool window_is_child_tooltip = (flags & ImGuiWindowFlags_ChildWindow) && (flags & ImGuiWindowFlags_Tooltip); // FIXME-WIP: Undocumented behavior of Child+Tooltip for pinned tooltip (#1345)
		UpdateWindowParentAndRootLinks(window, flags, parent_window);

		window->Active = true;
		window->BeginOrderWithinParent = 0;
		window->BeginOrderWithinContext = (short)(g.WindowsActiveCount++);
		window->BeginCount = 0;
		window->ClipRect = ImVec4(-FLT_MAX, -FLT_MAX, +FLT_MAX, +FLT_MAX);
		window->LastFrameActive = current_frame;
		window->IDStack.resize(1);

		// Update stored window name when it changes (which can _only_ happen with the "###" operator, so the ID would stay unchanged).
		// The title bar always display the 'name' parameter, so we only update the string storage if it needs to be visible to the end-user elsewhere.
		bool window_title_visible_elsewhere = false;
		if (g.NavWindowingList != NULL && (window->Flags & ImGuiWindowFlags_NoNavFocus) == 0)   // Window titles visible when using CTRL+TAB
			window_title_visible_elsewhere = true;
		if (window_title_visible_elsewhere && !window_just_created && strcmp(name, window->Name) != 0)
		{
			size_t buf_len = (size_t)window->NameBufLen;
			window->Name = ImStrdupcpy(window->Name, &buf_len, name);
			window->NameBufLen = (int)buf_len;
		}

		// UPDATE CONTENTS SIZE, UPDATE HIDDEN STATUS

		// Update contents size from last frame for auto-fitting (or use explicit size)
		window->SizeContents = CalcSizeContents(window);
		if (window->HiddenFramesRegular > 0)
			window->HiddenFramesRegular--;
		if (window->HiddenFramesForResize > 0)
			window->HiddenFramesForResize--;

		// Hide new windows for one frame until they calculate their size
		if (window_just_created && (!window_size_x_set_by_api || !window_size_y_set_by_api))
			window->HiddenFramesForResize = 1;

		// Hide popup/tooltip window when re-opening while we measure size (because we recycle the windows)
		// We reset Size/SizeContents for reappearing popups/tooltips early in this function, so further code won't be tempted to use the old size.
		if (window_just_activated_by_user && (flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) != 0)
		{
			window->HiddenFramesForResize = 1;
			if (flags & ImGuiWindowFlags_AlwaysAutoResize)
			{
				if (!window_size_x_set_by_api)
					window->Size.x = window->SizeFull.x = 0.f;
				if (!window_size_y_set_by_api)
					window->Size.y = window->SizeFull.y = 0.f;
				window->SizeContents = ImVec2(0.f, 0.f);
			}
		}

		SetCurrentWindow(window);

		// Lock border size and padding for the frame (so that altering them doesn't cause inconsistencies)
		window->WindowBorderSize = (flags & ImGuiWindowFlags_ChildWindow) ? style.ChildBorderSize : ((flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) && !(flags & ImGuiWindowFlags_Modal)) ? style.PopupBorderSize : style.WindowBorderSize;
		window->WindowPadding = style.WindowPadding;
		if ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & (ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_Popup)) && window->WindowBorderSize == 0.0f)
			window->WindowPadding = ImVec2(0.0f, (flags & ImGuiWindowFlags_MenuBar) ? style.WindowPadding.y : 0.0f);
		window->DC.MenuBarOffset.x = ImMax(ImMax(window->WindowPadding.x, style.ItemSpacing.x), g.NextWindowData.MenuBarOffsetMinVal.x);
		window->DC.MenuBarOffset.y = g.NextWindowData.MenuBarOffsetMinVal.y;

		// Collapse window by double-clicking on title bar
		// At this point we don't have a clipping rectangle setup yet, so we can use the title bar area for hit detection and drawing
		if (!(flags & ImGuiWindowFlags_NoTitleBar) && !(flags & ImGuiWindowFlags_NoCollapse))
		{
			// We don't use a regular button+id to test for double-click on title bar (mostly due to legacy reason, could be fixed), so verify that we don't have items over the title bar.
			ImRect title_bar_rect = window->TitleBarRect();
			if (g.HoveredWindow == window && g.HoveredId == 0 && g.HoveredIdPreviousFrame == 0 && IsMouseHoveringRect(title_bar_rect.Min, title_bar_rect.Max) && g.IO.MouseDoubleClicked[0])
				window->WantCollapseToggle = true;
			if (window->WantCollapseToggle)
			{
				window->Collapsed = !window->Collapsed;
				MarkIniSettingsDirty(window);
				FocusWindow(window);
			}
		}
		else
		{
			window->Collapsed = false;
		}
		window->WantCollapseToggle = false;

		// SIZE

		// Calculate auto-fit size, handle automatic resize
		const ImVec2 size_auto_fit = CalcSizeAutoFit(window, window->SizeContents);
		ImVec2 size_full_modified(FLT_MAX, FLT_MAX);
		if ((flags & ImGuiWindowFlags_AlwaysAutoResize) && !window->Collapsed)
		{
			// Using SetNextWindowSize() overrides ImGuiWindowFlags_AlwaysAutoResize, so it can be used on tooltips/popups, etc.
			if (!window_size_x_set_by_api)
				window->SizeFull.x = size_full_modified.x = size_auto_fit.x;
			if (!window_size_y_set_by_api)
				window->SizeFull.y = size_full_modified.y = size_auto_fit.y;
		}
		else if (window->AutoFitFramesX > 0 || window->AutoFitFramesY > 0)
		{
			// Auto-fit may only grow window during the first few frames
			// We still process initial auto-fit on collapsed windows to get a window width, but otherwise don't honor ImGuiWindowFlags_AlwaysAutoResize when collapsed.
			if (!window_size_x_set_by_api && window->AutoFitFramesX > 0)
				window->SizeFull.x = size_full_modified.x = window->AutoFitOnlyGrows ? ImMax(window->SizeFull.x, size_auto_fit.x) : size_auto_fit.x;
			if (!window_size_y_set_by_api && window->AutoFitFramesY > 0)
				window->SizeFull.y = size_full_modified.y = window->AutoFitOnlyGrows ? ImMax(window->SizeFull.y, size_auto_fit.y) : size_auto_fit.y;
			if (!window->Collapsed)
				MarkIniSettingsDirty(window);
		}

		// Apply minimum/maximum window size constraints and final size
		window->SizeFull = CalcSizeAfterConstraint(window, window->SizeFull);
		window->Size = window->Collapsed && !(flags & ImGuiWindowFlags_ChildWindow) ? window->TitleBarRect().GetSize() : window->SizeFull;

		if (flags & ImGuiWindowFlags_ChildFrame) {
			window->SizeFull.x = parent_window->Size.x - 64;
			window->Size.x = parent_window->Size.x - 64;
		}

		// SCROLLBAR STATUS

		// Update scrollbar status (based on the Size that was effective during last frame or the auto-resized Size).
		if (!window->Collapsed)
		{
			// When reading the current size we need to read it after size constraints have been applied
			float size_x_for_scrollbars = size_full_modified.x != FLT_MAX ? window->SizeFull.x : window->SizeFullAtLastBegin.x;
			float size_y_for_scrollbars = size_full_modified.y != FLT_MAX ? window->SizeFull.y : window->SizeFullAtLastBegin.y;
			window->ScrollbarY = (flags & ImGuiWindowFlags_AlwaysVerticalScrollbar) || ((window->SizeContents.y > size_y_for_scrollbars) && !(flags & ImGuiWindowFlags_NoScrollbar));
			window->ScrollbarX = (flags & ImGuiWindowFlags_AlwaysHorizontalScrollbar) || ((window->SizeContents.x > size_x_for_scrollbars - (window->ScrollbarY ? style.ScrollbarSize : 0.0f)) && !(flags & ImGuiWindowFlags_NoScrollbar) && (flags & ImGuiWindowFlags_HorizontalScrollbar));
			if (window->ScrollbarX && !window->ScrollbarY)
				window->ScrollbarY = (window->SizeContents.y > size_y_for_scrollbars - style.ScrollbarSize) && !(flags & ImGuiWindowFlags_NoScrollbar);
			window->ScrollbarSizes = ImVec2(window->ScrollbarY ? style.ScrollbarSize : 0.0f, window->ScrollbarX ? style.ScrollbarSize : 0.0f);
		}

		// POSITION

		// Popup latch its initial position, will position itself when it appears next frame
		if (window_just_activated_by_user)
		{
			window->AutoPosLastDirection = ImGuiDir_None;
			if ((flags & ImGuiWindowFlags_Popup) != 0 && !window_pos_set_by_api)
				window->Pos = g.BeginPopupStack.back().OpenPopupPos;
		}

		// Position child window
		if (flags & ImGuiWindowFlags_ChildWindow)
		{
			IM_ASSERT(parent_window && parent_window->Active);
			window->BeginOrderWithinParent = (short)parent_window->DC.ChildWindows.Size;
			parent_window->DC.ChildWindows.push_back(window);
			if (!(flags & ImGuiWindowFlags_Popup) && !window_pos_set_by_api && !window_is_child_tooltip)
				window->Pos = parent_window->DC.CursorPos + ImVec2(flags & ImGuiWindowFlags_ChildFrame ? 16 : 0, 0);
		}

		const bool window_pos_with_pivot = (window->SetWindowPosVal.x != FLT_MAX && window->HiddenFramesForResize == 0);
		if (window_pos_with_pivot)
			SetWindowPos(window, ImMax(style.DisplaySafeAreaPadding, window->SetWindowPosVal - window->SizeFull * window->SetWindowPosPivot), 0); // Position given a pivot (e.g. for centering)
		else if ((flags & ImGuiWindowFlags_ChildMenu) != 0)
			window->Pos = FindBestWindowPosForPopup(window);
		else if ((flags & ImGuiWindowFlags_Popup) != 0 && !window_pos_set_by_api && window_just_appearing_after_hidden_for_resize)
			window->Pos = FindBestWindowPosForPopup(window);
		else if ((flags & ImGuiWindowFlags_Tooltip) != 0 && !window_pos_set_by_api && !window_is_child_tooltip)
			window->Pos = FindBestWindowPosForPopup(window);

		// Clamp position so it stays visible
		// Ignore zero-sized display explicitly to avoid losing positions if a window manager reports zero-sized window when initializing or minimizing.
		if (!window_pos_set_by_api && !(flags & ImGuiWindowFlags_ChildWindow) && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0)
		{
			if (g.IO.DisplaySize.x > 0.0f && g.IO.DisplaySize.y > 0.0f) // Ignore zero-sized display explicitly to avoid losing positions if a window manager reports zero-sized window when initializing or minimizing.
			{
				ImVec2 padding = ImMax(style.DisplayWindowPadding, style.DisplaySafeAreaPadding);
				ImVec2 size_for_clamping = window->Size;
				window->Pos = ImMax(window->Pos + size_for_clamping, padding) - size_for_clamping;
				window->Pos = ImMin(window->Pos, g.IO.DisplaySize - padding);
			}
		}
		window->Pos = ImFloor(window->Pos);

		// Lock window rounding for the frame (so that altering them doesn't cause inconsistencies)
		window->WindowRounding = (flags & ImGuiWindowFlags_ChildWindow) ? style.ChildRounding : ((flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiWindowFlags_Modal)) ? style.PopupRounding : style.WindowRounding;

		// Prepare for item focus requests
		window->FocusIdxAllRequestCurrent = (window->FocusIdxAllRequestNext == INT_MAX || window->FocusIdxAllCounter == -1) ? INT_MAX : (window->FocusIdxAllRequestNext + (window->FocusIdxAllCounter + 1)) % (window->FocusIdxAllCounter + 1);
		window->FocusIdxTabRequestCurrent = (window->FocusIdxTabRequestNext == INT_MAX || window->FocusIdxTabCounter == -1) ? INT_MAX : (window->FocusIdxTabRequestNext + (window->FocusIdxTabCounter + 1)) % (window->FocusIdxTabCounter + 1);
		window->FocusIdxAllCounter = window->FocusIdxTabCounter = -1;
		window->FocusIdxAllRequestNext = window->FocusIdxTabRequestNext = INT_MAX;

		// Apply scrolling
		window->Scroll = CalcNextScrollFromScrollTargetAndClamp(window, true);
		window->ScrollTarget = ImVec2(FLT_MAX, FLT_MAX);

		// Apply window focus (new and reactivated windows are moved to front)
		bool want_focus = false;
		if (window_just_activated_by_user && !(flags & ImGuiWindowFlags_NoFocusOnAppearing))
		{
			if (flags & ImGuiWindowFlags_Popup)
				want_focus = true;
			else if ((flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Tooltip)) == 0)
				want_focus = true;
		}

		// Handle manual resize: Resize Grips, Borders, Gamepad
		int border_held = -1;
		ImU32 resize_grip_col[4] = { 0 };
		const int resize_grip_count = g.IO.ConfigWindowsResizeFromEdges ? 2 : 1; // 4
		const float grip_draw_size = (float)(int)ImMax(g.FontSize * 1.35f, window->WindowRounding + 1.0f + g.FontSize * 0.2f);
		if (!window->Collapsed)
			UpdateManualResize(window, size_auto_fit, &border_held, resize_grip_count, &resize_grip_col[0]);
		window->ResizeBorderHeld = (signed char)border_held;

		// Default item width. Make it proportional to window size if window manually resizes
		if (window->Size.x > 0.0f && !(flags & ImGuiWindowFlags_Tooltip) && !(flags & ImGuiWindowFlags_AlwaysAutoResize))
			window->ItemWidthDefault = (float)(int)(window->Size.x * 0.65f);
		else
			window->ItemWidthDefault = (float)(int)(g.FontSize * 16.0f);

		// DRAWING

		// Setup draw list and outer clipping rectangle
		window->DrawList->Clear();
		window->DrawList->Flags = (g.Style.AntiAliasedLines ? ImDrawListFlags_AntiAliasedLines : 0) | (g.Style.AntiAliasedFill ? ImDrawListFlags_AntiAliasedFill : 0);
		window->DrawList->PushTextureID(g.Font->ContainerAtlas->TexID);
		ImRect viewport_rect(GetViewportRect());

		if ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Popup) && !window_is_child_tooltip)
			PushClipRect(parent_window->ClipRect.Min, parent_window->ClipRect.Max, true);
		else
			PushClipRect(viewport_rect.Min, viewport_rect.Max, true);

		// Draw modal window background (darkens what is behind them, all viewports)
		const bool dim_bg_for_modal = (flags & ImGuiWindowFlags_Modal) && window == GetFrontMostPopupModal() && window->HiddenFramesForResize <= 0;
		const bool dim_bg_for_window_list = g.NavWindowingTargetAnim && (window == g.NavWindowingTargetAnim->RootWindow);
		if (dim_bg_for_modal || dim_bg_for_window_list)
		{
			const ImU32 dim_bg_col = GetColorU32(dim_bg_for_modal ? ImGuiCol_ModalWindowDimBg : ImGuiCol_NavWindowingDimBg, g.DimBgRatio);
			window->DrawList->AddRectFilled(viewport_rect.Min, viewport_rect.Max, dim_bg_col);
		}

		// Draw navigation selection/windowing rectangle background
		if (dim_bg_for_window_list && window == g.NavWindowingTargetAnim)
		{
			ImRect bb = window->Rect();
			bb.Expand(g.FontSize);
			if (!bb.Contains(viewport_rect)) // Avoid drawing if the window covers all the viewport anyway
				window->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(ImGuiCol_NavWindowingHighlight, g.NavWindowingHighlightAlpha * 0.25f), g.Style.WindowRounding);
		}

		// Draw window + handle manual resize
		// As we highlight the title bar when want_focus is set, multiple reappearing windows will have have their title bar highlighted on their reappearing frame.
		const float window_rounding = window->WindowRounding;
		const float window_border_size = window->WindowBorderSize;
		const ImGuiWindow* window_to_highlight = g.NavWindowingTarget ? g.NavWindowingTarget : g.NavWindow;
		const bool title_bar_is_highlight = want_focus || (window_to_highlight && window->RootWindowForTitleBarHighlight == window_to_highlight->RootWindowForTitleBarHighlight);
		const ImRect title_bar_rect = window->TitleBarRect();
		if (window->Collapsed)
		{
			// Title bar only
			float backup_border_size = style.FrameBorderSize;
			g.Style.FrameBorderSize = window->WindowBorderSize;
			ImU32 title_bar_col = GetColorU32((title_bar_is_highlight && !g.NavDisableHighlight) ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBgCollapsed);
			RenderFrame(title_bar_rect.Min, title_bar_rect.Max, title_bar_col, true, window_rounding);
			g.Style.FrameBorderSize = backup_border_size;
		}
		else
		{
			// Window background
			if (!(flags & ImGuiWindowFlags_NoBackground))
			{
				if (!(flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiWindowFlags_Tooltip)) {
					ImU32 bg_col = ImColor(19 / 255.f, 19 / 255.f, 19 / 255.f, g.Style.Alpha);
					float alpha = 1.0f;
					if (g.NextWindowData.BgAlphaCond != 0)
						alpha = g.NextWindowData.BgAlphaVal;
					if (alpha != 1.0f)
						bg_col = (bg_col & ~IM_COL32_A_MASK) | (IM_F32_TO_INT8_SAT(alpha) << IM_COL32_A_SHIFT);
					window->DrawList->AddRectFilled(window->Pos + ImVec2(0, window->TitleBarHeight()), window->Pos + window->Size, bg_col, window_rounding, (flags & ImGuiWindowFlags_NoTitleBar) ? ImDrawCornerFlags_All : ImDrawCornerFlags_Bot);

					window->DrawList->PushClipRect(window->Pos, window->Pos + window->Size, true);
					//window->DrawList->AddImage(c_menu::get()->get_texture_data(), window->Pos, window->Pos + ImVec2(4096, 4096), ImVec2(0, 0), ImVec2(1, 1), ImColor(1.f, 1.f, 1.f, g.Style.Alpha));
					window->DrawList->PopClipRect();

					static auto outline = [](ImGuiWindow* wnd, int i, ImColor col) -> void {
						wnd->DrawList->AddRect(wnd->Pos + ImVec2(i, i), wnd->Pos + wnd->Size - ImVec2(i, i), col);
					};

					outline(window, 0, ImColor(10 / 255.f, 10 / 255.f, 10 / 255.f, g.Style.Alpha));
					outline(window, 1, ImColor(60 / 255.f, 60 / 255.f, 60 / 255.f, g.Style.Alpha));
					outline(window, 2, ImColor(40 / 255.f, 40 / 255.f, 40 / 255.f, g.Style.Alpha));
					outline(window, 3, ImColor(40 / 255.f, 40 / 255.f, 40 / 255.f, g.Style.Alpha));
					outline(window, 4, ImColor(40 / 255.f, 40 / 255.f, 40 / 255.f, g.Style.Alpha));
					outline(window, 5, ImColor(60 / 255.f, 60 / 255.f, 60 / 255.f, g.Style.Alpha));
					outline(window, 6, ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));



					window->DrawList->AddRectFilledMultiColor(window->Pos + ImVec2(7, 7), window->Pos + ImVec2(window->Size.x - (window->Size.x / 2), 9), ImColor(55 / 255.f, 177 / 255.f, 218 / 255.f, g.Style.Alpha), ImColor(201 / 255.f, 72 / 255.f, 205 / 255.f, g.Style.Alpha), ImColor(107 / 255.f, 38 / 255.f, 109 / 255.f, g.Style.Alpha), ImColor(30 / 255.f, 94 / 255.f, 116 / 255.f, g.Style.Alpha));

					window->DrawList->AddRectFilledMultiColor(window->Pos + ImVec2(7 + (window->Size.x / 2) - 7, 7), window->Pos + ImVec2(window->Size.x - 7, 9), ImColor(201 / 255.f, 72 / 255.f, 205 / 255.f, g.Style.Alpha), ImColor(204 / 255.f, 227 / 255.f, 53 / 255.f, g.Style.Alpha), ImColor(109 / 255.f, 121 / 255.f, 28 / 255.f, g.Style.Alpha), ImColor(107 / 255.f, 38 / 255.f, 109 / 255.f, g.Style.Alpha));

					//window->DrawList->AddLine(window->Pos + ImVec2(4 + 92, 10), window->Pos + ImVec2(4 + 92, 4 + 10), ImColor(45 / 255.f, 45 / 255.f, 45 / 255.f, g.Style.Alpha));

				}
				else {
					if (flags & ImGuiWindowFlags_ChildWindow && !(flags & ImGuiWindowFlags_ChildFrame) && !(flags & ImGuiWindowFlags_Tooltip)) {
						ImGuiIO* io = &GetIO();
						PushFont(io->Fonts->Fonts[1]);
						ImVec2 textSize = CalcTextSize(name);
						PopFont();
						ImU32 bg_col = ImColor(23 / 255.f, 23 / 255.f, 23 / 255.f, g.Style.Alpha);
						float alpha = 1.0f;
						if (g.NextWindowData.BgAlphaCond != 0)
							alpha = g.NextWindowData.BgAlphaVal;
						if (alpha != 1.0f)
							bg_col = (bg_col & ~IM_COL32_A_MASK) | (IM_F32_TO_INT8_SAT(alpha) << IM_COL32_A_SHIFT);
						window->DrawList->AddRectFilled(window->Pos + ImVec2(0, window->TitleBarHeight()), window->Pos + window->Size, bg_col, window_rounding, (flags & ImGuiWindowFlags_NoTitleBar) ? ImDrawCornerFlags_All : ImDrawCornerFlags_Bot);

						window->DrawList->AddLine(window->Pos, window->Pos + ImVec2(0, window->Size.y), ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));
						window->DrawList->AddLine(window->Pos + ImVec2(0, window->Size.y), window->Pos + window->Size + ImVec2(1, 0), ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));
						window->DrawList->AddLine(window->Pos + window->Size, window->Pos + ImVec2(window->Size.x, 0), ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));
						window->DrawList->AddLine(window->Pos, window->Pos + ImVec2(10, 0), ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));
						window->DrawList->AddLine(window->Pos + ImVec2(16 + textSize.x, 0), window->Pos + ImVec2(window->Size.x, 0), ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));
						//koppel
						ImColor defcolor = /*ImColor(159 / 255.f, 202 / 255.f, 43 / 255.f, g.Style.Alpha);  :*/ ImColor(45 / 255.f, 45 / 255.f, 45 / 255.f, g.Style.Alpha);

						window->DrawList->AddLine(window->Pos + ImVec2(1, 1), window->Pos + ImVec2(1, window->Size.y - 1), defcolor);
						window->DrawList->AddLine(window->Pos + ImVec2(1, window->Size.y - 1), window->Pos + window->Size - ImVec2(1, 1), defcolor);
						window->DrawList->AddLine(window->Pos + window->Size - ImVec2(1, 1), window->Pos + ImVec2(window->Size.x - 1, 1), defcolor);
						window->DrawList->AddLine(window->Pos + ImVec2(1, 1), window->Pos + ImVec2(10, 1), defcolor);
						window->DrawList->AddLine(window->Pos + ImVec2(16 + textSize.x, 1), window->Pos + ImVec2(window->Size.x - 1, 1), defcolor);
						window->DrawList->AddTriangleFilled(window->Pos + ImVec2(-1, -1) + window->Size, window->Pos + ImVec2(-5, -1) + window->Size, window->Pos + ImVec2(-1, -5) + window->Size, ImColor(45 / 255.f, 45 / 255.f, 45 / 255.f, g.Style.Alpha));
					}
					else if (flags & ImGuiWindowFlags_Popup || flags & ImGuiWindowFlags_Tooltip || flags & ImGuiWindowFlags_ChildFrame) {
						ImU32 bg_col = ImColor(36 / 255.f, 36 / 255.f, 36 / 255.f, g.Style.Alpha);
						float alpha = 1.0f;
						if (g.NextWindowData.BgAlphaCond != 0)
							alpha = g.NextWindowData.BgAlphaVal;
						if (alpha != 1.0f)
							bg_col = (bg_col & ~IM_COL32_A_MASK) | (IM_F32_TO_INT8_SAT(alpha) << IM_COL32_A_SHIFT);
						window->DrawList->AddRectFilled(window->Pos + ImVec2(0, window->TitleBarHeight()), window->Pos + window->Size, bg_col, window_rounding, (flags & ImGuiWindowFlags_NoTitleBar) ? ImDrawCornerFlags_All : ImDrawCornerFlags_Bot);

						window->DrawList->AddRect(window->Pos, window->Pos + window->Size, ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));
					}
				}
			}
			g.NextWindowData.BgAlphaCond = 0;

			// Title bar
			if (!(flags & ImGuiWindowFlags_NoTitleBar))
			{
				ImU32 title_bar_col = GetColorU32(title_bar_is_highlight ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBg);
				window->DrawList->AddRectFilled(title_bar_rect.Min, title_bar_rect.Max, title_bar_col, window_rounding, ImDrawCornerFlags_Top);
			}

			// Menu bar
			if (flags & ImGuiWindowFlags_MenuBar)
			{
				ImRect menu_bar_rect = window->MenuBarRect();
				menu_bar_rect.ClipWith(window->Rect());  // Soft clipping, in particular child window don't have minimum size covering the menu bar so this is useful for them.
				window->DrawList->AddRectFilled(menu_bar_rect.Min, menu_bar_rect.Max, GetColorU32(ImGuiCol_MenuBarBg), (flags & ImGuiWindowFlags_NoTitleBar) ? window_rounding : 0.0f, ImDrawCornerFlags_Top);
				if (style.FrameBorderSize > 0.0f && menu_bar_rect.Max.y < window->Pos.y + window->Size.y)
					window->DrawList->AddLine(menu_bar_rect.GetBL(), menu_bar_rect.GetBR(), GetColorU32(ImGuiCol_Border), style.FrameBorderSize);
			}

			// Scrollbars
			if (window->ScrollbarX)
				Scrollbar(ImGuiLayoutType_Horizontal);
			if (window->ScrollbarY)
				Scrollbar(ImGuiLayoutType_Vertical);
		}

		// Store a backup of SizeFull which we will use next frame to decide if we need scrollbars.
		window->SizeFullAtLastBegin = window->SizeFull;

		// Update various regions. Variables they depends on are set above in this function.
		// FIXME: window->ContentsRegionRect.Max is currently very misleading / partly faulty, but some BeginChild() patterns relies on it.
		window->ContentsRegionRect.Min.x = window->Pos.x - window->Scroll.x + window->WindowPadding.x;
		window->ContentsRegionRect.Min.y = window->Pos.y - window->Scroll.y + window->WindowPadding.y + window->TitleBarHeight() + window->MenuBarHeight();
		window->ContentsRegionRect.Max.x = window->Pos.x - window->Scroll.x - window->WindowPadding.x + (window->SizeContentsExplicit.x != 0.0f ? window->SizeContentsExplicit.x : (window->Size.x - window->ScrollbarSizes.x));
		window->ContentsRegionRect.Max.y = window->Pos.y - window->Scroll.y - window->WindowPadding.y + (window->SizeContentsExplicit.y != 0.0f ? window->SizeContentsExplicit.y : (window->Size.y - window->ScrollbarSizes.y));

		// Setup drawing context
		// (NB: That term "drawing context / DC" lost its meaning a long time ago. Initially was meant to hold transient data only. Nowadays difference between window-> and window->DC-> is dubious.)
		window->DC.Indent.x = 0.0f + window->WindowPadding.x - window->Scroll.x;
		window->DC.GroupOffset.x = 0.0f;
		window->DC.ColumnsOffset.x = 0.0f;
		window->DC.CursorStartPos = window->Pos + ImVec2(window->DC.Indent.x + window->DC.ColumnsOffset.x, window->TitleBarHeight() + window->MenuBarHeight() + window->WindowPadding.y - window->Scroll.y);
		window->DC.CursorPos = window->DC.CursorStartPos;
		window->DC.CursorPosPrevLine = window->DC.CursorPos;
		window->DC.CursorMaxPos = window->DC.CursorStartPos;
		window->DC.CurrentLineSize = window->DC.PrevLineSize = ImVec2(0.0f, 0.0f);
		window->DC.CurrentLineTextBaseOffset = window->DC.PrevLineTextBaseOffset = 0.0f;
		window->DC.NavHideHighlightOneFrame = false;
		window->DC.NavHasScroll = (GetWindowScrollMaxY(window) > 0.0f);
		window->DC.NavLayerActiveMask = window->DC.NavLayerActiveMaskNext;
		window->DC.NavLayerActiveMaskNext = 0x00;
		window->DC.MenuBarAppending = false;
		window->DC.LogLinePosY = window->DC.CursorPos.y - 9999.0f;
		window->DC.ChildWindows.resize(0);
		window->DC.LayoutType = ImGuiLayoutType_Vertical;
		window->DC.ParentLayoutType = parent_window ? parent_window->DC.LayoutType : ImGuiLayoutType_Vertical;
		window->DC.ItemFlags = parent_window ? parent_window->DC.ItemFlags : ImGuiItemFlags_Default_;
		window->DC.ItemWidth = window->ItemWidthDefault;
		window->DC.TextWrapPos = -1.0f; // disabled
		window->DC.ItemFlagsStack.resize(0);
		window->DC.ItemWidthStack.resize(0);
		window->DC.TextWrapPosStack.resize(0);
		window->DC.ColumnsSet = NULL;
		window->DC.TreeDepth = 0;
		window->DC.TreeDepthMayJumpToParentOnPop = 0x00;
		window->DC.StateStorage = &window->StateStorage;
		window->DC.GroupStack.resize(0);
		window->MenuColumns.Update(3, style.ItemSpacing.x, window_just_activated_by_user);

		if (flags & ImGuiWindowFlags_Popup)
			window->DC.CursorPos.y += 4;

		if ((flags & ImGuiWindowFlags_ChildWindow) && (window->DC.ItemFlags != parent_window->DC.ItemFlags))
		{
			window->DC.ItemFlags = parent_window->DC.ItemFlags;
			window->DC.ItemFlagsStack.push_back(window->DC.ItemFlags);
		}

		if (window->AutoFitFramesX > 0)
			window->AutoFitFramesX--;
		if (window->AutoFitFramesY > 0)
			window->AutoFitFramesY--;

		// Apply focus (we need to call FocusWindow() AFTER setting DC.CursorStartPos so our initial navigation reference rectangle can start around there)
		if (want_focus)
		{
			FocusWindow(window);
			NavInitWindow(window, false);
		}

		// Title bar
		if (!(flags & ImGuiWindowFlags_NoTitleBar))
		{
			// Close & collapse button are on layer 1 (same as menus) and don't default focus
			const ImGuiItemFlags item_flags_backup = window->DC.ItemFlags;
			window->DC.ItemFlags |= ImGuiItemFlags_NoNavDefaultFocus;
			window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
			window->DC.NavLayerCurrentMask = (1 << ImGuiNavLayer_Menu);

			// Collapse button
			if (!(flags & ImGuiWindowFlags_NoCollapse))
				if (CollapseButton(window->GetID("#COLLAPSE"), window->Pos))
					window->WantCollapseToggle = true; // Defer collapsing to next frame as we are too far in the Begin() function

			// Close button
			if (p_open != NULL)
			{
				const float pad = style.FramePadding.y;
				const float rad = g.FontSize * 0.5f;
				if (CloseButton(window->GetID("#CLOSE"), window->Rect().GetTR() + ImVec2(-pad - rad, pad + rad), rad + 1))
					*p_open = false;
			}

			window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
			window->DC.NavLayerCurrentMask = (1 << ImGuiNavLayer_Main);
			window->DC.ItemFlags = item_flags_backup;

			// Title bar text (with: horizontal alignment, avoiding collapse/close button, optional "unsaved document" marker)
			// FIXME: Refactor text alignment facilities along with RenderText helpers, this is too much code..
			const char* UNSAVED_DOCUMENT_MARKER = "*";
			float marker_size_x = (flags & ImGuiWindowFlags_UnsavedDocument) ? CalcTextSize(UNSAVED_DOCUMENT_MARKER, NULL, false).x : 0.0f;
			ImVec2 text_size = CalcTextSize(name, NULL, true) + ImVec2(marker_size_x, 0.0f);
			ImRect text_r = title_bar_rect;
			float pad_left = (flags & ImGuiWindowFlags_NoCollapse) ? style.FramePadding.x : (style.FramePadding.x + g.FontSize + style.ItemInnerSpacing.x);
			float pad_right = (p_open == NULL) ? style.FramePadding.x : (style.FramePadding.x + g.FontSize + style.ItemInnerSpacing.x);
			if (style.WindowTitleAlign.x > 0.0f)
				pad_right = ImLerp(pad_right, pad_left, style.WindowTitleAlign.x);
			text_r.Min.x += pad_left;
			text_r.Max.x -= pad_right;
			ImRect clip_rect = text_r;
			clip_rect.Max.x = window->Pos.x + window->Size.x - (p_open ? title_bar_rect.GetHeight() - 3 : style.FramePadding.x); // Match the size of CloseButton()
			RenderTextClipped(text_r.Min, text_r.Max, name, NULL, &text_size, style.WindowTitleAlign, &clip_rect);
			if (flags & ImGuiWindowFlags_UnsavedDocument)
			{
				ImVec2 marker_pos = ImVec2(ImMax(text_r.Min.x, text_r.Min.x + (text_r.GetWidth() - text_size.x) * style.WindowTitleAlign.x) + text_size.x, text_r.Min.y) + ImVec2(2 - marker_size_x, 0.0f);
				ImVec2 off = ImVec2(0.0f, (float)(int)(-g.FontSize * 0.25f));
				RenderTextClipped(marker_pos + off, text_r.Max + off, UNSAVED_DOCUMENT_MARKER, NULL, NULL, ImVec2(0, style.WindowTitleAlign.y), &clip_rect);
			}
		}

		// Save clipped aabb so we can access it in constant-time in FindHoveredWindow()
		window->OuterRectClipped = window->Rect();
		window->OuterRectClipped.ClipWith(window->ClipRect);

		// Pressing CTRL+C while holding on a window copy its content to the clipboard
		// This works but 1. doesn't handle multiple Begin/End pairs, 2. recursing into another Begin/End pair - so we need to work that out and add better logging scope.
		// Maybe we can support CTRL+C on every element?
		/*
		if (g.ActiveId == move_id)
			if (g.IO.KeyCtrl && IsKeyPressedMap(ImGuiKey_C))
				LogToClipboard();
		*/

		// Inner rectangle
		// We set this up after processing the resize grip so that our clip rectangle doesn't lag by a frame
		// Note that if our window is collapsed we will end up with an inverted (~null) clipping rectangle which is the correct behavior.
		window->InnerMainRect.Min.x = title_bar_rect.Min.x + window->WindowBorderSize;
		window->InnerMainRect.Min.y = title_bar_rect.Max.y + window->MenuBarHeight() + (((flags & ImGuiWindowFlags_MenuBar) || !(flags & ImGuiWindowFlags_NoTitleBar)) ? style.FrameBorderSize : window->WindowBorderSize);
		window->InnerMainRect.Max.x = window->Pos.x + window->Size.x - window->ScrollbarSizes.x - window->WindowBorderSize;
		window->InnerMainRect.Max.y = window->Pos.y + window->Size.y - window->ScrollbarSizes.y - window->WindowBorderSize;
		//window->DrawList->AddRect(window->InnerRect.Min, window->InnerRect.Max, IM_COL32_WHITE);

		// Inner clipping rectangle
		// Force round operator last to ensure that e.g. (int)(max.x-min.x) in user's render code produce correct result.
		window->InnerClipRect.Min.x = ImFloor(0.5f + window->InnerMainRect.Min.x + ImMax(0.0f, ImFloor(window->WindowPadding.x * 0.5f - window->WindowBorderSize)));
		window->InnerClipRect.Min.y = ImFloor(0.5f + window->InnerMainRect.Min.y);
		window->InnerClipRect.Max.x = ImFloor(0.5f + window->InnerMainRect.Max.x - ImMax(0.0f, ImFloor(window->WindowPadding.x * 0.5f - window->WindowBorderSize)));
		window->InnerClipRect.Max.y = ImFloor(0.5f + window->InnerMainRect.Max.y);

		if (flags & ImGuiWindowFlags_ChildWindow) {
			window->InnerClipRect.Min.y += 2;
			window->InnerClipRect.Max.y -= 2;
		}

		/*if (flags & ImGuiWindowFlags_Popup) {
			window->InnerClipRect.Max.y += 4;
			window->InnerMainRect.Max.y -= 4;
		}*/

		// We fill last item data based on Title Bar, in order for IsItemHovered() and IsItemActive() to be usable after Begin().
		// This is useful to allow creating context menus on title bar only, etc.
		window->DC.LastItemId = window->MoveId;
		window->DC.LastItemStatusFlags = IsMouseHoveringRect(title_bar_rect.Min, title_bar_rect.Max, false) ? ImGuiItemStatusFlags_HoveredRect : 0;
		window->DC.LastItemRect = title_bar_rect;
	}

	PushClipRect(window->InnerClipRect.Min, window->InnerClipRect.Max, true);

	// Clear 'accessed' flag last thing (After PushClipRect which will set the flag. We want the flag to stay false when the default "Debug" window is unused)
	if (first_begin_of_the_frame)
		window->WriteAccessed = false;

	window->BeginCount++;
	g.NextWindowData.Clear();

	if (flags & ImGuiWindowFlags_ChildWindow)
	{
		// Child window can be out of sight and have "negative" clip windows.
		// Mark them as collapsed so commands are skipped earlier (we can't manually collapse them because they have no title bar).
		IM_ASSERT((flags & ImGuiWindowFlags_NoTitleBar) != 0);
		if (!(flags & ImGuiWindowFlags_AlwaysAutoResize) && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0)
			if (window->OuterRectClipped.Min.x >= window->OuterRectClipped.Max.x || window->OuterRectClipped.Min.y >= window->OuterRectClipped.Max.y)
				window->HiddenFramesRegular = 1;

		// Completely hide along with parent or if parent is collapsed
		if (parent_window && (parent_window->Collapsed || parent_window->Hidden))
			window->HiddenFramesRegular = 1;
	}

	// Don't render if style alpha is 0.0 at the time of Begin(). This is arbitrary and inconsistent but has been there for a long while (may remove at some point)
	if (style.Alpha <= 0.0f)
		window->HiddenFramesRegular = 1;

	// Update the Hidden flag
	window->Hidden = (window->HiddenFramesRegular > 0) || (window->HiddenFramesForResize > 0);

	// Return false if we don't intend to display anything to allow user to perform an early out optimization
	window->SkipItems = (window->Collapsed || !window->Active || window->Hidden) && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0 && window->HiddenFramesForResize <= 0;

	return !window->SkipItems;
	//koppel
	/*
	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	IM_ASSERT(name != NULL && name[0] != '\0');     // Window name required
	IM_ASSERT(g.FrameScopeActive);                  // Forgot to call ImGui::NewFrame()
	IM_ASSERT(g.FrameCountEnded != g.FrameCount);   // Called ImGui::Render() or ImGui::EndFrame() and haven't called ImGui::NewFrame() again yet

	// Find or create
	ImGuiWindow* window = FindWindowByName(name);
	const bool window_just_created = (window == NULL);
	if (window_just_created)
	{
		ImVec2 size_on_first_use = (g.NextWindowData.SizeCond != 0) ? g.NextWindowData.SizeVal : ImVec2(0.0f, 0.0f); // Any condition flag will do since we are creating a new window here.
		window = CreateNewWindow(name, size_on_first_use, flags);
	}

	// Automatically disable manual moving/resizing when NoInputs is set
	if ((flags & ImGuiWindowFlags_NoInputs) == ImGuiWindowFlags_NoInputs)
		flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

	if (flags & ImGuiWindowFlags_NavFlattened)
		IM_ASSERT(flags & ImGuiWindowFlags_ChildWindow);

	const int current_frame = g.FrameCount;
	const bool first_begin_of_the_frame = (window->LastFrameActive != current_frame);

	// Update Flags, LastFrameActive, BeginOrderXXX fields
	if (first_begin_of_the_frame)
		window->Flags = (ImGuiWindowFlags)flags;
	else
		flags = window->Flags;

	// Parent window is latched only on the first call to Begin() of the frame, so further append-calls can be done from a different window stack
	ImGuiWindow* parent_window_in_stack = g.CurrentWindowStack.empty() ? NULL : g.CurrentWindowStack.back();
	ImGuiWindow* parent_window = first_begin_of_the_frame ? ((flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Popup)) ? parent_window_in_stack : NULL) : window->ParentWindow;
	IM_ASSERT(parent_window != NULL || !(flags & ImGuiWindowFlags_ChildWindow));
	window->HasCloseButton = (p_open != NULL);

	// Update the Appearing flag
	bool window_just_activated_by_user = (window->LastFrameActive < current_frame - 1);   // Not using !WasActive because the implicit "Debug" window would always toggle off->on
	const bool window_just_appearing_after_hidden_for_resize = (window->HiddenFramesForResize > 0);
	if (flags & ImGuiWindowFlags_Popup)
	{
		ImGuiPopupRef& popup_ref = g.OpenPopupStack[g.BeginPopupStack.Size];
		window_just_activated_by_user |= (window->PopupId != popup_ref.PopupId); // We recycle popups so treat window as activated if popup id changed
		window_just_activated_by_user |= (window != popup_ref.Window);
	}
	window->Appearing = (window_just_activated_by_user || window_just_appearing_after_hidden_for_resize);
	if (window->Appearing)
		SetWindowConditionAllowFlags(window, ImGuiCond_Appearing, true);

	// Add to stack
	g.CurrentWindowStack.push_back(window);
	SetCurrentWindow(window);
	CheckStacksSize(window, true);
	if (flags & ImGuiWindowFlags_Popup)
	{
		ImGuiPopupRef& popup_ref = g.OpenPopupStack[g.BeginPopupStack.Size];
		popup_ref.Window = window;
		g.BeginPopupStack.push_back(popup_ref);
		window->PopupId = popup_ref.PopupId;
	}

	if (window_just_appearing_after_hidden_for_resize && !(flags & ImGuiWindowFlags_ChildWindow))
		window->NavLastIds[0] = 0;

	// Process SetNextWindow***() calls
	bool window_pos_set_by_api = false;
	bool window_size_x_set_by_api = false, window_size_y_set_by_api = false;
	if (g.NextWindowData.PosCond)
	{
		window_pos_set_by_api = (window->SetWindowPosAllowFlags & g.NextWindowData.PosCond) != 0;
		if (window_pos_set_by_api && ImLengthSqr(g.NextWindowData.PosPivotVal) > 0.00001f)
		{
			// May be processed on the next frame if this is our first frame and we are measuring size
			// FIXME: Look into removing the branch so everything can go through this same code path for consistency.
			window->SetWindowPosVal = g.NextWindowData.PosVal;
			window->SetWindowPosPivot = g.NextWindowData.PosPivotVal;
			window->SetWindowPosAllowFlags &= ~(ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing);
		}
		else
		{
			SetWindowPos(window, g.NextWindowData.PosVal, g.NextWindowData.PosCond);
		}
	}
	if (g.NextWindowData.SizeCond)
	{
		window_size_x_set_by_api = (window->SetWindowSizeAllowFlags & g.NextWindowData.SizeCond) != 0 && (g.NextWindowData.SizeVal.x > 0.0f);
		window_size_y_set_by_api = (window->SetWindowSizeAllowFlags & g.NextWindowData.SizeCond) != 0 && (g.NextWindowData.SizeVal.y > 0.0f);
		SetWindowSize(window, g.NextWindowData.SizeVal, g.NextWindowData.SizeCond);
	}
	if (g.NextWindowData.ContentSizeCond)
	{
		// Adjust passed "client size" to become a "window size"
		window->SizeContentsExplicit = g.NextWindowData.ContentSizeVal;
		if (window->SizeContentsExplicit.y != 0.0f)
			window->SizeContentsExplicit.y += window->TitleBarHeight() + window->MenuBarHeight();
	}
	else if (first_begin_of_the_frame)
	{
		window->SizeContentsExplicit = ImVec2(0.0f, 0.0f);
	}
	if (g.NextWindowData.CollapsedCond)
		SetWindowCollapsed(window, g.NextWindowData.CollapsedVal, g.NextWindowData.CollapsedCond);
	if (g.NextWindowData.FocusCond)
		FocusWindow(window);
	if (window->Appearing)
		SetWindowConditionAllowFlags(window, ImGuiCond_Appearing, false);

	// When reusing window again multiple times a frame, just append content (don't need to setup again)
	if (first_begin_of_the_frame)
	{
		// Initialize
		const bool window_is_child_tooltip = (flags & ImGuiWindowFlags_ChildWindow) && (flags & ImGuiWindowFlags_Tooltip); // FIXME-WIP: Undocumented behavior of Child+Tooltip for pinned tooltip (#1345)
		UpdateWindowParentAndRootLinks(window, flags, parent_window);

		window->Active = true;
		window->BeginOrderWithinParent = 0;
		window->BeginOrderWithinContext = (short)(g.WindowsActiveCount++);
		window->BeginCount = 0;
		window->ClipRect = ImVec4(-FLT_MAX, -FLT_MAX, +FLT_MAX, +FLT_MAX);
		window->LastFrameActive = current_frame;
		window->IDStack.resize(1);

		// Update stored window name when it changes (which can _only_ happen with the "###" operator, so the ID would stay unchanged).
		// The title bar always display the 'name' parameter, so we only update the string storage if it needs to be visible to the end-user elsewhere.
		bool window_title_visible_elsewhere = false;
		if (g.NavWindowingList != NULL && (window->Flags & ImGuiWindowFlags_NoNavFocus) == 0)   // Window titles visible when using CTRL+TAB
			window_title_visible_elsewhere = true;
		if (window_title_visible_elsewhere && !window_just_created && strcmp(name, window->Name) != 0)
		{
			size_t buf_len = (size_t)window->NameBufLen;
			window->Name = ImStrdupcpy(window->Name, &buf_len, name);
			window->NameBufLen = (int)buf_len;
		}

		// UPDATE CONTENTS SIZE, UPDATE HIDDEN STATUS

		// Update contents size from last frame for auto-fitting (or use explicit size)
		window->SizeContents = CalcSizeContents(window);
		if (window->HiddenFramesRegular > 0)
			window->HiddenFramesRegular--;
		if (window->HiddenFramesForResize > 0)
			window->HiddenFramesForResize--;

		// Hide new windows for one frame until they calculate their size
		if (window_just_created && (!window_size_x_set_by_api || !window_size_y_set_by_api))
			window->HiddenFramesForResize = 1;

		// Hide popup/tooltip window when re-opening while we measure size (because we recycle the windows)
		// We reset Size/SizeContents for reappearing popups/tooltips early in this function, so further code won't be tempted to use the old size.
		if (window_just_activated_by_user && (flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) != 0)
		{
			window->HiddenFramesForResize = 1;
			if (flags & ImGuiWindowFlags_AlwaysAutoResize)
			{
				if (!window_size_x_set_by_api)
					window->Size.x = window->SizeFull.x = 0.f;
				if (!window_size_y_set_by_api)
					window->Size.y = window->SizeFull.y = 0.f;
				window->SizeContents = ImVec2(0.f, 0.f);
			}
		}

		SetCurrentWindow(window);

		// Lock border size and padding for the frame (so that altering them doesn't cause inconsistencies)
		window->WindowBorderSize = (flags & ImGuiWindowFlags_ChildWindow) ? style.ChildBorderSize : ((flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) && !(flags & ImGuiWindowFlags_Modal)) ? style.PopupBorderSize : style.WindowBorderSize;
		window->WindowPadding = style.WindowPadding;
		if ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & (ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_Popup)) && window->WindowBorderSize == 0.0f)
			window->WindowPadding = ImVec2(0.0f, (flags & ImGuiWindowFlags_MenuBar) ? style.WindowPadding.y : 0.0f);
		window->DC.MenuBarOffset.x = ImMax(ImMax(window->WindowPadding.x, style.ItemSpacing.x), g.NextWindowData.MenuBarOffsetMinVal.x);
		window->DC.MenuBarOffset.y = g.NextWindowData.MenuBarOffsetMinVal.y;

		// Collapse window by double-clicking on title bar
		// At this point we don't have a clipping rectangle setup yet, so we can use the title bar area for hit detection and drawing
		if (!(flags & ImGuiWindowFlags_NoTitleBar) && !(flags & ImGuiWindowFlags_NoCollapse))
		{
			// We don't use a regular button+id to test for double-click on title bar (mostly due to legacy reason, could be fixed), so verify that we don't have items over the title bar.
			ImRect title_bar_rect = window->TitleBarRect();
			if (g.HoveredWindow == window && g.HoveredId == 0 && g.HoveredIdPreviousFrame == 0 && IsMouseHoveringRect(title_bar_rect.Min, title_bar_rect.Max) && g.IO.MouseDoubleClicked[0])
				window->WantCollapseToggle = true;
			if (window->WantCollapseToggle)
			{
				window->Collapsed = !window->Collapsed;
				MarkIniSettingsDirty(window);
				FocusWindow(window);
			}
		}
		else
		{
			window->Collapsed = false;
		}
		window->WantCollapseToggle = false;

		// SIZE

		// Calculate auto-fit size, handle automatic resize
		const ImVec2 size_auto_fit = CalcSizeAutoFit(window, window->SizeContents);
		ImVec2 size_full_modified(FLT_MAX, FLT_MAX);
		if ((flags & ImGuiWindowFlags_AlwaysAutoResize) && !window->Collapsed)
		{
			// Using SetNextWindowSize() overrides ImGuiWindowFlags_AlwaysAutoResize, so it can be used on tooltips/popups, etc.
			if (!window_size_x_set_by_api)
				window->SizeFull.x = size_full_modified.x = size_auto_fit.x;
			if (!window_size_y_set_by_api)
				window->SizeFull.y = size_full_modified.y = size_auto_fit.y;
		}
		else if (window->AutoFitFramesX > 0 || window->AutoFitFramesY > 0)
		{
			// Auto-fit may only grow window during the first few frames
			// We still process initial auto-fit on collapsed windows to get a window width, but otherwise don't honor ImGuiWindowFlags_AlwaysAutoResize when collapsed.
			if (!window_size_x_set_by_api && window->AutoFitFramesX > 0)
				window->SizeFull.x = size_full_modified.x = window->AutoFitOnlyGrows ? ImMax(window->SizeFull.x, size_auto_fit.x) : size_auto_fit.x;
			if (!window_size_y_set_by_api && window->AutoFitFramesY > 0)
				window->SizeFull.y = size_full_modified.y = window->AutoFitOnlyGrows ? ImMax(window->SizeFull.y, size_auto_fit.y) : size_auto_fit.y;
			if (!window->Collapsed)
				MarkIniSettingsDirty(window);
		}

		// Apply minimum/maximum window size constraints and final size
		window->SizeFull = CalcSizeAfterConstraint(window, window->SizeFull);
		window->Size = window->Collapsed && !(flags & ImGuiWindowFlags_ChildWindow) ? window->TitleBarRect().GetSize() : window->SizeFull;

		if (flags & ImGuiWindowFlags_ChildFrame) {
			window->SizeFull.x = parent_window->Size.x - 64;
			window->Size.x = parent_window->Size.x - 64;
		}

		// SCROLLBAR STATUS

		// Update scrollbar status (based on the Size that was effective during last frame or the auto-resized Size).
		if (!window->Collapsed)
		{
			// When reading the current size we need to read it after size constraints have been applied
			float size_x_for_scrollbars = size_full_modified.x != FLT_MAX ? window->SizeFull.x : window->SizeFullAtLastBegin.x;
			float size_y_for_scrollbars = size_full_modified.y != FLT_MAX ? window->SizeFull.y : window->SizeFullAtLastBegin.y;
			window->ScrollbarY = (flags & ImGuiWindowFlags_AlwaysVerticalScrollbar) || ((window->SizeContents.y > size_y_for_scrollbars) && !(flags & ImGuiWindowFlags_NoScrollbar));
			window->ScrollbarX = (flags & ImGuiWindowFlags_AlwaysHorizontalScrollbar) || ((window->SizeContents.x > size_x_for_scrollbars - (window->ScrollbarY ? style.ScrollbarSize : 0.0f)) && !(flags & ImGuiWindowFlags_NoScrollbar) && (flags & ImGuiWindowFlags_HorizontalScrollbar));
			if (window->ScrollbarX && !window->ScrollbarY)
				window->ScrollbarY = (window->SizeContents.y > size_y_for_scrollbars - style.ScrollbarSize) && !(flags & ImGuiWindowFlags_NoScrollbar);
			window->ScrollbarSizes = ImVec2(window->ScrollbarY ? style.ScrollbarSize : 0.0f, window->ScrollbarX ? style.ScrollbarSize : 0.0f);
		}

		// POSITION

		// Popup latch its initial position, will position itself when it appears next frame
		if (window_just_activated_by_user)
		{
			window->AutoPosLastDirection = ImGuiDir_None;
			if ((flags & ImGuiWindowFlags_Popup) != 0 && !window_pos_set_by_api)
				window->Pos = g.BeginPopupStack.back().OpenPopupPos;
		}

		// Position child window
		if (flags & ImGuiWindowFlags_ChildWindow)
		{
			IM_ASSERT(parent_window && parent_window->Active);
			window->BeginOrderWithinParent = (short)parent_window->DC.ChildWindows.Size;
			parent_window->DC.ChildWindows.push_back(window);
			if (!(flags & ImGuiWindowFlags_Popup) && !window_pos_set_by_api && !window_is_child_tooltip)
				window->Pos = parent_window->DC.CursorPos + ImVec2(flags & ImGuiWindowFlags_ChildFrame ? 16 : 0, 0);
		}

		const bool window_pos_with_pivot = (window->SetWindowPosVal.x != FLT_MAX && window->HiddenFramesForResize == 0);
		if (window_pos_with_pivot)
			SetWindowPos(window, ImMax(style.DisplaySafeAreaPadding, window->SetWindowPosVal - window->SizeFull * window->SetWindowPosPivot), 0); // Position given a pivot (e.g. for centering)
		else if ((flags & ImGuiWindowFlags_ChildMenu) != 0)
			window->Pos = FindBestWindowPosForPopup(window);
		else if ((flags & ImGuiWindowFlags_Popup) != 0 && !window_pos_set_by_api && window_just_appearing_after_hidden_for_resize)
			window->Pos = FindBestWindowPosForPopup(window);
		else if ((flags & ImGuiWindowFlags_Tooltip) != 0 && !window_pos_set_by_api && !window_is_child_tooltip)
			window->Pos = FindBestWindowPosForPopup(window);

		// Clamp position so it stays visible
		// Ignore zero-sized display explicitly to avoid losing positions if a window manager reports zero-sized window when initializing or minimizing.
		if (!window_pos_set_by_api && !(flags & ImGuiWindowFlags_ChildWindow) && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0)
		{
			if (g.IO.DisplaySize.x > 0.0f && g.IO.DisplaySize.y > 0.0f) // Ignore zero-sized display explicitly to avoid losing positions if a window manager reports zero-sized window when initializing or minimizing.
			{
				ImVec2 padding = ImMax(style.DisplayWindowPadding, style.DisplaySafeAreaPadding);
				ImVec2 size_for_clamping = window->Size;
				window->Pos = ImMax(window->Pos + size_for_clamping, padding) - size_for_clamping;
				window->Pos = ImMin(window->Pos, g.IO.DisplaySize - padding);
			}
		}
		window->Pos = ImFloor(window->Pos);

		// Lock window rounding for the frame (so that altering them doesn't cause inconsistencies)
		window->WindowRounding = (flags & ImGuiWindowFlags_ChildWindow) ? style.ChildRounding : ((flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiWindowFlags_Modal)) ? style.PopupRounding : style.WindowRounding;

		// Prepare for item focus requests
		window->FocusIdxAllRequestCurrent = (window->FocusIdxAllRequestNext == INT_MAX || window->FocusIdxAllCounter == -1) ? INT_MAX : (window->FocusIdxAllRequestNext + (window->FocusIdxAllCounter + 1)) % (window->FocusIdxAllCounter + 1);
		window->FocusIdxTabRequestCurrent = (window->FocusIdxTabRequestNext == INT_MAX || window->FocusIdxTabCounter == -1) ? INT_MAX : (window->FocusIdxTabRequestNext + (window->FocusIdxTabCounter + 1)) % (window->FocusIdxTabCounter + 1);
		window->FocusIdxAllCounter = window->FocusIdxTabCounter = -1;
		window->FocusIdxAllRequestNext = window->FocusIdxTabRequestNext = INT_MAX;

		// Apply scrolling
		window->Scroll = CalcNextScrollFromScrollTargetAndClamp(window, true);
		window->ScrollTarget = ImVec2(FLT_MAX, FLT_MAX);

		// Apply window focus (new and reactivated windows are moved to front)
		bool want_focus = false;
		if (window_just_activated_by_user && !(flags & ImGuiWindowFlags_NoFocusOnAppearing))
		{
			if (flags & ImGuiWindowFlags_Popup)
				want_focus = true;
			else if ((flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Tooltip)) == 0)
				want_focus = true;
		}

		// Handle manual resize: Resize Grips, Borders, Gamepad
		int border_held = -1;
		ImU32 resize_grip_col[4] = { 0 };
		const int resize_grip_count = g.IO.ConfigWindowsResizeFromEdges ? 2 : 1; // 4
		const float grip_draw_size = (float)(int)ImMax(g.FontSize * 1.35f, window->WindowRounding + 1.0f + g.FontSize * 0.2f);
		if (!window->Collapsed)
			UpdateManualResize(window, size_auto_fit, &border_held, resize_grip_count, &resize_grip_col[0]);
		window->ResizeBorderHeld = (signed char)border_held;

		// Default item width. Make it proportional to window size if window manually resizes
		if (window->Size.x > 0.0f && !(flags & ImGuiWindowFlags_Tooltip) && !(flags & ImGuiWindowFlags_AlwaysAutoResize))
			window->ItemWidthDefault = (float)(int)(window->Size.x * 0.65f);
		else
			window->ItemWidthDefault = (float)(int)(g.FontSize * 16.0f);

		// DRAWING

		// Setup draw list and outer clipping rectangle
		window->DrawList->Clear();
		window->DrawList->Flags = (g.Style.AntiAliasedLines ? ImDrawListFlags_AntiAliasedLines : 0) | (g.Style.AntiAliasedFill ? ImDrawListFlags_AntiAliasedFill : 0);
		window->DrawList->PushTextureID(g.Font->ContainerAtlas->TexID);
		ImRect viewport_rect(GetViewportRect());

		if ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Popup) && !window_is_child_tooltip)
			PushClipRect(parent_window->ClipRect.Min, parent_window->ClipRect.Max, true);
		else
			PushClipRect(viewport_rect.Min, viewport_rect.Max, true);

		// Draw modal window background (darkens what is behind them, all viewports)
		const bool dim_bg_for_modal = (flags & ImGuiWindowFlags_Modal) && window == GetFrontMostPopupModal() && window->HiddenFramesForResize <= 0;
		const bool dim_bg_for_window_list = g.NavWindowingTargetAnim && (window == g.NavWindowingTargetAnim->RootWindow);
		if (dim_bg_for_modal || dim_bg_for_window_list)
		{
			const ImU32 dim_bg_col = GetColorU32(dim_bg_for_modal ? ImGuiCol_ModalWindowDimBg : ImGuiCol_NavWindowingDimBg, g.DimBgRatio);
			window->DrawList->AddRectFilled(viewport_rect.Min, viewport_rect.Max, dim_bg_col);
		}

		// Draw navigation selection/windowing rectangle background
		if (dim_bg_for_window_list && window == g.NavWindowingTargetAnim)
		{
			ImRect bb = window->Rect();
			bb.Expand(g.FontSize);
			if (!bb.Contains(viewport_rect)) // Avoid drawing if the window covers all the viewport anyway
				window->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(ImGuiCol_NavWindowingHighlight, g.NavWindowingHighlightAlpha * 0.25f), g.Style.WindowRounding);
		}

		// Draw window + handle manual resize
		// As we highlight the title bar when want_focus is set, multiple reappearing windows will have have their title bar highlighted on their reappearing frame.
		const float window_rounding = window->WindowRounding;
		const float window_border_size = window->WindowBorderSize;
		const ImGuiWindow* window_to_highlight = g.NavWindowingTarget ? g.NavWindowingTarget : g.NavWindow;
		const bool title_bar_is_highlight = want_focus || (window_to_highlight && window->RootWindowForTitleBarHighlight == window_to_highlight->RootWindowForTitleBarHighlight);
		const ImRect title_bar_rect = window->TitleBarRect();
		if (window->Collapsed)
		{
			// Title bar only
			float backup_border_size = style.FrameBorderSize;
			g.Style.FrameBorderSize = window->WindowBorderSize;
			ImU32 title_bar_col = GetColorU32((title_bar_is_highlight && !g.NavDisableHighlight) ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBgCollapsed);
			RenderFrame(title_bar_rect.Min, title_bar_rect.Max, title_bar_col, true, window_rounding);
			g.Style.FrameBorderSize = backup_border_size;
		}
		else
		{
			// Window background
			if (!(flags & ImGuiWindowFlags_NoBackground))
			{
				if (!(flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiWindowFlags_Tooltip)) {
					ImU32 bg_col = ImColor(19 / 255.f, 19 / 255.f, 19 / 255.f, g.Style.Alpha);
					float alpha = 1.0f;
					if (g.NextWindowData.BgAlphaCond != 0)
						alpha = g.NextWindowData.BgAlphaVal;
					if (alpha != 1.0f)
						bg_col = (bg_col & ~IM_COL32_A_MASK) | (IM_F32_TO_INT8_SAT(alpha) << IM_COL32_A_SHIFT);
					window->DrawList->AddRectFilled(window->Pos + ImVec2(0, window->TitleBarHeight()), window->Pos + window->Size, bg_col, window_rounding, (flags & ImGuiWindowFlags_NoTitleBar) ? ImDrawCornerFlags_All : ImDrawCornerFlags_Bot);

					window->DrawList->PushClipRect(window->Pos, window->Pos + window->Size, true);

					window->DrawList->PopClipRect();

					static auto outline = [](ImGuiWindow* wnd, int i, ImColor col) -> void {
						wnd->DrawList->AddRect(wnd->Pos + ImVec2(i, i), wnd->Pos + wnd->Size - ImVec2(i, i), col);
					};

					outline(window, 0, ImColor(31 / 255.f, 31 / 255.f, 31 / 255.f, g.Style.Alpha));
					outline(window, 1, ImColor(60 / 255.f, 60 / 255.f, 60 / 255.f, g.Style.Alpha));
					outline(window, 2, ImColor(40 / 255.f, 40 / 255.f, 40 / 255.f, g.Style.Alpha));
					outline(window, 3, ImColor(40 / 255.f, 40 / 255.f, 40 / 255.f, g.Style.Alpha));
					outline(window, 4, ImColor(60 / 255.f, 60 / 255.f, 60 / 255.f, g.Style.Alpha));
					outline(window, 5, ImColor(31 / 255.f, 31 / 255.f, 31 / 255.f, g.Style.Alpha));



					static std::clock_t start = std::clock();
					static int hue1 = 0;
					static int hue2 = 90;
					static int hue3 = 180;
					static int hue4 = 270;
					if (std::clock() - start > std::chrono::milliseconds(20).count())
					{
						hue1 >= 360 ? hue1 = 0 : hue1++;
						hue2 >= 360 ? hue2 = 0 : hue2++;
						hue3 >= 360 ? hue3 = 0 : hue3++;
						hue4 >= 360 ? hue4 = 0 : hue4++;
						start = std::clock();
					}

					ImColor rainbow1 = ImColor::HSV(hue1 / 360.f, 0.95, 1);
					ImColor rainbow2 = ImColor::HSV(hue2 / 360.f, 0.95, 1);
					ImColor rainbow3 = ImColor::HSV(hue3 / 360.f, 0.95, 1);
					ImColor rainbow4 = ImColor::HSV(hue4 / 360.f, 0.95, 1);

					window->DrawList->AddRectFilledMultiColor(
						window->Pos + ImVec2(6, 6),
						window->Pos + ImVec2(window->Size.x - 6, 8),
						rainbow1, rainbow4,
						rainbow4, rainbow1);

				}
				else {
					if (flags & ImGuiWindowFlags_ChildWindow && !(flags & ImGuiWindowFlags_ChildFrame) && !(flags & ImGuiWindowFlags_Tooltip)) {
						ImVec2 textSize = CalcTextSize(name);

						ImU32 bg_col = ImColor(18 / 255.f, 18 / 255.f, 18 / 255.f, g.Style.Alpha);
						float alpha = 1.0f;
						if (g.NextWindowData.BgAlphaCond != 0)
							alpha = g.NextWindowData.BgAlphaVal;
						if (alpha != 1.0f)
							bg_col = (bg_col & ~IM_COL32_A_MASK) | (IM_F32_TO_INT8_SAT(alpha) << IM_COL32_A_SHIFT);
						window->DrawList->AddRectFilled(window->Pos + ImVec2(0, window->TitleBarHeight()), window->Pos + window->Size, bg_col, window_rounding, (flags & ImGuiWindowFlags_NoTitleBar) ? ImDrawCornerFlags_All : ImDrawCornerFlags_Bot);

						window->DrawList->AddLine(window->Pos, window->Pos + ImVec2(0, window->Size.y), ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));
						window->DrawList->AddLine(window->Pos + ImVec2(0, window->Size.y), window->Pos + window->Size + ImVec2(1, 0), ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));
						window->DrawList->AddLine(window->Pos + window->Size, window->Pos + ImVec2(window->Size.x, 0), ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));
						window->DrawList->AddLine(window->Pos, window->Pos + ImVec2(10, 0), ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));
						window->DrawList->AddLine(window->Pos + ImVec2(16 + textSize.x, 0), window->Pos + ImVec2(window->Size.x, 0), ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));

						window->DrawList->AddLine(window->Pos + ImVec2(1, 1), window->Pos + ImVec2(1, window->Size.y - 1), ImColor(45 / 255.f, 45 / 255.f, 45 / 255.f, g.Style.Alpha));
						window->DrawList->AddLine(window->Pos + ImVec2(1, window->Size.y - 1), window->Pos + window->Size - ImVec2(0, 1), ImColor(45 / 255.f, 45 / 255.f, 45 / 255.f, g.Style.Alpha));
						window->DrawList->AddLine(window->Pos + window->Size - ImVec2(1, 1), window->Pos + ImVec2(window->Size.x - 1, 1), ImColor(45 / 255.f, 45 / 255.f, 45 / 255.f, g.Style.Alpha));
						window->DrawList->AddLine(window->Pos + ImVec2(1, 1), window->Pos + ImVec2(10, 1), ImColor(45 / 255.f, 45 / 255.f, 45 / 255.f, g.Style.Alpha));
						window->DrawList->AddLine(window->Pos + ImVec2(16 + textSize.x, 1), window->Pos + ImVec2(window->Size.x - 1, 1), ImColor(45 / 255.f, 45 / 255.f, 45 / 255.f, g.Style.Alpha));
					}
					else if (flags & ImGuiWindowFlags_Popup || flags & ImGuiWindowFlags_Tooltip || flags & ImGuiWindowFlags_ChildFrame) {
						ImU32 bg_col = ImColor(36 / 255.f, 36 / 255.f, 36 / 255.f, g.Style.Alpha);
						float alpha = 1.0f;
						if (g.NextWindowData.BgAlphaCond != 0)
							alpha = g.NextWindowData.BgAlphaVal;
						if (alpha != 1.0f)
							bg_col = (bg_col & ~IM_COL32_A_MASK) | (IM_F32_TO_INT8_SAT(alpha) << IM_COL32_A_SHIFT);
						window->DrawList->AddRectFilled(window->Pos + ImVec2(0, window->TitleBarHeight()), window->Pos + window->Size, bg_col, window_rounding, (flags & ImGuiWindowFlags_NoTitleBar) ? ImDrawCornerFlags_All : ImDrawCornerFlags_Bot);

						window->DrawList->AddRect(window->Pos, window->Pos + window->Size, ImColor(12 / 255.f, 12 / 255.f, 12 / 255.f, g.Style.Alpha));
					}
				}
			}
			g.NextWindowData.BgAlphaCond = 0;

			// Title bar
			if (!(flags & ImGuiWindowFlags_NoTitleBar))
			{
				ImU32 title_bar_col = GetColorU32(title_bar_is_highlight ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBg);
				window->DrawList->AddRectFilled(title_bar_rect.Min, title_bar_rect.Max, title_bar_col, window_rounding, ImDrawCornerFlags_Top);
			}

			// Menu bar
			if (flags & ImGuiWindowFlags_MenuBar)
			{
				ImRect menu_bar_rect = window->MenuBarRect();
				menu_bar_rect.ClipWith(window->Rect());  // Soft clipping, in particular child window don't have minimum size covering the menu bar so this is useful for them.
				window->DrawList->AddRectFilled(menu_bar_rect.Min, menu_bar_rect.Max, GetColorU32(ImGuiCol_MenuBarBg), (flags & ImGuiWindowFlags_NoTitleBar) ? window_rounding : 0.0f, ImDrawCornerFlags_Top);
				if (style.FrameBorderSize > 0.0f && menu_bar_rect.Max.y < window->Pos.y + window->Size.y)
					window->DrawList->AddLine(menu_bar_rect.GetBL(), menu_bar_rect.GetBR(), GetColorU32(ImGuiCol_Border), style.FrameBorderSize);
			}

			// Scrollbars
			if (window->ScrollbarX)
				Scrollbar(ImGuiLayoutType_Horizontal);
			if (window->ScrollbarY)
				Scrollbar(ImGuiLayoutType_Vertical);
		}

		// Store a backup of SizeFull which we will use next frame to decide if we need scrollbars.
		window->SizeFullAtLastBegin = window->SizeFull;

		// Update various regions. Variables they depends on are set above in this function.
		// FIXME: window->ContentsRegionRect.Max is currently very misleading / partly faulty, but some BeginChild() patterns relies on it.
		window->ContentsRegionRect.Min.x = window->Pos.x - window->Scroll.x + window->WindowPadding.x;
		window->ContentsRegionRect.Min.y = window->Pos.y - window->Scroll.y + window->WindowPadding.y + window->TitleBarHeight() + window->MenuBarHeight();
		window->ContentsRegionRect.Max.x = window->Pos.x - window->Scroll.x - window->WindowPadding.x + (window->SizeContentsExplicit.x != 0.0f ? window->SizeContentsExplicit.x : (window->Size.x - window->ScrollbarSizes.x));
		window->ContentsRegionRect.Max.y = window->Pos.y - window->Scroll.y - window->WindowPadding.y + (window->SizeContentsExplicit.y != 0.0f ? window->SizeContentsExplicit.y : (window->Size.y - window->ScrollbarSizes.y));

		// Setup drawing context
		// (NB: That term "drawing context / DC" lost its meaning a long time ago. Initially was meant to hold transient data only. Nowadays difference between window-> and window->DC-> is dubious.)
		window->DC.Indent.x = 0.0f + window->WindowPadding.x - window->Scroll.x;
		window->DC.GroupOffset.x = 0.0f;
		window->DC.ColumnsOffset.x = 0.0f;
		window->DC.CursorStartPos = window->Pos + ImVec2(window->DC.Indent.x + window->DC.ColumnsOffset.x, window->TitleBarHeight() + window->MenuBarHeight() + window->WindowPadding.y - window->Scroll.y);
		window->DC.CursorPos = window->DC.CursorStartPos;
		window->DC.CursorPosPrevLine = window->DC.CursorPos;
		window->DC.CursorMaxPos = window->DC.CursorStartPos;
		window->DC.CurrentLineSize = window->DC.PrevLineSize = ImVec2(0.0f, 0.0f);
		window->DC.CurrentLineTextBaseOffset = window->DC.PrevLineTextBaseOffset = 0.0f;
		window->DC.NavHideHighlightOneFrame = false;
		window->DC.NavHasScroll = (GetWindowScrollMaxY(window) > 0.0f);
		window->DC.NavLayerActiveMask = window->DC.NavLayerActiveMaskNext;
		window->DC.NavLayerActiveMaskNext = 0x00;
		window->DC.MenuBarAppending = false;
		window->DC.LogLinePosY = window->DC.CursorPos.y - 9999.0f;
		window->DC.ChildWindows.resize(0);
		window->DC.LayoutType = ImGuiLayoutType_Vertical;
		window->DC.ParentLayoutType = parent_window ? parent_window->DC.LayoutType : ImGuiLayoutType_Vertical;
		window->DC.ItemFlags = parent_window ? parent_window->DC.ItemFlags : ImGuiItemFlags_Default_;
		window->DC.ItemWidth = window->ItemWidthDefault;
		window->DC.TextWrapPos = -1.0f; // disabled
		window->DC.ItemFlagsStack.resize(0);
		window->DC.ItemWidthStack.resize(0);
		window->DC.TextWrapPosStack.resize(0);
		window->DC.ColumnsSet = NULL;
		window->DC.TreeDepth = 0;
		window->DC.TreeDepthMayJumpToParentOnPop = 0x00;
		window->DC.StateStorage = &window->StateStorage;
		window->DC.GroupStack.resize(0);
		window->MenuColumns.Update(3, style.ItemSpacing.x, window_just_activated_by_user);

		if (flags & ImGuiWindowFlags_Popup)
			window->DC.CursorPos.y += 4;

		if ((flags & ImGuiWindowFlags_ChildWindow) && (window->DC.ItemFlags != parent_window->DC.ItemFlags))
		{
			window->DC.ItemFlags = parent_window->DC.ItemFlags;
			window->DC.ItemFlagsStack.push_back(window->DC.ItemFlags);
		}

		if (window->AutoFitFramesX > 0)
			window->AutoFitFramesX--;
		if (window->AutoFitFramesY > 0)
			window->AutoFitFramesY--;

		// Apply focus (we need to call FocusWindow() AFTER setting DC.CursorStartPos so our initial navigation reference rectangle can start around there)
		if (want_focus)
		{
			FocusWindow(window);
			NavInitWindow(window, false);
		}

		// Title bar
		if (!(flags & ImGuiWindowFlags_NoTitleBar))
		{
			// Close & collapse button are on layer 1 (same as menus) and don't default focus
			const ImGuiItemFlags item_flags_backup = window->DC.ItemFlags;
			window->DC.ItemFlags |= ImGuiItemFlags_NoNavDefaultFocus;
			window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
			window->DC.NavLayerCurrentMask = (1 << ImGuiNavLayer_Menu);

			// Collapse button
			if (!(flags & ImGuiWindowFlags_NoCollapse))
				if (CollapseButton(window->GetID("#COLLAPSE"), window->Pos))
					window->WantCollapseToggle = true; // Defer collapsing to next frame as we are too far in the Begin() function

			// Close button
			if (p_open != NULL)
			{
				const float pad = style.FramePadding.y;
				const float rad = g.FontSize * 0.5f;
				if (CloseButton(window->GetID("#CLOSE"), window->Rect().GetTR() + ImVec2(-pad - rad, pad + rad), rad + 1))
					*p_open = false;
			}

			window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
			window->DC.NavLayerCurrentMask = (1 << ImGuiNavLayer_Main);
			window->DC.ItemFlags = item_flags_backup;

			// Title bar text (with: horizontal alignment, avoiding collapse/close button, optional "unsaved document" marker)
			// FIXME: Refactor text alignment facilities along with RenderText helpers, this is too much code..
			const char* UNSAVED_DOCUMENT_MARKER = "*";
			float marker_size_x = (flags & ImGuiWindowFlags_UnsavedDocument) ? CalcTextSize(UNSAVED_DOCUMENT_MARKER, NULL, false).x : 0.0f;
			ImVec2 text_size = CalcTextSize(name, NULL, true) + ImVec2(marker_size_x, 0.0f);
			ImRect text_r = title_bar_rect;
			float pad_left = (flags & ImGuiWindowFlags_NoCollapse) ? style.FramePadding.x : (style.FramePadding.x + g.FontSize + style.ItemInnerSpacing.x);
			float pad_right = (p_open == NULL) ? style.FramePadding.x : (style.FramePadding.x + g.FontSize + style.ItemInnerSpacing.x);
			if (style.WindowTitleAlign.x > 0.0f)
				pad_right = ImLerp(pad_right, pad_left, style.WindowTitleAlign.x);
			text_r.Min.x += pad_left;
			text_r.Max.x -= pad_right;
			ImRect clip_rect = text_r;
			clip_rect.Max.x = window->Pos.x + window->Size.x - (p_open ? title_bar_rect.GetHeight() - 3 : style.FramePadding.x); // Match the size of CloseButton()
			RenderTextClipped(text_r.Min, text_r.Max, name, NULL, &text_size, style.WindowTitleAlign, &clip_rect);
			if (flags & ImGuiWindowFlags_UnsavedDocument)
			{
				ImVec2 marker_pos = ImVec2(ImMax(text_r.Min.x, text_r.Min.x + (text_r.GetWidth() - text_size.x) * style.WindowTitleAlign.x) + text_size.x, text_r.Min.y) + ImVec2(2 - marker_size_x, 0.0f);
				ImVec2 off = ImVec2(0.0f, (float)(int)(-g.FontSize * 0.25f));
				RenderTextClipped(marker_pos + off, text_r.Max + off, UNSAVED_DOCUMENT_MARKER, NULL, NULL, ImVec2(0, style.WindowTitleAlign.y), &clip_rect);
			}
		}

		// Save clipped aabb so we can access it in constant-time in FindHoveredWindow()
		window->OuterRectClipped = window->Rect();
		window->OuterRectClipped.ClipWith(window->ClipRect);

		// Pressing CTRL+C while holding on a window copy its content to the clipboard
		// This works but 1. doesn't handle multiple Begin/End pairs, 2. recursing into another Begin/End pair - so we need to work that out and add better logging scope.
		// Maybe we can support CTRL+C on every element?
		

		// Inner rectangle
		// We set this up after processing the resize grip so that our clip rectangle doesn't lag by a frame
		// Note that if our window is collapsed we will end up with an inverted (~null) clipping rectangle which is the correct behavior.
		window->InnerMainRect.Min.x = title_bar_rect.Min.x + window->WindowBorderSize;
		window->InnerMainRect.Min.y = title_bar_rect.Max.y + window->MenuBarHeight() + (((flags & ImGuiWindowFlags_MenuBar) || !(flags & ImGuiWindowFlags_NoTitleBar)) ? style.FrameBorderSize : window->WindowBorderSize);
		window->InnerMainRect.Max.x = window->Pos.x + window->Size.x - window->ScrollbarSizes.x - window->WindowBorderSize;
		window->InnerMainRect.Max.y = window->Pos.y + window->Size.y - window->ScrollbarSizes.y - window->WindowBorderSize;
		//window->DrawList->AddRect(window->InnerRect.Min, window->InnerRect.Max, IM_COL32_WHITE);

		// Inner clipping rectangle
		// Force round operator last to ensure that e.g. (int)(max.x-min.x) in user's render code produce correct result.
		window->InnerClipRect.Min.x = ImFloor(0.5f + window->InnerMainRect.Min.x + ImMax(0.0f, ImFloor(window->WindowPadding.x * 0.5f - window->WindowBorderSize)));
		window->InnerClipRect.Min.y = ImFloor(0.5f + window->InnerMainRect.Min.y);
		window->InnerClipRect.Max.x = ImFloor(0.5f + window->InnerMainRect.Max.x - ImMax(0.0f, ImFloor(window->WindowPadding.x * 0.5f - window->WindowBorderSize)));
		window->InnerClipRect.Max.y = ImFloor(0.5f + window->InnerMainRect.Max.y);

		if (flags & ImGuiWindowFlags_ChildWindow) {
			window->InnerClipRect.Min.y += 2;
			window->InnerClipRect.Max.y -= 2;
		}

		

		// We fill last item data based on Title Bar, in order for IsItemHovered() and IsItemActive() to be usable after Begin().
		// This is useful to allow creating context menus on title bar only, etc.
		window->DC.LastItemId = window->MoveId;
		window->DC.LastItemStatusFlags = IsMouseHoveringRect(title_bar_rect.Min, title_bar_rect.Max, false) ? ImGuiItemStatusFlags_HoveredRect : 0;
		window->DC.LastItemRect = title_bar_rect;
	}

	PushClipRect(window->InnerClipRect.Min, window->InnerClipRect.Max, true);

	// Clear 'accessed' flag last thing (After PushClipRect which will set the flag. We want the flag to stay false when the default "Debug" window is unused)
	if (first_begin_of_the_frame)
		window->WriteAccessed = false;

	window->BeginCount++;
	g.NextWindowData.Clear();

	if (flags & ImGuiWindowFlags_ChildWindow)
	{
		// Child window can be out of sight and have "negative" clip windows.
		// Mark them as collapsed so commands are skipped earlier (we can't manually collapse them because they have no title bar).
		IM_ASSERT((flags & ImGuiWindowFlags_NoTitleBar) != 0);
		if (!(flags & ImGuiWindowFlags_AlwaysAutoResize) && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0)
			if (window->OuterRectClipped.Min.x >= window->OuterRectClipped.Max.x || window->OuterRectClipped.Min.y >= window->OuterRectClipped.Max.y)
				window->HiddenFramesRegular = 1;

		// Completely hide along with parent or if parent is collapsed
		if (parent_window && (parent_window->Collapsed || parent_window->Hidden))
			window->HiddenFramesRegular = 1;
	}

	// Don't render if style alpha is 0.0 at the time of Begin(). This is arbitrary and inconsistent but has been there for a long while (may remove at some point)
	if (style.Alpha <= 0.0f)
		window->HiddenFramesRegular = 1;

	// Update the Hidden flag
	window->Hidden = (window->HiddenFramesRegular > 0) || (window->HiddenFramesForResize > 0);

	// Return false if we don't intend to display anything to allow user to perform an early out optimization
	window->SkipItems = (window->Collapsed || !window->Active || window->Hidden) && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0 && window->HiddenFramesForResize <= 0;

	return !window->SkipItems;
	*/
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ui::KeybindSkeet(const char* str_id, int* current_key, int* key_style) {
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	SameLine(window->Size.x - 28);

	ImGuiContext& g = *GImGui;

	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(str_id);
	ImGuiIO* io = &GetIO();

	const ImVec2 label_size = CalcTextSize(keys[*current_key]);
	const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + label_size);
	const ImRect total_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(window->Pos.x + window->Size.x - window->DC.CursorPos.x, label_size.y));
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id, &frame_bb))
		return false;

	const bool hovered = IsItemHovered();
	const bool edit_requested = hovered && io->MouseClicked[0];
	const bool style_requested = hovered && io->MouseClicked[1];

	if (edit_requested) {
		if (g.ActiveId != id) {
			memset(io->MouseDown, 0, sizeof(io->MouseDown));
			memset(io->KeysDown, 0, sizeof(io->KeysDown));
			*current_key = 0;
		}

		SetActiveID(id, window);
		FocusWindow(window);
	}
	else if (!hovered && io->MouseClicked[0] && g.ActiveId == id)
		ClearActiveID();

	bool value_changed = false;
	int key = *current_key;

	if (g.ActiveId == id) {
		for (auto i = 0; i < 5; i++) {
			if (io->MouseDown[i]) {
				switch (i) {
				case 0:
					key = VK_LBUTTON;
					break;
				case 1:
					key = VK_RBUTTON;
					break;
				case 2:
					key = VK_MBUTTON;
					break;
				case 3:
					key = VK_XBUTTON1;
					break;
				case 4:
					key = VK_XBUTTON2;
				}
				value_changed = true;
				ClearActiveID();
			}
		}

		if (!value_changed) {
			for (auto i = VK_BACK; i <= VK_RMENU; i++) {
				if (io->KeysDown[i]) {
					key = i;
					value_changed = true;
					ClearActiveID();
				}
			}
		}

		if (IsKeyPressedMap(ImGuiKey_Escape)) {
			*current_key = 0;
			ClearActiveID();
		}
		else
			*current_key = key;
	}
	else {
		if (key_style) {
			bool popup_open = IsPopupOpen(id);

			if (style_requested && !popup_open)
				OpenPopupEx(id);

			if (popup_open) {
				SetNextWindowSize(ImVec2(100, CalcMaxPopupHeightFromItemCount(4) + 5));

				char name[16];
				ImFormatString(name, IM_ARRAYSIZE(name), "##Combo_%02d", g.BeginPopupStack.Size); // Recycle windows based on depth

				// Peak into expected window size so we can position it
				if (ImGuiWindow* popup_window = FindWindowByName(name))
					if (popup_window->WasActive)
					{
						ImVec2 size_expected = CalcWindowExpectedSize(popup_window);
						ImRect r_outer = GetWindowAllowedExtentRect(popup_window);
						ImVec2 pos = FindBestWindowPosForPopupEx(frame_bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, frame_bb, ImGuiPopupPositionPolicy_ComboBox);
						SetNextWindowPos(pos);
					}

				// Horizontally align ourselves with the framed text
				ImGuiWindowFlags window_flags = ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar;
				PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(style.FramePadding.x, style.WindowPadding.y));
				bool ret = Begin(name, NULL, window_flags);
				PopStyleVar();

				if (Selectable("Always On", *key_style == 0))
					*key_style = 0;

				if (Selectable("On Hotkey", *key_style == 1))
					*key_style = 1;

				if (Selectable("Toggle", *key_style == 2))
					*key_style = 2;

				if (Selectable("Off Hotkey", *key_style == 3))
					*key_style = 3;

				EndPopup();
			}
		}
	}

	char buf_display[64] = "[-]";

	if (*current_key != 0 && g.ActiveId != id)
		strcpy_s(buf_display, keys[*current_key]);
	else if (g.ActiveId == id)
		strcpy_s(buf_display, "[-]");

	PushFont(skeet_menu.keybinds_font);
	window->DrawList->AddText(frame_bb.Min + ImVec2(1, 1), ImColor(0 / 255.f, 0 / 255.f, 0 / 255.f, g.Style.Alpha), buf_display);
	window->DrawList->AddText(frame_bb.Min, g.ActiveId == id ? ImColor(255 / 255.f, 16 / 255.f, 16 / 255.f, g.Style.Alpha) : ImColor(90 / 255.f, 90 / 255.f, 90 / 255.f, g.Style.Alpha), buf_display);
	PopFont();

	return value_changed;
	
	/*ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	SameLine(window->Size.x - 28);

	ImGuiContext& g = *GImGui;

	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(str_id);
	ImGuiIO* io = &GetIO();

	const ImVec2 label_size = CalcTextSize(keys[*current_key]);
	const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + label_size);
	const ImRect total_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(window->Pos.x + window->Size.x - window->DC.CursorPos.x, label_size.y));
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id, &frame_bb))
		return false;

	const bool hovered = IsItemHovered();
	const bool edit_requested = hovered && io->MouseClicked[0];
	const bool style_requested = hovered && io->MouseClicked[1];

	if (edit_requested) {
		if (g.ActiveId != id) {
			memset(io->MouseDown, 0, sizeof(io->MouseDown));
			memset(io->KeysDown, 0, sizeof(io->KeysDown));
			*current_key = 0;
		}

		SetActiveID(id, window);
		FocusWindow(window);
	}
	else if (!hovered && io->MouseClicked[0] && g.ActiveId == id)
		ClearActiveID();

	bool value_changed = false;
	int key = *current_key;

	if (g.ActiveId == id) {
		for (auto i = 0; i < 5; i++) {
			if (io->MouseDown[i]) {
				switch (i) {
				case 0:
					key = VK_LBUTTON;
					break;
				case 1:
					key = VK_RBUTTON;
					break;
				case 2:
					key = VK_MBUTTON;
					break;
				case 3:
					key = VK_XBUTTON1;
					break;
				case 4:
					key = VK_XBUTTON2;
				}
				value_changed = true;
				ClearActiveID();
			}
		}

		if (!value_changed) {
			for (auto i = VK_BACK; i <= VK_RMENU; i++) {
				if (io->KeysDown[i]) {
					key = i;
					value_changed = true;
					ClearActiveID();
				}
			}
		}

		if (IsKeyPressedMap(ImGuiKey_Escape)) {
			*current_key = 0;
			ClearActiveID();
		}
		else
			*current_key = key;
	}
	else {
		if (key_style) {
			bool popup_open = IsPopupOpen(id);

			if (style_requested && !popup_open)
				OpenPopupEx(id);

			if (popup_open) {
				SetNextWindowSize(ImVec2(100, CalcMaxPopupHeightFromItemCount(4)));

				char name[16];
				ImFormatString(name, IM_ARRAYSIZE(name), "##Combo_%02d", g.BeginPopupStack.Size); // Recycle windows based on depth

				// Peak into expected window size so we can position it
				if (ImGuiWindow* popup_window = FindWindowByName(name))
					if (popup_window->WasActive)
					{
						ImVec2 size_expected = CalcWindowExpectedSize(popup_window);
						ImRect r_outer = GetWindowAllowedExtentRect(popup_window);
						ImVec2 pos = FindBestWindowPosForPopupEx(frame_bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, frame_bb, ImGuiPopupPositionPolicy_ComboBox);
						SetNextWindowPos(pos);
					}

				// Horizontally align ourselves with the framed text
				ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
				PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(style.FramePadding.x, style.WindowPadding.y));
				bool ret = BeginSkeet(name, NULL, window_flags);
				PopStyleVar();

				if (SelectableSkeet("Always On", *key_style == 0))
					*key_style = 0;

				if (SelectableSkeet("On Hotkey", *key_style == 1))
					*key_style = 1;

				if (SelectableSkeet("Toggle", *key_style == 2))
					*key_style = 2;

				if (SelectableSkeet("Off Hotkey", *key_style == 3))
					*key_style = 3;

				EndPopupSkeet();
			}
		}
	}

	char buf_display[64] = "[-]";

	if (*current_key != 0 && g.ActiveId != id)
		strcpy_s(buf_display, keys[*current_key]);
	else if (g.ActiveId == id)
		strcpy_s(buf_display, "[-]");

	//PushFont(io->Fonts->Fonts[2]);
	window->DrawList->AddText(frame_bb.Min, g.ActiveId == id ? ImColor(255 / 255.f, 255 / 255.f, 255 / 255.f, g.Style.Alpha) : ImColor(90 / 255.f, 90 / 255.f, 90 / 255.f, g.Style.Alpha), buf_display);
	//PopFont();

	return value_changed;*/
}



static bool switch_entity_teams[100];

bool ui::ColorEdit4MultiFixed(const char* label, Color* col1, Color* col2, ImGuiColorEditFlags flags, int iz)
{
	ImVec4 vecColor1 = ImVec4{ col1->get_red() / 255.f, col1->get_green() / 255.f, col1->get_blue() / 255.f, col1->get_alpha() / 255.f };
	ImVec4 vecColor2 = ImVec4{ col2->get_red() / 255.f, col2->get_green() / 255.f, col2->get_blue() / 255.f, col2->get_alpha() / 255.f };


	//shonax is p
	//switch_entity_teams[iz] = false;
	const char* button_name0 = switch_entity_teams[iz] ? "Enemy" : "Team";



	//switch_entity_teams ? &vecColor1.x : &vecColor2.x

	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	SameLine(window->Size.x - 28);
	flags |= ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_AlphaBar;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const float square_sz = GetFrameHeight();
	const float w_extra = (flags & ImGuiColorEditFlags_NoSmallPreview) ? 0.0f : (square_sz + style.ItemInnerSpacing.x);
	const float w_items_all = CalcItemWidth() - w_extra;
	const char* label_display_end = FindRenderedTextEnd(label);

	BeginGroup();
	PushID(label);

	// If we're not showing any slider there's no point in doing any HSV conversions
	const ImGuiColorEditFlags flags_untouched = flags;
	if (flags & ImGuiColorEditFlags_NoInputs)
		flags = (flags & (~ImGuiColorEditFlags__InputsMask)) | ImGuiColorEditFlags_RGB | ImGuiColorEditFlags_NoOptions;

	// Context menu: display and modify options (before defaults are applied)
	//if (!(flags & ImGuiColorEditFlags_NoOptions))
	//	ColorEditOptionsPopup(switch_entity_teams[iz] ? &vecColor1.x : &vecColor2.x, flags);

	// Read stored options
	if (!(flags & ImGuiColorEditFlags__InputsMask))
		flags |= (g.ColorEditOptions & ImGuiColorEditFlags__InputsMask);
	if (!(flags & ImGuiColorEditFlags__DataTypeMask))
		flags |= (g.ColorEditOptions & ImGuiColorEditFlags__DataTypeMask);
	if (!(flags & ImGuiColorEditFlags__PickerMask))
		flags |= (g.ColorEditOptions & ImGuiColorEditFlags__PickerMask);
	flags |= (g.ColorEditOptions & ~(ImGuiColorEditFlags__InputsMask | ImGuiColorEditFlags__DataTypeMask | ImGuiColorEditFlags__PickerMask));

	const bool alpha = (flags & ImGuiColorEditFlags_NoAlpha) == 0;
	const bool hdr = (flags & ImGuiColorEditFlags_HDR) != 0;
	const int components = alpha ? 4 : 3;

	// Convert to the formats we need

	float f[4] = { vecColor1.x, vecColor1.y, vecColor1.z, alpha ? vecColor1.w : 1.0f };
	// float f[4] = { col[0], col[1], col[2], alpha ? col[3] : 1.0f };
	if (flags & ImGuiColorEditFlags_HSV)
		ColorConvertRGBtoHSV(f[0], f[1], f[2], f[0], f[1], f[2]);
	int i[4] = { IM_F32_TO_INT8_UNBOUND(f[0]), IM_F32_TO_INT8_UNBOUND(f[1]), IM_F32_TO_INT8_UNBOUND(f[2]), IM_F32_TO_INT8_UNBOUND(f[3]) };

	bool value_changed = false;
	bool value_changed_as_float = false;

	if ((flags & (ImGuiColorEditFlags_RGB | ImGuiColorEditFlags_HSV)) != 0 && (flags & ImGuiColorEditFlags_NoInputs) == 0)
	{
		// RGB/HSV 0..255 Sliders
		const float w_item_one = ImMax(1.0f, (float)(int)((w_items_all - (style.ItemInnerSpacing.x) * (components - 1)) / (float)components));
		const float w_item_last = ImMax(1.0f, (float)(int)(w_items_all - (w_item_one + style.ItemInnerSpacing.x) * (components - 1)));

		const bool hide_prefix = (w_item_one <= CalcTextSize((flags & ImGuiColorEditFlags_Float) ? "M:0.000" : "M:000").x);
		const char* ids[4] = { "##X", "##Y", "##Z", "##W" };
		const char* fmt_table_int[3][4] =
		{
			{   "%3d",   "%3d",   "%3d",   "%3d" }, // Short display
			{ "R:%3d", "G:%3d", "B:%3d", "A:%3d" }, // Long display for RGBA
			{ "H:%3d", "S:%3d", "V:%3d", "A:%3d" }  // Long display for HSVA
		};
		const char* fmt_table_float[3][4] =
		{
			{   "%0.3f",   "%0.3f",   "%0.3f",   "%0.3f" }, // Short display
			{ "R:%0.3f", "G:%0.3f", "B:%0.3f", "A:%0.3f" }, // Long display for RGBA
			{ "H:%0.3f", "S:%0.3f", "V:%0.3f", "A:%0.3f" }  // Long display for HSVA
		};
		const int fmt_idx = hide_prefix ? 0 : (flags & ImGuiColorEditFlags_HSV) ? 2 : 1;

		PushItemWidth(w_item_one);
		for (int n = 0; n < components; n++)
		{
			if (n > 0)
				SameLine(0, style.ItemInnerSpacing.x);
			if (n + 1 == components)
				PushItemWidth(w_item_last);
			if (flags & ImGuiColorEditFlags_Float)
			{
				value_changed |= DragFloat(ids[n], &f[n], 1.0f / 255.0f, 0.0f, hdr ? 0.0f : 1.0f, fmt_table_float[fmt_idx][n]);
				value_changed_as_float |= value_changed;
			}
			else
			{
				value_changed |= DragInt(ids[n], &i[n], 1.0f, 0, hdr ? 0 : 255, fmt_table_int[fmt_idx][n]);
			}
			if (!(flags & ImGuiColorEditFlags_NoOptions))
				OpenPopupOnItemClick("context");
		}
		PopItemWidth();
		PopItemWidth();
	}
	else if ((flags & ImGuiColorEditFlags_HEX) != 0 && (flags & ImGuiColorEditFlags_NoInputs) == 0)
	{
		// RGB Hexadecimal Input
		char buf[64];
		if (alpha)
			ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X%02X", ImClamp(i[0], 0, 255), ImClamp(i[1], 0, 255), ImClamp(i[2], 0, 255), ImClamp(i[3], 0, 255));
		else
			ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X", ImClamp(i[0], 0, 255), ImClamp(i[1], 0, 255), ImClamp(i[2], 0, 255));
		PushItemWidth(w_items_all);
		//if (InputText("##Text", buf, IM_ARRAYSIZE(buf), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase))
		//{
		//	value_changed = true;
		//	char* p = buf;
		//	while (*p == '#' || ImCharIsBlankA(*p))
		//		p++;
		//	i[0] = i[1] = i[2] = i[3] = 0;
		//	if (alpha)
		//		sscanf(p, "%02X%02X%02X%02X", (unsigned int*)&i[0], (unsigned int*)&i[1], (unsigned int*)&i[2], (unsigned int*)&i[3]); // Treat at unsigned (%X is unsigned)
		//	else
		//		sscanf(p, "%02X%02X%02X", (unsigned int*)&i[0], (unsigned int*)&i[1], (unsigned int*)&i[2]);
		//}
		if (!(flags & ImGuiColorEditFlags_NoOptions))
			OpenPopupOnItemClick("context");
		PopItemWidth();
	}

	ImGuiWindow* picker_active_window = NULL;
	if (!(flags & ImGuiColorEditFlags_NoSmallPreview))
	{
		if (!(flags & ImGuiColorEditFlags_NoInputs))
			SameLine(0, style.ItemInnerSpacing.x);



		ImVec4 col_v4 = switch_entity_teams[iz] ? vecColor1 : vecColor2;
		// ImVec4 col_v4(col[0], col[1], col[2], alpha ? col[3] : 1.0f);
		ImVec4 old_col_v4 = col_v4;
		if (ColorButtonSkeet("##ColorButton", col_v4, flags))
		{
			if (!(flags & ImGuiColorEditFlags_NoPicker))
			{
				// Store current color and open a picker
				g.ColorPickerRef = col_v4;
				OpenPopup("picker");
				SetNextWindowPos(window->DC.LastItemRect.GetBL() + ImVec2(-1, style.ItemSpacing.y));
			}
		}
		if (!(flags & ImGuiColorEditFlags_NoOptions))
			OpenPopupOnItemClick("context");

		if (old_col_v4.x != col_v4.x ||
			old_col_v4.y != col_v4.y ||
			old_col_v4.z != col_v4.z ||
			old_col_v4.w != col_v4.w) {
			if (switch_entity_teams[iz])
				col1->set(col_v4.x * 255, col_v4.y * 255, col_v4.z * 255, col_v4.w * 255);
			else
				col2->set(col_v4.x * 255, col_v4.y * 255, col_v4.z * 255, col_v4.w * 255);
		}
		// SetNextWindowSize(ImVec2(224, 180));
		SetNextWindowSize(ImVec2(224 - 10.7, 230 - 9.1));
		//SetNextWindowSize(ImVec2(224 - 10.7, 230 - 38.1));
		if (BeginPopupSkeet("picker", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar))
		{
			picker_active_window = g.CurrentWindow;
			if (label != label_display_end)
			{
				TextUnformatted(label, label_display_end);
				Spacing();
			}
			ImGuiColorEditFlags picker_flags_to_forward = ImGuiColorEditFlags__DataTypeMask | ImGuiColorEditFlags__PickerMask | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_AlphaBar;
			ImGuiColorEditFlags picker_flags = (flags_untouched & picker_flags_to_forward) | ImGuiColorEditFlags__InputsMask | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreviewHalf;
			PushItemWidth(square_sz * 12.0f); // Use 256 + bar sizes?

			if (iz == 3)
			{
				const char* button_name0 = switch_entity_teams[iz] ? "Server" : "Client";
				ui::SetCursorPosX(ui::GetCursorPosX() - 12); //
				if (ui::ButtonSkeet(button_name0, ImVec2(215 - 10.7, 0)))
					switch_entity_teams[iz] = !switch_entity_teams[iz];
			}
			else
			{
				const char* button_name0 = switch_entity_teams[iz] ? "Terrorists" : "Counter - Terrorists";
			
			
			
				ui::SetCursorPosX(ui::GetCursorPosX() - 12); //
				if (ui::ButtonSkeet(button_name0, ImVec2(215 - 10.7, 0)))
					switch_entity_teams[iz] = !switch_entity_teams[iz];
			}

			value_changed |= ColorPicker4Skeet("##picker", switch_entity_teams[iz] ? &vecColor1.x : &vecColor2.x, picker_flags, &g.ColorPickerRef.x);
			PopItemWidth();
			EndPopupSkeet();
		}
	}

	if (label != label_display_end && !(flags & ImGuiColorEditFlags_NoLabel))
	{
		SameLine(0, style.ItemInnerSpacing.x);
		TextUnformatted(label, label_display_end);
	}

	// Convert back
	if (picker_active_window == NULL)
	{
		if (!value_changed_as_float)
			for (int n = 0; n < 4; n++)
				f[n] = i[n] / 255.0f;
		if (flags & ImGuiColorEditFlags_HSV)
			ColorConvertHSVtoRGB(f[0], f[1], f[2], f[0], f[1], f[2]);
		if (value_changed)
		{
			if (switch_entity_teams[iz])
				col1->set(f[0] * 255, f[1] * 255, f[2] * 255, f[3] * 255);
			else
				col2->set(f[0] * 255, f[1] * 255, f[2] * 255, f[3] * 255);
		}
	}

	PopID();
	EndGroup();

	// Drag and Drop Target
	// NB: The flag test is merely an optional micro-optimization, BeginDragDropTarget() does the same test.
	if ((window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_HoveredRect) && !(flags & ImGuiColorEditFlags_NoDragDrop) && BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
		{
			memcpy((float*)switch_entity_teams[iz] ? &vecColor1.x : &vecColor2.x, payload->Data, sizeof(float) * 3); // Preserve alpha if any //-V512
			value_changed = true;
		}
		if (const ImGuiPayload* payload = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
		{
			memcpy((float*)switch_entity_teams[iz] ? &vecColor1.x : &vecColor2.x, payload->Data, sizeof(float) * components);
			value_changed = true;
		}
		EndDragDropTarget();
	}

	// When picker is being actively used, use its active id so IsItemActive() will function on ColorEdit4().
	if (picker_active_window && g.ActiveId != 0 && g.ActiveIdWindow == picker_active_window)
		window->DC.LastItemId = g.ActiveId;

	if (value_changed)
		MarkItemEdited(window->DC.LastItemId);

	if (switch_entity_teams[iz])
		col1->set(vecColor1.x * 255, vecColor1.y * 255, vecColor1.z * 255, vecColor1.w * 255);
	else
		col2->set(vecColor2.x * 255, vecColor2.y * 255, vecColor2.z * 255, vecColor2.w * 255);

	return value_changed;
}








bool ui::ColorEdit4Fix(const char* label, Color* col1, ImGuiColorEditFlags flags)
{
	
	ImVec4 vecColor1 = ImVec4{ col1->get_red() / 255.f, col1->get_green() / 255.f, col1->get_blue() / 255.f, col1->get_alpha() / 255.f };
	
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	SameLine(window->Size.x - 28);
	flags |= ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_AlphaBar;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const float square_sz = GetFrameHeight();
	const float w_extra = (flags & ImGuiColorEditFlags_NoSmallPreview) ? 0.0f : (square_sz + style.ItemInnerSpacing.x);
	const float w_items_all = CalcItemWidth() - w_extra;
	const char* label_display_end = FindRenderedTextEnd(label);

	BeginGroup();
	PushID(label);

	// If we're not showing any slider there's no point in doing any HSV conversions
	const ImGuiColorEditFlags flags_untouched = flags;
	if (flags & ImGuiColorEditFlags_NoInputs)
		flags = (flags & (~ImGuiColorEditFlags__InputsMask)) | ImGuiColorEditFlags_RGB | ImGuiColorEditFlags_NoOptions;

	// Context menu: display and modify options (before defaults are applied)
	//if (!(flags & ImGuiColorEditFlags_NoOptions))
	//	ColorEditOptionsPopup(switch_entity_teams[iz] ? &vecColor1.x : &vecColor2.x, flags);

	// Read stored options
	if (!(flags & ImGuiColorEditFlags__InputsMask))
		flags |= (g.ColorEditOptions & ImGuiColorEditFlags__InputsMask);
	if (!(flags & ImGuiColorEditFlags__DataTypeMask))
		flags |= (g.ColorEditOptions & ImGuiColorEditFlags__DataTypeMask);
	if (!(flags & ImGuiColorEditFlags__PickerMask))
		flags |= (g.ColorEditOptions & ImGuiColorEditFlags__PickerMask);
	flags |= (g.ColorEditOptions & ~(ImGuiColorEditFlags__InputsMask | ImGuiColorEditFlags__DataTypeMask | ImGuiColorEditFlags__PickerMask));

	const bool alpha = (flags & ImGuiColorEditFlags_NoAlpha) == 0;
	const bool hdr = (flags & ImGuiColorEditFlags_HDR) != 0;
	const int components = alpha ? 4 : 3;

	// Convert to the formats we need

	float f[4] = { vecColor1.x, vecColor1.y, vecColor1.z, alpha ? vecColor1.w : 1.0f };
	// float f[4] = { col[0], col[1], col[2], alpha ? col[3] : 1.0f };
	if (flags & ImGuiColorEditFlags_HSV)
		ColorConvertRGBtoHSV(f[0], f[1], f[2], f[0], f[1], f[2]);
	int i[4] = { IM_F32_TO_INT8_UNBOUND(f[0]), IM_F32_TO_INT8_UNBOUND(f[1]), IM_F32_TO_INT8_UNBOUND(f[2]), IM_F32_TO_INT8_UNBOUND(f[3]) };

	bool value_changed = false;
	bool value_changed_as_float = false;

	if ((flags & (ImGuiColorEditFlags_RGB | ImGuiColorEditFlags_HSV)) != 0 && (flags & ImGuiColorEditFlags_NoInputs) == 0)
	{
		// RGB/HSV 0..255 Sliders
		const float w_item_one = ImMax(1.0f, (float)(int)((w_items_all - (style.ItemInnerSpacing.x) * (components - 1)) / (float)components));
		const float w_item_last = ImMax(1.0f, (float)(int)(w_items_all - (w_item_one + style.ItemInnerSpacing.x) * (components - 1)));

		const bool hide_prefix = (w_item_one <= CalcTextSize((flags & ImGuiColorEditFlags_Float) ? "M:0.000" : "M:000").x);
		const char* ids[4] = { "##X", "##Y", "##Z", "##W" };
		const char* fmt_table_int[3][4] =
		{
			{   "%3d",   "%3d",   "%3d",   "%3d" }, // Short display
			{ "R:%3d", "G:%3d", "B:%3d", "A:%3d" }, // Long display for RGBA
			{ "H:%3d", "S:%3d", "V:%3d", "A:%3d" }  // Long display for HSVA
		};
		const char* fmt_table_float[3][4] =
		{
			{   "%0.3f",   "%0.3f",   "%0.3f",   "%0.3f" }, // Short display
			{ "R:%0.3f", "G:%0.3f", "B:%0.3f", "A:%0.3f" }, // Long display for RGBA
			{ "H:%0.3f", "S:%0.3f", "V:%0.3f", "A:%0.3f" }  // Long display for HSVA
		};
		const int fmt_idx = hide_prefix ? 0 : (flags & ImGuiColorEditFlags_HSV) ? 2 : 1;

		PushItemWidth(w_item_one);
		for (int n = 0; n < components; n++)
		{
			if (n > 0)
				SameLine(0, style.ItemInnerSpacing.x);
			if (n + 1 == components)
				PushItemWidth(w_item_last);
			if (flags & ImGuiColorEditFlags_Float)
			{
				value_changed |= DragFloat(ids[n], &f[n], 1.0f / 255.0f, 0.0f, hdr ? 0.0f : 1.0f, fmt_table_float[fmt_idx][n]);
				value_changed_as_float |= value_changed;
			}
			else
			{
				value_changed |= DragInt(ids[n], &i[n], 1.0f, 0, hdr ? 0 : 255, fmt_table_int[fmt_idx][n]);
			}
			if (!(flags & ImGuiColorEditFlags_NoOptions))
				OpenPopupOnItemClick("context");
		}
		PopItemWidth();
		PopItemWidth();
	}
	else if ((flags & ImGuiColorEditFlags_HEX) != 0 && (flags & ImGuiColorEditFlags_NoInputs) == 0)
	{
		// RGB Hexadecimal Input
		char buf[64];
		if (alpha)
			ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X%02X", ImClamp(i[0], 0, 255), ImClamp(i[1], 0, 255), ImClamp(i[2], 0, 255), ImClamp(i[3], 0, 255));
		else
			ImFormatString(buf, IM_ARRAYSIZE(buf), "#%02X%02X%02X", ImClamp(i[0], 0, 255), ImClamp(i[1], 0, 255), ImClamp(i[2], 0, 255));
		PushItemWidth(w_items_all);
		//if (InputText("##Text", buf, IM_ARRAYSIZE(buf), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase))
		//{
		//	value_changed = true;
		//	char* p = buf;
		//	while (*p == '#' || ImCharIsBlankA(*p))
		//		p++;
		//	i[0] = i[1] = i[2] = i[3] = 0;
		//	if (alpha)
		//		sscanf(p, "%02X%02X%02X%02X", (unsigned int*)&i[0], (unsigned int*)&i[1], (unsigned int*)&i[2], (unsigned int*)&i[3]); // Treat at unsigned (%X is unsigned)
		//	else
		//		sscanf(p, "%02X%02X%02X", (unsigned int*)&i[0], (unsigned int*)&i[1], (unsigned int*)&i[2]);
		//}
		if (!(flags & ImGuiColorEditFlags_NoOptions))
			OpenPopupOnItemClick("context");
		PopItemWidth();
	}

	ImGuiWindow* picker_active_window = NULL;
	if (!(flags & ImGuiColorEditFlags_NoSmallPreview))
	{
		if (!(flags & ImGuiColorEditFlags_NoInputs))
			SameLine(0, style.ItemInnerSpacing.x);



		ImVec4 col_v4 = vecColor1;
	
		ImVec4 old_col_v4 = col_v4;
		if (ColorButtonSkeet("##ColorButton", col_v4, flags))
		{
			if (!(flags & ImGuiColorEditFlags_NoPicker))
			{
				// Store current color and open a picker
				g.ColorPickerRef = col_v4;
				OpenPopup("picker");
				SetNextWindowPos(window->DC.LastItemRect.GetBL() + ImVec2(-1, style.ItemSpacing.y));
			}
		}
		if (!(flags & ImGuiColorEditFlags_NoOptions))
			OpenPopupOnItemClick("context");

		if (old_col_v4.x != col_v4.x ||
			old_col_v4.y != col_v4.y ||
			old_col_v4.z != col_v4.z ||
			old_col_v4.w != col_v4.w)
		{
				col1->set(col_v4.x * 255, col_v4.y * 255, col_v4.z * 255, col_v4.w * 255);
			
		}
		SetNextWindowSize(ImVec2(224 - 10.7, 230 - 58.1));
		if (BeginPopupSkeet("picker", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar))
		{
			picker_active_window = g.CurrentWindow;
			if (label != label_display_end)
			{
				TextUnformatted(label, label_display_end);
				Spacing();
			}
			ImGuiColorEditFlags picker_flags_to_forward = ImGuiColorEditFlags__DataTypeMask | ImGuiColorEditFlags__PickerMask | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_AlphaBar;
			ImGuiColorEditFlags picker_flags = (flags_untouched & picker_flags_to_forward) | ImGuiColorEditFlags__InputsMask | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreviewHalf;
			PushItemWidth(square_sz * 12.0f); // Use 256 + bar sizes?

			value_changed |= ColorPicker4Skeet("##picker",  &vecColor1.x, picker_flags, &g.ColorPickerRef.x);
			PopItemWidth();
			EndPopupSkeet();
		}
	}

	if (label != label_display_end && !(flags & ImGuiColorEditFlags_NoLabel))
	{
		SameLine(0, style.ItemInnerSpacing.x);
		TextUnformatted(label, label_display_end);
	}

	// Convert back
	if (picker_active_window == NULL)
	{
		if (!value_changed_as_float)
			for (int n = 0; n < 4; n++)
				f[n] = i[n] / 255.0f;
		if (flags & ImGuiColorEditFlags_HSV)
			ColorConvertHSVtoRGB(f[0], f[1], f[2], f[0], f[1], f[2]);
		if (value_changed)
		{
			col1->set(f[0] * 255, f[1] * 255, f[2] * 255, f[3] * 255);
		}
	}

	PopID();
	EndGroup();

	// Drag and Drop Target
	// NB: The flag test is merely an optional micro-optimization, BeginDragDropTarget() does the same test.
	if ((window->DC.LastItemStatusFlags & ImGuiItemStatusFlags_HoveredRect) && !(flags & ImGuiColorEditFlags_NoDragDrop) && BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
		{
			memcpy((float*)&vecColor1.x, payload->Data, sizeof(float) * 3); // Preserve alpha if any //-V512
			value_changed = true;
		}
		if (const ImGuiPayload* payload = AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
		{
			memcpy((float*)&vecColor1.x, payload->Data, sizeof(float) * components);
			value_changed = true;
		}
		EndDragDropTarget();
	}

	// When picker is being actively used, use its active id so IsItemActive() will function on ColorEdit4().
	if (picker_active_window && g.ActiveId != 0 && g.ActiveIdWindow == picker_active_window)
		window->DC.LastItemId = g.ActiveId;

	if (value_changed)
		MarkItemEdited(window->DC.LastItemId);

	col1->set(vecColor1.x * 255, vecColor1.y * 255, vecColor1.z * 255, vecColor1.w * 255);
	

	return value_changed;

}




bool ui::ColorEdit4Fixed(const char* label, Color* col1,  ImGuiColorEditFlags flags)
{
	if (ui::ColorEdit4Fix(label, col1, flags))
	{
		return true;
	}
	return false;
}

bool ui::ColorEdit4Multi(const char* label, Color* col1, Color* col2, ImGuiColorEditFlags flags, int iz)
{
	if (ui::ColorEdit4MultiFixed(label, col1, col2, flags, iz))
	{
		return true;
	}
	return false;
}



bool ui::ColorEdit3Multi(const char* label, Color* col1, Color* col2, ImGuiColorEditFlags flags, int iz)
{
	return ColorEdit4Multi(label, col1, col2, flags | ImGuiColorEditFlags_NoAlpha, iz);
}


void ui::EndPopupSkeet()
{
	ImGuiContext& g = *GImGui;
	IM_ASSERT(g.CurrentWindow->Flags & ImGuiWindowFlags_Popup);  // Mismatched BeginPopup()/EndPopup() calls
	IM_ASSERT(g.BeginPopupStack.Size > 0);

	// Make all menus and popups wrap around for now, may need to expose that policy.
	NavMoveRequestTryWrapping(g.CurrentWindow, ImGuiNavMoveFlags_LoopY);

	EndSkeet();
}


//ot
//int                     HiddenFramesRegular;           // Hide the window for N frames
//int                     HiddenFramesForResize;        // Hide the window for N frames while allowing items to be submitted so we can measure their size


//int                     HiddenFramesRegular;                // Hide the window for N frames
//int                     HiddenFramesForResize;              // Hide the window for N frames while allowing items to be submitted so we can measure their size

bool ui::BeginOT(const char* name, bool* p_open, ImGuiWindowFlags flags)
{
	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	IM_ASSERT(name != NULL && name[0] != '\0');     // Window name required
	IM_ASSERT(g.FrameScopeActive);                  // Forgot to call ImGui::NewFrame()
	IM_ASSERT(g.FrameCountEnded != g.FrameCount);   // Called ImGui::Render() or ImGui::EndFrame() and haven't called ImGui::NewFrame() again yet
	//PushFont(_shon->tabf);
	// Find or create
	ImGuiWindow* window = FindWindowByName(name);
	const bool window_just_created = (window == NULL);
	if (window_just_created)
	{
		ImVec2 size_on_first_use = (g.NextWindowData.SizeCond != 0) ? g.NextWindowData.SizeVal : ImVec2(0.0f, 0.0f); // Any condition flag will do since we are creating a new window here.
		window = CreateNewWindow(name, size_on_first_use, flags);
	}

	// Automatically disable manual moving/resizing when NoInputs is set
	if ((flags & ImGuiWindowFlags_NoInputs) == ImGuiWindowFlags_NoInputs)
		flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

	if (flags & ImGuiWindowFlags_NavFlattened)
		IM_ASSERT(flags & ImGuiWindowFlags_ChildWindow);

	const int current_frame = g.FrameCount;
	const bool first_begin_of_the_frame = (window->LastFrameActive != current_frame);

	// Update Flags, LastFrameActive, BeginOrderXXX fields
	if (first_begin_of_the_frame)
		window->Flags = (ImGuiWindowFlags)flags;
	else
		flags = window->Flags;

	// Parent window is latched only on the first call to Begin() of the frame, so further append-calls can be done from a different window stack
	ImGuiWindow* parent_window_in_stack = g.CurrentWindowStack.empty() ? NULL : g.CurrentWindowStack.back();
	ImGuiWindow* parent_window = first_begin_of_the_frame ? ((flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Popup)) ? parent_window_in_stack : NULL) : window->ParentWindow;
	IM_ASSERT(parent_window != NULL || !(flags & ImGuiWindowFlags_ChildWindow));
	window->HasCloseButton = (p_open != NULL);

	// Update the Appearing flag
	bool window_just_activated_by_user = (window->LastFrameActive < current_frame - 1);   // Not using !WasActive because the implicit "Debug" window would always toggle off->on
	const bool window_just_appearing_after_hidden_for_resize = (window->HiddenFramesForResize > 0);
	if (flags & ImGuiWindowFlags_Popup)
	{
		ImGuiPopupRef& popup_ref = g.OpenPopupStack[g.BeginPopupStack.Size];
		window_just_activated_by_user |= (window->PopupId != popup_ref.PopupId); // We recycle popups so treat window as activated if popup id changed
		window_just_activated_by_user |= (window != popup_ref.Window);
	}
	window->Appearing = (window_just_activated_by_user || window_just_appearing_after_hidden_for_resize);
	if (window->Appearing)
		SetWindowConditionAllowFlags(window, ImGuiCond_Appearing, true);

	// Add to stack
	g.CurrentWindowStack.push_back(window);
	SetCurrentWindow(window);
	CheckStacksSize(window, true);
	if (flags & ImGuiWindowFlags_Popup)
	{
		ImGuiPopupRef& popup_ref = g.OpenPopupStack[g.BeginPopupStack.Size];
		popup_ref.Window = window;
		g.BeginPopupStack.push_back(popup_ref);
		window->PopupId = popup_ref.PopupId;
	}

	if (window_just_appearing_after_hidden_for_resize && !(flags & ImGuiWindowFlags_ChildWindow))
		window->NavLastIds[0] = 0;

	// Process SetNextWindow***() calls
	bool window_pos_set_by_api = false;
	bool window_size_x_set_by_api = false, window_size_y_set_by_api = false;
	if (g.NextWindowData.PosCond)
	{
		window_pos_set_by_api = (window->SetWindowPosAllowFlags & g.NextWindowData.PosCond) != 0;
		if (window_pos_set_by_api && ImLengthSqr(g.NextWindowData.PosPivotVal) > 0.00001f)
		{
			// May be processed on the next frame if this is our first frame and we are measuring size
			// FIXME: Look into removing the branch so everything can go through this same code path for consistency.
			window->SetWindowPosVal = g.NextWindowData.PosVal;
			window->SetWindowPosPivot = g.NextWindowData.PosPivotVal;
			window->SetWindowPosAllowFlags &= ~(ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing);
		}
		else
		{
			SetWindowPos(window, g.NextWindowData.PosVal, g.NextWindowData.PosCond);
		}
	}
	if (g.NextWindowData.SizeCond)
	{
		window_size_x_set_by_api = (window->SetWindowSizeAllowFlags & g.NextWindowData.SizeCond) != 0 && (g.NextWindowData.SizeVal.x > 0.0f);
		window_size_y_set_by_api = (window->SetWindowSizeAllowFlags & g.NextWindowData.SizeCond) != 0 && (g.NextWindowData.SizeVal.y > 0.0f);
		SetWindowSize(window, g.NextWindowData.SizeVal, g.NextWindowData.SizeCond);
	}
	if (g.NextWindowData.ContentSizeCond)
	{
		// Adjust passed "client size" to become a "window size"
		window->SizeContentsExplicit = g.NextWindowData.ContentSizeVal;
		if (window->SizeContentsExplicit.y != 0.0f)
			window->SizeContentsExplicit.y += window->TitleBarHeight() + window->MenuBarHeight();
	}
	else if (first_begin_of_the_frame)
	{
		window->SizeContentsExplicit = ImVec2(0.0f, 0.0f);
	}
	if (g.NextWindowData.CollapsedCond)
		SetWindowCollapsed(window, g.NextWindowData.CollapsedVal, g.NextWindowData.CollapsedCond);
	if (g.NextWindowData.FocusCond)
		FocusWindow(window);
	if (window->Appearing)
		SetWindowConditionAllowFlags(window, ImGuiCond_Appearing, false);

	// When reusing window again multiple times a frame, just append content (don't need to setup again)
	if (first_begin_of_the_frame)
	{
		// Initialize
		const bool window_is_child_tooltip = (flags & ImGuiWindowFlags_ChildWindow) && (flags & ImGuiWindowFlags_Tooltip); // FIXME-WIP: Undocumented behavior of Child+Tooltip for pinned tooltip (#1345)
		UpdateWindowParentAndRootLinks(window, flags, parent_window);

		window->Active = true;
		window->BeginOrderWithinParent = 0;
		window->BeginOrderWithinContext = (short)(g.WindowsActiveCount++);
		window->BeginCount = 0;
		window->ClipRect = ImVec4(-FLT_MAX, -FLT_MAX, +FLT_MAX, +FLT_MAX);
		window->LastFrameActive = current_frame;
		window->IDStack.resize(1);

		// Update stored window name when it changes (which can _only_ happen with the "###" operator, so the ID would stay unchanged).
		// The title bar always display the 'name' parameter, so we only update the string storage if it needs to be visible to the end-user elsewhere.
		bool window_title_visible_elsewhere = false;
		if (g.NavWindowingList != NULL && (window->Flags & ImGuiWindowFlags_NoNavFocus) == 0)   // Window titles visible when using CTRL+TAB
			window_title_visible_elsewhere = true;
		if (window_title_visible_elsewhere && !window_just_created && strcmp(name, window->Name) != 0)
		{
			size_t buf_len = (size_t)window->NameBufLen;
			window->Name = ImStrdupcpy(window->Name, &buf_len, name);
			window->NameBufLen = (int)buf_len;
		}

		// UPDATE CONTENTS SIZE, UPDATE HIDDEN STATUS

		// Update contents size from last frame for auto-fitting (or use explicit size)
		window->SizeContents = CalcSizeContents(window);
		if (window->HiddenFramesRegular > 0)
			window->HiddenFramesRegular--;
		if (window->HiddenFramesForResize > 0)
			window->HiddenFramesForResize--;

		// Hide new windows for one frame until they calculate their size
		if (window_just_created && (!window_size_x_set_by_api || !window_size_y_set_by_api))
			window->HiddenFramesForResize = 1;

		// Hide popup/tooltip window when re-opening while we measure size (because we recycle the windows)
		// We reset Size/SizeContents for reappearing popups/tooltips early in this function, so further code won't be tempted to use the old size.
		if (window_just_activated_by_user && (flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) != 0)
		{
			window->HiddenFramesForResize = 1;
			if (flags & ImGuiWindowFlags_AlwaysAutoResize)
			{
				if (!window_size_x_set_by_api)
					window->Size.x = window->SizeFull.x = 0.f;
				if (!window_size_y_set_by_api)
					window->Size.y = window->SizeFull.y = 0.f;
				window->SizeContents = ImVec2(0.f, 0.f);
			}
		}

		SetCurrentWindow(window);

		// Lock border size and padding for the frame (so that altering them doesn't cause inconsistencies)
		window->WindowBorderSize = (flags & ImGuiWindowFlags_ChildWindow) ? style.ChildBorderSize : ((flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) && !(flags & ImGuiWindowFlags_Modal)) ? style.PopupBorderSize : style.WindowBorderSize;
		window->WindowPadding = style.WindowPadding;
		if ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & (ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_Popup)) && window->WindowBorderSize == 0.0f)
			window->WindowPadding = ImVec2(0.0f, (flags & ImGuiWindowFlags_MenuBar) ? style.WindowPadding.y : 0.0f);
		window->DC.MenuBarOffset.x = ImMax(ImMax(window->WindowPadding.x, style.ItemSpacing.x), g.NextWindowData.MenuBarOffsetMinVal.x);
		window->DC.MenuBarOffset.y = g.NextWindowData.MenuBarOffsetMinVal.y;

		// Collapse window by double-clicking on title bar
		// At this point we don't have a clipping rectangle setup yet, so we can use the title bar area for hit detection and drawing
		if (!(flags & ImGuiWindowFlags_NoTitleBar) && !(flags & ImGuiWindowFlags_NoCollapse))
		{
			// We don't use a regular button+id to test for double-click on title bar (mostly due to legacy reason, could be fixed), so verify that we don't have items over the title bar.
			ImRect title_bar_rect = window->TitleBarRect();
			if (g.HoveredWindow == window && g.HoveredId == 0 && g.HoveredIdPreviousFrame == 0 && IsMouseHoveringRect(title_bar_rect.Min, title_bar_rect.Max) && g.IO.MouseDoubleClicked[0])
				window->WantCollapseToggle = true;
			if (window->WantCollapseToggle)
			{
				window->Collapsed = !window->Collapsed;
				MarkIniSettingsDirty(window);
				FocusWindow(window);
			}
		}
		else
		{
			window->Collapsed = false;
		}
		window->WantCollapseToggle = false;

		// SIZE

		// Calculate auto-fit size, handle automatic resize
		const ImVec2 size_auto_fit = CalcSizeAutoFit(window, window->SizeContents);
		ImVec2 size_full_modified(FLT_MAX, FLT_MAX);
		if ((flags & ImGuiWindowFlags_AlwaysAutoResize) && !window->Collapsed)
		{
			// Using SetNextWindowSize() overrides ImGuiWindowFlags_AlwaysAutoResize, so it can be used on tooltips/popups, etc.
			if (!window_size_x_set_by_api)
				window->SizeFull.x = size_full_modified.x = size_auto_fit.x;
			if (!window_size_y_set_by_api)
				window->SizeFull.y = size_full_modified.y = size_auto_fit.y;
		}
		else if (window->AutoFitFramesX > 0 || window->AutoFitFramesY > 0)
		{
			// Auto-fit may only grow window during the first few frames
			// We still process initial auto-fit on collapsed windows to get a window width, but otherwise don't honor ImGuiWindowFlags_AlwaysAutoResize when collapsed.
			if (!window_size_x_set_by_api && window->AutoFitFramesX > 0)
				window->SizeFull.x = size_full_modified.x = window->AutoFitOnlyGrows ? ImMax(window->SizeFull.x, size_auto_fit.x) : size_auto_fit.x;
			if (!window_size_y_set_by_api && window->AutoFitFramesY > 0)
				window->SizeFull.y = size_full_modified.y = window->AutoFitOnlyGrows ? ImMax(window->SizeFull.y, size_auto_fit.y) : size_auto_fit.y;
			if (!window->Collapsed)
				MarkIniSettingsDirty(window);
		}

		// Apply minimum/maximum window size constraints and final size
		window->SizeFull = CalcSizeAfterConstraint(window, window->SizeFull);
		window->Size = window->Collapsed && !(flags & ImGuiWindowFlags_ChildWindow) ? window->TitleBarRect().GetSize() : window->SizeFull;

		if (flags & ImGuiWindowFlags_ChildFrame) {
			window->SizeFull.x = parent_window->Size.x - 64;
			window->Size.x = parent_window->Size.x - 64;
		}

		// SCROLLBAR STATUS

		// Update scrollbar status (based on the Size that was effective during last frame or the auto-resized Size).
		if (!window->Collapsed)
		{
			// When reading the current size we need to read it after size constraints have been applied
			float size_x_for_scrollbars = size_full_modified.x != FLT_MAX ? window->SizeFull.x : window->SizeFullAtLastBegin.x;
			float size_y_for_scrollbars = size_full_modified.y != FLT_MAX ? window->SizeFull.y : window->SizeFullAtLastBegin.y;
			window->ScrollbarY = (flags & ImGuiWindowFlags_AlwaysVerticalScrollbar) || ((window->SizeContents.y > size_y_for_scrollbars) && !(flags & ImGuiWindowFlags_NoScrollbar));
			window->ScrollbarX = (flags & ImGuiWindowFlags_AlwaysHorizontalScrollbar) || ((window->SizeContents.x > size_x_for_scrollbars - (window->ScrollbarY ? style.ScrollbarSize : 0.0f)) && !(flags & ImGuiWindowFlags_NoScrollbar) && (flags & ImGuiWindowFlags_HorizontalScrollbar));
			if (window->ScrollbarX && !window->ScrollbarY)
				window->ScrollbarY = (window->SizeContents.y > size_y_for_scrollbars - style.ScrollbarSize) && !(flags & ImGuiWindowFlags_NoScrollbar);
			window->ScrollbarSizes = ImVec2(window->ScrollbarY ? style.ScrollbarSize : 0.0f, window->ScrollbarX ? style.ScrollbarSize : 0.0f);
		}

		// POSITION

		// Popup latch its initial position, will position itself when it appears next frame
		if (window_just_activated_by_user)
		{
			window->AutoPosLastDirection = ImGuiDir_None;
			if ((flags & ImGuiWindowFlags_Popup) != 0 && !window_pos_set_by_api)
				window->Pos = g.BeginPopupStack.back().OpenPopupPos;
		}

		// Position child window
		if (flags & ImGuiWindowFlags_ChildWindow)
		{
			IM_ASSERT(parent_window && parent_window->Active);
			window->BeginOrderWithinParent = (short)parent_window->DC.ChildWindows.Size;
			parent_window->DC.ChildWindows.push_back(window);
			if (!(flags & ImGuiWindowFlags_Popup) && !window_pos_set_by_api && !window_is_child_tooltip)
				window->Pos = parent_window->DC.CursorPos + ImVec2(flags & ImGuiWindowFlags_ChildFrame ? 16 : 0, 0);
		}

		const bool window_pos_with_pivot = (window->SetWindowPosVal.x != FLT_MAX && window->HiddenFramesForResize == 0);
		if (window_pos_with_pivot)
			SetWindowPos(window, ImMax(style.DisplaySafeAreaPadding, window->SetWindowPosVal - window->SizeFull * window->SetWindowPosPivot), 0); // Position given a pivot (e.g. for centering)
		else if ((flags & ImGuiWindowFlags_ChildMenu) != 0)
			window->Pos = FindBestWindowPosForPopup(window);
		else if ((flags & ImGuiWindowFlags_Popup) != 0 && !window_pos_set_by_api && window_just_appearing_after_hidden_for_resize)
			window->Pos = FindBestWindowPosForPopup(window);
		else if ((flags & ImGuiWindowFlags_Tooltip) != 0 && !window_pos_set_by_api && !window_is_child_tooltip)
			window->Pos = FindBestWindowPosForPopup(window);

		// Clamp position so it stays visible
		// Ignore zero-sized display explicitly to avoid losing positions if a window manager reports zero-sized window when initializing or minimizing.
		if (!window_pos_set_by_api && !(flags & ImGuiWindowFlags_ChildWindow) && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0)
		{
			if (g.IO.DisplaySize.x > 0.0f && g.IO.DisplaySize.y > 0.0f) // Ignore zero-sized display explicitly to avoid losing positions if a window manager reports zero-sized window when initializing or minimizing.
			{
				ImVec2 padding = ImMax(style.DisplayWindowPadding, style.DisplaySafeAreaPadding);
				ImVec2 size_for_clamping = window->Size;
				window->Pos = ImMax(window->Pos + size_for_clamping, padding) - size_for_clamping;
				window->Pos = ImMin(window->Pos, g.IO.DisplaySize - padding);
			}
		}
		window->Pos = ImFloor(window->Pos);

		// Lock window rounding for the frame (so that altering them doesn't cause inconsistencies)
		window->WindowRounding = (flags & ImGuiWindowFlags_ChildWindow) ? style.ChildRounding : ((flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiWindowFlags_Modal)) ? style.PopupRounding : style.WindowRounding;

		// Prepare for item focus requests
		window->FocusIdxAllRequestCurrent = (window->FocusIdxAllRequestNext == INT_MAX || window->FocusIdxAllCounter == -1) ? INT_MAX : (window->FocusIdxAllRequestNext + (window->FocusIdxAllCounter + 1)) % (window->FocusIdxAllCounter + 1);
		window->FocusIdxTabRequestCurrent = (window->FocusIdxTabRequestNext == INT_MAX || window->FocusIdxTabCounter == -1) ? INT_MAX : (window->FocusIdxTabRequestNext + (window->FocusIdxTabCounter + 1)) % (window->FocusIdxTabCounter + 1);
		window->FocusIdxAllCounter = window->FocusIdxTabCounter = -1;
		window->FocusIdxAllRequestNext = window->FocusIdxTabRequestNext = INT_MAX;

		// Apply scrolling
		window->Scroll = CalcNextScrollFromScrollTargetAndClamp(window, true);
		window->ScrollTarget = ImVec2(FLT_MAX, FLT_MAX);

		// Apply window focus (new and reactivated windows are moved to front)
		bool want_focus = false;
		if (window_just_activated_by_user && !(flags & ImGuiWindowFlags_NoFocusOnAppearing))
		{
			if (flags & ImGuiWindowFlags_Popup)
				want_focus = true;
			else if ((flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Tooltip)) == 0)
				want_focus = true;
		}

		// Handle manual resize: Resize Grips, Borders, Gamepad
		int border_held = -1;
		ImU32 resize_grip_col[4] = { 0 };
		const int resize_grip_count = g.IO.ConfigWindowsResizeFromEdges ? 2 : 1; // 4
		const float grip_draw_size = (float)(int)ImMax(g.FontSize * 1.35f, window->WindowRounding + 1.0f + g.FontSize * 0.2f);
		if (!window->Collapsed)
			UpdateManualResize(window, size_auto_fit, &border_held, resize_grip_count, &resize_grip_col[0]);
		window->ResizeBorderHeld = (signed char)border_held;

		// Default item width. Make it proportional to window size if window manually resizes
		if (window->Size.x > 0.0f && !(flags & ImGuiWindowFlags_Tooltip) && !(flags & ImGuiWindowFlags_AlwaysAutoResize))
			window->ItemWidthDefault = (float)(int)(window->Size.x * 0.65f);
		else
			window->ItemWidthDefault = (float)(int)(g.FontSize * 16.0f);

		// DRAWING

		// Setup draw list and outer clipping rectangle
		window->DrawList->Clear();
		window->DrawList->Flags = (g.Style.AntiAliasedLines ? ImDrawListFlags_AntiAliasedLines : 0) | (g.Style.AntiAliasedFill ? ImDrawListFlags_AntiAliasedFill : 0);
		window->DrawList->PushTextureID(g.Font->ContainerAtlas->TexID);
		ImRect viewport_rect(GetViewportRect());

		if ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Popup) && !window_is_child_tooltip)
			PushClipRect(parent_window->ClipRect.Min, parent_window->ClipRect.Max, true);
		else
			PushClipRect(viewport_rect.Min, viewport_rect.Max, true);

		// Draw modal window background (darkens what is behind them, all viewports)
		const bool dim_bg_for_modal = (flags & ImGuiWindowFlags_Modal) && window == GetFrontMostPopupModal() && window->HiddenFramesForResize <= 0;
		const bool dim_bg_for_window_list = g.NavWindowingTargetAnim && (window == g.NavWindowingTargetAnim->RootWindow);
		if (dim_bg_for_modal || dim_bg_for_window_list)
		{
			const ImU32 dim_bg_col = GetColorU32(dim_bg_for_modal ? ImGuiCol_ModalWindowDimBg : ImGuiCol_NavWindowingDimBg, g.DimBgRatio);
			window->DrawList->AddRectFilled(viewport_rect.Min, viewport_rect.Max, dim_bg_col);
		}

		// Draw navigation selection/windowing rectangle background
		if (dim_bg_for_window_list && window == g.NavWindowingTargetAnim)
		{
			ImRect bb = window->Rect();
			bb.Expand(g.FontSize);
			if (!bb.Contains(viewport_rect)) // Avoid drawing if the window covers all the viewport anyway
				window->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(ImGuiCol_NavWindowingHighlight, g.NavWindowingHighlightAlpha * 0.25f), g.Style.WindowRounding);
		}

		// Draw window + handle manual resize
		// As we highlight the title bar when want_focus is set, multiple reappearing windows will have have their title bar highlighted on their reappearing frame.
		const float window_rounding = window->WindowRounding;
		const float window_border_size = window->WindowBorderSize;
		const ImGuiWindow* window_to_highlight = g.NavWindowingTarget ? g.NavWindowingTarget : g.NavWindow;
		const bool title_bar_is_highlight = want_focus || (window_to_highlight && window->RootWindowForTitleBarHighlight == window_to_highlight->RootWindowForTitleBarHighlight);
		const ImRect title_bar_rect = window->TitleBarRect();
		if (window->Collapsed)
		{
			// Title bar only
			float backup_border_size = style.FrameBorderSize;
			g.Style.FrameBorderSize = window->WindowBorderSize;
			ImU32 title_bar_col = GetColorU32((title_bar_is_highlight && !g.NavDisableHighlight) ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBgCollapsed);
			RenderFrame(title_bar_rect.Min, title_bar_rect.Max, title_bar_col, true, window_rounding);
			g.Style.FrameBorderSize = backup_border_size;
		}
		else
		{

			int alpha = 255;
			int alpha_sec = g.Style.secondalpha * 255;
			// Window background
			if (!(flags & ImGuiWindowFlags_NoBackground) && (flags & 31))
			{
				auto draw = window->DrawList;
				if (!(flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiWindowFlags_Tooltip))
				{
					auto ws = window->Size;
					auto wp = window->Pos;
					//main


					//221, 163, 92 -> 117, 187, 253
					draw->AddRectFilled(ImVec2(wp.x, wp.y), ImVec2(wp.x + ws.x, wp.y + ws.y), ImColor(47, 47, 55, alpha), 10);//background

					draw->AddRect(ImVec2(wp.x + 1, wp.y + 1), ImVec2(wp.x + ws.x - 1, wp.y + ws.y - 1), ImColor(72, 79, 87, alpha), 10);//grey outline
					draw->AddRect(ImVec2(wp.x, wp.y), ImVec2(wp.x + ws.x, wp.y + ws.y), ImColor(30, 28, 26, alpha), 10);//black outline

					draw->AddRectFilled(ImVec2(wp.x - 1, wp.y), ImVec2(wp.x + ws.x + 1, wp.y + 12), ImColor(117, 187, 253, alpha), 10);//main line
					draw->AddRectFilled(ImVec2(wp.x, wp.y + 10), ImVec2(wp.x + ws.x, wp.y + 15), ImColor(45, 48, 57));//for main line

					draw->AddRectFilledMultiColor(ImVec2(wp.x, wp.y + 10), ImVec2(wp.x + ws.x, wp.y + 22), ImColor(117, 187, 253, 150), ImColor(117, 187, 253, 150), ImColor(117, 187, 253, 0), ImColor(117, 187, 253, 0));//gradient for line

					//lines
					draw->AddRectFilled(ImVec2(wp.x + 120, wp.y + 24), ImVec2(wp.x + 121, wp.y + 60), ImColor(54, 57, 67));
					draw->AddRectFilled(ImVec2(wp.x + 15, wp.y + 70), ImVec2(wp.x + ws.x - 15, wp.y + 71), ImColor(54, 57, 67));
					draw->AddRectFilled(ImVec2(wp.x + 15, wp.y + ws.y - 30), ImVec2(wp.x + ws.x - 15, wp.y + ws.y - 29), ImColor(54, 57, 67));


					//text

					PushFont(skeet_menu.f1);
					draw->AddText(ImVec2(wp.x - 15 + 52, wp.y + 34), ImColor(214, 216, 226), "shonax");
					draw->AddText(ImVec2(wp.x + 20, wp.y + ws.y - 25), ImColor(214, 216, 226), "credits: shonax");
					PopFont();
				}
				else {
					if (flags & ImGuiWindowFlags_ChildWindow && !(flags & ImGuiWindowFlags_ChildFrame) && !(flags & ImGuiWindowFlags_Tooltip)) {
						ImVec2 textSize = CalcTextSize(name);
						auto ws = window->Size;
						auto wp = window->Pos;
						//200, 187, 169 ->  58, 131, 253
						draw->AddRectFilled(ImVec2(wp.x, wp.y), ImVec2(wp.x + ws.x, wp.y + ws.y), ImColor(47, 47, 55, alpha_sec));
						draw->AddRect(ImVec2(wp.x, wp.y), ImVec2(wp.x + ws.x, wp.y + ws.y), ImColor(57, 59, 69, alpha_sec));
						draw->AddRectFilled(ImVec2(wp.x, wp.y), ImVec2(wp.x + ws.x / 2 - textSize.x / 2 - 5, wp.y + 1), ImColor(58, 131, 253, alpha_sec));
						draw->AddRectFilled(ImVec2(wp.x + ws.x / 2 + textSize.x / 2 + 5, wp.y), ImVec2(wp.x + ws.x, wp.y + 1), ImColor(58, 131, 253, alpha_sec));
					}
					else if (flags & ImGuiWindowFlags_Popup || flags & ImGuiWindowFlags_Tooltip || flags & ImGuiWindowFlags_ChildFrame) {
						ImU32 bg_col = ImColor(32, 32, 38);

						window->DrawList->AddRectFilled(window->Pos + ImVec2(0, window->TitleBarHeight()), window->Pos + window->Size, bg_col, window_rounding, (flags & ImGuiWindowFlags_NoTitleBar) ? ImDrawCornerFlags_All : ImDrawCornerFlags_Bot);

						window->DrawList->AddRect(window->Pos, window->Pos + window->Size, ImColor(117, 187, 253));
					}
				}
			}
			g.NextWindowData.BgAlphaCond = 0;

			// Title bar
			if (!(flags & ImGuiWindowFlags_NoTitleBar))
			{
				ImU32 title_bar_col = GetColorU32(title_bar_is_highlight ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBg);
				window->DrawList->AddRectFilled(title_bar_rect.Min, title_bar_rect.Max, title_bar_col, window_rounding, ImDrawCornerFlags_Top);
			}

			// Menu bar
			if (flags & ImGuiWindowFlags_MenuBar)
			{
				ImRect menu_bar_rect = window->MenuBarRect();
				menu_bar_rect.ClipWith(window->Rect());  // Soft clipping, in particular child window don't have minimum size covering the menu bar so this is useful for them.
				window->DrawList->AddRectFilled(menu_bar_rect.Min, menu_bar_rect.Max, GetColorU32(ImGuiCol_MenuBarBg), (flags & ImGuiWindowFlags_NoTitleBar) ? window_rounding : 0.0f, ImDrawCornerFlags_Top);
				if (style.FrameBorderSize > 0.0f && menu_bar_rect.Max.y < window->Pos.y + window->Size.y)
					window->DrawList->AddLine(menu_bar_rect.GetBL(), menu_bar_rect.GetBR(), GetColorU32(ImGuiCol_Border), style.FrameBorderSize);
			}

			// Scrollbars
			if (window->ScrollbarX)
				ScrollbarOT(ImGuiAxis_X);
			if (window->ScrollbarY)
				ScrollbarOT(ImGuiAxis_Y);
		}

		// Store a backup of SizeFull which we will use next frame to decide if we need scrollbars.
		window->SizeFullAtLastBegin = window->SizeFull;

		// Update various regions. Variables they depends on are set above in this function.
		// FIXME: window->ContentsRegionRect.Max is currently very misleading / partly faulty, but some BeginChild() patterns relies on it.
		window->ContentsRegionRect.Min.x = window->Pos.x - window->Scroll.x + window->WindowPadding.x;
		window->ContentsRegionRect.Min.y = window->Pos.y - window->Scroll.y + window->WindowPadding.y + window->TitleBarHeight() + window->MenuBarHeight();
		window->ContentsRegionRect.Max.x = window->Pos.x - window->Scroll.x - window->WindowPadding.x + (window->SizeContentsExplicit.x != 0.0f ? window->SizeContentsExplicit.x : (window->Size.x - window->ScrollbarSizes.x));
		window->ContentsRegionRect.Max.y = window->Pos.y - window->Scroll.y - window->WindowPadding.y + (window->SizeContentsExplicit.y != 0.0f ? window->SizeContentsExplicit.y : (window->Size.y - window->ScrollbarSizes.y));

		// Setup drawing context
		// (NB: That term "drawing context / DC" lost its meaning a long time ago. Initially was meant to hold transient data only. Nowadays difference between window-> and window->DC-> is dubious.)
		window->DC.Indent.x = 0.0f + window->WindowPadding.x - window->Scroll.x;
		window->DC.GroupOffset.x = 0.0f;
		window->DC.ColumnsOffset.x = 0.0f;
		window->DC.CursorStartPos = window->Pos + ImVec2(window->DC.Indent.x + window->DC.ColumnsOffset.x, window->TitleBarHeight() + window->MenuBarHeight() + window->WindowPadding.y - window->Scroll.y);
		window->DC.CursorPos = window->DC.CursorStartPos;
		window->DC.CursorPosPrevLine = window->DC.CursorPos;
		window->DC.CursorMaxPos = window->DC.CursorStartPos;
		window->DC.CurrentLineSize = window->DC.PrevLineSize = ImVec2(0.0f, 0.0f);
		window->DC.CurrentLineTextBaseOffset = window->DC.PrevLineTextBaseOffset = 0.0f;
		window->DC.NavHideHighlightOneFrame = false;
		window->DC.NavHasScroll = (GetWindowScrollMaxY(window) > 0.0f);
		window->DC.NavLayerActiveMask = window->DC.NavLayerActiveMaskNext;
		window->DC.NavLayerActiveMaskNext = 0x00;
		window->DC.MenuBarAppending = false;
		window->DC.LogLinePosY = window->DC.CursorPos.y - 9999.0f;
		window->DC.ChildWindows.resize(0);
		window->DC.LayoutType = ImGuiLayoutType_Vertical;
		window->DC.ParentLayoutType = parent_window ? parent_window->DC.LayoutType : ImGuiLayoutType_Vertical;
		window->DC.ItemFlags = parent_window ? parent_window->DC.ItemFlags : ImGuiItemFlags_Default_;
		window->DC.ItemWidth = window->ItemWidthDefault;
		window->DC.TextWrapPos = -1.0f; // disabled
		window->DC.ItemFlagsStack.resize(0);
		window->DC.ItemWidthStack.resize(0);
		window->DC.TextWrapPosStack.resize(0);
		window->DC.TreeDepth = 0;
		window->DC.TreeDepthMayJumpToParentOnPop = 0x00;
		window->DC.StateStorage = &window->StateStorage;
		window->DC.GroupStack.resize(0);
		window->MenuColumns.Update(3, style.ItemSpacing.x, window_just_activated_by_user);

		if (flags & ImGuiWindowFlags_Popup)
			window->DC.CursorPos.y += 4;

		if ((flags & ImGuiWindowFlags_ChildWindow) && (window->DC.ItemFlags != parent_window->DC.ItemFlags))
		{
			window->DC.ItemFlags = parent_window->DC.ItemFlags;
			window->DC.ItemFlagsStack.push_back(window->DC.ItemFlags);
		}

		if (window->AutoFitFramesX > 0)
			window->AutoFitFramesX--;
		if (window->AutoFitFramesY > 0)
			window->AutoFitFramesY--;

		// Apply focus (we need to call FocusWindow() AFTER setting DC.CursorStartPos so our initial navigation reference rectangle can start around there)
		if (want_focus)
		{
			FocusWindow(window);
			NavInitWindow(window, false);
		}

		// Title bar
		if (!(flags & ImGuiWindowFlags_NoTitleBar))
		{
			// Close & collapse button are on layer 1 (same as menus) and don't default focus
			const ImGuiItemFlags item_flags_backup = window->DC.ItemFlags;
			window->DC.ItemFlags |= ImGuiItemFlags_NoNavDefaultFocus;
			window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
			window->DC.NavLayerCurrentMask = (1 << ImGuiNavLayer_Menu);

			// Collapse button
			if (!(flags & ImGuiWindowFlags_NoCollapse))
				if (CollapseButton(window->GetID("#COLLAPSE"), window->Pos))
					window->WantCollapseToggle = true; // Defer collapsing to next frame as we are too far in the Begin() function

			// Close button
			if (p_open != NULL)
			{
				const float pad = style.FramePadding.y;
				const float rad = g.FontSize * 0.5f;
				if (CloseButton(window->GetID("#CLOSE"), window->Rect().GetTR() + ImVec2(-pad - rad, pad + rad), rad + 1))
					*p_open = false;
			}

			window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
			window->DC.NavLayerCurrentMask = (1 << ImGuiNavLayer_Main);
			window->DC.ItemFlags = item_flags_backup;

			// Title bar text (with: horizontal alignment, avoiding collapse/close button, optional "unsaved document" marker)
			// FIXME: Refactor text alignment facilities along with RenderText helpers, this is too much code..
			const char* UNSAVED_DOCUMENT_MARKER = "*";
			float marker_size_x = (flags & ImGuiWindowFlags_UnsavedDocument) ? CalcTextSize(UNSAVED_DOCUMENT_MARKER, NULL, false).x : 0.0f;
			ImVec2 text_size = CalcTextSize(name, NULL, true) + ImVec2(marker_size_x, 0.0f);
			ImRect text_r = title_bar_rect;
			float pad_left = (flags & ImGuiWindowFlags_NoCollapse) ? style.FramePadding.x : (style.FramePadding.x + g.FontSize + style.ItemInnerSpacing.x);
			float pad_right = (p_open == NULL) ? style.FramePadding.x : (style.FramePadding.x + g.FontSize + style.ItemInnerSpacing.x);
			if (style.WindowTitleAlign.x > 0.0f)
				pad_right = ImLerp(pad_right, pad_left, style.WindowTitleAlign.x);
			text_r.Min.x += pad_left;
			text_r.Max.x -= pad_right;
			ImRect clip_rect = text_r;
			clip_rect.Max.x = window->Pos.x + window->Size.x - (p_open ? title_bar_rect.GetHeight() - 3 : style.FramePadding.x); // Match the size of CloseButton()

			RenderTextClipped(text_r.Min, text_r.Max, name, NULL, &text_size, style.WindowTitleAlign, &clip_rect);
			if (flags & ImGuiWindowFlags_UnsavedDocument)
			{
				ImVec2 marker_pos = ImVec2(ImMax(text_r.Min.x, text_r.Min.x + (text_r.GetWidth() - text_size.x) * style.WindowTitleAlign.x) + text_size.x, text_r.Min.y) + ImVec2(2 - marker_size_x, 0.0f);
				ImVec2 off = ImVec2(0.0f, (float)(int)(-g.FontSize * 0.25f));
				RenderTextClipped(marker_pos + off, text_r.Max + off, UNSAVED_DOCUMENT_MARKER, NULL, NULL, ImVec2(0, style.WindowTitleAlign.y), &clip_rect);
			}

		}

		// Save clipped aabb so we can access it in constant-time in FindHoveredWindow()
		window->OuterRectClipped = window->Rect();
		window->OuterRectClipped.ClipWith(window->ClipRect);

		// Pressing CTRL+C while holding on a window copy its content to the clipboard
		// This works but 1. doesn't handle multiple Begin/End pairs, 2. recursing into another Begin/End pair - so we need to work that out and add better logging scope.
		// Maybe we can support CTRL+C on every element?
		/*
		if (g.ActiveId == move_id)
			if (g.IO.KeyCtrl && IsKeyPressedMap(ImGuiKey_C))
				LogToClipboard();
		*/

		// Inner rectangle
		// We set this up after processing the resize grip so that our clip rectangle doesn't lag by a frame
		// Note that if our window is collapsed we will end up with an inverted (~null) clipping rectangle which is the correct behavior.
		window->InnerMainRect.Min.x = title_bar_rect.Min.x + window->WindowBorderSize;
		window->InnerMainRect.Min.y = title_bar_rect.Max.y + window->MenuBarHeight() + (((flags & ImGuiWindowFlags_MenuBar) || !(flags & ImGuiWindowFlags_NoTitleBar)) ? style.FrameBorderSize : window->WindowBorderSize);
		window->InnerMainRect.Max.x = window->Pos.x + window->Size.x - window->ScrollbarSizes.x - window->WindowBorderSize;
		window->InnerMainRect.Max.y = window->Pos.y + window->Size.y - window->ScrollbarSizes.y - window->WindowBorderSize;
		//window->DrawList->AddRect(window->InnerRect.Min, window->InnerRect.Max, IM_COL32_WHITE);

		// Inner clipping rectangle
		// Force round operator last to ensure that e.g. (int)(max.x-min.x) in user's render code produce correct result.
		window->InnerClipRect.Min.x = ImFloor(0.5f + window->InnerMainRect.Min.x + ImMax(0.0f, ImFloor(window->WindowPadding.x * 0.5f - window->WindowBorderSize)));
		window->InnerClipRect.Min.y = ImFloor(0.5f + window->InnerMainRect.Min.y);
		window->InnerClipRect.Max.x = ImFloor(0.5f + window->InnerMainRect.Max.x - ImMax(0.0f, ImFloor(window->WindowPadding.x * 0.5f - window->WindowBorderSize)));
		window->InnerClipRect.Max.y = ImFloor(0.5f + window->InnerMainRect.Max.y);

		if (flags & ImGuiWindowFlags_ChildWindow) {
			window->InnerClipRect.Min.y += 2;
			window->InnerClipRect.Max.y -= 2;
		}

		/*if (flags & ImGuiWindowFlags_Popup) {
			window->InnerClipRect.Max.y += 4;
			window->InnerMainRect.Max.y -= 4;
		}*/

		// We fill last item data based on Title Bar, in order for IsItemHovered() and IsItemActive() to be usable after Begin().
		// This is useful to allow creating context menus on title bar only, etc.
		window->DC.LastItemId = window->MoveId;
		window->DC.LastItemStatusFlags = IsMouseHoveringRect(title_bar_rect.Min, title_bar_rect.Max, false) ? ImGuiItemStatusFlags_HoveredRect : 0;
		window->DC.LastItemRect = title_bar_rect;
	}

	PushClipRect(window->InnerClipRect.Min, window->InnerClipRect.Max, true);

	// Clear 'accessed' flag last thing (After PushClipRect which will set the flag. We want the flag to stay false when the default "Debug" window is unused)
	if (first_begin_of_the_frame)
		window->WriteAccessed = false;

	window->BeginCount++;
	g.NextWindowData.Clear();

	if (flags & ImGuiWindowFlags_ChildWindow)
	{
		// Child window can be out of sight and have "negative" clip windows.
		// Mark them as collapsed so commands are skipped earlier (we can't manually collapse them because they have no title bar).
		IM_ASSERT((flags & ImGuiWindowFlags_NoTitleBar) != 0);
		if (!(flags & ImGuiWindowFlags_AlwaysAutoResize) && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0)
			if (window->OuterRectClipped.Min.x >= window->OuterRectClipped.Max.x || window->OuterRectClipped.Min.y >= window->OuterRectClipped.Max.y)
				window->HiddenFramesRegular = 1;

		// Completely hide along with parent or if parent is collapsed
		if (parent_window && (parent_window->Collapsed || parent_window->Hidden))
			window->HiddenFramesRegular = 1;
	}

	// Don't render if style alpha is 0.0 at the time of Begin(). This is arbitrary and inconsistent but has been there for a long while (may remove at some point)
	if (style.Alpha <= 0.0f)
		window->HiddenFramesRegular = 1;

	// Update the Hidden flag
	window->Hidden = (window->HiddenFramesRegular > 0) || (window->HiddenFramesForResize > 0);

	// Return false if we don't intend to display anything to allow user to perform an early out optimization
	window->SkipItems = (window->Collapsed || !window->Active || window->Hidden) && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0 && window->HiddenFramesForResize <= 0;
	//PopFont();
	return !window->SkipItems;
}


ImGuiID ui::GetScrollbarID(ImGuiWindow* window, ImGuiAxis axis)
{
	return window->GetIDNoKeepAlive(axis == ImGuiAxis_X ? "#SCROLLX" : "#SCROLLY");
}

void ui::ScrollbarOT(ImGuiAxis direction)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;

	const bool horizontal = (direction == ImGuiLayoutType_Horizontal);
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = GetScrollbarID(window, direction);

	//    // Render background
	bool other_scrollbar = (horizontal ? window->ScrollbarY : window->ScrollbarX);
	float other_scrollbar_size_w = other_scrollbar ? style.ScrollbarSize : 0.0f;
	const ImRect window_rect = window->Rect();
	const float border_size = window->WindowBorderSize;
	ImRect bb = horizontal
		? ImRect(window->Pos.x + border_size, window_rect.Max.y - style.ScrollbarSize, window_rect.Max.x - other_scrollbar_size_w - border_size, window_rect.Max.y - border_size)
		: ImRect(window_rect.Max.x - style.ScrollbarSize, window->Pos.y + border_size, window_rect.Max.x - border_size, window_rect.Max.y - other_scrollbar_size_w - border_size);
	if (!horizontal)
		bb.Min.y += window->TitleBarHeight() + ((window->Flags & ImGuiWindowFlags_MenuBar) ? window->MenuBarHeight() : 0.0f);

	const float bb_height = bb.GetHeight();
	if (bb.GetWidth() <= 0.0f || bb_height <= 0.0f)
		return;

	// When we are too small, start hiding and disabling the grab (this reduce visual noise on very small window and facilitate using the resize grab)
	float alpha = 1.0f;
	if ((direction == ImGuiLayoutType_Vertical) && bb_height < g.FontSize + g.Style.FramePadding.y * 2.0f)
	{
		alpha = ImSaturate((bb_height - g.FontSize) / (g.Style.FramePadding.y * 2.0f));
		if (alpha <= 0.0f)
			return;
	}
	const bool allow_interaction = (alpha >= 1.0f);

	int window_rounding_corners;
	if (horizontal)
		window_rounding_corners = ImDrawCornerFlags_BotLeft | (other_scrollbar ? 0 : ImDrawCornerFlags_BotRight);
	else
		window_rounding_corners = (((window->Flags & ImGuiWindowFlags_NoTitleBar) && !(window->Flags & ImGuiWindowFlags_MenuBar)) ? ImDrawCornerFlags_TopRight : 0) | (other_scrollbar ? 0 : ImDrawCornerFlags_BotRight);
	window->DrawList->AddRectFilled(bb.Min + ImVec2(0, 1), bb.Max, GetColorU32(ImGuiCol_ScrollbarBg), window->WindowRounding, window_rounding_corners);
	bb.Expand(ImVec2(-ImClamp((float)(int)((bb.Max.x - bb.Min.x - 2.0f) * 0.5f), 0.0f, 3.0f), -ImClamp((float)(int)((bb.Max.y - bb.Min.y - 2.0f) * 0.5f), 0.0f, 3.0f)));

	// V denote the main, longer axis of the scrollbar (= height for a vertical scrollbar)
	float scrollbar_size_v = horizontal ? bb.GetWidth() : bb.GetHeight();
	float scroll_v = horizontal ? window->Scroll.x : window->Scroll.y;
	float win_size_avail_v = (horizontal ? window->SizeFull.x : window->SizeFull.y) - other_scrollbar_size_w;
	float win_size_contents_v = horizontal ? window->SizeContents.x : window->SizeContents.y;

	// Calculate the height of our grabbable box. It generally represent the amount visible (vs the total scrollable amount)
	// But we maintain a minimum size in pixel to allow for the user to still aim inside.
	IM_ASSERT(ImMax(win_size_contents_v, win_size_avail_v) > 0.0f); // Adding this assert to check if the ImMax(XXX,1.0f) is still needed. PLEASE CONTACT ME if this triggers.
	const float win_size_v = ImMax(ImMax(win_size_contents_v, win_size_avail_v), 1.0f);
	const float grab_h_pixels = ImClamp(scrollbar_size_v * (win_size_avail_v / win_size_v), style.GrabMinSize, scrollbar_size_v);
	const float grab_h_norm = grab_h_pixels / scrollbar_size_v;

	// Handle input right away. None of the code of Begin() is relying on scrolling position before calling Scrollbar().
	bool held = false;
	bool hovered = false;
	const bool previously_held = (g.ActiveId == id);
	ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_NoNavFocus);

	float scroll_max = ImMax(1.0f, win_size_contents_v - win_size_avail_v);
	float scroll_ratio = ImSaturate(scroll_v / scroll_max);
	float grab_v_norm = scroll_ratio * (scrollbar_size_v - grab_h_pixels) / scrollbar_size_v;
	if (held && allow_interaction && grab_h_norm < 1.0f)
	{
		float scrollbar_pos_v = horizontal ? bb.Min.x : bb.Min.y;
		float mouse_pos_v = horizontal ? g.IO.MousePos.x : g.IO.MousePos.y;
		float* click_delta_to_grab_center_v = horizontal ? &g.ScrollbarClickDeltaToGrabCenter.x : &g.ScrollbarClickDeltaToGrabCenter.y;

		// Click position in scrollbar normalized space (0.0f->1.0f)
		const float clicked_v_norm = ImSaturate((mouse_pos_v - scrollbar_pos_v) / scrollbar_size_v);
		SetHoveredID(id);

		bool seek_absolute = false;
		if (!previously_held)
		{
			// On initial click calculate the distance between mouse and the center of the grab
			if (clicked_v_norm >= grab_v_norm && clicked_v_norm <= grab_v_norm + grab_h_norm)
			{
				*click_delta_to_grab_center_v = clicked_v_norm - grab_v_norm - grab_h_norm * 0.5f;
			}
			else
			{
				seek_absolute = true;
				*click_delta_to_grab_center_v = 0.0f;
			}
		}

		// Apply scroll
		// It is ok to modify Scroll here because we are being called in Begin() after the calculation of SizeContents and before setting up our starting position
		const float scroll_v_norm = ImSaturate((clicked_v_norm - *click_delta_to_grab_center_v - grab_h_norm * 0.5f) / (1.0f - grab_h_norm));
		scroll_v = (float)(int)(0.5f + scroll_v_norm * scroll_max);//(win_size_contents_v - win_size_v));
		if (horizontal)
			window->Scroll.x = scroll_v;
		else
			window->Scroll.y = scroll_v;

		// Update values for rendering
		scroll_ratio = ImSaturate(scroll_v / scroll_max);
		grab_v_norm = scroll_ratio * (scrollbar_size_v - grab_h_pixels) / scrollbar_size_v;

		// Update distance to grab now that we have seeked and saturated
		if (seek_absolute)
			*click_delta_to_grab_center_v = clicked_v_norm - grab_v_norm - grab_h_norm * 0.5f;
	}

	// Render grab
	const ImU32 grab_col = GetColorU32(ImGuiCol_ScrollbarGrab, alpha);
	ImRect grab_rect;
	if (horizontal)
		grab_rect = ImRect(ImLerp(bb.Min.x, bb.Max.x, grab_v_norm), bb.Min.y, ImMin(ImLerp(bb.Min.x, bb.Max.x, grab_v_norm) + grab_h_pixels, window_rect.Max.x), bb.Max.y);
	else
		grab_rect = ImRect(bb.Min.x - 1, ImLerp(bb.Min.y, bb.Max.y, grab_v_norm), bb.Max.x + 1, ImMin(ImLerp(bb.Min.y, bb.Max.y, grab_v_norm) + grab_h_pixels, window_rect.Max.y));
	window->DrawList->AddRectFilled(grab_rect.Min, grab_rect.Max, grab_col, style.ScrollbarRounding);




}





void ui::TabButtonOT(const char* label, int* selected, int num, int total) {
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);

	ImVec2 pos = window->Pos + ImVec2(160, 30) + ImVec2((window->Size.x - 180) / total * num, 0);
	ImVec2 size = ImVec2((window->Size.x - 180) / total, 25);

	const ImRect bb(pos, pos + size);
	ItemSize(size, style.FramePadding.y);
	if (!ItemAdd(bb, id))
		return;

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, 0);
	if (pressed)
		MarkItemEdited(id);

	if (pressed)
		*selected = num;
	static int alpha = 255;
	if (alpha > 255)
		alpha = 255;
	if (alpha < 0)
		alpha = 0;

	if (*selected == num)
	{
		window->DrawList->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + size.x, pos.y + size.y), ImColor(32, 32, 38, 255), 5);
	}

	ImColor textColor = ImColor(214, 216, 226);

	PushFont(skeet_menu.f1);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);
	window->DrawList->AddText(ImVec2(pos.x + size.x / 2 - label_size.x / 2, pos.y + size.y / 2 - label_size.y / 2 - 1 + 2), textColor, label);
	PopFont();
}


void ui::EndOT()
{
	ImGuiContext& g = *GImGui;

	if (g.CurrentWindowStack.Size <= 1 && g.FrameScopePushedImplicitWindow)
	{
		IM_ASSERT(g.CurrentWindowStack.Size > 1 && "Calling End() too many times!");
		return; // FIXME-ERRORHANDLING
	}
	IM_ASSERT(g.CurrentWindowStack.Size > 0);

	ImGuiWindow* window = g.CurrentWindow;

	if (window->DC.ColumnsSet != NULL)
		EndColumns();
	PopClipRect();   // Inner window clip rectangle
	if (window->Flags & ImGuiWindowFlags_ChildWindow && !(window->Flags & ImGuiWindowFlags_ChildFrame))
	{
		ui::PushFont(skeet_menu.f1);
		ImVec2 ts = CalcTextSize(window->Name);
		window->DrawList->AddText(window->Pos + ImVec2(window->Size.x / 2 - ts.x / 2, ts.y / -2), ImColor(205 / 255.f, 205 / 255.f, 205 / 255.f, g.Style.Alpha), window->Name);
		ui::PopFont();
	}
	// Stop logging
	if (!(window->Flags & ImGuiWindowFlags_ChildWindow))    // FIXME: add more options for scope of logging
		LogFinish();

	// Pop from window stack
	g.CurrentWindowStack.pop_back();
	if (window->Flags & ImGuiWindowFlags_Popup)
		g.BeginPopupStack.pop_back();
	CheckStacksSize(window, false);
	SetCurrentWindow(g.CurrentWindowStack.empty() ? NULL : g.CurrentWindowStack.back());
}


static bool ui::BeginChildExOT(const char* name, ImGuiID id, const ImVec2& size_arg, bool border, ImGuiWindowFlags flags)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* parent_window = g.CurrentWindow;

	flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_ChildWindow;
	flags |= (parent_window->Flags & ImGuiWindowFlags_NoMove);  // Inherit the NoMove flag

	// Size
	const ImVec2 content_avail = GetContentRegionAvail();
	ImVec2 size = ImFloor(size_arg);
	const int auto_fit_axises = ((size.x == 0.0f) ? (1 << ImGuiAxis_X) : 0x00) | ((size.y == 0.0f) ? (1 << ImGuiAxis_Y) : 0x00);
	if (size.x <= 0.0f)
		size.x = ImMax(content_avail.x + size.x, 4.0f); // Arbitrary minimum child size (0.0f causing too much issues)
	if (size.y <= 0.0f)
		size.y = ImMax(content_avail.y + size.y, 4.0f);
	SetNextWindowSize(size);

	// Build up name. If you need to append to a same child from multiple location in the ID stack, use BeginChild(ImGuiID id) with a stable value.
	char title[256];
	ImFormatString(title, IM_ARRAYSIZE(title), "%s", name);

	const float backup_border_size = g.Style.ChildBorderSize;
	if (!border)
		g.Style.ChildBorderSize = 0.0f;
	bool ret = BeginOT(title, NULL, flags);
	g.Style.ChildBorderSize = backup_border_size;

	ImGuiWindow* child_window = g.CurrentWindow;
	child_window->ChildId = id;
	child_window->AutoFitChildAxises = auto_fit_axises;

	// Set the cursor to handle case where the user called SetNextWindowPos()+BeginChild() manually.
	// While this is not really documented/defined, it seems that the expected thing to do.
	if (child_window->BeginCount == 1)
		parent_window->DC.CursorPos = child_window->Pos;

	if (!(flags & ImGuiWindowFlags_ChildFrame))
		child_window->DC.CursorPos += ImVec2(16, 16);



	// Process navigation-in immediately so NavInit can run on first frame
	if (g.NavActivateId == id && !(flags & ImGuiWindowFlags_NavFlattened) && (child_window->DC.NavLayerActiveMask != 0 || child_window->DC.NavHasScroll))
	{
		FocusWindow(child_window);
		NavInitWindow(child_window, false);
		SetActiveID(id + 1, child_window); // Steal ActiveId with a dummy id so that key-press won't activate child item
		g.ActiveIdSource = ImGuiInputSource_Nav;
	}
	return ret;
}

bool ui::BeginChildOT(const char* str_id, const ImVec2& size_arg, bool border, ImGuiWindowFlags extra_flags)
{
	ImGuiWindow* window = GetCurrentWindow();
	return BeginChildExOT(str_id, window->GetID(str_id), size_arg, border, extra_flags);
}

void ui::EndChildOT()
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;

	IM_ASSERT(window->Flags & ImGuiWindowFlags_ChildWindow);   // Mismatched BeginChild()/EndChild() callss

	ItemSize(ImVec2(0, 11));
	if (window->BeginCount > 1)
	{
		EndOT();
	}
	else
	{
		ImVec2 sz = window->Size;
		if (window->AutoFitChildAxises & (1 << ImGuiAxis_X)) // Arbitrary minimum zero-ish child size of 4.0f causes less trouble than a 0.0f
			sz.x = ImMax(4.0f, sz.x);
		if (window->AutoFitChildAxises & (1 << ImGuiAxis_Y))
			sz.y = ImMax(4.0f, sz.y);
		EndOT();

		ImGuiWindow* parent_window = g.CurrentWindow;
		ImRect bb(parent_window->DC.CursorPos, parent_window->DC.CursorPos + sz);
		ItemSize(sz);
		if ((window->DC.NavLayerActiveMask != 0 || window->DC.NavHasScroll) && !(window->Flags & ImGuiWindowFlags_NavFlattened))
		{
			ItemAdd(bb, window->ChildId);
			RenderNavHighlight(bb, window->ChildId);

			// When browsing a window that has no activable items (scroll only) we keep a highlight on the child
			if (window->DC.NavLayerActiveMask == 0 && window == g.NavWindow)
				RenderNavHighlight(ImRect(bb.Min - ImVec2(2, 2), bb.Max + ImVec2(2, 2)), g.NavId, ImGuiNavHighlightFlags_TypeThin);
		}
		else
		{
			// Not navigable into
			ItemAdd(bb, 0);
		}
	}
}

bool ui::SliderScalarOT(const char* label, ImGuiDataType data_type, void* v, const void* v_min, const void* v_max, const char* format, float power, int remove_from_fmt)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const float w = ImClamp(window->Size.x - 64, 155.f, 200.f);

	const ImVec2 label_size = CalcTextSize(label, NULL, true);
	const ImRect frame_bb(window->DC.CursorPos + ImVec2(16, 0), window->DC.CursorPos + ImVec2(w, label_size.x > 0 ? label_size.y + 10 : 10) + ImVec2(16, 0));
	const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id, &frame_bb))
		return false;

	// Default format string when passing NULL
	// Patch old "%.0f" format string to use "%d", read function comments for more details.
	IM_ASSERT(data_type >= 0 && data_type < ImGuiDataType_COUNT);
	if (format == NULL)
		format = GDataTypeInfo[data_type].PrintFmt;
	else if (data_type == ImGuiDataType_S32 && strcmp(format, "%d") != 0)
		format = PatchFormatStringFloatToInt(format);

	// Tabbing or CTRL-clicking on Slider turns it into an input box
	bool start_text_input = false;
	const bool tab_focus_requested = FocusableItemRegister(window, id);
	const bool hovered = ItemHoverable(frame_bb, id);
	if (tab_focus_requested || (hovered && g.IO.MouseClicked[0]) || g.NavActivateId == id || (g.NavInputId == id && g.ScalarAsInputTextId != id))
	{
		SetActiveID(id, window);
		SetFocusID(id, window);
		FocusWindow(window);
		g.ActiveIdAllowNavDirFlags = (1 << ImGuiDir_Up) | (1 << ImGuiDir_Down);
	}

	if (std::string(label).at(0) != '#' && std::string(label).at(1) != '#')
		window->DrawList->AddText(frame_bb.Min, ImColor(205 / 255.f, 205 / 255.f, 205 / 255.f, g.Style.Alpha), label);

	ImRect slider_bb = ImRect(frame_bb.Min + ImVec2(0, label_size.x > 0 ? label_size.y + 3 : 3), frame_bb.Max);

	int percent = 0;
	ImRect grab_bb;
	const bool value_changed = SliderBehavior(frame_bb, id, data_type, v, v_min, v_max, format, power, ImGuiSliderFlags_None, &grab_bb, &percent);
	if (value_changed)
		MarkItemEdited(id);

	window->DrawList->AddRectFilled(slider_bb.Min, slider_bb.Max, ImColor(30, 33, 40), 50);

	window->DrawList->AddRectFilled(slider_bb.Min, ImVec2(slider_bb.Min.x + floorf((float)percent / 100.f * (slider_bb.Max.x - slider_bb.Min.x)), slider_bb.Max.y), ImColor(117, 187, 253), 50);
	//117, 187, 253
	if (IsItemHovered())
		window->DrawList->AddRect(slider_bb.Min, slider_bb.Max, ImColor(117, 187, 253), 50);
	else
		window->DrawList->AddRect(slider_bb.Min, slider_bb.Max, ImColor(58, 61, 68, 255), 50);

	if (data_type == ImGuiDataType_S32)
		*(int*)v -= remove_from_fmt;

	char value_buf[64];
	const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, v, format);
	auto textSize = CalcTextSize(value_buf);

	if (data_type == ImGuiDataType_S32)
		*(int*)v += remove_from_fmt;

	if (IsItemHovered())
	{
		//window->DrawList->AddRectFilled(ImVec2(slider_bb.Min.x + 10 + floorf((float)percent / 100.f * (slider_bb.Max.x - slider_bb.Min.x)) - textSize.x / 2 - 5, slider_bb.Max.y - 4 - textSize.y / 2 - 5), ImVec2(slider_bb.Min.x + floorf((float)percent / 100.f * (slider_bb.Max.x - slider_bb.Min.x)) - textSize.x / 2 + 30, slider_bb.Max.y - textSize.y / 2 + 15), ImColor(30, 33, 40), 5);
		//window->DrawList->AddRect(ImVec2(slider_bb.Min.x + 10 + floorf((float)percent / 100.f * (slider_bb.Max.x - slider_bb.Min.x)) - textSize.x / 2 - 5, slider_bb.Max.y - 4 - textSize.y / 2 - 5), ImVec2(slider_bb.Min.x + floorf((float)percent / 100.f * (slider_bb.Max.x - slider_bb.Min.x)) - textSize.x / 2 + 30, slider_bb.Max.y - textSize.y / 2 + 15), ImColor(58, 61, 68, 255), 5);

		PushFont(skeet_menu.f3);
		window->DrawList->AddText(ImVec2(slider_bb.Min.x + 10 + floorf((float)percent / 100.f * (slider_bb.Max.x - slider_bb.Min.x)) - textSize.x / 2, slider_bb.Max.y - 4 - textSize.y / 2 - 1), ImColor(255 / 255.f, 255 / 255.f, 255 / 255.f, g.Style.Alpha), value_buf);
		PopFont();
	}
	
	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags);
	return value_changed;
}
bool ui::SliderFloatOT(const char* label, float* v, float v_min, float v_max, const char* format, float power)
{
	return SliderScalarOT(label, ImGuiDataType_Float, v, &v_min, &v_max, format, power, 0);
}
bool ui::SliderIntOT(const char* label, int* v, float v_min, float v_max, const char* format, float power)
{
	return SliderScalarOT(label, ImGuiDataType_Float, v, &v_min, &v_max, format, power, 0);
}

bool ui::CheckboxOT(const char* label, bool* v)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	const ImVec2 pos = window->DC.CursorPos;
	const ImRect total_bb(pos, pos + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, label_size.y));
	ItemSize(total_bb, style.FramePadding.y + 10);
	if (!ItemAdd(total_bb, id))
		return false;

	bool hovered, held;
	bool pressed = ButtonBehavior(total_bb, id, &hovered, &held, 0);
	if (pressed)
	{
		*v = !(*v);
		MarkItemEdited(id);
	}
	//117, 187, 253
	if (*v)
	{
		window->DrawList->AddRectFilled(pos, pos + ImVec2(13, 13), ImColor(117 / 255.f, 187 / 255.f, 253 / 255.f, g.Style.Alpha), 3);
	}
	else
	{
		if (hovered)
		{
			window->DrawList->AddRectFilled(pos, pos + ImVec2(13, 13), ImColor(30 / 255.f, 32 / 255.f, 42 / 255.f, g.Style.Alpha), 25);
			window->DrawList->AddRect(pos, pos + ImVec2(13, 13), ImColor(117, 187, 253), 3);
		}
		else
		{
			window->DrawList->AddRectFilled(pos, pos + ImVec2(13, 13), ImColor(30 / 255.f, 32 / 255.f, 42 / 255.f, g.Style.Alpha), 5);
			window->DrawList->AddRect(pos, pos + ImVec2(13, 13), ImColor(54, 57, 67), 3);
		}
	}
	window->DrawList->AddText(pos + ImVec2(16, 0), ImColor(205 / 255.f, 205 / 255.f, 205 / 255.f, g.Style.Alpha), label);

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
	return pressed;
}


bool ui::BeginComboOT(const char* label, const char* preview_value, ImGuiComboFlags flags, int items)
{
	// Always consume the SetNextWindowSizeConstraint() call in our early return paths
	ImGuiContext& g = *GImGui;
	ImGuiCond backup_next_window_size_constraint = g.NextWindowData.SizeConstraintCond;
	g.NextWindowData.SizeConstraintCond = 0;

	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	IM_ASSERT((flags & (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)) != (ImGuiComboFlags_NoArrowButton | ImGuiComboFlags_NoPreview)); // Can't use both flags together

	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);

	const float arrow_size = (flags & ImGuiComboFlags_NoArrowButton) ? 0.0f : GetFrameHeight();
	const ImVec2 label_size = CalcTextSize(label, NULL, true);
	const float w = ImClamp(window->Size.x - 48, 155.f, 200.f + 16.f);
	const ImRect frame_bb(window->DC.CursorPos + ImVec2(16, 0), window->DC.CursorPos + ImVec2(w, label_size.x > 0 ? label_size.y + 25 : 25));
	const ImRect total_bb(frame_bb.Min, frame_bb.Max);
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id, &frame_bb))
		return false;

	bool hovered, held;
	bool pressed = ButtonBehavior(frame_bb, id, &hovered, &held, 0);
	bool popup_open = IsPopupOpen(id);

	const ImRect value_bb(frame_bb.Min, frame_bb.Max - ImVec2(arrow_size, 0.0f));
	ImRect cb_bb = ImRect(frame_bb.Min + ImVec2(0, label_size.x > 0 ? label_size.y + 5 : 5), frame_bb.Max);

	window->DrawList->AddRectFilled(cb_bb.Min, cb_bb.Max, ImColor(32, 32, 38), 3);
	if (hovered || popup_open)
	{
		window->DrawList->AddRect(cb_bb.Min, cb_bb.Max, ImColor(117, 187, 253), 3);
	}
	else
	{
		window->DrawList->AddRect(cb_bb.Min, cb_bb.Max, ImColor(54, 57, 67), 3);
	}

	if (label_size.x > 0)
		window->DrawList->AddText(frame_bb.Min + ImVec2(0, 2 + 5), ImColor(205 / 255.f, 205 / 255.f, 205 / 255.f, g.Style.Alpha), label);

	window->DrawList->PushClipRect(cb_bb.Min, cb_bb.Max - ImVec2(20, 0), true);
	window->DrawList->AddText(cb_bb.Min + ImVec2(8, 0 + 5), ImColor(205 / 255.f, 205 / 255.f, 205 / 255.f, g.Style.Alpha), preview_value);
	window->DrawList->PopClipRect();

	if (!popup_open)
		RenderArrow(cb_bb.Max - ImVec2(16, 16), ImGuiDir_Down, 1.0f);
	else
		RenderArrow(cb_bb.Max - ImVec2(16, 16), ImGuiDir_Up, 1.0f);

	if ((pressed || g.NavActivateId == id) && !popup_open)
	{
		if (window->DC.NavLayerCurrent == 0)
			window->NavLastIds[0] = id;
		OpenPopupEx(id);
		popup_open = true;
	}

	if (!popup_open)
		return false;

	SetNextWindowSize(ImVec2(w - 16, CalcMaxPopupHeightFromItemCount(items) + 50));

	char name[16];
	ImFormatString(name, IM_ARRAYSIZE(name), "##Combo_%02d", g.BeginPopupStack.Size); // Recycle windows based on depth

	// Peak into expected window size so we can position it
	if (ImGuiWindow* popup_window = FindWindowByName(name))
		if (popup_window->WasActive)
		{
			ImVec2 size_expected = CalcWindowExpectedSize(popup_window);
			if (flags & ImGuiComboFlags_PopupAlignLeft)
				popup_window->AutoPosLastDirection = ImGuiDir_Left;
			ImRect r_outer = GetWindowAllowedExtentRect(popup_window);
			ImVec2 pos = FindBestWindowPosForPopupEx(frame_bb.GetBL(), size_expected, &popup_window->AutoPosLastDirection, r_outer, frame_bb, ImGuiPopupPositionPolicy_ComboBox);
			SetNextWindowPos(pos);
		}

	// Horizontally align ourselves with the framed text
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
	PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(style.FramePadding.x, style.WindowPadding.y));
	bool ret = BeginOT(name, NULL, window_flags);
	PopStyleVar();
	if (!ret)
	{
		EndPopup();
		IM_ASSERT(0);   // This should never happen as we tested for IsPopupOpen() above
		return false;
	}
	return true;
}

void ui::EndPopupOT()
{
	ImGuiContext& g = *GImGui;
	IM_ASSERT(g.CurrentWindow->Flags & ImGuiWindowFlags_Popup);  // Mismatched BeginPopup()/EndPopup() calls
	IM_ASSERT(g.BeginPopupStack.Size > 0);

	// Make all menus and popups wrap around for now, may need to expose that policy.
	NavMoveRequestTryWrapping(g.CurrentWindow, ImGuiNavMoveFlags_LoopY);

	EndOT();
}
void ui::EndComboOT()
{
	EndPopupOT();
}

bool ui::SelectableOT(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size_arg)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.ColumnsSet) // FIXME-OPT: Avoid if vertically clipped.
		PopClipRect();

	ImGuiID id = window->GetID(label);
	ImVec2 label_size = CalcTextSize(label, NULL, true);
	ImVec2 size(size_arg.x != 0.0f ? size_arg.x : label_size.x, size_arg.y != 0.0f ? size_arg.y : label_size.y + 2);
	ImVec2 pos = window->DC.CursorPos;
	pos.y += window->DC.CurrentLineTextBaseOffset;
	ImRect bb_inner(pos, pos + size);
	ItemSize(bb_inner);

	// Fill horizontal space.
	ImVec2 window_padding = window->WindowPadding;
	float max_x = (flags & ImGuiSelectableFlags_SpanAllColumns) ? GetWindowContentRegionMax().x : GetContentRegionMax().x;
	float w_draw = ImMax(label_size.x, window->Pos.x + max_x - window_padding.x - pos.x);
	ImVec2 size_draw((size_arg.x != 0 && !(flags & ImGuiSelectableFlags_DrawFillAvailWidth)) ? size_arg.x : w_draw, size_arg.y != 0.0f ? size_arg.y : size.y);
	ImRect bb(pos, pos + size_draw);
	if (size_arg.x == 0.0f || (flags & ImGuiSelectableFlags_DrawFillAvailWidth))
		bb.Max.x += window_padding.x;

	// Selectables are tightly packed together, we extend the box to cover spacing between selectable.
	float spacing_L = (float)(int)(style.ItemSpacing.x * 0.5f);
	float spacing_U = (float)(int)(style.ItemSpacing.y * 0.5f);
	float spacing_R = style.ItemSpacing.x - spacing_L;
	float spacing_D = style.ItemSpacing.y - spacing_U;
	bb.Min.x -= spacing_L;
	bb.Min.y -= spacing_U;
	bb.Max.x += spacing_R;
	bb.Max.y += spacing_D;
	if (!ItemAdd(bb, id))
	{
		if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.ColumnsSet)
			PushColumnClipRect();
		return false;
	}

	// We use NoHoldingActiveID on menus so user can click and _hold_ on a menu then drag to browse child entries
	ImGuiButtonFlags button_flags = 0;
	if (flags & ImGuiSelectableFlags_NoHoldingActiveID) button_flags |= ImGuiButtonFlags_NoHoldingActiveID;
	if (flags & ImGuiSelectableFlags_PressedOnClick) button_flags |= ImGuiButtonFlags_PressedOnClick;
	if (flags & ImGuiSelectableFlags_PressedOnRelease) button_flags |= ImGuiButtonFlags_PressedOnRelease;
	if (flags & ImGuiSelectableFlags_Disabled) button_flags |= ImGuiButtonFlags_Disabled;
	if (flags & ImGuiSelectableFlags_AllowDoubleClick) button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick;
	if (flags & ImGuiSelectableFlags_Disabled)
		selected = false;

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, button_flags);
	// Hovering selectable with mouse updates NavId accordingly so navigation can be resumed with gamepad/keyboard (this doesn't happen on most widgets)
	if (pressed || hovered)
		if (!g.NavDisableMouseHover && g.NavWindow == window && g.NavLayer == window->DC.NavLayerCurrent)
		{
			g.NavDisableHighlight = true;
			SetNavID(id, window->DC.NavLayerCurrent);
		}

	if (pressed)
		MarkItemEdited(id);

	window->DrawList->AddRectFilled(bb.Min, bb.Max, ImColor(32, 32, 38));

	window->DrawList->PushClipRect(bb.Min, bb.Max, true);
	auto textcolor = ImColor(0, 0, 0, 255);


	//221, 155, 88 -> 117, 187, 253
	if (hovered)
	{
		textcolor = ImColor(117, 187, 253);
	}
	else if (selected)
	{
		textcolor = ImColor(117, 187, 253);
	}
	else
	{
		textcolor = ImColor(150, 150, 150);
	}
	window->DrawList->AddText(bb.Min + ImVec2(8, size.y / 2 - label_size.y / 2 + 2), textcolor, label);
	window->DrawList->PopClipRect();

	if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.ColumnsSet)
	{
		PushColumnClipRect();
		bb.Max.x -= (GetContentRegionMax().x - max_x);
	}

	// Automatically close popups
	if (pressed && (window->Flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiSelectableFlags_DontClosePopups) && !(window->DC.ItemFlags & ImGuiItemFlags_SelectableDontClosePopup))
		CloseCurrentPopup();
	return pressed;
}


bool ui::ComboOT(const char* label, int* current_item, std::vector<const char*> items) {
	*current_item = ImClamp(*current_item, 0, int(items.size() - 1));

	int old_item = *current_item;

	if (ui::BeginComboOT(label, items.at(*current_item), 0, items.size())) {
		for (int i = 0; i < items.size(); i++)
			if (ui::SelectableOT(items.at(i), *current_item == i))
				*current_item = i;

		ui::EndComboOT();
	}
	return old_item != *current_item;
}



void ui::TabButton_littOT(const char* label, const char* label2, int* selected, int num, int total, ImFont* whatever) {
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	//const ImVec2 label_size = CalcTextSize(label, NULL, true);

	ImVec2 pos = window->Pos + ImVec2(15, 80) + ImVec2((window->Size.x - 30) / total * num, 0);
	ImVec2 size = ImVec2((window->Size.x - 30) / total, 50);

	const ImRect bb(pos, pos + size);
	ItemSize(size, style.FramePadding.y);
	if (!ItemAdd(bb, id))
		return;

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, 0);
	if (pressed)
		MarkItemEdited(id);

	if (pressed)
		*selected = num;

	ImColor textColor = ImColor(80 / 255.f, 80 / 255.f, 80 / 255.f, 255 / 255.f);
	if (hovered)
		textColor = ImColor(140 / 255.f, 140 / 255.f, 140 / 255.f, 255 / 255.f);
	if (*selected == num)
		textColor = ImColor(170 / 255.f, 170 / 255.f, 170 / 255.f, 255 / 255.f);

	ui::PushFont(whatever);
	const ImVec2 label_size2 = CalcTextSize(label2, NULL, true);
	window->DrawList->AddText(ImVec2(pos.x + size.x / 2 - label_size2.x / 2, pos.y + size.y / 2 - label_size2.y / 2 - 1 - 5), textColor, label2);
	ui::PopFont();

	ui::PushFont(skeet_menu.f1);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);
	window->DrawList->AddText(ImVec2(pos.x + size.x / 2 - label_size.x / 2, pos.y + size.y / 2 - label_size.y / 2 - 1 + 10), textColor, label);
	ui::PopFont();
}


























static void ui::RenderOuterBorders(ImGuiWindow* window)
{
	ImGuiContext& g = *GImGui;
	float rounding = window->WindowRounding;
	float border_size = window->WindowBorderSize;
	if (border_size > 0.0f && !(window->Flags & ImGuiWindowFlags_NoBackground))
		window->DrawList->AddRect(window->Pos, window->Pos + window->Size, GetColorU32(ImGuiCol_Border), rounding, ImDrawCornerFlags_All, border_size);

	int border_held = window->ResizeBorderHeld;
	if (border_held != -1)
	{
		struct ImGuiResizeBorderDef
		{
			ImVec2 InnerDir;
			ImVec2 CornerPosN1, CornerPosN2;
			float  OuterAngle;
		};
		static const ImGuiResizeBorderDef resize_border_def[4] =
		{
			{ ImVec2(0,+1), ImVec2(0,0), ImVec2(1,0), IM_PI * 1.50f }, // Top
			{ ImVec2(-1,0), ImVec2(1,0), ImVec2(1,1), IM_PI * 0.00f }, // Right
			{ ImVec2(0,-1), ImVec2(1,1), ImVec2(0,1), IM_PI * 0.50f }, // Bottom
			{ ImVec2(+1,0), ImVec2(0,1), ImVec2(0,0), IM_PI * 1.00f }  // Left
		};
		const ImGuiResizeBorderDef& def = resize_border_def[border_held];
		ImRect border_r = GetResizeBorderRect(window, border_held, rounding, 0.0f);
		window->DrawList->PathArcTo(ImLerp(border_r.Min, border_r.Max, def.CornerPosN1) + ImVec2(0.5f, 0.5f) + def.InnerDir * rounding, rounding, def.OuterAngle - IM_PI * 0.25f, def.OuterAngle);
		window->DrawList->PathArcTo(ImLerp(border_r.Min, border_r.Max, def.CornerPosN2) + ImVec2(0.5f, 0.5f) + def.InnerDir * rounding, rounding, def.OuterAngle, def.OuterAngle + IM_PI * 0.25f);
		window->DrawList->PathStroke(GetColorU32(ImGuiCol_SeparatorActive), false, ImMax(2.0f, border_size)); // Thicker than usual
	}
	if (g.Style.FrameBorderSize > 0 && !(window->Flags & ImGuiWindowFlags_NoTitleBar))
	{
		float y = window->Pos.y + window->TitleBarHeight() - 1;
		window->DrawList->AddLine(ImVec2(window->Pos.x + border_size, y), ImVec2(window->Pos.x + window->Size.x - border_size, y), GetColorU32(ImGuiCol_Border), g.Style.FrameBorderSize);
	}
}





static inline void ClampWindowRect(ImGuiWindow* window, const ImRect& rect, const ImVec2& padding)
{
	ImGuiContext& g = *GImGui;
	ImVec2 size_for_clamping = (g.IO.ConfigWindowsMoveFromTitleBarOnly && !(window->Flags & ImGuiWindowFlags_NoTitleBar)) ? ImVec2(window->Size.x, window->TitleBarHeight()) : window->Size;
	window->Pos = ImMin(rect.Max - padding, ImMax(window->Pos + size_for_clamping, rect.Min + padding) - size_for_clamping);
}






bool ui::BeginNemesis(const char* name, bool* p_open, ImGuiWindowFlags flags)
{
	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	IM_ASSERT(name != NULL && name[0] != '\0');     // Window name required
	IM_ASSERT(g.FrameScopeActive);                  // Forgot to call ImGui::NewFrame()
	IM_ASSERT(g.FrameCountEnded != g.FrameCount);   // Called ImGui::Render() or ImGui::EndFrame() and haven't called ImGui::NewFrame() again yet

	// Find or create
	ImGuiWindow* window = FindWindowByName(name);
	const bool window_just_created = (window == NULL);
	if (window_just_created)
	{
		ImVec2 size_on_first_use = (g.NextWindowData.SizeCond != 0) ? g.NextWindowData.SizeVal : ImVec2(0.0f, 0.0f); // Any condition flag will do since we are creating a new window here.
		window = CreateNewWindow(name, size_on_first_use, flags);
	}

	// Automatically disable manual moving/resizing when NoInputs is set
	if ((flags & ImGuiWindowFlags_NoInputs) == ImGuiWindowFlags_NoInputs)
		flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

	if (flags & ImGuiWindowFlags_NavFlattened)
		IM_ASSERT(flags & ImGuiWindowFlags_ChildWindow);

	const int current_frame = g.FrameCount;
	const bool first_begin_of_the_frame = (window->LastFrameActive != current_frame);

	// Update Flags, LastFrameActive, BeginOrderXXX fields
	if (first_begin_of_the_frame)
		window->Flags = (ImGuiWindowFlags)flags;
	else
		flags = window->Flags;

	// Parent window is latched only on the first call to Begin() of the frame, so further append-calls can be done from a different window stack
	ImGuiWindow* parent_window_in_stack = g.CurrentWindowStack.empty() ? NULL : g.CurrentWindowStack.back();
	ImGuiWindow* parent_window = first_begin_of_the_frame ? ((flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Popup)) ? parent_window_in_stack : NULL) : window->ParentWindow;
	IM_ASSERT(parent_window != NULL || !(flags & ImGuiWindowFlags_ChildWindow));
	window->HasCloseButton = (p_open != NULL);

	// Update the Appearing flag
	bool window_just_activated_by_user = (window->LastFrameActive < current_frame - 1);   // Not using !WasActive because the implicit "Debug" window would always toggle off->on
	const bool window_just_appearing_after_hidden_for_resize = (window->HiddenFramesForResize > 0);
	if (flags & ImGuiWindowFlags_Popup)
	{
		ImGuiPopupRef& popup_ref = g.OpenPopupStack[g.BeginPopupStack.Size];
		window_just_activated_by_user |= (window->PopupId != popup_ref.PopupId); // We recycle popups so treat window as activated if popup id changed
		window_just_activated_by_user |= (window != popup_ref.Window);
	}
	window->Appearing = (window_just_activated_by_user || window_just_appearing_after_hidden_for_resize);
	if (window->Appearing)
		SetWindowConditionAllowFlags(window, ImGuiCond_Appearing, true);

	// Add to stack
	g.CurrentWindowStack.push_back(window);
	SetCurrentWindow(window);
	CheckStacksSize(window, true);
	if (flags & ImGuiWindowFlags_Popup)
	{
		ImGuiPopupRef& popup_ref = g.OpenPopupStack[g.BeginPopupStack.Size];
		popup_ref.Window = window;
		g.BeginPopupStack.push_back(popup_ref);
		window->PopupId = popup_ref.PopupId;
	}

	if (window_just_appearing_after_hidden_for_resize && !(flags & ImGuiWindowFlags_ChildWindow))
		window->NavLastIds[0] = 0;

	// Process SetNextWindow***() calls
	bool window_pos_set_by_api = false;
	bool window_size_x_set_by_api = false, window_size_y_set_by_api = false;
	if (g.NextWindowData.PosCond)
	{
		window_pos_set_by_api = (window->SetWindowPosAllowFlags & g.NextWindowData.PosCond) != 0;
		if (window_pos_set_by_api && ImLengthSqr(g.NextWindowData.PosPivotVal) > 0.00001f)
		{
			// May be processed on the next frame if this is our first frame and we are measuring size
			// FIXME: Look into removing the branch so everything can go through this same code path for consistency.
			window->SetWindowPosVal = g.NextWindowData.PosVal;
			window->SetWindowPosPivot = g.NextWindowData.PosPivotVal;
			window->SetWindowPosAllowFlags &= ~(ImGuiCond_Once | ImGuiCond_FirstUseEver | ImGuiCond_Appearing);
		}
		else
		{
			SetWindowPos(window, g.NextWindowData.PosVal, g.NextWindowData.PosCond);
		}
	}
	if (g.NextWindowData.SizeCond)
	{
		window_size_x_set_by_api = (window->SetWindowSizeAllowFlags & g.NextWindowData.SizeCond) != 0 && (g.NextWindowData.SizeVal.x > 0.0f);
		window_size_y_set_by_api = (window->SetWindowSizeAllowFlags & g.NextWindowData.SizeCond) != 0 && (g.NextWindowData.SizeVal.y > 0.0f);
		SetWindowSize(window, g.NextWindowData.SizeVal, g.NextWindowData.SizeCond);
	}
	if (g.NextWindowData.ContentSizeCond)
	{
		// Adjust passed "client size" to become a "window size"
		window->SizeContentsExplicit = g.NextWindowData.ContentSizeVal;
		if (window->SizeContentsExplicit.y != 0.0f)
			window->SizeContentsExplicit.y += window->TitleBarHeight() + window->MenuBarHeight();
	}
	else if (first_begin_of_the_frame)
	{
		window->SizeContentsExplicit = ImVec2(0.0f, 0.0f);
	}
	if (g.NextWindowData.CollapsedCond)
		SetWindowCollapsed(window, g.NextWindowData.CollapsedVal, g.NextWindowData.CollapsedCond);
	if (g.NextWindowData.FocusCond)
		FocusWindow(window);
	if (window->Appearing)
		SetWindowConditionAllowFlags(window, ImGuiCond_Appearing, false);

	// When reusing window again multiple times a frame, just append content (don't need to setup again)
	if (first_begin_of_the_frame)
	{
		// Initialize
		const bool window_is_child_tooltip = (flags & ImGuiWindowFlags_ChildWindow) && (flags & ImGuiWindowFlags_Tooltip); // FIXME-WIP: Undocumented behavior of Child+Tooltip for pinned tooltip (#1345)
		UpdateWindowParentAndRootLinks(window, flags, parent_window);

		window->Active = true;
		window->BeginOrderWithinParent = 0;
		window->BeginOrderWithinContext = (short)(g.WindowsActiveCount++);
		window->BeginCount = 0;
		window->ClipRect = ImVec4(-FLT_MAX, -FLT_MAX, +FLT_MAX, +FLT_MAX);
		window->LastFrameActive = current_frame;
		window->IDStack.resize(1);

		// Update stored window name when it changes (which can _only_ happen with the "###" operator, so the ID would stay unchanged).
		// The title bar always display the 'name' parameter, so we only update the string storage if it needs to be visible to the end-user elsewhere.
		bool window_title_visible_elsewhere = false;
		if (g.NavWindowingList != NULL && (window->Flags & ImGuiWindowFlags_NoNavFocus) == 0)   // Window titles visible when using CTRL+TAB
			window_title_visible_elsewhere = true;
		if (window_title_visible_elsewhere && !window_just_created && strcmp(name, window->Name) != 0)
		{
			size_t buf_len = (size_t)window->NameBufLen;
			window->Name = ImStrdupcpy(window->Name, &buf_len, name);
			window->NameBufLen = (int)buf_len;
		}

		// UPDATE CONTENTS SIZE, UPDATE HIDDEN STATUS

		// Update contents size from last frame for auto-fitting (or use explicit size)
		window->SizeContents = CalcSizeContents(window);
		if (window->HiddenFramesRegular > 0)
			window->HiddenFramesRegular--;
		if (window->HiddenFramesForResize > 0)
			window->HiddenFramesForResize--;

		// Hide new windows for one frame until they calculate their size
		if (window_just_created && (!window_size_x_set_by_api || !window_size_y_set_by_api))
			window->HiddenFramesForResize = 1;

		// Hide popup/tooltip window when re-opening while we measure size (because we recycle the windows)
		// We reset Size/SizeContents for reappearing popups/tooltips early in this function, so further code won't be tempted to use the old size.
		if (window_just_activated_by_user && (flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) != 0)
		{
			window->HiddenFramesForResize = 1;
			if (flags & ImGuiWindowFlags_AlwaysAutoResize)
			{
				if (!window_size_x_set_by_api)
					window->Size.x = window->SizeFull.x = 0.f;
				if (!window_size_y_set_by_api)
					window->Size.y = window->SizeFull.y = 0.f;
				window->SizeContents = ImVec2(0.f, 0.f);
			}
		}

		SetCurrentWindow(window);

		// Lock border size and padding for the frame (so that altering them doesn't cause inconsistencies)
		window->WindowBorderSize = (flags & ImGuiWindowFlags_ChildWindow) ? style.ChildBorderSize : ((flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) && !(flags & ImGuiWindowFlags_Modal)) ? style.PopupBorderSize : style.WindowBorderSize;
		window->WindowPadding = style.WindowPadding;
		if ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & (ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_Popup)) && window->WindowBorderSize == 0.0f)
			window->WindowPadding = ImVec2(0.0f, (flags & ImGuiWindowFlags_MenuBar) ? style.WindowPadding.y : 0.0f);
		window->DC.MenuBarOffset.x = ImMax(ImMax(window->WindowPadding.x, style.ItemSpacing.x), g.NextWindowData.MenuBarOffsetMinVal.x);
		window->DC.MenuBarOffset.y = g.NextWindowData.MenuBarOffsetMinVal.y;

		// Collapse window by double-clicking on title bar
		// At this point we don't have a clipping rectangle setup yet, so we can use the title bar area for hit detection and drawing
		if (!(flags & ImGuiWindowFlags_NoTitleBar) && !(flags & ImGuiWindowFlags_NoCollapse))
		{
			// We don't use a regular button+id to test for double-click on title bar (mostly due to legacy reason, could be fixed), so verify that we don't have items over the title bar.
			ImRect title_bar_rect = window->TitleBarRect();
			if (g.HoveredWindow == window && g.HoveredId == 0 && g.HoveredIdPreviousFrame == 0 && IsMouseHoveringRect(title_bar_rect.Min, title_bar_rect.Max) && g.IO.MouseDoubleClicked[0])
				window->WantCollapseToggle = true;
			if (window->WantCollapseToggle)
			{
				window->Collapsed = !window->Collapsed;
				MarkIniSettingsDirty(window);
				FocusWindow(window);
			}
		}
		else
		{
			window->Collapsed = false;
		}
		window->WantCollapseToggle = false;

		// SIZE

		// Calculate auto-fit size, handle automatic resize
		const ImVec2 size_auto_fit = CalcSizeAutoFit(window, window->SizeContents);
		ImVec2 size_full_modified(FLT_MAX, FLT_MAX);
		if ((flags & ImGuiWindowFlags_AlwaysAutoResize) && !window->Collapsed)
		{
			// Using SetNextWindowSize() overrides ImGuiWindowFlags_AlwaysAutoResize, so it can be used on tooltips/popups, etc.
			if (!window_size_x_set_by_api)
				window->SizeFull.x = size_full_modified.x = size_auto_fit.x;
			if (!window_size_y_set_by_api)
				window->SizeFull.y = size_full_modified.y = size_auto_fit.y;
		}
		else if (window->AutoFitFramesX > 0 || window->AutoFitFramesY > 0)
		{
			// Auto-fit may only grow window during the first few frames
			// We still process initial auto-fit on collapsed windows to get a window width, but otherwise don't honor ImGuiWindowFlags_AlwaysAutoResize when collapsed.
			if (!window_size_x_set_by_api && window->AutoFitFramesX > 0)
				window->SizeFull.x = size_full_modified.x = window->AutoFitOnlyGrows ? ImMax(window->SizeFull.x, size_auto_fit.x) : size_auto_fit.x;
			if (!window_size_y_set_by_api && window->AutoFitFramesY > 0)
				window->SizeFull.y = size_full_modified.y = window->AutoFitOnlyGrows ? ImMax(window->SizeFull.y, size_auto_fit.y) : size_auto_fit.y;
			if (!window->Collapsed)
				MarkIniSettingsDirty(window);
		}

		// Apply minimum/maximum window size constraints and final size
		window->SizeFull = CalcSizeAfterConstraint(window, window->SizeFull);
		window->Size = window->Collapsed && !(flags & ImGuiWindowFlags_ChildWindow) ? window->TitleBarRect().GetSize() : window->SizeFull;

		// SCROLLBAR STATUS

		// Update scrollbar status (based on the Size that was effective during last frame or the auto-resized Size).
		if (!window->Collapsed)
		{
			// When reading the current size we need to read it after size constraints have been applied
			float size_x_for_scrollbars = size_full_modified.x != FLT_MAX ? window->SizeFull.x : window->SizeFullAtLastBegin.x;
			float size_y_for_scrollbars = size_full_modified.y != FLT_MAX ? window->SizeFull.y : window->SizeFullAtLastBegin.y;
			window->ScrollbarY = (flags & ImGuiWindowFlags_AlwaysVerticalScrollbar) || ((window->SizeContents.y > size_y_for_scrollbars) && !(flags & ImGuiWindowFlags_NoScrollbar));
			window->ScrollbarX = (flags & ImGuiWindowFlags_AlwaysHorizontalScrollbar) || ((window->SizeContents.x > size_x_for_scrollbars - (window->ScrollbarY ? style.ScrollbarSize : 0.0f)) && !(flags & ImGuiWindowFlags_NoScrollbar) && (flags & ImGuiWindowFlags_HorizontalScrollbar));
			if (window->ScrollbarX && !window->ScrollbarY)
				window->ScrollbarY = (window->SizeContents.y > size_y_for_scrollbars - style.ScrollbarSize) && !(flags & ImGuiWindowFlags_NoScrollbar);
			window->ScrollbarSizes = ImVec2(window->ScrollbarY ? style.ScrollbarSize : 0.0f, window->ScrollbarX ? style.ScrollbarSize : 0.0f);
		}

		// POSITION

		// Popup latch its initial position, will position itself when it appears next frame
		if (window_just_activated_by_user)
		{
			window->AutoPosLastDirection = ImGuiDir_None;
			if ((flags & ImGuiWindowFlags_Popup) != 0 && !window_pos_set_by_api)
				window->Pos = g.BeginPopupStack.back().OpenPopupPos;
		}

		// Position child window
		if (flags & ImGuiWindowFlags_ChildWindow)
		{
			IM_ASSERT(parent_window && parent_window->Active);
			window->BeginOrderWithinParent = (short)parent_window->DC.ChildWindows.Size;
			parent_window->DC.ChildWindows.push_back(window);
			if (!(flags & ImGuiWindowFlags_Popup) && !window_pos_set_by_api && !window_is_child_tooltip)
				window->Pos = parent_window->DC.CursorPos;
		}

		const bool window_pos_with_pivot = (window->SetWindowPosVal.x != FLT_MAX && window->HiddenFramesForResize == 0);
		if (window_pos_with_pivot)
			SetWindowPos(window, ImMax(style.DisplaySafeAreaPadding, window->SetWindowPosVal - window->SizeFull * window->SetWindowPosPivot), 0); // Position given a pivot (e.g. for centering)
		else if ((flags & ImGuiWindowFlags_ChildMenu) != 0)
			window->Pos = FindBestWindowPosForPopup(window);
		else if ((flags & ImGuiWindowFlags_Popup) != 0 && !window_pos_set_by_api && window_just_appearing_after_hidden_for_resize)
			window->Pos = FindBestWindowPosForPopup(window);
		else if ((flags & ImGuiWindowFlags_Tooltip) != 0 && !window_pos_set_by_api && !window_is_child_tooltip)
			window->Pos = FindBestWindowPosForPopup(window);

		// Clamp position so it stays visible
		// Ignore zero-sized display explicitly to avoid losing positions if a window manager reports zero-sized window when initializing or minimizing.
		if (!window_pos_set_by_api && !(flags & ImGuiWindowFlags_ChildWindow) && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0)
		{
			if (g.IO.DisplaySize.x > 0.0f && g.IO.DisplaySize.y > 0.0f) // Ignore zero-sized display explicitly to avoid losing positions if a window manager reports zero-sized window when initializing or minimizing.
			{
				ImVec2 padding = ImMax(style.DisplayWindowPadding, style.DisplaySafeAreaPadding);
				ImVec2 size_for_clamping = ((g.IO.ConfigWindowsMoveFromTitleBarOnly) && !(window->Flags & ImGuiWindowFlags_NoTitleBar)) ? ImVec2(window->Size.x, window->TitleBarHeight()) : window->Size;
				window->Pos = ImMax(window->Pos + size_for_clamping, padding) - size_for_clamping;
				window->Pos = ImMin(window->Pos, g.IO.DisplaySize - padding);
			}
		}
		window->Pos = ImFloor(window->Pos);

		// Lock window rounding for the frame (so that altering them doesn't cause inconsistencies)
		window->WindowRounding = (flags & ImGuiWindowFlags_ChildWindow) ? style.ChildRounding : ((flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiWindowFlags_Modal)) ? style.PopupRounding : style.WindowRounding;

		// Prepare for item focus requests
		window->FocusIdxAllRequestCurrent = (window->FocusIdxAllRequestNext == INT_MAX || window->FocusIdxAllCounter == -1) ? INT_MAX : (window->FocusIdxAllRequestNext + (window->FocusIdxAllCounter + 1)) % (window->FocusIdxAllCounter + 1);
		window->FocusIdxTabRequestCurrent = (window->FocusIdxTabRequestNext == INT_MAX || window->FocusIdxTabCounter == -1) ? INT_MAX : (window->FocusIdxTabRequestNext + (window->FocusIdxTabCounter + 1)) % (window->FocusIdxTabCounter + 1);
		window->FocusIdxAllCounter = window->FocusIdxTabCounter = -1;
		window->FocusIdxAllRequestNext = window->FocusIdxTabRequestNext = INT_MAX;

		// Apply scrolling
		window->Scroll = CalcNextScrollFromScrollTargetAndClamp(window, true);
		window->ScrollTarget = ImVec2(FLT_MAX, FLT_MAX);

		// Apply window focus (new and reactivated windows are moved to front)
		bool want_focus = false;
		if (window_just_activated_by_user && !(flags & ImGuiWindowFlags_NoFocusOnAppearing))
		{
			if (flags & ImGuiWindowFlags_Popup)
				want_focus = true;
			else if ((flags & (ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_Tooltip)) == 0)
				want_focus = true;
		}

		// Handle manual resize: Resize Grips, Borders, Gamepad
		int border_held = -1;
		ImU32 resize_grip_col[4] = { 0 };
		const int resize_grip_count = g.IO.ConfigWindowsResizeFromEdges ? 2 : 1; // 4
		const float grip_draw_size = (float)(int)ImMax(g.FontSize * 1.35f, window->WindowRounding + 1.0f + g.FontSize * 0.2f);
		if (!window->Collapsed)
			UpdateManualResize(window, size_auto_fit, &border_held, resize_grip_count, &resize_grip_col[0]);
		window->ResizeBorderHeld = (signed char)border_held;

		// Default item width. Make it proportional to window size if window manually resizes
		if (window->Size.x > 0.0f && !(flags & ImGuiWindowFlags_Tooltip) && !(flags & ImGuiWindowFlags_AlwaysAutoResize))
			window->ItemWidthDefault = (float)(int)(window->Size.x * 0.65f);
		else
			window->ItemWidthDefault = (float)(int)(g.FontSize * 16.0f);

		// DRAWING

		// Setup draw list and outer clipping rectangle
		window->DrawList->Clear();
		window->DrawList->Flags = (g.Style.AntiAliasedLines ? ImDrawListFlags_AntiAliasedLines : 0) | (g.Style.AntiAliasedFill ? ImDrawListFlags_AntiAliasedFill : 0);
		window->DrawList->PushTextureID(g.Font->ContainerAtlas->TexID);
		ImRect viewport_rect(GetViewportRect());
		if ((flags & ImGuiWindowFlags_ChildWindow) && !(flags & ImGuiWindowFlags_Popup) && !window_is_child_tooltip)
			PushClipRect(parent_window->ClipRect.Min, parent_window->ClipRect.Max, true);
		else
			PushClipRect(viewport_rect.Min, viewport_rect.Max, true);

		// Draw modal window background (darkens what is behind them, all viewports)
		const bool dim_bg_for_modal = (flags & ImGuiWindowFlags_Modal) && window == GetFrontMostPopupModal() && window->HiddenFramesForResize <= 0;
		const bool dim_bg_for_window_list = g.NavWindowingTargetAnim && (window == g.NavWindowingTargetAnim->RootWindow);
		if (dim_bg_for_modal || dim_bg_for_window_list)
		{
			const ImU32 dim_bg_col = GetColorU32(dim_bg_for_modal ? ImGuiCol_ModalWindowDimBg : ImGuiCol_NavWindowingDimBg, g.DimBgRatio);
			window->DrawList->AddRectFilled(viewport_rect.Min, viewport_rect.Max, dim_bg_col);
		}

		// Draw navigation selection/windowing rectangle background
		if (dim_bg_for_window_list && window == g.NavWindowingTargetAnim)
		{
			ImRect bb = window->Rect();
			bb.Expand(g.FontSize);
			if (!bb.Contains(viewport_rect)) // Avoid drawing if the window covers all the viewport anyway
				window->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(ImGuiCol_NavWindowingHighlight, g.NavWindowingHighlightAlpha * 0.25f), g.Style.WindowRounding);
		}

		// Draw window + handle manual resize
		// As we highlight the title bar when want_focus is set, multiple reappearing windows will have have their title bar highlighted on their reappearing frame.
		const float window_rounding = window->WindowRounding;
		const float window_border_size = window->WindowBorderSize;
		const ImGuiWindow* window_to_highlight = g.NavWindowingTarget ? g.NavWindowingTarget : g.NavWindow;
		const bool title_bar_is_highlight = want_focus || (window_to_highlight && window->RootWindowForTitleBarHighlight == window_to_highlight->RootWindowForTitleBarHighlight);
		const ImRect title_bar_rect = window->TitleBarRect();
		if (window->Collapsed)
		{
			// Title bar only
			float backup_border_size = style.FrameBorderSize;
			g.Style.FrameBorderSize = window->WindowBorderSize;
			ImU32 title_bar_col = GetColorU32((title_bar_is_highlight && !g.NavDisableHighlight) ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBgCollapsed);
			RenderFrame(title_bar_rect.Min, title_bar_rect.Max, title_bar_col, true, window_rounding);
			g.Style.FrameBorderSize = backup_border_size;
		}
		else
		{
			// Window background
			if (!(flags & ImGuiWindowFlags_NoBackground))
			{
				ImU32 bg_col = GetColorU32(GetWindowBgColorIdxFromFlags(flags));
				float alpha = 1.0f;
				if (g.NextWindowData.BgAlphaCond != 0)
					alpha = g.NextWindowData.BgAlphaVal;
				if (alpha != 1.0f)
					bg_col = (bg_col & ~IM_COL32_A_MASK) | (IM_F32_TO_INT8_SAT(alpha) << IM_COL32_A_SHIFT);
				window->DrawList->AddRectFilled(window->Pos + ImVec2(0, window->TitleBarHeight()), window->Pos + window->Size, bg_col, window_rounding, (flags & ImGuiWindowFlags_NoTitleBar) ? ImDrawCornerFlags_All : ImDrawCornerFlags_Bot);
			}
			g.NextWindowData.BgAlphaCond = 0;

			// Title bar
			if (!(flags & ImGuiWindowFlags_NoTitleBar))
			{
				ImU32 title_bar_col = GetColorU32(title_bar_is_highlight ? ImGuiCol_TitleBgActive : ImGuiCol_TitleBg);
				window->DrawList->AddRectFilled(title_bar_rect.Min, title_bar_rect.Max, title_bar_col, window_rounding, ImDrawCornerFlags_Top);
			}

			// Menu bar
			if (flags & ImGuiWindowFlags_MenuBar)
			{
				ImRect menu_bar_rect = window->MenuBarRect();
				menu_bar_rect.ClipWith(window->Rect());  // Soft clipping, in particular child window don't have minimum size covering the menu bar so this is useful for them.
				window->DrawList->AddRectFilled(menu_bar_rect.Min, menu_bar_rect.Max, GetColorU32(ImGuiCol_MenuBarBg), (flags & ImGuiWindowFlags_NoTitleBar) ? window_rounding : 0.0f, ImDrawCornerFlags_Top);
				if (style.FrameBorderSize > 0.0f && menu_bar_rect.Max.y < window->Pos.y + window->Size.y)
					window->DrawList->AddLine(menu_bar_rect.GetBL(), menu_bar_rect.GetBR(), GetColorU32(ImGuiCol_Border), style.FrameBorderSize);
			}

			// Scrollbars
			if (window->ScrollbarX)
				Scrollbar(ImGuiLayoutType_Horizontal);
			if (window->ScrollbarY)
				Scrollbar(ImGuiLayoutType_Vertical);

			// Render resize grips (after their input handling so we don't have a frame of latency)
			if (!(flags & ImGuiWindowFlags_NoResize))
			{
				for (int resize_grip_n = 0; resize_grip_n < resize_grip_count; resize_grip_n++)
				{
					const ImGuiResizeGripDef& grip = resize_grip_def[resize_grip_n];
					const ImVec2 corner = ImLerp(window->Pos, window->Pos + window->Size, grip.CornerPosN);
					window->DrawList->PathLineTo(corner + grip.InnerDir * ((resize_grip_n & 1) ? ImVec2(window_border_size, grip_draw_size) : ImVec2(grip_draw_size, window_border_size)));
					window->DrawList->PathLineTo(corner + grip.InnerDir * ((resize_grip_n & 1) ? ImVec2(grip_draw_size, window_border_size) : ImVec2(window_border_size, grip_draw_size)));
					window->DrawList->PathArcToFast(ImVec2(corner.x + grip.InnerDir.x * (window_rounding + window_border_size), corner.y + grip.InnerDir.y * (window_rounding + window_border_size)), window_rounding, grip.AngleMin12, grip.AngleMax12);
					window->DrawList->PathFillConvex(resize_grip_col[resize_grip_n]);
				}
			}

			// Borders
			RenderOuterBorders(window);
		}

		// Draw navigation selection/windowing rectangle border
		if (g.NavWindowingTargetAnim == window)
		{
			float rounding = ImMax(window->WindowRounding, g.Style.WindowRounding);
			ImRect bb = window->Rect();
			bb.Expand(g.FontSize);
			if (bb.Contains(viewport_rect)) // If a window fits the entire viewport, adjust its highlight inward
			{
				bb.Expand(-g.FontSize - 1.0f);
				rounding = window->WindowRounding;
			}
			window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(ImGuiCol_NavWindowingHighlight, g.NavWindowingHighlightAlpha), rounding, ~0, 3.0f);
		}

		// Store a backup of SizeFull which we will use next frame to decide if we need scrollbars.
		window->SizeFullAtLastBegin = window->SizeFull;

		// Update various regions. Variables they depends on are set above in this function.
		// FIXME: window->ContentsRegionRect.Max is currently very misleading / partly faulty, but some BeginChild() patterns relies on it.
		window->ContentsRegionRect.Min.x = window->Pos.x - window->Scroll.x + window->WindowPadding.x;
		window->ContentsRegionRect.Min.y = window->Pos.y - window->Scroll.y + window->WindowPadding.y + window->TitleBarHeight() + window->MenuBarHeight();
		window->ContentsRegionRect.Max.x = window->Pos.x - window->Scroll.x - window->WindowPadding.x + (window->SizeContentsExplicit.x != 0.0f ? window->SizeContentsExplicit.x : (window->Size.x - window->ScrollbarSizes.x));
		window->ContentsRegionRect.Max.y = window->Pos.y - window->Scroll.y - window->WindowPadding.y + (window->SizeContentsExplicit.y != 0.0f ? window->SizeContentsExplicit.y : (window->Size.y - window->ScrollbarSizes.y));

		// Setup drawing context
		// (NB: That term "drawing context / DC" lost its meaning a long time ago. Initially was meant to hold transient data only. Nowadays difference between window-> and window->DC-> is dubious.)
		window->DC.Indent.x = 0.0f + window->WindowPadding.x - window->Scroll.x;
		window->DC.GroupOffset.x = 0.0f;
		window->DC.ColumnsOffset.x = 0.0f;
		window->DC.CursorStartPos = window->Pos + ImVec2(window->DC.Indent.x + window->DC.ColumnsOffset.x, window->TitleBarHeight() + window->MenuBarHeight() + window->WindowPadding.y - window->Scroll.y);
		window->DC.CursorPos = window->DC.CursorStartPos;
		window->DC.CursorPosPrevLine = window->DC.CursorPos;
		window->DC.CursorMaxPos = window->DC.CursorStartPos;
		window->DC.CurrentLineSize = window->DC.PrevLineSize = ImVec2(0.0f, 0.0f);
		window->DC.CurrentLineTextBaseOffset = window->DC.PrevLineTextBaseOffset = 0.0f;
		window->DC.NavHideHighlightOneFrame = false;
		window->DC.NavHasScroll = (GetWindowScrollMaxY(window) > 0.0f);
		window->DC.NavLayerActiveMask = window->DC.NavLayerActiveMaskNext;
		window->DC.NavLayerActiveMaskNext = 0x00;
		window->DC.MenuBarAppending = false;
		window->DC.LogLinePosY = window->DC.CursorPos.y - 9999.0f;
		window->DC.ChildWindows.resize(0);
		window->DC.LayoutType = ImGuiLayoutType_Vertical;
		window->DC.ParentLayoutType = parent_window ? parent_window->DC.LayoutType : ImGuiLayoutType_Vertical;
		window->DC.ItemFlags = parent_window ? parent_window->DC.ItemFlags : ImGuiItemFlags_Default_;
		window->DC.ItemWidth = window->ItemWidthDefault;
		window->DC.TextWrapPos = -1.0f; // disabled
		window->DC.ItemFlagsStack.resize(0);
		window->DC.ItemWidthStack.resize(0);
		window->DC.TextWrapPosStack.resize(0);
		window->DC.ColumnsSet = NULL;
		window->DC.TreeDepth = 0;
		window->DC.TreeDepthMayJumpToParentOnPop = 0x00;
		window->DC.StateStorage = &window->StateStorage;
		window->DC.GroupStack.resize(0);
		window->MenuColumns.Update(3, style.ItemSpacing.x, window_just_activated_by_user);

		if ((flags & ImGuiWindowFlags_ChildWindow) && (window->DC.ItemFlags != parent_window->DC.ItemFlags))
		{
			window->DC.ItemFlags = parent_window->DC.ItemFlags;
			window->DC.ItemFlagsStack.push_back(window->DC.ItemFlags);
		}

		if (window->AutoFitFramesX > 0)
			window->AutoFitFramesX--;
		if (window->AutoFitFramesY > 0)
			window->AutoFitFramesY--;

		// Apply focus (we need to call FocusWindow() AFTER setting DC.CursorStartPos so our initial navigation reference rectangle can start around there)
		if (want_focus)
		{
			FocusWindow(window);
			NavInitWindow(window, false);
		}

		// Title bar
		if (!(flags & ImGuiWindowFlags_NoTitleBar))
		{
			// Close & collapse button are on layer 1 (same as menus) and don't default focus
			const ImGuiItemFlags item_flags_backup = window->DC.ItemFlags;
			window->DC.ItemFlags |= ImGuiItemFlags_NoNavDefaultFocus;
			window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
			window->DC.NavLayerCurrentMask = (1 << ImGuiNavLayer_Menu);

			// Collapse button
			if (!(flags & ImGuiWindowFlags_NoCollapse))
				if (CollapseButton(window->GetID("#COLLAPSE"), window->Pos))
					window->WantCollapseToggle = true; // Defer collapsing to next frame as we are too far in the Begin() function

			// Close button
			if (p_open != NULL)
			{
				const float pad = style.FramePadding.y;
				const float rad = g.FontSize * 0.5f;
				if (CloseButton(window->GetID("#CLOSE"), window->Rect().GetTR() + ImVec2(-pad - rad, pad + rad), rad + 1))
					*p_open = false;
			}

			window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
			window->DC.NavLayerCurrentMask = (1 << ImGuiNavLayer_Main);
			window->DC.ItemFlags = item_flags_backup;

			// Title bar text (with: horizontal alignment, avoiding collapse/close button, optional "unsaved document" marker)
			// FIXME: Refactor text alignment facilities along with RenderText helpers, this is too much code..
			const char* UNSAVED_DOCUMENT_MARKER = "*";
			float marker_size_x = (flags & ImGuiWindowFlags_UnsavedDocument) ? CalcTextSize(UNSAVED_DOCUMENT_MARKER, NULL, false).x : 0.0f;
			ImVec2 text_size = CalcTextSize(name, NULL, true) + ImVec2(marker_size_x, 0.0f);
			ImRect text_r = title_bar_rect;
			float pad_left = (flags & ImGuiWindowFlags_NoCollapse) ? style.FramePadding.x : (style.FramePadding.x + g.FontSize + style.ItemInnerSpacing.x);
			float pad_right = (p_open == NULL) ? style.FramePadding.x : (style.FramePadding.x + g.FontSize + style.ItemInnerSpacing.x);
			if (style.WindowTitleAlign.x > 0.0f)
				pad_right = ImLerp(pad_right, pad_left, style.WindowTitleAlign.x);
			text_r.Min.x += pad_left;
			text_r.Max.x -= pad_right;
			ImRect clip_rect = text_r;
			clip_rect.Max.x = window->Pos.x + window->Size.x - (p_open ? title_bar_rect.GetHeight() - 3 : style.FramePadding.x); // Match the size of CloseButton()
			RenderTextClipped(text_r.Min, text_r.Max, name, NULL, &text_size, style.WindowTitleAlign, &clip_rect);
			if (flags & ImGuiWindowFlags_UnsavedDocument)
			{
				ImVec2 marker_pos = ImVec2(ImMax(text_r.Min.x, text_r.Min.x + (text_r.GetWidth() - text_size.x) * style.WindowTitleAlign.x) + text_size.x, text_r.Min.y) + ImVec2(2 - marker_size_x, 0.0f);
				ImVec2 off = ImVec2(0.0f, (float)(int)(-g.FontSize * 0.25f));
				RenderTextClipped(marker_pos + off, text_r.Max + off, UNSAVED_DOCUMENT_MARKER, NULL, NULL, ImVec2(0, style.WindowTitleAlign.y), &clip_rect);
		}
	}

		// Save clipped aabb so we can access it in constant-time in FindHoveredWindow()
		window->OuterRectClipped = window->Rect();
		window->OuterRectClipped.ClipWith(window->ClipRect);

		// Pressing CTRL+C while holding on a window copy its content to the clipboard
		// This works but 1. doesn't handle multiple Begin/End pairs, 2. recursing into another Begin/End pair - so we need to work that out and add better logging scope.
		// Maybe we can support CTRL+C on every element?
		/*
		if (g.ActiveId == move_id)
			if (g.IO.KeyCtrl && IsKeyPressedMap(ImGuiKey_C))
				LogToClipboard();
		*/

		// Inner rectangle
		// We set this up after processing the resize grip so that our clip rectangle doesn't lag by a frame
		// Note that if our window is collapsed we will end up with an inverted (~null) clipping rectangle which is the correct behavior.
		window->InnerMainRect.Min.x = title_bar_rect.Min.x + window->WindowBorderSize;
		window->InnerMainRect.Min.y = title_bar_rect.Max.y + window->MenuBarHeight() + (((flags & ImGuiWindowFlags_MenuBar) || !(flags & ImGuiWindowFlags_NoTitleBar)) ? style.FrameBorderSize : window->WindowBorderSize);
		window->InnerMainRect.Max.x = window->Pos.x + window->Size.x - window->ScrollbarSizes.x - window->WindowBorderSize;
		window->InnerMainRect.Max.y = window->Pos.y + window->Size.y - window->ScrollbarSizes.y - window->WindowBorderSize;
		//window->DrawList->AddRect(window->InnerRect.Min, window->InnerRect.Max, IM_COL32_WHITE);

		// Inner clipping rectangle
		// Force round operator last to ensure that e.g. (int)(max.x-min.x) in user's render code produce correct result.
		window->InnerClipRect.Min.x = ImFloor(0.5f + window->InnerMainRect.Min.x + ImMax(0.0f, ImFloor(window->WindowPadding.x * 0.5f - window->WindowBorderSize)));
		window->InnerClipRect.Min.y = ImFloor(0.5f + window->InnerMainRect.Min.y);
		window->InnerClipRect.Max.x = ImFloor(0.5f + window->InnerMainRect.Max.x - ImMax(0.0f, ImFloor(window->WindowPadding.x * 0.5f - window->WindowBorderSize)));
		window->InnerClipRect.Max.y = ImFloor(0.5f + window->InnerMainRect.Max.y);

		// We fill last item data based on Title Bar, in order for IsItemHovered() and IsItemActive() to be usable after Begin().
		// This is useful to allow creating context menus on title bar only, etc.
		window->DC.LastItemId = window->MoveId;
		window->DC.LastItemStatusFlags = IsMouseHoveringRect(title_bar_rect.Min, title_bar_rect.Max, false) ? ImGuiItemStatusFlags_HoveredRect : 0;
		window->DC.LastItemRect = title_bar_rect;
}

	PushClipRect(window->InnerClipRect.Min, window->InnerClipRect.Max, true);

	// Clear 'accessed' flag last thing (After PushClipRect which will set the flag. We want the flag to stay false when the default "Debug" window is unused)
	if (first_begin_of_the_frame)
		window->WriteAccessed = false;

	window->BeginCount++;
	g.NextWindowData.Clear();

	if (flags & ImGuiWindowFlags_ChildWindow)
	{
		// Child window can be out of sight and have "negative" clip windows.
		// Mark them as collapsed so commands are skipped earlier (we can't manually collapse them because they have no title bar).
		IM_ASSERT((flags & ImGuiWindowFlags_NoTitleBar) != 0);
		if (!(flags & ImGuiWindowFlags_AlwaysAutoResize) && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0)
			if (window->OuterRectClipped.Min.x >= window->OuterRectClipped.Max.x || window->OuterRectClipped.Min.y >= window->OuterRectClipped.Max.y)
				window->HiddenFramesRegular = 1;

		// Completely hide along with parent or if parent is collapsed
		if (parent_window && (parent_window->Collapsed || parent_window->Hidden))
			window->HiddenFramesRegular = 1;
	}

	// Don't render if style alpha is 0.0 at the time of Begin(). This is arbitrary and inconsistent but has been there for a long while (may remove at some point)
	if (style.Alpha <= 0.0f)
		window->HiddenFramesRegular = 1;

	// Update the Hidden flag
	window->Hidden = (window->HiddenFramesRegular > 0) || (window->HiddenFramesForResize > 0);

	// Return false if we don't intend to display anything to allow user to perform an early out optimization
	window->SkipItems = (window->Collapsed || !window->Active || window->Hidden) && window->AutoFitFramesX <= 0 && window->AutoFitFramesY <= 0 && window->HiddenFramesForResize <= 0;

	return !window->SkipItems;
}

void ui::EndNemesis()
{
	ImGuiContext& g = *GImGui;

	if (g.CurrentWindowStack.Size <= 1 && g.FrameScopePushedImplicitWindow)
	{
		IM_ASSERT(g.CurrentWindowStack.Size > 1 && "Calling End() too many times!");
		return; // FIXME-ERRORHANDLING
	}
	IM_ASSERT(g.CurrentWindowStack.Size > 0);

	ImGuiWindow* window = g.CurrentWindow;

	if (window->DC.ColumnsSet != NULL)
		EndColumns();
	PopClipRect();   // Inner window clip rectangle

	// Stop logging
	if (!(window->Flags & ImGuiWindowFlags_ChildWindow))    // FIXME: add more options for scope of logging
		LogFinish();

	// Pop from window stack
	g.CurrentWindowStack.pop_back();
	if (window->Flags & ImGuiWindowFlags_Popup)
		g.BeginPopupStack.pop_back();
	CheckStacksSize(window, false);
	SetCurrentWindow(g.CurrentWindowStack.empty() ? NULL : g.CurrentWindowStack.back());
}



bool ui::BeginChildExNemesis(const char* name, ImGuiID id, const ImVec2& size_arg, bool border, ImGuiWindowFlags flags)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* parent_window = g.CurrentWindow;
	
	flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_ChildWindow;
	flags |= (parent_window->Flags & ImGuiWindowFlags_NoMove);  // Inherit the NoMove flag
	
	// Size
	const ImVec2 content_avail = GetContentRegionAvail();
	ImVec2 size = ImFloor(size_arg);
	const int auto_fit_axises = ((size.x == 0.0f) ? (1 << ImGuiAxis_X) : 0x00) | ((size.y == 0.0f) ? (1 << ImGuiAxis_Y) : 0x00);
	if (size.x <= 0.0f)
		size.x = ImMax(content_avail.x + size.x, 4.0f); // Arbitrary minimum child size (0.0f causing too much issues)
	if (size.y <= 0.0f)
		size.y = ImMax(content_avail.y + size.y, 4.0f);
	SetNextWindowSize(size);
	
	// Build up name. If you need to append to a same child from multiple location in the ID stack, use BeginChild(ImGuiID id) with a stable value.
	char title[256];
	if (name)
		ImFormatString(title, IM_ARRAYSIZE(title), "%s/%s_%08X", parent_window->Name, name, id);
	else
		ImFormatString(title, IM_ARRAYSIZE(title), "%s/%08X", parent_window->Name, id);
	
	const float backup_border_size = g.Style.ChildBorderSize;
	if (!border)
		g.Style.ChildBorderSize = 0.0f;
	bool ret = BeginNemesis(title, NULL, flags);
	g.Style.ChildBorderSize = backup_border_size;
	
	ImGuiWindow* child_window = g.CurrentWindow;
	child_window->ChildId = id;
	child_window->AutoFitChildAxises = auto_fit_axises;
	
	// Set the cursor to handle case where the user called SetNextWindowPos()+BeginChild() manually.
	// While this is not really documented/defined, it seems that the expected thing to do.
	if (child_window->BeginCount == 1)
		parent_window->DC.CursorPos = child_window->Pos;
	
	// Process navigation-in immediately so NavInit can run on first frame
	if (g.NavActivateId == id && !(flags & ImGuiWindowFlags_NavFlattened) && (child_window->DC.NavLayerActiveMask != 0 || child_window->DC.NavHasScroll))
	{
		FocusWindow(child_window);
		NavInitWindow(child_window, false);
		SetActiveID(id + 1, child_window); // Steal ActiveId with a dummy id so that key-press won't activate child item
		g.ActiveIdSource = ImGuiInputSource_Nav;
	}
	return ret;
	//ImGuiContext& g = *GImGui;
	//ImGuiWindow* parent_window = g.CurrentWindow;
	//
	//flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_ChildWindow;
	//flags |= (parent_window->Flags & ImGuiWindowFlags_NoMove);  // Inherit the NoMove flag
	//
	//// Size
	//const ImVec2 content_avail = GetContentRegionAvail();
	//ImVec2 size = ImFloor(size_arg);
	//const int auto_fit_axises = ((size.x == 0.0f) ? (1 << ImGuiAxis_X) : 0x00) | ((size.y == 0.0f) ? (1 << ImGuiAxis_Y) : 0x00);
	//if (size.x <= 0.0f)
	//	size.x = ImMax(content_avail.x + size.x, 4.0f); // Arbitrary minimum child size (0.0f causing too much issues)
	//if (size.y <= 0.0f)
	//	size.y = ImMax(content_avail.y + size.y, 4.0f);
	//SetNextWindowSize(size);
	//
	//// Build up name. If you need to append to a same child from multiple location in the ID stack, use BeginChild(ImGuiID id) with a stable value.
	//char title[256];
	//if (name)
	//	ImFormatString(title, IM_ARRAYSIZE(title), "%s/%s_%08X", parent_window->Name, name, id);
	//else
	//	ImFormatString(title, IM_ARRAYSIZE(title), "%s/%08X", parent_window->Name, id);
	//
	//const float backup_border_size = g.Style.ChildBorderSize;
	//if (!border)
	//	g.Style.ChildBorderSize = 0.0f;
	//bool ret = Begin(title, NULL, flags);
	//g.Style.ChildBorderSize = backup_border_size;
	//
	//ImGuiWindow* child_window = g.CurrentWindow;
	//child_window->ChildId = id;
	//child_window->AutoFitChildAxises = auto_fit_axises;
	//
	//// Set the cursor to handle case where the user called SetNextWindowPos()+BeginChild() manually.
	//// While this is not really documented/defined, it seems that the expected thing to do.
	//if (child_window->BeginCount == 1)
	//	parent_window->DC.CursorPos = child_window->Pos;
	//
	//// Process navigation-in immediately so NavInit can run on first frame
	//if (g.NavActivateId == id && !(flags & ImGuiWindowFlags_NavFlattened) && (child_window->DC.NavLayerActiveMask != 0 || child_window->DC.NavHasScroll))
	//{
	//	FocusWindow(child_window);
	//	NavInitWindow(child_window, false);
	//	SetActiveID(id + 1, child_window); // Steal ActiveId with another arbitrary id so that key-press won't activate child item
	//	g.ActiveIdSource = ImGuiInputSource_Nav;
	//}
	///*parent_window->DrawList->AddRectFilled(ImGui::GetWindowPos(), ImGui::GetWindowPos() + size_arg, ImColor(19, 19, 19), 5);
	//parent_window->DrawList->AddRect(ImGui::GetWindowPos(), ImGui::GetWindowPos() + size_arg, ImColor(37, 37, 37), 5);*/
	//return ret;








}


bool ui::BeginChildNemesis(const char* str_id, const ImVec2& size_arg, bool border, ImGuiWindowFlags extra_flags)
{
	ImGuiWindow* window = GetCurrentWindow();
	return BeginChildExNemesis(str_id, window->GetID(str_id), size_arg, border, extra_flags);
}



void ui::EndChildNemesis()
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;

	IM_ASSERT(window->Flags & ImGuiWindowFlags_ChildWindow);   // Mismatched BeginChild()/EndChild() callss
	if (window->BeginCount > 1)
	{
		EndNemesis();
	}
	else
	{
		ImVec2 sz = window->Size;
		if (window->AutoFitChildAxises & (1 << ImGuiAxis_X)) // Arbitrary minimum zero-ish child size of 4.0f causes less trouble than a 0.0f
			sz.x = ImMax(4.0f, sz.x);
		if (window->AutoFitChildAxises & (1 << ImGuiAxis_Y))
			sz.y = ImMax(4.0f, sz.y);
		EndNemesis();

		ImGuiWindow* parent_window = g.CurrentWindow;
		ImRect bb(parent_window->DC.CursorPos, parent_window->DC.CursorPos + sz);
		ItemSize(sz);
		if ((window->DC.NavLayerActiveMask != 0 || window->DC.NavHasScroll) && !(window->Flags & ImGuiWindowFlags_NavFlattened))
		{
			ItemAdd(bb, window->ChildId);
			RenderNavHighlight(bb, window->ChildId);

			// When browsing a window that has no activable items (scroll only) we keep a highlight on the child
			if (window->DC.NavLayerActiveMask == 0 && window == g.NavWindow)
				RenderNavHighlight(ImRect(bb.Min - ImVec2(2, 2), bb.Max + ImVec2(2, 2)), g.NavId, ImGuiNavHighlightFlags_TypeThin);
		}
		else
		{
			// Not navigable into
			ItemAdd(bb, 0);
		}
	}
}
void ui::EndPopupNemesis()
{
	ImGuiContext& g = *GImGui;
	IM_ASSERT(g.CurrentWindow->Flags & ImGuiWindowFlags_Popup);  // Mismatched BeginPopup()/EndPopup() calls
	IM_ASSERT(g.BeginPopupStack.Size > 0);

	// Make all menus and popups wrap around for now, may need to expose that policy.
	NavMoveRequestTryWrapping(g.CurrentWindow, ImGuiNavMoveFlags_LoopY);

	EndNemesis(); //important
}
void ui::EndComboNemesis()
{
	EndPopupNemesis();
}







// FIXME: Move some of the code into SliderBehavior(). Current responsability is larger than what the equivalent DragBehaviorT<> does, we also do some rendering, etc.
template<typename TYPE, typename SIGNEDTYPE, typename FLOATTYPE>
bool SliderBehaviorTNemesis(const ImRect& bb, ImGuiID id, ImGuiDataType data_type, TYPE* v, const TYPE v_min, const TYPE v_max, const char* format, float power, ImGuiSliderFlags flags, ImRect* out_grab_bb)
{
	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	const ImGuiAxis axis = (flags & ImGuiSliderFlags_Vertical) ? ImGuiAxis_Y : ImGuiAxis_X;
	const bool is_decimal = (data_type == ImGuiDataType_Float) || (data_type == ImGuiDataType_Double);
	const bool is_power = (power != 1.0f) && is_decimal;

	const float grab_padding = 2.0f;
	const float slider_sz = (bb.Max[axis] - bb.Min[axis]) - grab_padding * 2.0f;
	float grab_sz = style.GrabMinSize;
	SIGNEDTYPE v_range = (v_min < v_max ? v_max - v_min : v_min - v_max);
	if (!is_decimal && v_range >= 0)                                             // v_range < 0 may happen on integer overflows
		grab_sz = ImMax((float)(slider_sz / (v_range + 1)), style.GrabMinSize);  // For integer sliders: if possible have the grab size represent 1 unit
	grab_sz = ImMin(grab_sz, slider_sz);
	const float slider_usable_sz = slider_sz - grab_sz;
	const float slider_usable_pos_min = bb.Min[axis] + grab_padding + grab_sz * 0.5f;
	const float slider_usable_pos_max = bb.Max[axis] - grab_padding - grab_sz * 0.5f;

	// For power curve sliders that cross over sign boundary we want the curve to be symmetric around 0.0f
	float linear_zero_pos;   // 0.0->1.0f
	if (is_power && v_min * v_max < 0.0f)
	{
		// Different sign
		const FLOATTYPE linear_dist_min_to_0 = ImPow(v_min >= 0 ? (FLOATTYPE)v_min : -(FLOATTYPE)v_min, (FLOATTYPE)1.0f / power);
		const FLOATTYPE linear_dist_max_to_0 = ImPow(v_max >= 0 ? (FLOATTYPE)v_max : -(FLOATTYPE)v_max, (FLOATTYPE)1.0f / power);
		linear_zero_pos = (float)(linear_dist_min_to_0 / (linear_dist_min_to_0 + linear_dist_max_to_0));
	}
	else
	{
		// Same sign
		linear_zero_pos = v_min < 0.0f ? 1.0f : 0.0f;
	}

	// Process interacting with the slider
	bool value_changed = false;
	if (g.ActiveId == id)
	{
		bool set_new_value = false;
		float clicked_t = 0.0f;
		if (g.ActiveIdSource == ImGuiInputSource_Mouse)
		{
			if (!g.IO.MouseDown[0])
			{
				ui::ClearActiveID();
			}
			else
			{
				const float mouse_abs_pos = g.IO.MousePos[axis];
				clicked_t = (slider_usable_sz > 0.0f) ? ImClamp((mouse_abs_pos - slider_usable_pos_min) / slider_usable_sz, 0.0f, 1.0f) : 0.0f;
				if (axis == ImGuiAxis_Y)
					clicked_t = 1.0f - clicked_t;
				set_new_value = true;
			}
		}
		else if (g.ActiveIdSource == ImGuiInputSource_Nav)
		{
			const ImVec2 delta2 = ui::GetNavInputAmount2d(ImGuiNavDirSourceFlags_Keyboard | ImGuiNavDirSourceFlags_PadDPad, ImGuiInputReadMode_RepeatFast, 0.0f, 0.0f);
			float delta = (axis == ImGuiAxis_X) ? delta2.x : -delta2.y;
			if (g.NavActivatePressedId == id && !g.ActiveIdIsJustActivated)
			{
				ui::ClearActiveID();
			}
			else if (delta != 0.0f)
			{
				clicked_t = ui::SliderCalcRatioFromValueT<TYPE, FLOATTYPE>(data_type, *v, v_min, v_max, power, linear_zero_pos);
				const int decimal_precision = is_decimal ? ImParseFormatPrecision(format, 3) : 0;
				if ((decimal_precision > 0) || is_power)
				{
					delta /= 100.0f;    // Gamepad/keyboard tweak speeds in % of slider bounds
					if (ui::IsNavInputDown(ImGuiNavInput_TweakSlow))
						delta /= 10.0f;
				}
				else
				{
					if ((v_range >= -100.0f && v_range <= 100.0f) || ui::IsNavInputDown(ImGuiNavInput_TweakSlow))
						delta = ((delta < 0.0f) ? -1.0f : +1.0f) / (float)v_range; // Gamepad/keyboard tweak speeds in integer steps
					else
						delta /= 100.0f;
				}
				if (ui::IsNavInputDown(ImGuiNavInput_TweakFast))
					delta *= 10.0f;
				set_new_value = true;
				if ((clicked_t >= 1.0f && delta > 0.0f) || (clicked_t <= 0.0f && delta < 0.0f)) // This is to avoid applying the saturation when already past the limits
					set_new_value = false;
				else
					clicked_t = ImSaturate(clicked_t + delta);
			}
		}

		if (set_new_value)
		{
			TYPE v_new;
			if (is_power)
			{
				// Account for power curve scale on both sides of the zero
				if (clicked_t < linear_zero_pos)
				{
					// Negative: rescale to the negative range before powering
					float a = 1.0f - (clicked_t / linear_zero_pos);
					a = ImPow(a, power);
					v_new = ImLerp(ImMin(v_max, (TYPE)0), v_min, a);
				}
				else
				{
					// Positive: rescale to the positive range before powering
					float a;
					if (ImFabs(linear_zero_pos - 1.0f) > 1.e-6f)
						a = (clicked_t - linear_zero_pos) / (1.0f - linear_zero_pos);
					else
						a = clicked_t;
					a = ImPow(a, power);
					v_new = ImLerp(ImMax(v_min, (TYPE)0), v_max, a);
				}
			}
			else
			{
				// Linear slider
				if (is_decimal)
				{
					v_new = ImLerp(v_min, v_max, clicked_t);
				}
				else
				{
					// For integer values we want the clicking position to match the grab box so we round above
					// This code is carefully tuned to work with large values (e.g. high ranges of U64) while preserving this property..
					FLOATTYPE v_new_off_f = (v_max - v_min) * clicked_t;
					TYPE v_new_off_floor = (TYPE)(v_new_off_f);
					TYPE v_new_off_round = (TYPE)(v_new_off_f + (FLOATTYPE)0.5);
					if (!is_decimal && v_new_off_floor < v_new_off_round)
						v_new = v_min + v_new_off_round;
					else
						v_new = v_min + v_new_off_floor;
				}
			}

			// Round to user desired precision based on format string
			v_new = ui::RoundScalarWithFormatT<TYPE, SIGNEDTYPE>(format, data_type, v_new);

			// Apply result
			if (*v != v_new)
			{
				*v = v_new;
				value_changed = true;
			}
		}
	}

	// Output grab position so it can be displayed by the caller
	float grab_t = ui::SliderCalcRatioFromValueT<TYPE, FLOATTYPE>(data_type, *v, v_min, v_max, power, linear_zero_pos);
	if (axis == ImGuiAxis_Y)
		grab_t = 1.0f - grab_t;
	const float grab_pos = ImLerp(slider_usable_pos_min, slider_usable_pos_max, grab_t);
	if (axis == ImGuiAxis_X)
		*out_grab_bb = ImRect(grab_pos - grab_sz * 0.5f, bb.Min.y + grab_padding, grab_pos + grab_sz * 0.5f, bb.Max.y - grab_padding);
	else
		*out_grab_bb = ImRect(bb.Min.x + grab_padding, grab_pos - grab_sz * 0.5f, bb.Max.x - grab_padding, grab_pos + grab_sz * 0.5f);

	return value_changed;
}

// For 32-bits and larger types, slider bounds are limited to half the natural type range.
// So e.g. an integer Slider between INT_MAX-10 and INT_MAX will fail, but an integer Slider between INT_MAX/2-10 and INT_MAX/2 will be ok.
// It would be possible to lift that limitation with some work but it doesn't seem to be worth it for sliders.
bool ui::SliderBehaviorNemesis(const ImRect& bb, ImGuiID id, ImGuiDataType data_type, void* v, const void* v_min, const void* v_max, const char* format, float power, ImGuiSliderFlags flags, ImRect* out_grab_bb)
{
	switch (data_type)
	{
	case ImGuiDataType_S32:
		IM_ASSERT(*(const ImS32*)v_min >= IM_S32_MIN / 2 && *(const ImS32*)v_max <= IM_S32_MAX / 2);
		return SliderBehaviorTNemesis<ImS32, ImS32, float >(bb, id, data_type, (ImS32*)v, *(const ImS32*)v_min, *(const ImS32*)v_max, format, power, flags, out_grab_bb);
	case ImGuiDataType_U32:
		IM_ASSERT(*(const ImU32*)v_min <= IM_U32_MAX / 2);
		return SliderBehaviorTNemesis<ImU32, ImS32, float >(bb, id, data_type, (ImU32*)v, *(const ImU32*)v_min, *(const ImU32*)v_max, format, power, flags, out_grab_bb);
	case ImGuiDataType_S64:
		IM_ASSERT(*(const ImS64*)v_min >= IM_S64_MIN / 2 && *(const ImS64*)v_max <= IM_S64_MAX / 2);
		return SliderBehaviorTNemesis<ImS64, ImS64, double>(bb, id, data_type, (ImS64*)v, *(const ImS64*)v_min, *(const ImS64*)v_max, format, power, flags, out_grab_bb);
	case ImGuiDataType_U64:
		IM_ASSERT(*(const ImU64*)v_min <= IM_U64_MAX / 2);
		return SliderBehaviorTNemesis<ImU64, ImS64, double>(bb, id, data_type, (ImU64*)v, *(const ImU64*)v_min, *(const ImU64*)v_max, format, power, flags, out_grab_bb);
	case ImGuiDataType_Float:
		IM_ASSERT(*(const float*)v_min >= -FLT_MAX / 2.0f && *(const float*)v_max <= FLT_MAX / 2.0f);
		return SliderBehaviorTNemesis<float, float, float >(bb, id, data_type, (float*)v, *(const float*)v_min, *(const float*)v_max, format, power, flags, out_grab_bb);
	case ImGuiDataType_Double:
		IM_ASSERT(*(const double*)v_min >= -DBL_MAX / 2.0f && *(const double*)v_max <= DBL_MAX / 2.0f);
		return SliderBehaviorTNemesis<double, double, double>(bb, id, data_type, (double*)v, *(const double*)v_min, *(const double*)v_max, format, power, flags, out_grab_bb);
	case ImGuiDataType_COUNT: break;
	}
	IM_ASSERT(0);
	return false;
}

bool ui::ButtonExDef(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags)
{
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	ImVec2 pos = window->DC.CursorPos;
	if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrentLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
		pos.y += window->DC.CurrentLineTextBaseOffset - style.FramePadding.y;
	ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

	const ImRect bb(pos, pos + size);
	ItemSize(size, style.FramePadding.y);
	if (!ItemAdd(bb, id))
		return false;

	if (window->DC.ItemFlags & ImGuiItemFlags_ButtonRepeat)
		flags |= ImGuiButtonFlags_Repeat;
	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);
	if (pressed)
		MarkItemEdited(id);

	// Render
	const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	RenderNavHighlight(bb, id);
	RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);
	RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);

	// Automatically close popups
	//if (pressed && !(flags & ImGuiButtonFlags_DontClosePopups) && (window->Flags & ImGuiWindowFlags_Popup))
	//    CloseCurrentPopup();

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.LastItemStatusFlags);
	return pressed;
}

bool ui::ButtonDef(const char* label, const ImVec2& size_arg)
{
	return ButtonExDef(label, size_arg, 0);
}