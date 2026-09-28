// WolfAttack.cpp - Spawns a wolf pack that hunts down the selected player

#include "game/frontend/submenus/Player/WaveCommand.hpp"

namespace YimMenu::Features
{
    static AttackWaves::Config GetWolfConfig()
    {
        AttackWaves::Config config{};
        config.models    = {"A_C_Wolf", "A_C_Wolf_Medium", "A_C_Wolf_Small", "MP_A_C_Wolf_01"};
        config.count     = 8;
        config.distance  = 40.f;
        config.move_rate = 1.6f;
        config.accuracy  = 60.f;
        return config;
    }

    static AttackWaves::WaveCommand _WolfAttack{"wolfattack", "Wolf Attack", "Spawns a wolf pack that hunts down the selected player.",
        GetWolfConfig, {}, "Releasing the pack on", "wolves"};
}
