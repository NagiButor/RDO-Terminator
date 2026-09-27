// MenuShell: the actual cheat-menu window, shared by the in-game DLL and the
// visual-test harness. It only depends on UIManager, the Settings submenu
// (window geometry + theme colors) and an injected ShellContext, so it can
// run without Renderer, GUI, MapEditor, FiberPool or any game backend.
#include "game/frontend/Menu.hpp"

#include "core/frontend/manager/UIManager.hpp"
#include "game/frontend/fonts/Fonts.hpp"
#include "game/frontend/submenus/Settings.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>
#include <Windows.h>

namespace YimMenu::Menu
{
	void SetupStyle()
	{
		ImGuiStyle& style = ImGui::GetStyle();

		style.FramePadding = ImVec2(style.FramePadding.x * 0.5f, style.FramePadding.y * 0.4f); // Reduce FramePadding
		style.CellPadding  = ImVec2(style.CellPadding.x * 0.6f, style.CellPadding.y * 0.2f); // Reduce CellPadding
		style.ItemSpacing  = ImVec2(style.ItemSpacing.x * 0.5f, style.ItemSpacing.y * 0.3f); // Reduced vertical ItemSpacing
		style.ItemInnerSpacing = ImVec2(style.ItemInnerSpacing.x * 0.5f, style.ItemInnerSpacing.y * 0.5f); // Reduce ItemInnerSpacing
		style.WindowPadding    = ImVec2(style.WindowPadding.x * 0.5f, style.WindowPadding.y * 0.4f); // Reduced vertical WindowPadding

		YimMenu::Submenus::ApplyMenuColors();

		style.GrabRounding = style.FrameRounding = style.ChildRounding = style.WindowRounding = 6.0f; // Apply rounding
	}

	void SetupFonts(const std::string& iniPath)
	{
		auto& IO = ImGui::GetIO();
		static std::string path;
		if (iniPath.empty())
		{
			auto file_path = std::filesystem::path(std::getenv("appdata")) / "Mi5hlenMenu" / "imgui.ini";
			path           = file_path.string();
		}
		else
		{
			path = iniPath;
		}
		IO.IniFilename = path.c_str();
		IO.LogFilename = NULL;
		ImFontConfig FontCfg{};
		FontCfg.FontDataOwnedByAtlas = false;

		Menu::Font::g_DefaultFont = IO.Fonts->AddFontFromMemoryTTF(
		    const_cast<std::uint8_t*>(Fonts::MainFont), sizeof(Fonts::MainFont), 19.0f, &FontCfg);
		Menu::Font::g_OptionsFont = IO.Fonts->AddFontFromMemoryTTF(
		    const_cast<std::uint8_t*>(Fonts::MainFont), sizeof(Fonts::MainFont), 19.0f, &FontCfg);
		Menu::Font::g_ChildTitleFont = IO.Fonts->AddFontFromMemoryTTF(
		    const_cast<std::uint8_t*>(Fonts::MainFont), sizeof(Fonts::MainFont), 19.0f, &FontCfg);
		Menu::Font::g_ChatFont = IO.Fonts->AddFontFromMemoryTTF(
		    const_cast<std::uint8_t*>(Fonts::MainFont), sizeof(Fonts::MainFont), 22.0f, &FontCfg);
		Menu::Font::g_OverlayFont = IO.Fonts->AddFontFromMemoryTTF(
		    const_cast<std::uint8_t*>(Fonts::MainFont), sizeof(Fonts::MainFont), 16.0f, &FontCfg);
		Menu::Font::g_EspFont = IO.Fonts->AddFontDefault();

		UIManager::SetOptionsFont(Menu::Font::g_OptionsFont);
	}

	void DrawShell(const ShellContext& ctx)
	{
		if (!ctx.m_IsOpen || !ctx.m_Settings)
			return;

		ImGui::PushFont(Menu::Font::g_DefaultFont);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImU32(ImColor(0, 0, 0, 100)));

		static bool firstFrame = true;
		static int lastPosX = ctx.m_Settings->GetWindowPosX();
		static int lastPosY = ctx.m_Settings->GetWindowPosY();
		static int lastWidth = ctx.m_Settings->GetWindowWidth();
		static int lastHeight = ctx.m_Settings->GetWindowHeight();
		static bool lastMaximized = ctx.m_Settings->IsWindowMaximized();

