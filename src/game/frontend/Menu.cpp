#include "Menu.hpp"

#include "core/commands/Commands.hpp"
#include "core/frontend/manager/UIManager.hpp"
#include "core/renderer/Renderer.hpp"
#include "game/backend/FiberPool.hpp"
#include "game/backend/MapEditor/MapEditor.hpp"
#include "game/backend/ScriptMgr.hpp"
#include "game/frontend/fonts/Fonts.hpp"
#include "game/pointers/Pointers.hpp"
#include "submenus/Debug.hpp"
#include "submenus/Network.hpp"
#include "submenus/Players.hpp"
#include "submenus/Self.hpp"
#include "submenus/Settings.hpp"
#include "submenus/Teleport.hpp"
#include "submenus/World.hpp"
#include "submenus/Recovery.hpp"

#include <algorithm>
#include <cmath>
#include <Windows.h>

namespace YimMenu
{
	// bump on every release so users can verify which build is loaded in game
	constexpr const char* kMenuVersion = "2026.09.22";

	static YimMenu::Submenus::Settings g_SettingsInstance;

	void Menu::Init()
	{
		g_SettingsInstance.LoadSettings();

		UIManager::AddSubmenu(std::make_shared<Submenus::Self>());
		UIManager::AddSubmenu(std::make_shared<Submenus::Teleport>());
		UIManager::AddSubmenu(std::make_shared<Submenus::Network>());
		UIManager::AddSubmenu(std::make_shared<Submenus::Players>());
		UIManager::AddSubmenu(std::make_shared<Submenus::World>());
		UIManager::AddSubmenu(std::make_shared<Submenus::Recovery>());
		UIManager::AddSubmenu(std::make_shared<Submenus::Settings>());
		UIManager::AddSubmenu(std::make_shared<Submenus::Debug>());

		// one-time theme migration for installs that still carry the old blue colors
		FiberPool::Push([] {
			ScriptMgr::Yield(std::chrono::seconds(5));
			Submenus::MigrateThemeOnce();
		});

		Renderer::AddRendererCallBack(
			[&] {
				if (!GUI::IsOpen())
					return;

				ImGui::PushFont(Menu::Font::g_DefaultFont);
				ImGui::PushStyleColor(ImGuiCol_WindowBg, ImU32(ImColor(0, 0, 0, 100)));

				static bool firstFrame = true;
				static int lastPosX = g_SettingsInstance.GetWindowPosX();
				static int lastPosY = g_SettingsInstance.GetWindowPosY();
				static int lastWidth = g_SettingsInstance.GetWindowWidth();
				static int lastHeight = g_SettingsInstance.GetWindowHeight();
				static bool lastMaximized = g_SettingsInstance.IsWindowMaximized();

				int screenW = GetSystemMetrics(SM_CXSCREEN);
				int screenH = GetSystemMetrics(SM_CYSCREEN);
				const int minW = 300, minH = 200, maxW = screenW, maxH = screenH;
				int posX = std::clamp(g_SettingsInstance.GetWindowPosX(), 0, screenW - minW);
				int posY = std::clamp(g_SettingsInstance.GetWindowPosY(), 0, screenH - minH);
				int width = std::clamp(g_SettingsInstance.GetWindowWidth(), minW, maxW);
				int height = std::clamp(g_SettingsInstance.GetWindowHeight(), minH, maxH);

				if (firstFrame) {
					ImGui::SetNextWindowPos(ImVec2((float)posX, (float)posY), ImGuiCond_Once);
					ImGui::SetNextWindowSize(ImVec2((float)width, (float)height), ImGuiCond_Once);
				}

				const bool window_visible = ImGui::Begin("##Mi5hlenMenu", nullptr,
					ImGuiWindowFlags_NoTitleBar |
					ImGuiWindowFlags_NoCollapse |
					ImGuiWindowFlags_AlwaysUseWindowPadding |
					ImGuiWindowFlags_NoSavedSettings);
				if (window_visible)
				{
				if (ImTextureID logo = Renderer::GetLogoTexture())
				{
					ImGui::Image(logo, ImVec2(30, 30));
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
					if (ImGui::Button(MapEditor::IsOpen() ? "God Mode: ON" : "God Mode", ImVec2(140, 0)))
					{
						FiberPool::Push([] {
							MapEditor::Toggle();
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

					if (firstFrame ||
						curPosX != lastPosX || curPosY != lastPosY ||
						curWidth != lastWidth || curHeight != lastHeight ||
						maximized != lastMaximized)
					{
						g_SettingsInstance.SaveIfWindowChanged(curWidth, curHeight, curPosX, curPosY, maximized);
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
			},
			-1);
	}

	void Menu::SetupStyle()
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

	void Menu::SetupFonts()
	{
		auto& IO       = ImGui::GetIO();
		auto file_path = std::filesystem::path(std::getenv("appdata")) / "Mi5hlenMenu" / "imgui.ini";
		static auto path = file_path.string();
		IO.IniFilename   = path.c_str();
		IO.LogFilename   = NULL;
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

		UIManager::SetOptionsFont(Menu::Font::g_OptionsFont);
	}
}