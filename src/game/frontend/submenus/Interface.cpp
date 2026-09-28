#include "Interface.hpp"

#include "core/commands/BoolCommand.hpp"
#include "core/commands/Commands.hpp"
#include "core/commands/HotkeySystem.hpp"
#include "game/frontend/items/Items.hpp"

namespace YimMenu::Submenus
{
    static BoolCommand g_UseInsertForMenuToggle("togglemenukey", "Menu Toggle Key", "Un-Ticked = F5, Ticked = Insert", false);

    static void DrawHotkeySettings()
    {
        ImGui::BulletText("Hold the command name clicked to change its hotkey");
        ImGui::BulletText("Press any registered key to remove");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        for (auto& [Hash, Command] : Commands::GetCommands())
        {
            ImGui::PushID(Hash);

            if (g_HotkeySystem.m_CommandHotkeys.find(Hash) != g_HotkeySystem.m_CommandHotkeys.end())
                HotkeySetter(Hash).Draw();

            ImGui::Spacing();
            ImGui::PopID();
        }
    }

    Interface::Interface() :
        Submenu::Submenu("Interface")
    {
        auto hotkeys = std::make_shared<Category>("Hotkeys");
        auto menu    = std::make_shared<Category>("Menu");

        auto miscGroup = std::make_shared<Group>("Menu");

        hotkeys->AddItem(std::make_shared<ImGuiItem>([] {
            ImGui::TextDisabled("Keyboard shortcuts for any command. Hold a name, press a key.");
        }));
        hotkeys->AddItem(std::make_shared<ImGuiItem>(DrawHotkeySettings));

        miscGroup->AddItem(std::make_shared<BoolCommandItem>("togglemenukey"_J));

        menu->AddItem(miscGroup);

        AddCategory(std::move(hotkeys));
        AddCategory(std::move(menu));
    }
}
