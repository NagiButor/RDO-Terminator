#include "Visuals.hpp"

#include "core/commands/Commands.hpp"
#include "core/commands/IntCommand.hpp"
#include "game/frontend/items/Items.hpp"

#include <string>

namespace YimMenu::Submenus
{
    static std::string FormatMeters(int meters)
    {
        std::string s = std::to_string(meters);
        for (int i = (int)s.size() - 3; i > 0; i -= 3)
            s.insert((size_t)i, " ");
        return s + " m";
    }    Visuals::Visuals() :
        Submenu::Submenu("Visuals")
    {
        auto esp     = std::make_shared<Category>("ESP");
        auto overlay = std::make_shared<Category>("Overlay");

        auto playerEsp = std::make_shared<Group>("Player ESP", 10);
        auto pedEsp    = std::make_shared<Group>("Ped ESP", 10);
        auto overlayGroup = std::make_shared<Group>("Overlay");
        auto context   = std::make_shared<Group>("Context Menu");

        esp->AddItem(std::make_shared<ImGuiItem>([] {
            ImGui::TextDisabled("Outlines, boxes, bars and tags drawn over actors in the world.");
        }));
        playerEsp->AddItem(std::make_shared<BoolCommandItem>("espdrawplayers"_J));
        playerEsp->AddItem(std::make_shared<ConditionalItem>("espdrawplayers"_J, std::make_shared<BoolCommandItem>("espdrawdeadplayers"_J)));

        playerEsp->AddItem(std::make_shared<ConditionalItem>("espdrawplayers"_J, std::make_shared<BoolCommandItem>("espnameplayers"_J, "Player Name")));
        playerEsp->AddItem(std::make_shared<ConditionalItem>("espdrawplayers"_J, std::make_shared<ColorCommandItem>("namecolorplayers"_J)));
        playerEsp->AddItem(std::make_shared<ConditionalItem>("espdrawplayers"_J, std::make_shared<BoolCommandItem>("esphealthplayers"_J, "Player Health")));

        playerEsp->AddItem(std::make_shared<ConditionalItem>("espdrawplayers"_J, std::make_shared<BoolCommandItem>("espdistanceplayers"_J, "Player Distance")));
        playerEsp->AddItem(std::make_shared<ConditionalItem>("espdrawplayers"_J, std::make_shared<ColorCommandItem>("distancecolorplayers"_J)));

        playerEsp->AddItem(std::make_shared<ConditionalItem>("espdrawplayers"_J, std::make_shared<BoolCommandItem>("espskeletonplayers"_J, "Player Skeleton")));
        playerEsp->AddItem(std::make_shared<ConditionalItem>("espdrawplayers"_J, std::make_shared<ColorCommandItem>("skeletoncolorplayers"_J)));

        playerEsp->AddItem(std::make_shared<ConditionalItem>("espdrawplayers"_J, std::make_shared<ListCommandItem>("espboxplayers"_J, "Box Style")));
        playerEsp->AddItem(std::make_shared<ConditionalItem>("espdrawplayers"_J, std::make_shared<BoolCommandItem>("espsnaplineplayers"_J, "Snapline")));
        playerEsp->AddItem(std::make_shared<ConditionalItem>("espdrawplayers"_J, std::make_shared<IntCommandItem>("esprenddistplayers"_J, "Render Distance")));
        playerEsp->AddItem(std::make_shared<ConditionalItem>("espdrawplayers"_J, std::make_shared<ImGuiItem>([] {
            if (auto cmd = Commands::GetCommand<IntCommand>("esprenddistplayers"_J))
                ImGui::TextDisabled("Draws players up to %s (Ctrl+Click the slider for an exact value)", FormatMeters(cmd->GetState()).c_str());
        })));

        pedEsp->AddItem(std::make_shared<BoolCommandItem>("espdrawpeds"_J));
        pedEsp->AddItem(std::make_shared<ConditionalItem>("espdrawpeds"_J, std::make_shared<BoolCommandItem>("espdrawdeadpeds"_J)));

        pedEsp->AddItem(std::make_shared<ConditionalItem>("espdrawpeds"_J, std::make_shared<BoolCommandItem>("espmodelspeds"_J, "Ped Hashes")));
        pedEsp->AddItem(std::make_shared<ConditionalItem>("espdrawpeds"_J, std::make_shared<ColorCommandItem>("hashcolorpeds"_J)));

        pedEsp->AddItem(std::make_shared<ConditionalItem>("espdrawpeds"_J, std::make_shared<BoolCommandItem>("espnetinfopeds"_J, "Ped Net Info")));
        pedEsp->AddItem(std::make_shared<ConditionalItem>("espdrawpeds"_J, std::make_shared<BoolCommandItem>("espscriptinfopeds"_J, "Ped Script Info")));

        pedEsp->AddItem(std::make_shared<ConditionalItem>("espdrawpeds"_J, std::make_shared<BoolCommandItem>("espdistancepeds"_J, "Ped Distance")));
        pedEsp->AddItem(std::make_shared<ConditionalItem>("espdrawpeds"_J, std::make_shared<ColorCommandItem>("distancecolorpeds"_J)));

        pedEsp->AddItem(std::make_shared<ConditionalItem>("espdrawpeds"_J, std::make_shared<BoolCommandItem>("espskeletonpeds"_J, "Ped Skeleton")));
        pedEsp->AddItem(std::make_shared<ConditionalItem>("espdrawpeds"_J, std::make_shared<ColorCommandItem>("skeletoncolorpeds"_J)));

        pedEsp->AddItem(std::make_shared<ConditionalItem>("espdrawpeds"_J, std::make_shared<BoolCommandItem>("espskeletonhorse"_J, "Horse Skeleton")));
        pedEsp->AddItem(std::make_shared<ConditionalItem>("espdrawpeds"_J, std::make_shared<ColorCommandItem>("skeletoncolorhorse"_J)));

        pedEsp->AddItem(std::make_shared<ConditionalItem>("espdrawpeds"_J, std::make_shared<ListCommandItem>("espboxpeds"_J, "Box Style")));
        pedEsp->AddItem(std::make_shared<ConditionalItem>("espdrawpeds"_J, std::make_shared<BoolCommandItem>("espsnaplinepeds"_J, "Snapline")));
        pedEsp->AddItem(std::make_shared<ConditionalItem>("espdrawpeds"_J, std::make_shared<IntCommandItem>("esprenddistpeds"_J, "Render Distance")));
        pedEsp->AddItem(std::make_shared<ConditionalItem>("espdrawpeds"_J, std::make_shared<ImGuiItem>([] {
            if (auto cmd = Commands::GetCommand<IntCommand>("esprenddistpeds"_J))
                ImGui::TextDisabled("Draws peds up to %s (Ctrl+Click the slider for an exact value)", FormatMeters(cmd->GetState()).c_str());
        })));

        overlayGroup->AddItem(std::make_shared<BoolCommandItem>("overlay"_J));
        overlayGroup->AddItem(std::make_shared<ConditionalItem>("overlay"_J, std::make_shared<BoolCommandItem>("overlayfps"_J)));

        context->AddItem(std::make_shared<BoolCommandItem>("ctxmenu"_J));
        context->AddItem(std::make_shared<ConditionalItem>("ctxmenu"_J, std::make_shared<BoolCommandItem>("ctxmenuplayers"_J, "Players")));
        context->AddItem(std::make_shared<ConditionalItem>("ctxmenu"_J, std::make_shared<BoolCommandItem>("ctxmenupeds"_J, "Peds")));
        context->AddItem(std::make_shared<ConditionalItem>("ctxmenu"_J, std::make_shared<BoolCommandItem>("ctxmenuvehicles"_J, "Vehicles")));
        context->AddItem(std::make_shared<ConditionalItem>("ctxmenu"_J, std::make_shared<BoolCommandItem>("ctxmenuobjects"_J, "Objects")));

        esp->AddItem(playerEsp);
        esp->AddItem(pedEsp);
        overlay->AddItem(overlayGroup);
        overlay->AddItem(context);

        AddCategory(std::move(esp));
        AddCategory(std::move(overlay));
    }
}
