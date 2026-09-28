#include "About.hpp"

#include "core/commands/Commands.hpp"
#include "game/backend/FiberPool.hpp"
#include "game/frontend/items/Items.hpp"
#include "game/frontend/Menu.hpp"

namespace YimMenu::Submenus
{
    About::About() :
        Submenu::Submenu("About")
    {
        auto info = std::make_shared<Category>("About");

        info->AddItem(std::make_shared<ImGuiItem>([] {
            ImGui::TextUnformatted("RDO Terminator");
            ImGui::SameLine(0.f, 8.f);
            ImGui::TextDisabled(YimMenu::Menu::kMenuVersion);
            ImGui::Spacing();
            ImGui::TextDisabled("Universal game interface. One product, one signature look.");
            ImGui::Spacing();
            ImGui::TextUnformatted("Creator: tg:@Mi5hlen");
            ImGui::Spacing();
            ImGui::Separator();
            if (ImGui::Button("Unload cheat"))
            {
                FiberPool::Push([] {
                    Commands::Shutdown();
                    g_Running = false;
                });
            }
        }));

        AddCategory(std::move(info));
    }
}
