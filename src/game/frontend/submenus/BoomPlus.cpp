#include "BoomPlus.hpp"
#include "game/frontend/items/Items.hpp"
#include "game/features/Features.hpp"
#include "game/frontend/submenus/Player/NpcAttack.hpp"

namespace YimMenu::Submenus {
    std::shared_ptr<Category> BuildBoomPlusMenu() {
        auto menu = std::make_shared<Category>("BoomPlus");

        auto explosions = std::make_shared<Group>("Explosions", 5);
        explosions->AddItem(std::make_shared<PlayerCommandItem>("firework"_J, "FireWorks"));
        explosions->AddItem(std::make_shared<PlayerCommandItem>("sparks_with_fire_afterwards"_J, "Ol's Sparky"));
        explosions->AddItem(std::make_shared<PlayerCommandItem>("fire_starts_small_fire"_J, "Small fire"));
        explosions->AddItem(std::make_shared<PlayerCommandItem>("molotov_type_explosion_fire"_J, "Molotov-Fire"));
        explosions->AddItem(std::make_shared<PlayerCommandItem>("molotov_style_explosion2"_J, "Molotov Boom!"));
        explosions->AddItem(std::make_shared<PlayerCommandItem>("big_explosion"_J, "Boom!"));
        explosions->AddItem(std::make_shared<PlayerCommandItem>("big_explosion_metallic_sound"_J, "Bang"));
        explosions->AddItem(std::make_shared<PlayerCommandItem>("big_rock_explosion"_J, "Quake"));
        explosions->AddItem(std::make_shared<PlayerCommandItem>("big_explosion_25"_J, "Big Boom!"));
        explosions->AddItem(std::make_shared<PlayerCommandItem>("big_explosion_31"_J, "BADA BOOM!"));
        explosions->AddItem(std::make_shared<PlayerCommandItem>("huge_explosion"_J, "Very Big Bada Boom!"));
        explosions->AddItem(std::make_shared<PlayerCommandItem>("green_splash_small"_J, "Stinky"));
        explosions->AddItem(std::make_shared<PlayerCommandItem>("glass_smashing_sound_toxic_gas_cloud"_J, "Breath it in"));
        explosions->AddItem(std::make_shared<PlayerCommandItem>("big_geyser"_J, "The Enema"));
        explosions->AddItem(std::make_shared<PlayerCommandItem>("kill"_J));
        explosions->AddItem(std::make_shared<PlayerCommandItem>("explode"_J));
        explosions->AddItem(std::make_shared<PlayerCommandItem>("lightning"_J));

        auto plus = std::make_shared<Group>("Plus");
        plus->AddItem(std::make_shared<PlayerCommandItem>("beatdown"_J, "Beatdown"));
        plus->AddItem(std::make_shared<PlayerCommandItem>("beatdown2"_J, "Beat Down 2"));
        plus->AddItem(std::make_shared<PlayerCommandItem>("removeweapons"_J, "Remove Weapons"));
        plus->AddItem(std::make_shared<PlayerCommandItem>("rideemcowboy"_J, "Rideem Cowboy"));
        plus->AddItem(std::make_shared<PlayerCommandItem>("spankdatass"_J, "Spank Dat Ass"));

        // Insert Poodle Attack button after "Spank Dat Ass"
        plus->AddItem(std::make_shared<PlayerCommandItem>("poodleattack"_J, "Poodle Attack"));

        plus->AddItem(std::make_shared<BoolCommandItem>("indianattack"_J));
        plus->AddItem(std::make_shared<BoolCommandItem>("indianknifeattack"_J));
        plus->AddItem(std::make_shared<BoolCommandItem>("armyattack"_J));
        plus->AddItem(std::make_shared<BoolCommandItem>("policeattack"_J));
        plus->AddItem(std::make_shared<BoolCommandItem>("wolfattack"_J));
        plus->AddItem(std::make_shared<BoolCommandItem>("npcattack"_J));
        //plus->AddItem(std::make_shared<BoolCommandItem>("zombieslogging"_J));
        plus->AddItem(std::make_shared<BoolCommandItem>("hardmode"_J));

        // Configuration for the "Npc Attack" toggle: pick the ped/animal, the amount and where it spawns
        auto npc_attack = std::make_shared<Group>("Npc Attack (config)", 1);
        npc_attack->AddItem(std::make_shared<InputTextWithHint>("##npcmodel", "model, e.g. A_C_Bear_01 (Tab = autocomplete)",
            &Features::NpcAttackOptions::ConfiguredModel(), ImGuiInputTextFlags_CallbackCompletion, nullptr, Features::NpcAttackOptions::ModelInputCallback));
        npc_attack->AddItem(std::make_shared<ImGuiItem>([] {
            ImGui::SliderInt("Count", &Features::NpcAttackOptions::ConfiguredCount(), 1, 20);
            ImGui::SliderFloat("Distance", &Features::NpcAttackOptions::ConfiguredDistance(), 2.f, 150.f, "%.0f m");

            int side = (int)Features::NpcAttackOptions::ConfiguredSide();
            if (ImGui::Combo("Spawn point", &side, "Around the player\0In front of the player\0Behind the player\0"))
                Features::NpcAttackOptions::ConfiguredSideIndex() = side;

            ImGui::TextDisabled("Bear = A_C_Bear_01 | Cougar = A_C_Cougar_01 | Wolf = A_C_Wolf");
            ImGui::TextDisabled("Turn on \"Npc Attack\" in the Plus tab to use these settings.");
        }));

        menu->AddItem(explosions);
        menu->AddItem(plus);
        menu->AddItem(npc_attack);

        return menu;
    }
}