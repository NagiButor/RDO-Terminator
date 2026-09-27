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
				Menu::ShellContext ctx{};
				ctx.m_IsOpen = GUI::IsOpen();
				ctx.m_Logo   = Renderer::GetLogoTexture();
				ctx.m_IsGodMode = [] {
					return MapEditor::IsOpen();
				};
				ctx.m_ToggleGodMode = [] {
					MapEditor::Toggle();
				};
				ctx.m_PushTask = [](std::function<void()> task) {
					FiberPool::Push(std::move(task));
				};
				ctx.m_Settings = &g_SettingsInstance;
				Menu::DrawShell(ctx);
			},
			-1);
	}
}
