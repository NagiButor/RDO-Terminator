#include "BoomPlus.hpp"
#include "game/frontend/items/Items.hpp"
#include "game/features/Features.hpp"
#include "game/features/players/toxic/LightningStrike.hpp"
#include "game/frontend/submenus/Player/NpcAttack.hpp"
#include "game/backend/FiberPool.hpp"
#include "game/rdr/data/PedModels.hpp"
#include "game/rdr/data/WeaponTypes.hpp"
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

        bool IsWeaponInList(const std::string& weapon)
        {
            if (weapon.empty())
                return true; // empty = fists / claws, always valid
            for (const char* entry : Data::g_WeaponTypes)
            {
                if (entry && weapon == entry)
                    return true;
            }
            return false;
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
        plus->AddItem(std::make_shared<BoolCommandItem>("cavalryattack"_J, "Cavalry Attack"));
        plus->AddItem(std::make_shared<BoolCommandItem>("maximwagonattack"_J, "Maxim Wagon"));
        //plus->AddItem(std::make_shared<BoolCommandItem>("zombieslogging"_J));
        plus->AddItem(std::make_shared<BoolCommandItem>("hardmode"_J));

        // Configuration for the "Npc Attack" toggle: pick the ped/animal, the amount and where it spawns
        auto npc_attack = std::make_shared<Group>("Npc Attack (config)", 1);
        npc_attack->AddItem(std::make_shared<ImGuiItem>([] {
            // --- model input with a live "closest matches" dropdown, same as World -> Spawner -> Ped ---
            // CallbackEdit transliterates RU-layout typing to Latin on the fly (font has no Cyrillic glyphs)
            InputTextWithHint("##npcmodel", "model, e.g. A_C_Bear_01 (click / type to see matches)",
                &Features::NpcAttackOptions::ConfiguredModel(),
                ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackEdit,
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
                const std::string buffer_lower = ToLowerCopy(Features::NpcAttackOptions::FixRuKeyboardLayout(buffer));
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

            ImGui::SliderInt("Count", &Features::NpcAttackOptions::ConfiguredCount(), 1, 10);
            ImGui::SliderFloat("Distance", &Features::NpcAttackOptions::ConfiguredDistance(), 2.f, 150.f, "%.0f m");
            ImGui::SliderInt("HP", &Features::NpcAttackOptions::ConfiguredHealth(), 0, 5000, "%d (0 = default)");

            // --- weapon input with a live "closest matches" dropdown, same UX as the model field ---
            InputTextWithHint("##npcweapon", "weapon, e.g. WEAPON_REVOLVER_CATTLEMAN (empty = none)",
                &Features::NpcAttackOptions::ConfiguredWeapon(),
                ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackEdit,
                nullptr, Features::NpcAttackOptions::WeaponInputCallback)
                .Draw();

            const bool is_weapon_active = ImGui::IsItemActive();
            const std::string weapon_buffer = Features::NpcAttackOptions::ConfiguredWeapon();

            if (!IsWeaponInList(weapon_buffer))
            {
                if (is_weapon_active)
                    ImGui::OpenPopup("##npc_weapon_popup");
            }

            const ImVec2 weapon_pos  = ImGui::GetItemRectMin();
            const ImVec2 weapon_size = ImGui::GetItemRectSize();

            ImGui::SetNextWindowPos({weapon_pos.x, weapon_pos.y + weapon_size.y});
            ImGui::SetNextWindowSize({weapon_size.x, 0});
            ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, max_dropdown_height));

            if (ImGui::BeginPopup("##npc_weapon_popup", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_ChildWindow))
            {
                const std::string weapon_lower = ToLowerCopy(Features::NpcAttackOptions::FixRuKeyboardLayout(weapon_buffer));
                for (const char* weapon : Data::g_WeaponTypes)
                {
                    if (!weapon)
                        continue;

                    std::string entry_lower = weapon;
                    std::transform(entry_lower.begin(), entry_lower.end(), entry_lower.begin(), [](unsigned char c) { return (char)std::tolower(c); });
                    if (!weapon_lower.empty() && entry_lower.find(weapon_lower) == std::string::npos)
                        continue;

                    if (ImGui::Selectable(weapon))
                    {
                        Features::NpcAttackOptions::ConfiguredWeapon() = weapon;
                        ImGui::CloseCurrentPopup();
                    }
                }
                if (!is_weapon_active && !ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
                    ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

            int side = (int)Features::NpcAttackOptions::ConfiguredSide();
            if (ImGui::Combo("Spawn point", &side, "Around the player\0In front of the player\0Behind the player\0"))
                Features::NpcAttackOptions::ConfiguredSideIndex() = side;

            ImGui::Checkbox("Lasso target", &Features::NpcAttackOptions::ConfiguredLasso());
            ImGui::TextDisabled("Humans hogtie with a lasso (also cavalry riders). Animals fight normally.");

            if (ImGui::Button("Spawn One NPC", ImVec2(-1, 0)))
                FiberPool::Push([] {
                    Features::NpcAttackOptions::SpawnSingleNow();
                });

            if (ImGui::Button("Clear NPCs", ImVec2(-1, 0)))
                FiberPool::Push([] {
                    Features::NpcAttackOptions::ClearAllNow();
                });

            ImGui::TextDisabled("Bear = A_C_Bear_01 | Cougar = A_C_Cougar_01 | Wolf = A_C_Wolf");
            ImGui::TextDisabled("One click = one NPC (2s cooldown, max 5 manual, oldest auto-removed).");
            ImGui::TextDisabled("Typing in RU layout auto-converts to EN. Leave weapon empty for claws / fists.");
            ImGui::TextDisabled("Turn on \"Npc Attack\" in the Plus tab to use these settings.");
        }));

        menu->AddItem(explosions);

        // Real lightning bolt: flash + thunder + damage at a random point around the selected player
        auto lightning = std::make_shared<Group>("Lightning (config)", 1);
        lightning->AddItem(std::make_shared<ImGuiItem>([] {
            ImGui::SliderFloat("Distance", &Features::LightningOptions::StrikeDistance(), 0.f, 150.f, "%.0f m");

            if (ImGui::Button("Strike Lightning", ImVec2(-1, 0)))
                FiberPool::Push([] {
                    Features::LightningOptions::StrikeNow();
                });

            ImGui::TextDisabled("0 m = right on target. Also works via the Lightning button above.");
        }));
        menu->AddItem(lightning);

        menu->AddItem(plus);
        menu->AddItem(npc_attack);

        return menu;
    }
}