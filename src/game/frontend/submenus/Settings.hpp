#pragma once
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace YimMenu::Submenus
{
	// Window geometry persistence for the menu shell. This is intentionally
	// NOT a submenu anymore: interface sections live in their own submenus
	// (Visuals, Protection, Interface, About).
	class Settings
	{
	public:
		Settings() = default;

		int GetWindowWidth() const;
		int GetWindowHeight() const;
		int GetWindowPosX() const;
		int GetWindowPosY() const;
		bool IsWindowMaximized() const;

		void SetWindowWidth(int w);
		void SetWindowHeight(int h);
		void SetWindowPosX(int x);
		void SetWindowPosY(int y);
		void SetWindowMaximized(bool maximized);

		void LoadSettings();
		void SaveSettings();
		void SaveIfWindowChanged(int width, int height, int posX, int posY, bool maximized);

		// test harness override: when set, settings load/save here instead of %appdata%/RDOTerminator
		static void SetSettingsFileOverride(const std::string& path);
		static void ClearSettingsFileOverride();

	private:
		int m_WindowWidth = 800;
		int m_WindowHeight = 600;
		int m_WindowPosX = 100;
		int m_WindowPosY = 100;
		bool m_WindowMaximized = false;

		int m_LastSavedWidth = 800;
		int m_LastSavedHeight = 600;
		int m_LastSavedPosX = 100;
		int m_LastSavedPosY = 100;
		bool m_LastSavedMaximized = false;

		void LoadWindowSettings(const nlohmann::json& j);
		void SaveWindowSettings(nlohmann::json& j) const;
		std::string GetSettingsFilePath() const;
	};
}
