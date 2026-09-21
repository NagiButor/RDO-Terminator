// WolfAttack.cpp - Spawns a wolf pack that hunts down the selected player

#include "core/commands/LoopedCommand.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/backend/Players.hpp"
#include "game/frontend/submenus/Player/AttackWaves.hpp"
#include "game/rdr/Ped.hpp"

#include <string>

namespace YimMenu::Features
{
    static std::vector<int> wolf_index;

    static AttackWaves::Config GetWolfConfig()
    {
        AttackWaves::Config config{};
        config.models = {"A_C_Wolf", "A_C_Wolf_Medium", "A_C_Wolf_Small", "MP_A_C_Wolf_01"};
        config.count = 8;
        config.distance = 40.f;
        config.move_rate = 1.6f;
        config.accuracy = 60.f;
        return config;
    }

    class WolfAttack : public LoopedCommand
    {
        using LoopedCommand::LoopedCommand;

        void OnTick() override
        {
            Player target_player = Players::GetSelected();
            Ped player_ped = target_player.GetPed();

            if (!player_ped.IsValid() || player_ped.GetHealth() <= 0)
            {
                AttackWaves::Cleanup(wolf_index);
                return;
            }

            if (wolf_index.empty())
                AttackWaves::SpawnWave(player_ped.GetHandle(), GetWolfConfig(), wolf_index);

            AttackWaves::Cleanup(wolf_index);
        }

        void OnEnable() override
        {
            Notifications::Show("Wolf Attack", std::string("Releasing the pack on ") + Players::GetSelected().GetName(), NotificationType::Info);
        }

        void OnDisable() override
        {
            AttackWaves::Cleanup(wolf_index, true);
            Notifications::Show("Wolf Attack", std::string("Cleared wolves (target: ") + Players::GetSelected().GetName() + ")", NotificationType::Info);
        }
    };

    static WolfAttack _WolfAttack{"wolfattack", "Wolf Attack", "Spawns a wolf pack that hunts down the selected player."};
}
