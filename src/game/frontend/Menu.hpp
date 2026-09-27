#pragma once
#include "game/frontend/GUI.hpp"
#include "game/frontend/submenus/Settings.hpp"

#include <functional>
#include <string>

#include "imgui.h"

namespace YimMenu::Menu
{
	// bump on every release so users can verify which build is loaded in game
	inline constexpr const char* kMenuVersion = "2026.09.22";

	extern void Init();
	extern void SetupStyle();
	extern void SetupFonts(const std::string& iniPath = {});

	// Dependencies the menu shell needs from its host (game or visual test).
	// Keeps MenuShell.cpp free of Renderer/GUI/MapEditor/FiberPool includes.
	struct ShellContext
	{
		bool m_IsOpen = false;
		ImTextureID m_Logo = 0;
		std::function<bool()> m_IsGodMode;
		std::function<void()> m_ToggleGodMode;
		std::function<void(std::function<void()>)> m_PushTask;
		YimMenu::Submenus::Settings* m_Settings = nullptr;
	};

	extern void DrawShell(const ShellContext& ctx);

	namespace Font
	{
		inline ImFont* g_DefaultFont   = nullptr;
		inline float g_DefaultFontSize = 19.0f;

	    inline ImFont* g_OptionsFont   = nullptr;
		inline float g_OptionsFontSize = 17.0f;

		inline ImFont* g_ChildTitleFont   = nullptr;
		inline float g_ChildTitleFontSize   = 14.5f;

		inline ImFont* g_ChatFont   = nullptr;
		inline float g_ChatFontSize = 24.0f;

		inline ImFont* g_OverlayFont = nullptr;
		inline float g_OverlayFontSize  = 30.0f;
	}
}