#include "Menu.hpp"

#include "core/commands/Commands.hpp"
#include "core/frontend/manager/UIManager.hpp"
#include "core/renderer/Renderer.hpp"
#include "game/backend/FiberPool.hpp"
#include "game/backend/MapEditor/MapEditor.hpp"
#include "game/backend/ScriptMgr.hpp"
#include "game/frontend/fonts/Fonts.hpp"
#include "game/pointers/Pointers.hpp"
#include "submenus/About.hpp"
#include "submenus/Debug.hpp"
#include "submenus/Interface.hpp"
#include "submenus/Network.hpp"
#include "submenus/Players.hpp"
#include "submenus/Player.hpp"
#include "submenus/Protection.hpp"
#include "submenus/Vehicle.hpp"
#include "submenus/Visuals.hpp"
#include "submenus/Weapons.hpp"
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

		UIManager::AddSubmenu(std::make_shared<Submenus::Player>());
		UIManager::AddSubmenu(std::make_shared<Submenus::Visuals>());
		UIManager::AddSubmenu(std::make_shared<Submenus::Weapons>());
		UIManager::AddSubmenu(std::make_shared<Submenus::Vehicle>());
		UIManager::AddSubmenu(std::make_shared<Submenus::World>());
		UIManager::AddSubmenu(std::make_shared<Submenus::Players>());
		UIManager::AddSubmenu(std::make_shared<Submenus::Network>());
		UIManager::AddSubmenu(std::make_shared<Submenus::Protection>());
		UIManager::AddSubmenu(std::make_shared<Submenus::Recovery>());
		UIManager::AddSubmenu(std::make_shared<Submenus::Interface>());
		UIManager::AddSubmenu(std::make_shared<Submenus::About>());
		UIManager::AddSubmenu(std::make_shared<Submenus::Debug>());

		Renderer::AddRendererCallBack(
			[&] {
				static Menu::ShellContext ctx = [] {
					Menu::ShellContext c{};
					c.m_Actions.push_back({"God Mode",
					    [] {
						    return MapEditor::IsOpen();
					    },
					    [] {
						    MapEditor::Toggle();
					    }});
					c.m_OnClose = [] {
						GUI::SetOpen(false);
					};
					c.m_PushTask = [](std::function<void()> task) {
						FiberPool::Push(std::move(task));
					};
					c.m_Settings = &g_SettingsInstance;
					return c;
				}();
				ctx.m_IsOpen = GUI::IsOpen();
				ctx.m_Logo   = Renderer::GetLogoTexture();
				Menu::DrawShell(ctx);
			},
			-1);
	}
}
