#include "Vehicle.hpp"

#include "game/backend/FiberPool.hpp"
#include "game/backend/Self.hpp"
#include "game/frontend/items/Items.hpp"

namespace YimMenu::Submenus
{
    Vehicle::Vehicle() :
        Submenu::Submenu("Vehicle")
    {
        auto horse = std::make_shared<Category>("Horse");
        auto horseGlobalsGroup = std::make_shared<Group>("Globals");
        horseGlobalsGroup->AddItem(std::make_shared<BoolCommandItem>("horsegodmode"_J));
        horseGlobalsGroup->AddItem(std::make_shared<BoolCommandItem>("horsenoragdoll"_J));
        horseGlobalsGroup->AddItem(std::make_shared<BoolCommandItem>("horsesuperrun"_J));
        horseGlobalsGroup->AddItem(std::make_shared<BoolCommandItem>("keephorseclean"_J));
        horseGlobalsGroup->AddItem(std::make_shared<BoolCommandItem>("horseclimbsteepslopes"_J));
        horseGlobalsGroup->AddItem(std::make_shared<BoolCommandItem>("keephorsebarsfilled"_J));
        horseGlobalsGroup->AddItem(std::make_shared<BoolCommandItem>("keephorsecoresfilled"_J));
        horseGlobalsGroup->AddItem(std::make_shared<BoolCommandItem>("keephorseagitationlow"_J));
        horseGlobalsGroup->AddItem(std::make_shared<BoolCommandItem>("flaminghooves"_J));
        horseGlobalsGroup->AddItem(std::make_shared<CommandItem>("tpmounttoself"_J));

        static float horseScale = 1.0f;
        horseGlobalsGroup->AddItem(std::make_shared<ImGuiItem>([] {
            ImGui::Text("Horse Scale");
            ImGui::SetNextItemWidth(100.0f);
            if (ImGui::InputFloat("##HorseScale", &horseScale))
            {
                FiberPool::Push([] {
                    YimMenu::Self::GetMount().SetScale(horseScale);
                });
            }
        }));
        horse->AddItem(horseGlobalsGroup);
        AddCategory(std::move(horse));

        auto vehicle = std::make_shared<Category>("Vehicle");
        auto vehicleGlobalsGroup = std::make_shared<Group>("Globals");
        auto vehicleFunGroup = std::make_shared<Group>("Fun");
        vehicleGlobalsGroup->AddItem(std::make_shared<BoolCommandItem>("vehiclegodmode"_J));
        vehicleGlobalsGroup->AddItem(std::make_shared<BoolCommandItem>("vehiclenodetach"_J));
        vehicleGlobalsGroup->AddItem(std::make_shared<BoolCommandItem>("flaminghoovesdraft"_J));
        vehicleGlobalsGroup->AddItem(std::make_shared<CommandItem>("repairvehicle"_J));
        vehicleFunGroup->AddItem(std::make_shared<BoolCommandItem>("superdrive"_J));
        vehicleFunGroup->AddItem(std::make_shared<ConditionalItem>("superdrive"_J, std::make_shared<BoolCommandItem>("superdrivedirectional"_J, "Directional")));
        vehicleFunGroup->AddItem(std::make_shared<ConditionalItem>("superdrive"_J, std::make_shared<IntCommandItem>("superdriveforce"_J, "Force")));
        vehicleFunGroup->AddItem(std::make_shared<BoolCommandItem>("superbrake"_J));
        vehicle->AddItem(vehicleGlobalsGroup);
        vehicle->AddItem(vehicleFunGroup);
        AddCategory(std::move(vehicle));
    }
}
