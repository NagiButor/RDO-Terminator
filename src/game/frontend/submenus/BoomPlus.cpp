#include "BoomPlus.hpp"
#include "game/frontend/items/Items.hpp"
#include "game/features/Features.hpp"
#include "game/frontend/submenus/Player/NpcAttack.hpp"
#include "game/rdr/data/PedModels.hpp"
#include "util/Joaat.hpp"

#include <algorithm>
#include <cctype>

namespace YimMenu::Submenus {
    namespace
    {
        bool IsPedModelInList(const std::string& model)
        {
            return Data::g_PedModels.contains(Joaat(model));
        }

        std::string ToLowerCopy(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return (char)std::tolower(c); });
            return value;
        }
    }

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
        npc_attack->AddItem(std::make_shared<ImGuiItem>([] {
            // --- model input with a live "closest matches" dropdown, same as World -> Spawner -> Ped ---
            InputTextWithHint("##npcmodel", "model, e.g. A_C_Bear_01 (click / type to see matches)",
                &Features::NpcAttackOptions::ConfiguredModel(), ImGuiInputTextFlags_CallbackCompletion,
                nullptr, Features::NpcAttackOptions::ModelInputCallback)
                .Draw();

            const bool is_input_active = ImGui::IsItemActive();
            const std::string buffer   = Features::NpcAttackOptions::ConfiguredModel();

            if (!buffer.empty() && !IsPedModelInList(buffer))
            {
                if (is_input_active)
                    ImGui::OpenPopup("##npc_search_popup");
            }

            const ImVec2 input_pos = ImGui::GetItemRectMin();
            const ImVec2 input_size = ImGui::GetItemRectSize();

            float item_height        = ImGui::GetTextLineHeightWithSpacing();
            float max_dropdown_height = (item_height * 10.0f) + (ImGui::GetStyle().WindowPadding.y * 2.0f);

            ImGui::SetNextWindowPos({input_pos.x, input_pos.y + input_size.y});
            ImGui::SetNextWindowSize({input_size.x, 0});
            ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, max_dropdown_height));

            if (ImGui::BeginPopup("##npc_search_popup", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_ChildWindow))
            {
                const std::string buffer_lower = ToLowerCopy(buffer);
                for (const auto& [hash, model] : Data::g_PedModels)
                {
                    if (!model)
                        continue;

                    std::string model_lower = model;
                    std::transform(model_lower.begin(), model_lower.end(), model_lower.begin(), [](unsigned char c) { return (char)std::tolower(c); });
                    if (model_lower.find(buffer_lower) == std::string::npos)
                        continue;

                    if (ImGui::Selectable(model))
                    {
                        Features::NpcAttackOptions::ConfiguredModel() = model;
                        ImGui::CloseCurrentPopup();
                    }
                }
                if (!is_input_active && !ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
                    ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

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