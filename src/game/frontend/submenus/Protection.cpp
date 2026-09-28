#include "Protection.hpp"

#include "game/frontend/items/Items.hpp"

namespace YimMenu::Submenus
{
    Protection::Protection() :
        Submenu::Submenu("Protection")
    {
        auto protection = std::make_shared<Category>("Protection");

        auto syncGroup         = std::make_shared<Group>("Sync");
        auto networkEventGroup = std::make_shared<Group>("Network Events");
        auto scriptEventGroup  = std::make_shared<Group>("Script Events");

        protection->AddItem(std::make_shared<ImGuiItem>([] {
            ImGui::TextDisabled("Block hostile sync, network and script events aimed at you.");
        }));

        syncGroup->AddItem(std::make_shared<BoolCommandItem>("blockspectate"_J));
        syncGroup->AddItem(std::make_shared<BoolCommandItem>("blockspectatesession"_J));
        syncGroup->AddItem(std::make_shared<BoolCommandItem>("blockattach"_J));
        syncGroup->AddItem(std::make_shared<BoolCommandItem>("blockvehflood"_J));

        networkEventGroup->AddItem(std::make_shared<BoolCommandItem>("blockexplosions"_J));
        networkEventGroup->AddItem(std::make_shared<BoolCommandItem>("blockptfx"_J));
        networkEventGroup->AddItem(std::make_shared<BoolCommandItem>("blockclearpedtasks"_J));
        networkEventGroup->AddItem(std::make_shared<BoolCommandItem>("blockscriptcommand"_J));
        networkEventGroup->AddItem(std::make_shared<BoolCommandItem>("attacklogfile"_J, "Attack Log File"));
        networkEventGroup->AddItem(std::make_shared<BoolCommandItem>("userelaycxns"_J));

        scriptEventGroup->AddItem(std::make_shared<BoolCommandItem>("blockhonormanipulation"_J));
        scriptEventGroup->AddItem(std::make_shared<BoolCommandItem>("blockdefensive"_J));
        scriptEventGroup->AddItem(std::make_shared<BoolCommandItem>("blockoffensive"_J));
        scriptEventGroup->AddItem(std::make_shared<BoolCommandItem>("blockpresscharges"_J));
        scriptEventGroup->AddItem(std::make_shared<BoolCommandItem>("blockstartparlay"_J));
        scriptEventGroup->AddItem(std::make_shared<BoolCommandItem>("blockendparlay"_J));
        scriptEventGroup->AddItem(std::make_shared<BoolCommandItem>("blocktickerspam"_J));
        scriptEventGroup->AddItem(std::make_shared<BoolCommandItem>("blockstableevents"_J));
        scriptEventGroup->AddItem(std::make_shared<BoolCommandItem>("blockkickfrommissionlobby"_J));

        protection->AddItem(syncGroup);
        protection->AddItem(networkEventGroup);
        protection->AddItem(scriptEventGroup);

        AddCategory(std::move(protection));
    }
}
