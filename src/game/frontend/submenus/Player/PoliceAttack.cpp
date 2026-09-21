// PoliceAttack.cpp - Spawns waves of lawmen that hunt down the selected player

#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Players.hpp"
#include "game/frontend/submenus/Player/AttackWaves.hpp"
#include "game/rdr/Ped.hpp"

namespace YimMenu::Features
{
    static std::vector<int> police_index;

    static AttackWaves::Config GetPoliceConfig()
    {
        AttackWaves::Config config{};
        config.models = {"S_M_M_DispatchPolice_01", "S_M_M_DispatchLeaderPolice_01", "S_M_M_AmbientBlWPolice_01", "S_M_M_AmbientSDPolice_01", "U_M_M_RhdSheriff_01", "A_M_M_VALDEPUTYRESIDENT_01"};
        config.weapons = {"WEAPON_REVOLVER_SCHOFIELD", "WEAPON_REVOLVER_CATTLEMAN", "WEAPON_REPEATER_WINCHESTER", "WEAPON_SHOTGUN_DOUBLEBARREL"};
        config.count = 10;
        config.distance = 45.f;
        config.move_rate = 1.3f;
        config.accuracy = 65.f;
        return config;
    }

    class PoliceAttack : public LoopedCommand
    {
        using LoopedCommand::LoopedCommand;

        void OnTick() override
        {
            Player target_player = Players::GetSelected();
            Ped player_ped = target_player.GetPed();

            if (!player_ped.IsValid() || player_ped.GetHealth() <= 0)
            {
                AttackWaves::Cleanup(police_index);
                return;
            }

            if (police_index.empty())
                AttackWaves::SpawnWave(player_ped.GetHandle(), GetPoliceConfig(), police_index);

            AttackWaves::Cleanup(police_index);
        }

        void OnDisable() override
        {
            AttackWaves::Cleanup(police_index, true);
        }
    };

    static PoliceAttack _PoliceAttack{"policeattack", "Police Attack", "Spawns waves of lawmen that hunt down the selected player."};
}
