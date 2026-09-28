// PoliceAttack.cpp - Spawns waves of lawmen that hunt down the selected player

#include "game/frontend/submenus/Player/WaveCommand.hpp"

namespace YimMenu::Features
{
    static AttackWaves::Config GetPoliceConfig()
    {
        AttackWaves::Config config{};
        config.models    = {"S_M_M_DispatchPolice_01", "S_M_M_DispatchLeaderPolice_01", "S_M_M_AmbientBlWPolice_01", "S_M_M_AmbientSDPolice_01", "U_M_M_RhdSheriff_01", "A_M_M_VALDEPUTYRESIDENT_01"};
        config.weapons   = {"WEAPON_REVOLVER_SCHOFIELD", "WEAPON_REVOLVER_CATTLEMAN", "WEAPON_REPEATER_WINCHESTER", "WEAPON_SHOTGUN_DOUBLEBARREL"};
        config.count     = 10;
        config.distance  = 45.f;
        config.move_rate = 1.3f;
        config.accuracy  = 65.f;
        return config;
    }

    static AttackWaves::WaveCommand _PoliceAttack{"policeattack", "Police Attack", "Spawns waves of lawmen that hunt down the selected player.",
        GetPoliceConfig, {}, "Spawning waves of lawmen for", "lawmen"};
}
