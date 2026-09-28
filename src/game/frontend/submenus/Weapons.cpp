#include "Weapons.hpp"

#include "core/commands/Commands.hpp"
#include "core/commands/IntCommand.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/backend/FiberPool.hpp"
#include "game/backend/Self.hpp"
#include "game/features/self/GiveAllFunctions.hpp"
#include "game/frontend/items/Items.hpp"
#include "game/rdr/Natives.hpp"

namespace YimMenu::Submenus
{
    Weapons::Weapons() :
        Submenu::Submenu("Weapons")
    {
        auto weapons = std::make_shared<Category>("Weapons");
        auto weaponsGlobalsGroup = std::make_shared<Group>("Globals");
        weaponsGlobalsGroup->AddItem(std::make_shared<BoolCommandItem>("infiniteammo"_J));
        weaponsGlobalsGroup->AddItem(std::make_shared<BoolCommandItem>("infiniteclip"_J));
        weaponsGlobalsGroup->AddItem(std::make_shared<BoolCommandItem>("nospread"_J));
        weaponsGlobalsGroup->AddItem(std::make_shared<BoolCommandItem>("autocock"_J));
        weaponsGlobalsGroup->AddItem(std::make_shared<BoolCommandItem>("keepgunsclean"_J));
        weaponsGlobalsGroup->AddItem(std::make_shared<BoolCommandItem>("hidecrosshair"_J));

        weaponsGlobalsGroup->AddItem(std::make_shared<ImGuiItem>([] {
            if (ImGui::Button("Give All Weapons"))
            {
                FiberPool::Push([] {
                    YimMenu::Features::TriggerGiveAllWeapons();
                });
            }
            if (ImGui::Button("Give All Ammo"))
            {
                FiberPool::Push([] {
                    YimMenu::Features::TriggerGiveAllAmmo();
                });
            }
        }));

        weapons->AddItem(weaponsGlobalsGroup);

        auto rareWeaponsGroup = std::make_shared<Group>("Rare & Scopes");
        rareWeaponsGroup->AddItem(std::make_shared<ImGuiItem>([] {
            auto give_rare = [](Hash weapon, const char* label) {
                FiberPool::Push([weapon, label] {
                    const int ped = YimMenu::Self::GetPed().GetHandle();
                    if (!ENTITY::DOES_ENTITY_EXIST(ped))
                        return;
                    WEAPON::GIVE_WEAPON_TO_PED(ped, weapon, 200, true, false, 0, true, 0.5f, 1.0f, 0x2CD419DC, true, 0.0f, false);
                    WEAPON::SET_CURRENT_PED_WEAPON(ped, weapon, true, 0, false, false);
                    // verify instead of blindly reporting success
                    if (WEAPON::HAS_PED_GOT_WEAPON(ped, weapon, 0, false))
                        Notifications::Show("Weapons", std::string(label) + " added and equipped.", NotificationType::Success);
                    else
                        Notifications::Show("Weapons", std::string(label) + ": game refused it (inventory sync).", NotificationType::Error);
                });
            };

            if (ImGui::Button("Give M1899 Pistol", ImVec2(-1, 0)))
                give_rare("WEAPON_PISTOL_M1899"_J, "M1899 Pistol");
            if (ImGui::Button("Give Midnight's Pistol", ImVec2(-1, 0)))
                give_rare("WEAPON_PISTOL_MAUSER_DRUNK"_J, "Midnight's Pistol");

            ImGui::Separator();

            ImGui::TextDisabled("M1899 works via Give All too.");
        }));
        rareWeaponsGroup->AddItem(std::make_shared<BoolCommandItem>("nosniperscope"_J));
        weapons->AddItem(rareWeaponsGroup);

        weapons->AddItem(std::make_shared<ImGuiItem>([] {
            ImGui::SeparatorText("Old Deadeye");
        }));
        weapons->AddItem(std::make_shared<BoolCommandItem>("olddeadeye"_J));
        weapons->AddItem(std::make_shared<ConditionalItem>("olddeadeye"_J, std::make_shared<BoolCommandItem>("unlockdeadeyeabilities"_J)));
        weapons->AddItem(std::make_shared<ConditionalItem>("olddeadeye"_J, std::make_shared<BoolCommandItem>("enhanceddeadeye"_J)));
        weapons->AddItem(std::make_shared<ConditionalItem>("olddeadeye"_J, std::make_shared<BoolCommandItem>("slowdeadeyedrain"_J)));
        weapons->AddItem(std::make_shared<ConditionalItem>("olddeadeye"_J, std::make_shared<BoolCommandItem>("slipperybastardaccuracy"_J)));
        weapons->AddItem(std::make_shared<ConditionalItem>("olddeadeye"_J, std::make_shared<BoolCommandItem>("momenttorecuperate"_J)));
        weapons->AddItem(std::make_shared<ConditionalItem>("olddeadeye"_J, std::make_shared<BoolCommandItem>("deadeyetagging"_J)));
        weapons->AddItem(std::make_shared<ConditionalItem>("deadeyetagging"_J, std::make_shared<ImGuiItem>([] {
            static int current_item_index = 0;
            const char* items[] = {"Enemies", "Animals", "All"};

            auto command = Commands::GetCommand<IntCommand>("deadeyetaggingconfig"_J);
            if (!command)
                return;

            // Note: The IntCommand methods might need to be implemented or accessed differently
            // For now, we'll use a static variable to track the state
            static int currentValue = 7;

            if (currentValue == 6)
                current_item_index = 0;
            else if (currentValue == 8)
                current_item_index = 1;
            else if (currentValue == 7)
                current_item_index = 2;

            ImGui::Text("Auto Tag");
            ImGui::SetNextItemWidth(150.f);

            if (ImGui::Combo("##deadeyetaggingdropdown", &current_item_index, items, IM_ARRAYSIZE(items)))
            {
                int newValue = 6; // Default to Enemies
                if (current_item_index == 0)
                    newValue = 6; // Enemies
                else if (current_item_index == 1)
                    newValue = 8; // Animals
                else if (current_item_index == 2)
                    newValue = 7; // All

                currentValue = newValue;
                // TODO: Set the command value when the proper method is available
                // FiberPool::Push([=] {
                //     command->SetValue(newValue);
                // });
            }
        })));

        AddCategory(std::move(weapons));
    }
}
