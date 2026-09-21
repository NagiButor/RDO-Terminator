// ArmyAttack.cpp - Spawns waves of army soldiers that hunt down the selected player

#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Players.hpp"
#include "game/frontend/submenus/Player/AttackWaves.hpp"
#include "game/rdr/Ped.hpp"

namespace YimMenu::Features
{
    static std::vector<int> army_index;

    static AttackWaves::Config GetArmyConfig()
    {
        AttackWaves::Config config{};
        config.models = {"S_M_Y_Army_01", "S_M_M_Army_01", "U_M_M_ARMYTRN4_01"};
        config.weapons = {"WEAPON_RIFLE_BOLTACTION", "WEAPON_RIFLE_SPRINGFIELD", "WEAPON_REPEATER_CARBINE", "WEAPON_REVOLVER_SCHOFIELD"};
        config.count = 10;
        config.distance = 60.f;
        config.move_rate = 1.25f;
        config.accuracy = 70.f;
        return config;
    }

    class ArmyAttack : public LoopedCommand
    {
        using LoopedCommand::LoopedCommand;

        void OnTick() override
        {
            Player target_player = Players::GetSelected();
            Ped player_ped = target_player.GetPed();

            if (!player_ped.IsValid() || player_ped.GetHealth() <= 0)
            {
                AttackWaves::Cleanup(army_index);
                return;
            }

            if (army_index.empty())
                AttackWaves::SpawnWave(player_ped.GetHandle(), GetArmyConfig(), army_index);

            AttackWaves::Cleanup(army_index);
        }

        void OnDisable() override
        {
            AttackWaves::Cleanup(army_index, true);
        }
    };

    static ArmyAttack _ArmyAttack{"armyattack", "Army Attack", "Spawns waves of army soldiers that hunt down the selected player."};
}
