// NpcAttack.cpp - Spawns a freely configurable ped/animal near the selected player and lets it hunt them

#include "NpcAttack.hpp"

#include "core/commands/LoopedCommand.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/backend/Players.hpp"
#include "game/rdr/Ped.hpp"
#include "game/rdr/data/PedModels.hpp"
#include "util/Joaat.hpp"

#include <algorithm>
#include <cctype>

namespace YimMenu::Features
{
    namespace NpcAttackOptions
    {
        static std::string s_model = "A_C_Bear_01";
        static int s_count = 3;
        static float s_distance = 25.f;
        static int s_side_index = 0;

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

        static std::string ToLower(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return (char)std::tolower(c); });
            return value;
        }

        std::string ResolveModelName(const std::string& input)
        {
            if (input.empty())
                return {};

            if (Data::g_PedModels.contains(Joaat(input)))
                return input;

            const auto input_lower = ToLower(input);
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
    }

    static std::vector<int> npc_index;
    static int npc_spawn_cooldown = 0;

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
                npc_spawn_cooldown = 300; // don't spam, try again in a few seconds
                return;
            }

            AttackWaves::Config config{};
            config.models = {model_name.c_str()};
            config.count = std::max(1, NpcAttackOptions::ConfiguredCount());
            config.distance = std::max(1.f, NpcAttackOptions::ConfiguredDistance());
            config.side = NpcAttackOptions::ConfiguredSide();
            config.move_rate = 1.9f;
            config.accuracy = 80.f;

            if (AttackWaves::SpawnWave(player_ped.GetHandle(), config, npc_index) == 0)
            {
                Notifications::Show("Npc Attack", "Could not spawn the configured ped.", NotificationType::Error);
                npc_spawn_cooldown = 300;
                return;
            }

            AttackWaves::Cleanup(npc_index);
        }

        void OnDisable() override
        {
            AttackWaves::Cleanup(npc_index, true);
        }
    };

    static NpcAttack _NpcAttack{"npcattack", "Npc Attack", "Spawns the configured ped/animal (model, count, distance) that hunts down the selected player."};
}
