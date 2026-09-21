// NpcAttack.cpp - Spawns a freely configurable ped/animal near the selected player and lets it hunt them

#include "NpcAttack.hpp"

#include "core/commands/LoopedCommand.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/backend/Players.hpp"
#include "game/rdr/Ped.hpp"
#include "game/rdr/data/PedModels.hpp"
#include "game/rdr/data/WeaponTypes.hpp"
#include "util/Joaat.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>

namespace YimMenu::Features
{
    static std::vector<int> npc_index;
    static std::vector<int> npc_manual_index;
    static int npc_spawn_cooldown = 0;
    static std::chrono::steady_clock::time_point s_last_spawn_time = std::chrono::steady_clock::time_point{};

    namespace NpcAttackOptions
    {
        static std::string ToLower(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return (char)std::tolower(c); });
            return value;
        }

        static std::string s_model  = "A_C_Bear_01";
        static std::string s_weapon = "";
        static int s_count    = 3;
        static float s_distance = 25.f;
        static int s_side_index = 0;

        // persistent storage for BuildConfig() string views:
        // pointers inside AttackWaves::Config must stay valid until SpawnWave returns
        static std::string s_cached_model  = s_model;
        static std::string s_cached_weapon = s_weapon;

        std::string& ConfiguredModel()
        {
            return s_model;
        }

        int& ConfiguredCount()
        {
            return s_count;
        }

        float& ConfiguredDistance()
        {
            return s_distance;
        }

        int& ConfiguredSideIndex()
        {
            return s_side_index;
        }

        AttackWaves::SpawnSide ConfiguredSide()
        {
            switch (s_side_index)
            {
            case 1:
                return AttackWaves::SpawnSide::InFront;
            case 2:
                return AttackWaves::SpawnSide::Behind;
            case 0:
            default:
                return AttackWaves::SpawnSide::Around;
            }
        }

        std::string& ConfiguredWeapon()
        {
            return s_weapon;
        }

        std::string ResolveWeaponName(const std::string& input)
        {
            const std::string fixed = FixRuKeyboardLayout(input);
            if (fixed.empty() || ToLower(fixed) == "none")
                return {};

            for (const char* weapon : Data::g_WeaponTypes)
            {
                if (!weapon)
                    continue;

                if (Joaat(fixed) == Joaat(weapon))
                    return weapon;
            }

            const auto input_lower = ToLower(fixed);
            for (const char* weapon : Data::g_WeaponTypes)
            {
                if (!weapon)
                    continue;

                if (ToLower(weapon).find(input_lower) != std::string::npos)
                    return weapon;
            }

            return {};
        }

        int WeaponInputCallback(ImGuiInputTextCallbackData* data)
        {
            if (ApplyRuFixToImGuiInput(data))
                return 0;

            if (data->EventFlag != ImGuiInputTextFlags_CallbackCompletion)
                return 0;

            const std::string match = ResolveWeaponName(data->Buf);
            if (!match.empty())
            {
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, match.c_str());
            }

            return 0;
        }

        static AttackWaves::Config BuildBaseConfig(const std::string& model_name, const std::string& weapon_name, int count)
        {
            // refresh persistent storage so config.models / config.weapons
            // never point at temporaries (spawn happens on another fiber)
            s_cached_model  = model_name;
            s_cached_weapon = weapon_name;

            AttackWaves::Config config{};
            config.models = {s_cached_model.c_str()};
            if (!s_cached_weapon.empty())
            {
                config.weapons     = {s_cached_weapon.c_str()};
                config.force_equip = true;
            }
            config.count    = std::max(1, count);
            config.distance = std::max(1.f, s_distance);
            config.side     = ConfiguredSide();
            config.move_rate = 1.9f;
            config.accuracy  = 80.f;
            return config;
        }

        AttackWaves::Config BuildConfig()
        {
            const std::string model_name  = ResolveModelName(s_model);
            const std::string weapon_name = ResolveWeaponName(s_weapon);
            return BuildBaseConfig(model_name.empty() ? std::string{} : model_name, weapon_name, s_count);
        }

        bool SpawnSingleNow()
        {
            Player target_player = Players::GetSelected();
            Ped player_ped       = target_player.GetPed();

            if (!player_ped.IsValid() || player_ped.GetHealth() <= 0)
            {
                Notifications::Show("Npc Attack", "Invalid target player.", NotificationType::Error);
                return false;
            }

            // manual spawn cooldown: one click = one networked ped, never a burst
            const auto now = std::chrono::steady_clock::now();
            if (s_last_spawn_time != std::chrono::steady_clock::time_point{} && now - s_last_spawn_time < std::chrono::seconds(2))
            {
                Notifications::Show("Npc Attack", "Wait a second before spawning another NPC.", NotificationType::Error);
                return false;
            }

            const std::string model_name = ResolveModelName(s_model);
            if (model_name.empty())
            {
                Notifications::Show("Npc Attack", "Unknown ped model, please check the model name.", NotificationType::Error);
                return false;
            }

            // live attackers cap: spawning is networked, so never flood the session.
            // singles buffer holds max kMaxManualAttackers living NPCs -- the
            // oldest one is auto-removed to make room (requested auto-cleanup).
            AttackWaves::Cleanup(npc_manual_index);
            AttackWaves::Cleanup(npc_index);
            while ((int)npc_manual_index.size() >= AttackWaves::kMaxManualAttackers)
                AttackWaves::RemoveOldest(npc_manual_index, 1);
            // combined budget shared with the loop wave: trim oldest singles
            // first, then the wave itself, instead of failing
            while (npc_manual_index.size() + npc_index.size() >= (std::size_t)AttackWaves::kMaxLiveAttackers)
            {
                if (!npc_manual_index.empty())
                    AttackWaves::RemoveOldest(npc_manual_index, 1);
                else
                    AttackWaves::RemoveOldest(npc_index, 1);
            }

            const std::string weapon_name = ResolveWeaponName(s_weapon);
            AttackWaves::Config config    = BuildBaseConfig(model_name, weapon_name, 1);

            const int spawned = AttackWaves::SpawnWave(player_ped.GetHandle(), config, npc_manual_index);
            if (spawned == 0)
            {
                Notifications::Show("Npc Attack", "Could not spawn the configured ped.", NotificationType::Error);
                return false;
            }

            AttackWaves::Cleanup(npc_manual_index);

            s_last_spawn_time = std::chrono::steady_clock::now();

            std::string info = std::string("Spawned ") + model_name;
            if (!weapon_name.empty())
                info += std::string(" [") + weapon_name + "]";
            info += std::string(" on ") + target_player.GetName();
            Notifications::Show("Npc Attack", info, NotificationType::Info);
            return true;
        }

        std::string ResolveModelName(const std::string& input)
        {
            const std::string fixed = FixRuKeyboardLayout(input);
            if (fixed.empty())
                return {};

            if (Data::g_PedModels.contains(Joaat(fixed)))
                return fixed;

            const auto input_lower = ToLower(fixed);
            for (const auto& entry : Data::g_PedModels)
            {
                if (!entry.second)
                    continue;

                if (ToLower(entry.second).find(input_lower) != std::string::npos)
                    return entry.second;
            }

            return {};
        }

        int ModelInputCallback(ImGuiInputTextCallbackData* data)
        {
            if (ApplyRuFixToImGuiInput(data))
                return 0;

            if (data->EventFlag != ImGuiInputTextFlags_CallbackCompletion)
                return 0;

            const std::string match = ResolveModelName(data->Buf);
            if (!match.empty())
            {
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, match.c_str());
            }

            return 0;
        }

        void ClearAllNow()
        {
            AttackWaves::Cleanup(npc_manual_index, true);
            AttackWaves::Cleanup(npc_index, true);
            Notifications::Show("Npc Attack", "Cleared all spawned NPCs.", NotificationType::Info);
        }
    }

    class NpcAttack : public LoopedCommand
    {
        using LoopedCommand::LoopedCommand;

        void OnTick() override
        {
            Player target_player = Players::GetSelected();
            Ped player_ped = target_player.GetPed();

            if (!player_ped.IsValid() || player_ped.GetHealth() <= 0)
            {
                AttackWaves::Cleanup(npc_index);
                return;
            }

            if (!npc_index.empty())
            {
                AttackWaves::Cleanup(npc_index);
                return;
            }

            if (npc_spawn_cooldown > 0)
            {
                --npc_spawn_cooldown;
                return;
            }

            const std::string model_name = NpcAttackOptions::ResolveModelName(NpcAttackOptions::ConfiguredModel());
            if (model_name.empty())
            {
                Notifications::Show("Npc Attack", "Unknown ped model, please check the model name.", NotificationType::Error);
                npc_spawn_cooldown = 600; // don't spam, try again later (clone spam = disconnect)
                return;
            }

            // keep the toggle cheap on the network: too many live attackers
            // means waiting instead of spawning another wave
            AttackWaves::Cleanup(npc_index);
            AttackWaves::Cleanup(npc_manual_index);
            if (npc_index.size() + npc_manual_index.size() >= (std::size_t)AttackWaves::kMaxLiveAttackers)
            {
                npc_spawn_cooldown = 300;
                return;
            }

            AttackWaves::Config config = NpcAttackOptions::BuildConfig();
            if (config.models.empty() || config.models[0] == nullptr)
            {
                Notifications::Show("Npc Attack", "Unknown ped model, please check the model name.", NotificationType::Error);
                npc_spawn_cooldown = 600;
                return;
            }

            if (AttackWaves::SpawnWave(player_ped.GetHandle(), config, npc_index) == 0)
            {
                Notifications::Show("Npc Attack", "Could not spawn the configured ped.", NotificationType::Error);
                npc_spawn_cooldown = 600;
                return;
            }

            npc_spawn_cooldown = 120; // small gap between waves, avoids clone bursts
            AttackWaves::Cleanup(npc_index);
        }

        void OnEnable() override
        {
            const std::string model_name = NpcAttackOptions::ResolveModelName(NpcAttackOptions::ConfiguredModel());
            Notifications::Show("Npc Attack", std::string("Releasing ") + (model_name.empty() ? NpcAttackOptions::ConfiguredModel() : model_name) + " on " + Players::GetSelected().GetName(), NotificationType::Info);
        }

        void OnDisable() override
        {
            AttackWaves::Cleanup(npc_index, true);
            AttackWaves::Cleanup(npc_manual_index, true);
            Notifications::Show("Npc Attack", std::string("Cleared npcs (target: ") + Players::GetSelected().GetName() + ")", NotificationType::Info);
        }
    };

    static NpcAttack _NpcAttack{"npcattack", "Npc Attack", "Spawns the configured ped/animal (model, count, distance) that hunts down the selected player."};
}
