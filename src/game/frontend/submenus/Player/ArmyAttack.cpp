// ArmyAttack.cpp - Spawns waves of army soldiers that hunt down the selected player

#include "game/frontend/submenus/Player/WaveCommand.hpp"

namespace YimMenu::Features
{
    static AttackWaves::Config GetArmyConfig()
    {
        AttackWaves::Config config{};
        config.models    = {"S_M_Y_Army_01", "S_M_M_Army_01", "U_M_M_ARMYTRN4_01"};
        config.weapons   = {"WEAPON_RIFLE_BOLTACTION", "WEAPON_RIFLE_SPRINGFIELD", "WEAPON_REPEATER_CARBINE", "WEAPON_REVOLVER_SCHOFIELD"};
        config.count     = 10;
        config.distance  = 60.f;
        config.move_rate = 1.25f;
        config.accuracy  = 70.f;
        return config;
    }

    static AttackWaves::WaveCommand _ArmyAttack{"armyattack", "Army Attack", "Spawns waves of army soldiers that hunt down the selected player.",
        GetArmyConfig, {}, "Spawning waves of soldiers for", "soldiers"};
}
