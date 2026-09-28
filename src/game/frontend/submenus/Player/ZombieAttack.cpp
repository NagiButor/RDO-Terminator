// ZombieAttack.cpp - Endless waves of cholera-corpse zombies piling onto the selected player.
// Half of them (coin flip each) get a knife forced into their hands.

#include "game/frontend/submenus/Player/WaveCommand.hpp"

namespace YimMenu::Features
{
    static AttackWaves::Config GetZombieConfig()
    {
        AttackWaves::Config config{};
        config.models        = {"A_M_M_ARMCHOLERACORPSE_01", "A_F_M_ARMCHOLERACORPSE_01"};
        config.weapons       = {"WEAPON_MELEE_KNIFE"};
        config.weapon_chance = 0.5f; // every zombie flips a coin for the knife
        config.unlimited     = true; // waves pile into a horde (capped by kMaxPileUp)
        config.count         = 10;
        config.distance      = 45.f;
        config.move_rate     = 1.1f; // shambling horde, still catches walking targets
        config.accuracy      = 50.f;
        config.force_equip   = true;
        return config;
    }

    static AttackWaves::WaveOptions ZombieOptions()
    {
        AttackWaves::WaveOptions options{};
        options.pile_up          = true;
        options.pile_interval_ms = 10000; // next 10 join the horde every 10 s
        return options;
    }

    static AttackWaves::WaveCommand _ZombieAttack{"zombieattack", "Zombie Attack", "Endless waves of 10 cholera zombies (half armed with knives) piling onto the selected player.",
        GetZombieConfig, ZombieOptions(), "Raising a horde on", "zombies"};
}
