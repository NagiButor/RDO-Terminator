#include "Visuals.hpp"

#include "core/commands/Commands.hpp"
#include "core/commands/IntCommand.hpp"
#include "core/commands/ListCommand.hpp"
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
    }

    // shows `item` only while the master toggle is on
    static std::shared_ptr<UIItem> When(joaat_t toggle, std::shared_ptr<UIItem> item)
    {
        return std::make_shared<ConditionalItem>(toggle, std::move(item));
    }

    static std::shared_ptr<UIItem> Hint(std::function<void()> draw)
    {
        return std::make_shared<ImGuiItem>(std::move(draw));
    }

    // custom-color swatch, only while "Color By" is set to Custom
    static std::shared_ptr<UIItem> CustomColorSwatch(joaat_t colorMode, joaat_t color)
    {
        auto swatch = std::make_shared<ColorCommandItem>(color);
        return Hint([colorMode, swatch] {
            if (auto mode = Commands::GetCommand<ListCommand>(colorMode); mode && mode->GetState() == 2)
                swatch->Draw();
        });
    }

    static std::shared_ptr<Category> BuildPlayerEsp()
    {
        constexpr joaat_t on = "espdrawplayers"_J;
        auto cat = std::make_shared<Category>("Player ESP");

        auto general = std::make_shared<Group>("General");
        general->AddItem(std::make_shared<BoolCommandItem>(on));
        general->AddItem(When(on, std::make_shared<BoolCommandItem>("espdrawdeadplayers"_J, "Include Dead")));
        general->AddItem(When(on, std::make_shared<IntCommandItem>("esprenddistplayers"_J, "Render Distance")));
        general->AddItem(When(on, Hint([] {
            if (auto cmd = Commands::GetCommand<IntCommand>("esprenddistplayers"_J))
                ImGui::TextDisabled("Up to %s  -  Ctrl+Click the slider to type a value", FormatMeters(cmd->GetState()).c_str());
        })));

        auto info = std::make_shared<Group>("Info");
        info->AddItem(std::make_shared<BoolCommandItem>("espnameplayers"_J, "Name"));
        info->AddItem(std::make_shared<ColorCommandItem>("namecolorplayers"_J));
        info->AddItem(std::make_shared<BoolCommandItem>("esphealthplayers"_J, "Health Bar"));
        info->AddItem(std::make_shared<BoolCommandItem>("espdistanceplayers"_J, "Distance"));
        info->AddItem(std::make_shared<ColorCommandItem>("distancecolorplayers"_J));

        auto shape = std::make_shared<Group>("Box & Lines");
        shape->AddItem(std::make_shared<ListCommandItem>("espboxplayers"_J, "Box Style"));
        shape->AddItem(std::make_shared<ListCommandItem>("espcolormodeplayers"_J, "Color By"));
        shape->AddItem(CustomColorSwatch("espcolormodeplayers"_J, "boxcolorplayers"_J));
        shape->AddItem(std::make_shared<BoolCommandItem>("espsnaplineplayers"_J, "Snapline"));
        shape->AddItem(std::make_shared<BoolCommandItem>("espskeletonplayers"_J, "Skeleton"));
        shape->AddItem(std::make_shared<ColorCommandItem>("skeletoncolorplayers"_J));

        cat->AddItem(general);
        cat->AddItem(When(on, info));
        cat->AddItem(When(on, shape));
        return cat;
    }

    static std::shared_ptr<Category> BuildPedEsp()
    {
        constexpr joaat_t on = "espdrawpeds"_J;
        auto cat = std::make_shared<Category>("Ped ESP");

        auto general = std::make_shared<Group>("General");
        general->AddItem(std::make_shared<BoolCommandItem>(on));
        general->AddItem(When(on, std::make_shared<BoolCommandItem>("espdrawdeadpeds"_J, "Include Dead")));
        general->AddItem(When(on, std::make_shared<BoolCommandItem>("espanimalspeds"_J)));
        general->AddItem(When(on, std::make_shared<IntCommandItem>("esprenddistpeds"_J, "Render Distance")));
        general->AddItem(When(on, Hint([] {
            if (auto cmd = Commands::GetCommand<IntCommand>("esprenddistpeds"_J))
                ImGui::TextDisabled("Up to %s  -  Ctrl+Click the slider to type a value", FormatMeters(cmd->GetState()).c_str());
        })));

        auto info = std::make_shared<Group>("Info");
        info->AddItem(std::make_shared<BoolCommandItem>("espmodelspeds"_J, "Model / Hash"));
        info->AddItem(std::make_shared<ColorCommandItem>("hashcolorpeds"_J));
        info->AddItem(std::make_shared<BoolCommandItem>("espnetinfopeds"_J, "Network Owner"));
        info->AddItem(std::make_shared<BoolCommandItem>("espscriptinfopeds"_J, "Script"));
        info->AddItem(std::make_shared<BoolCommandItem>("esphealthpeds"_J, "Health Bar"));
        info->AddItem(std::make_shared<BoolCommandItem>("espdistancepeds"_J, "Distance"));
        info->AddItem(std::make_shared<ColorCommandItem>("distancecolorpeds"_J));

        auto shape = std::make_shared<Group>("Box & Lines");
        shape->AddItem(std::make_shared<ListCommandItem>("espboxpeds"_J, "Box Style"));
        shape->AddItem(std::make_shared<ListCommandItem>("espcolormodepeds"_J, "Color By"));
        shape->AddItem(CustomColorSwatch("espcolormodepeds"_J, "boxcolorpeds"_J));
        shape->AddItem(std::make_shared<BoolCommandItem>("espsnaplinepeds"_J, "Snapline"));
        shape->AddItem(std::make_shared<BoolCommandItem>("espskeletonpeds"_J, "Skeleton"));
        shape->AddItem(std::make_shared<ColorCommandItem>("skeletoncolorpeds"_J));
        shape->AddItem(std::make_shared<BoolCommandItem>("espskeletonhorse"_J, "Horse Skeleton"));
        shape->AddItem(std::make_shared<ColorCommandItem>("skeletoncolorhorse"_J));

        cat->AddItem(general);
        cat->AddItem(When(on, info));
        cat->AddItem(When(on, shape));
        return cat;
    }

    static std::shared_ptr<Category> BuildEspStyle()
    {
        auto cat = std::make_shared<Category>("ESP Style");

        auto look = std::make_shared<Group>("Look");
        look->AddItem(std::make_shared<BoolCommandItem>("espoutline"_J));
        look->AddItem(std::make_shared<BoolCommandItem>("esptagbg"_J));
        look->AddItem(std::make_shared<ListCommandItem>("espfontstyle"_J));
        look->AddItem(std::make_shared<BoolCommandItem>("espskeletonhead"_J));

        auto behavior = std::make_shared<Group>("Behavior");
        behavior->AddItem(std::make_shared<BoolCommandItem>("espdistancefade"_J));
        behavior->AddItem(std::make_shared<BoolCommandItem>("espvisibilitycheck"_J));
        behavior->AddItem(Hint([] {
            ImGui::TextDisabled("Targets without line of sight are drawn dimmed.");
        }));
        behavior->AddItem(std::make_shared<ListCommandItem>("espsnaporigin"_J));

        cat->AddItem(look);
        cat->AddItem(behavior);
        return cat;
    }

    Visuals::Visuals() :
        Submenu::Submenu("Visuals")
    {
        auto overlay = std::make_shared<Category>("Overlay");

        auto overlayGroup = std::make_shared<Group>("Overlay");
        auto context      = std::make_shared<Group>("Context Menu");

        overlayGroup->AddItem(std::make_shared<BoolCommandItem>("overlay"_J));
        overlayGroup->AddItem(std::make_shared<ConditionalItem>("overlay"_J, std::make_shared<BoolCommandItem>("overlayfps"_J)));

        context->AddItem(std::make_shared<BoolCommandItem>("ctxmenu"_J));
        context->AddItem(std::make_shared<ConditionalItem>("ctxmenu"_J, std::make_shared<BoolCommandItem>("ctxmenuplayers"_J, "Players")));
        context->AddItem(std::make_shared<ConditionalItem>("ctxmenu"_J, std::make_shared<BoolCommandItem>("ctxmenupeds"_J, "Peds")));
        context->AddItem(std::make_shared<ConditionalItem>("ctxmenu"_J, std::make_shared<BoolCommandItem>("ctxmenuvehicles"_J, "Vehicles")));
        context->AddItem(std::make_shared<ConditionalItem>("ctxmenu"_J, std::make_shared<BoolCommandItem>("ctxmenuobjects"_J, "Objects")));

        overlay->AddItem(overlayGroup);
        overlay->AddItem(context);

        AddCategory(BuildPlayerEsp());
        AddCategory(BuildPedEsp());
        AddCategory(BuildEspStyle());
        AddCategory(std::move(overlay));
    }
}
