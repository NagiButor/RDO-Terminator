// RDO Terminator signature theme: the single, hardcoded visual identity.
// No theme switcher exists by design: one product, one look.
// Dark blue-charcoal surfaces, one signal-red accent, warm off-white text.
#pragma once

#include "imgui.h"

namespace YimMenu::Theme
{
	inline ImVec4 Accent()        { return ImVec4(0.90f, 0.16f, 0.19f, 1.f); }
	inline ImVec4 AccentBright()  { return ImVec4(1.00f, 0.30f, 0.32f, 1.f); }
	inline ImVec4 AccentDim()     { return ImVec4(0.45f, 0.11f, 0.12f, 1.f); }
	inline ImVec4 OkGreen()       { return ImVec4(0.30f, 0.75f, 0.35f, 1.f); }
	inline ImVec4 MutedText()     { return ImVec4(0.55f, 0.56f, 0.59f, 1.f); }

	inline void ApplySignatureTheme()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4* colors    = style.Colors;

		// single spacing grid for the whole product
		style.WindowPadding    = ImVec2(10.f, 8.f);
		style.FramePadding     = ImVec2(8.f, 5.f);
		style.CellPadding      = ImVec2(6.f, 3.f);
		style.ItemSpacing      = ImVec2(8.f, 6.f);
		style.ItemInnerSpacing = ImVec2(6.f, 4.f);
		style.ScrollbarSize    = 13.f;
		style.GrabMinSize      = 12.f;

		style.WindowRounding    = 8.f;
		style.ChildRounding     = 6.f;
		style.FrameRounding     = 6.f;
		style.PopupRounding     = 6.f;
		style.ScrollbarRounding = 6.f;
		style.GrabRounding      = 6.f;
		style.TabRounding       = 4.f;
		style.WindowBorderSize  = 1.f;
		style.ChildBorderSize   = 1.f;
		style.FrameBorderSize   = 0.f;
		style.PopupBorderSize   = 1.f;

		colors[ImGuiCol_Text]                  = ImVec4(0.93f, 0.91f, 0.87f, 1.00f);
		colors[ImGuiCol_TextDisabled]          = ImVec4(0.47f, 0.47f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg]              = ImVec4(0.043f, 0.051f, 0.070f, 1.00f);
		colors[ImGuiCol_ChildBg]               = ImVec4(0.063f, 0.078f, 0.106f, 1.00f);
		colors[ImGuiCol_PopupBg]               = ImVec4(0.075f, 0.094f, 0.129f, 0.98f);
		colors[ImGuiCol_Border]                = ImVec4(0.28f, 0.11f, 0.12f, 0.55f);
		colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg]               = ImVec4(0.094f, 0.118f, 0.157f, 1.00f);
		colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.35f, 0.11f, 0.12f, 1.00f);
		colors[ImGuiCol_FrameBgActive]         = ImVec4(0.55f, 0.13f, 0.14f, 1.00f);
		colors[ImGuiCol_TitleBg]               = ImVec4(0.043f, 0.051f, 0.070f, 1.00f);
		colors[ImGuiCol_TitleBgActive]         = ImVec4(0.060f, 0.070f, 0.095f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.043f, 0.051f, 0.070f, 1.00f);
		colors[ImGuiCol_MenuBarBg]             = ImVec4(0.060f, 0.070f, 0.095f, 1.00f);
		colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.040f, 0.050f, 0.070f, 1.00f);
		colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.22f, 0.25f, 0.31f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.35f, 0.15f, 0.16f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.55f, 0.15f, 0.16f, 1.00f);
		colors[ImGuiCol_CheckMark]             = ImVec4(0.95f, 0.28f, 0.30f, 1.00f);
		colors[ImGuiCol_SliderGrab]            = ImVec4(0.85f, 0.18f, 0.20f, 1.00f);
		colors[ImGuiCol_SliderGrabActive]      = ImVec4(1.00f, 0.30f, 0.32f, 1.00f);
		colors[ImGuiCol_Button]                = ImVec4(0.12f, 0.15f, 0.20f, 1.00f);
		colors[ImGuiCol_ButtonHovered]         = ImVec4(0.62f, 0.15f, 0.16f, 1.00f);
		colors[ImGuiCol_ButtonActive]          = ImVec4(0.80f, 0.18f, 0.20f, 1.00f);
		colors[ImGuiCol_Header]                = ImVec4(0.094f, 0.118f, 0.157f, 1.00f);
		colors[ImGuiCol_HeaderHovered]         = ImVec4(0.30f, 0.11f, 0.12f, 1.00f);
		colors[ImGuiCol_HeaderActive]          = ImVec4(0.55f, 0.13f, 0.14f, 1.00f);
		colors[ImGuiCol_Separator]             = ImVec4(0.30f, 0.13f, 0.14f, 0.55f);
		colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.55f, 0.15f, 0.16f, 1.00f);
		colors[ImGuiCol_SeparatorActive]       = ImVec4(0.80f, 0.18f, 0.20f, 1.00f);
		colors[ImGuiCol_ResizeGrip]            = ImVec4(0.35f, 0.15f, 0.16f, 0.6f);
		colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.62f, 0.15f, 0.16f, 1.00f);
		colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.85f, 0.18f, 0.20f, 1.00f);
		colors[ImGuiCol_Tab]                   = ImVec4(0.094f, 0.118f, 0.157f, 1.00f);
		colors[ImGuiCol_TabHovered]            = ImVec4(0.35f, 0.15f, 0.16f, 1.00f);
		colors[ImGuiCol_TabActive]             = ImVec4(0.45f, 0.12f, 0.13f, 1.00f);
		colors[ImGuiCol_TabUnfocused]          = ImVec4(0.070f, 0.085f, 0.115f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.13f, 0.16f, 0.21f, 1.00f);
		colors[ImGuiCol_PlotLines]             = ImVec4(0.85f, 0.25f, 0.28f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.35f, 0.37f, 1.00f);
		colors[ImGuiCol_PlotHistogram]         = ImVec4(0.75f, 0.20f, 0.22f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.90f, 0.28f, 0.30f, 1.00f);
		colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.10f, 0.12f, 0.16f, 1.00f);
		colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.28f, 0.11f, 0.12f, 0.55f);
		colors[ImGuiCol_TableBorderLight]      = ImVec4(0.16f, 0.18f, 0.23f, 1.00f);
		colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.03f);
		colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.55f, 0.13f, 0.14f, 0.55f);
		colors[ImGuiCol_DragDropTarget]        = ImVec4(0.90f, 0.20f, 0.22f, 0.90f);
		colors[ImGuiCol_NavHighlight]          = ImVec4(0.90f, 0.20f, 0.22f, 0.80f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.02f, 0.02f, 0.03f, 0.65f);
	}

	// Page/section header: accent mark + title + optional one-line description.
	// This is the single hierarchy primitive for every screen in the product.
	inline void DrawSectionTitle(const char* title, const char* desc = nullptr)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, Accent());
		ImGui::TextUnformatted("//");
		ImGui::PopStyleColor();
		ImGui::SameLine(0.f, 6.f);
		ImGui::TextUnformatted(title);
		if (desc)
		{
			ImGui::SameLine(0.f, 8.f);
			ImGui::PushStyleColor(ImGuiCol_Text, MutedText());
			ImGui::TextUnformatted(desc);
			ImGui::PopStyleColor();
		}
	}
}
