// Signature theme + custom widgets for the menu shell.
//
// Everything visual goes through this file: the palette, the ImGui style, and
// the handful of drawn widgets (switch, stepper, card, tab, sidebar entry,
// vector icons, neon frame). Nothing here knows about the game, so the whole
// look can be reused for another product by editing the palette below and the
// strings in Branding.hpp.
#pragma once

#include "imgui.h"

#include <string_view>
#include <utility>
#include <vector>

namespace YimMenu::Theme
{
	// ---- palette -----------------------------------------------------------
	inline ImVec4 Accent()       { return ImVec4(0.92f, 0.10f, 0.13f, 1.f); }
	inline ImVec4 AccentBright() { return ImVec4(1.00f, 0.27f, 0.29f, 1.f); }
	inline ImVec4 AccentDim()    { return ImVec4(0.42f, 0.06f, 0.08f, 1.f); }
	inline ImVec4 OkGreen()      { return ImVec4(0.30f, 0.80f, 0.40f, 1.f); }
	inline ImVec4 Text()         { return ImVec4(0.93f, 0.93f, 0.95f, 1.f); }
	inline ImVec4 MutedText()    { return ImVec4(0.56f, 0.58f, 0.63f, 1.f); }
	inline ImVec4 WindowBg()     { return ImVec4(0.031f, 0.035f, 0.047f, 0.97f); }
	inline ImVec4 PanelBg()      { return ImVec4(0.047f, 0.055f, 0.071f, 1.f); }
	inline ImVec4 CardBg()       { return ImVec4(0.055f, 0.063f, 0.082f, 1.f); }
	inline ImVec4 ControlBg()    { return ImVec4(0.110f, 0.122f, 0.149f, 1.f); }

	// ImVec4 -> ImU32 with an extra alpha multiplier
	inline ImU32 Col(const ImVec4& c, float alpha = 1.f)
	{
		return ImGui::ColorConvertFloat4ToU32(ImVec4(c.x, c.y, c.z, c.w * alpha));
	}

	void ApplySignatureTheme();

	// ---- fonts (set by Menu::SetupFonts; fall back to the current font) ----
	inline ImFont* g_TitleFont   = nullptr; // big header title
	inline ImFont* g_HeadingFont = nullptr; // tabs, card titles, sidebar
	inline ImFont* g_SmallFont   = nullptr; // captions, status line

	// ---- drawing primitives -----------------------------------------------
	// Vector icons, drawn with the draw list so no icon font is needed.
	// Known names: gear user users eye crosshair shield globe wheel coin info
	// bug monitor network layers keyboard emblem dot
	void DrawIcon(ImDrawList* dl, std::string_view icon, ImVec2 center, float size, ImU32 col);
	// Best-effort icon for a submenu/category/group name ("Player ESP" -> user).
	// Returns "" when nothing matches.
	std::string_view IconForName(std::string_view name);

	// Chamfered (cut-corner) outline with a soft outer glow.
	void DrawNeonFrame(ImDrawList* dl, ImVec2 min, ImVec2 max, float cut, ImU32 col, float glow = 1.f);
	void FillChamfered(ImDrawList* dl, ImVec2 min, ImVec2 max, float cut, ImU32 col);
	// The red "//" mark used in front of section titles.
	void DrawSlashes(ImDrawList* dl, ImVec2 pos, float height, ImU32 col);
	// Text with extra spacing between letters (for small caps captions).
	void DrawSpacedText(ImDrawList* dl, ImVec2 pos, ImU32 col, const char* text, float spacing, ImFont* font = nullptr, float fontSize = 0.f);
	float CalcSpacedTextWidth(const char* text, float spacing, ImFont* font = nullptr, float fontSize = 0.f);

	// ---- widgets -----------------------------------------------------------
	// Animated on/off switch with a label on the right. Returns true on toggle.
	bool Switch(const char* label, bool* v);
	// "Label   [ <  Value  > ]" selector. Arrows cycle, the middle opens a list.
	bool Stepper(const char* label, const std::vector<std::pair<int, const char*>>& list, int* value);
	// Moves the cursor onto the current line at the shared value column
	// (color swatches, steppers). Call right after the row's first item.
	void SameLineValueColumn();
	// Small square color swatch with a picker popup. Returns true on change.
	bool ColorSwatch(const char* id, float* rgba);

	// Full-width panel with a "// icon Title" header; auto-sizes to its content.
	// Always pair with EndCard(), regardless of the return value.
	bool BeginCard(const char* title, const void* id = nullptr);
	void EndCard();

	// Left rail entry. Returns true when clicked.
	bool SidebarEntry(const char* label, std::string_view icon, bool active, float height);
	// Top tab. Returns true when clicked.
	bool TabButton(const char* label, std::string_view icon, bool active);
	// Square header button (close / minimize). Returns true when clicked.
	bool HeaderButton(const char* id, std::string_view glyph, ImVec2 size, bool danger);
	// Outlined pill for header actions; filled when active.
	bool Chip(const char* label, bool active);

	// Legacy one-line header, kept for callers outside the shell.
	void DrawSectionTitle(const char* title, const char* desc = nullptr);
}
