// Simplified IndianAttack.cpp - Spawns basic Indian peds that target the selected player

#include "core/commands/LoopedCommand.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/backend/ScriptMgr.hpp"
#include "game/backend/FiberPool.hpp"
#include "game/backend/Self.hpp"
#include "game/backend/Players.hpp"
#include "game/rdr/Natives.hpp"
#include "game/rdr/Ped.hpp"
#include "game/frontend/submenus/Player/AttackWaves.hpp"

#include <array>
#include <random>
#include <string>

namespace YimMenu::Features
{
    static constexpr int max_indians_per_wave = 10;
    static constexpr float spawn_distance = 50.f;

    static std::vector<int> indian_index;

    static void CleanupIndians(bool delete_all)
    {
        auto it = indian_index.begin();
        while (it != indian_index.end())
        {
            const bool exists = ENTITY::DOES_ENTITY_EXIST(*it);

            if (delete_all)
            {
                // feature turned off: remove only the living ones (so they stop
                // attacking); dead ones stay as normal corpses
                if (exists && !ENTITY::IS_ENTITY_DEAD(*it))
                    PED::DELETE_PED(&(*it));
                it = indian_index.erase(it);
            }
            else if (!exists || ENTITY::IS_ENTITY_DEAD(*it))
            {
                // NOTE: never DELETE_PED a dead indian -- that is exactly what
                // made corpses vanish instantly. Just stop tracking it; the
                // corpse stays in the world like a typical RDO corpse
                // (lootable / skinnable) until the game cleans it up itself.
                it = indian_index.erase(it);
            }
            else
                ++it;
        }
    }

    static void SpawnIndians(YimMenu::Player target_player)
    {
        int targetPed = target_player.GetPed().GetHandle();
        Vector3 targetPos = target_player.GetPed().GetPosition();

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(-1.f, 1.f);
        std::uniform_int_distribution<int> modelDist(0, 3);

        constexpr std::array<const char*, 4> indianModels = {
            "a_m_m_wapwarriors_01",
            "u_m_m_nbxindianowner_01",
            "a_f_m_wapnative_01",
            "u_m_m_bht_skinnersearch_01"
        };

        for (int i = 0; i < max_indians_per_wave; ++i)
        {
            float x = dist(gen) * spawn_distance;
            float y = dist(gen) * spawn_distance;
            Vector3 spawnPos = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(targetPed, x, y, 0.0f);

            const char* modelName = indianModels[modelDist(gen)];
            Hash modelHash = Joaat(modelName);
            
            if (!STREAMING::IS_MODEL_VALID(modelHash))
                continue;

            STREAMING::REQUEST_MODEL(modelHash, false);
            int tries = 0;
            while (!STREAMING::HAS_MODEL_LOADED(modelHash) && tries < 100)
            {
                ScriptMgr::Yield();
                ++tries;
            }

            if (!STREAMING::HAS_MODEL_LOADED(modelHash))
                continue;

            Ped indian = Ped::Create(modelHash, spawnPos, 0.f);

            if (!indian.IsValid())
                continue;

            int indianHandle = indian.GetHandle();

            TASK::TASK_COMBAT_PED(indianHandle, targetPed, 0, 16);
            indian_index.push_back(indianHandle);
        }
    }

    static std::vector<int> knife_indian_index;

    static AttackWaves::Config GetKnifeIndianConfig()
    {
        AttackWaves::Config config{};
        config.models = {
            "a_m_m_wapwarriors_01",
            "u_m_m_nbxindianowner_01",
            "a_f_m_wapnative_01",
            "u_m_m_bht_skinnersearch_01"
        };
        // knives only: one random knife per ped, forced into their hands
        config.weapons = {
            "WEAPON_MELEE_KNIFE",
            "WEAPON_MELEE_KNIFE_JAWBONE",
            "WEAPON_MELEE_KNIFE_MINER",
            "WEAPON_MELEE_KNIFE_RUSTIC",
            "WEAPON_MELEE_KNIFE_HORROR",
            "WEAPON_MELEE_KNIFE_BEAR",
            "WEAPON_MELEE_KNIFE_CIVIL_WAR"
        };
        config.count       = 10;
        config.distance    = 50.f;
        config.side        = AttackWaves::SpawnSide::Around;
        config.move_rate   = 1.6f; // knife fighters must be able to catch their target
        config.accuracy    = 70.f;
        config.force_equip = true;
        return config;
    }

    class KnifeIndianAttack : public LoopedCommand
    {
        using LoopedCommand::LoopedCommand;

        void OnTick() override
        {
            Player target_player = Players::GetSelected();
            Ped player_ped       = target_player.GetPed();

            if (!player_ped.IsValid() || player_ped.GetHealth() <= 0)
            {
                AttackWaves::Cleanup(knife_indian_index);
                return;
            }

            if (knife_indian_index.empty())
                AttackWaves::SpawnWave(player_ped.GetHandle(), GetKnifeIndianConfig(), knife_indian_index);

            AttackWaves::Cleanup(knife_indian_index);
        }

        void OnEnable() override
        {
            Notifications::Show("Indian Attack", std::string("Spawning knife-only Indians for ") + Players::GetSelected().GetName(), NotificationType::Info);
        }

        void OnDisable() override
        {
            AttackWaves::Cleanup(knife_indian_index, true);
            Notifications::Show("Indian Attack", std::string("Cleared knife Indians (target: ") + Players::GetSelected().GetName() + ")", NotificationType::Info);
        }
    };

    static KnifeIndianAttack _KnifeIndianAttack{"indianknifeattack", "Indian Attack (Knives)", "Spawns waves of Indian peds armed with knives only that attack the selected player."};

    class SimpleIndianAttack : public LoopedCommand
    {
        using LoopedCommand::LoopedCommand;

        void OnTick() override
        {
            YimMenu::Player target_player = YimMenu::Players::GetSelected();
            Ped player_ped = target_player.GetPed();

            if (!player_ped.IsValid() || player_ped.GetHealth() <= 0)
            {
                CleanupIndians(false);
                return;
            }

            if (indian_index.empty())
                SpawnIndians(target_player);

            CleanupIndians(false);
        }

        void OnEnable() override
        {
            Notifications::Show("Indian Attack", std::string("Spawning waves of Indians for ") + Players::GetSelected().GetName(), NotificationType::Info);
        }

        void OnDisable() override
        {
            CleanupIndians(true);
            Notifications::Show("Indian Attack", std::string("Cleared Indians (target: ") + Players::GetSelected().GetName() + ")", NotificationType::Info);
        }
    };

    static SimpleIndianAttack _SimpleIndianAttack{"indianattack", "Indian Attack", "Spawns waves of Indian peds that attack the selected player."};
}