		int screenW = GetSystemMetrics(SM_CXSCREEN);
		int screenH = GetSystemMetrics(SM_CYSCREEN);
		const int minW = 300, minH = 200, maxW = screenW, maxH = screenH;
		int posX = std::clamp(ctx.m_Settings->GetWindowPosX(), 0, screenW - minW);
		int posY = std::clamp(ctx.m_Settings->GetWindowPosY(), 0, screenH - minH);
		int width = std::clamp(ctx.m_Settings->GetWindowWidth(), minW, maxW);
		int height = std::clamp(ctx.m_Settings->GetWindowHeight(), minH, maxH);

		if (firstFrame) {
			ImGui::SetNextWindowPos(ImVec2((float)posX, (float)posY), ImGuiCond_Once);
			ImGui::SetNextWindowSize(ImVec2((float)width, (float)height), ImGuiCond_Once);
		}

		// never let the window (or a game minimize/restore cycle) squeeze it
		// into a slit the user then has to stretch back manually
		ImGui::SetNextWindowSizeConstraints(ImVec2((float)minW, (float)minH), ImVec2((float)maxW, (float)maxH));
		const bool window_visible = ImGui::Begin("##Mi5hlenMenu", nullptr,
		    ImGuiWindowFlags_NoTitleBar |
		    ImGuiWindowFlags_NoCollapse |
		    ImGuiWindowFlags_AlwaysUseWindowPadding |
		    ImGuiWindowFlags_NoSavedSettings);
		if (window_visible)
		{
			if (ctx.m_Logo)
			{
				ImGui::Image(ctx.m_Logo, ImVec2(30, 30));
				ImGui::SameLine();
			}
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
			ImGui::Text("RDO Terminator");
			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.25f, 0.25f, 1.0f));
			ImGui::Text("| by Mi5hlen");
			ImGui::SameLine();
			ImGui::TextDisabled(kMenuVersion);
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();

			float rainbow_t = std::fmod((float)ImGui::GetTime() * 0.3f, 1.0f);
			float rainbow_r, rainbow_g, rainbow_b;
			ImGui::ColorConvertHSVtoRGB(rainbow_t, 0.85f, 1.0f, rainbow_r, rainbow_g, rainbow_b);
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(rainbow_r, rainbow_g, rainbow_b, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.03f, 0.03f, 0.9f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.06f, 0.06f, 1.0f));
			if (ImGui::Button(ctx.m_IsGodMode && ctx.m_IsGodMode() ? "God Mode: ON" : "God Mode", ImVec2(140, 0)))
			{
				if (ctx.m_ToggleGodMode)
					ctx.m_PushTask([toggle = ctx.m_ToggleGodMode] {
						toggle();
					});
			}
			ImGui::PopStyleColor(3);

			UIManager::Draw();

			ImVec2 winPos = ImGui::GetWindowPos();
			ImVec2 winSize = ImGui::GetWindowSize();
			bool maximized = false;

			int curPosX = (int)winPos.x;
			int curPosY = (int)winPos.y;
			int curWidth = (int)winSize.x;
			int curHeight = (int)winSize.y;

			if ((firstFrame ||
			    curPosX != lastPosX || curPosY != lastPosY ||
			    curWidth != lastWidth || curHeight != lastHeight ||
			    maximized != lastMaximized)
			    && !ImGui::IsWindowCollapsed() && curWidth >= minW && curHeight >= minH)
			{
				// skip degenerate geometry (e.g. while the game is minimized)
				// so reopening the menu doesn't restore a collapsed slit
				ctx.m_Settings->SaveIfWindowChanged(curWidth, curHeight, curPosX, curPosY, maximized);
				lastPosX = curPosX;
				lastPosY = curPosY;
				lastWidth = curWidth;
				lastHeight = curHeight;
				lastMaximized = maximized;
			}
			firstFrame = false;
		}

		// End() must always be called, even when Begin() returned false (window clipped
		// or not visible). Skipping it corrupts the ImGui stack and every subsequent
		// frame renders broken.
		ImGui::End();

		ImGui::PopStyleColor();
		ImGui::PopFont();
	}
}
