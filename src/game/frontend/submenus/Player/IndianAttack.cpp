// IndianAttack.cpp - Waves of Native warriors (firearms or knives only) that hunt down the selected player

#include "game/frontend/submenus/Player/WaveCommand.hpp"

namespace YimMenu::Features
{
    static const std::vector<const char*> kIndianModels = {
        "a_m_m_wapwarriors_01",
        "u_m_m_nbxindianowner_01",
        "a_f_m_wapnative_01",
        "u_m_m_bht_skinnersearch_01",
    };

    static AttackWaves::Config GetIndianConfig()
    {
        AttackWaves::Config config{};
        config.models    = kIndianModels;
        config.weapons   = {"WEAPON_BOW", "WEAPON_REPEATER_CARBINE", "WEAPON_REVOLVER_CATTLEMAN", "WEAPON_THROWN_TOMAHAWK"};
        config.count     = 10;
        config.distance  = 50.f;
        config.move_rate = 1.4f;
        config.accuracy  = 65.f;
        return config;
    }

    static AttackWaves::Config GetKnifeIndianConfig()
    {
        AttackWaves::Config config{};
        config.models = kIndianModels;
        // knives only: one random knife per ped, forced into their hands
        config.weapons = {
            "WEAPON_MELEE_KNIFE",
            "WEAPON_MELEE_KNIFE_JAWBONE",
            "WEAPON_MELEE_KNIFE_MINER",
            "WEAPON_MELEE_KNIFE_RUSTIC",
            "WEAPON_MELEE_KNIFE_HORROR",
            "WEAPON_MELEE_KNIFE_BEAR",
            "WEAPON_MELEE_KNIFE_CIVIL_WAR",
        };
        config.count       = 10;
        config.distance    = 50.f;
        config.move_rate   = 1.6f; // knife fighters must be able to catch their target
        config.accuracy    = 70.f;
        config.force_equip = true;
        return config;
    }

    static AttackWaves::WaveCommand _SimpleIndianAttack{"indianattack", "Indian Attack", "Spawns waves of Indian peds that attack the selected player.",
        GetIndianConfig, {}, "Spawning waves of Indians for", "Indians"};

    static AttackWaves::WaveCommand _KnifeIndianAttack{"indianknifeattack", "Indian Attack (Knives)", "Spawns waves of Indian peds armed with knives only that attack the selected player.",
        GetKnifeIndianConfig, {}, "Spawning knife-only Indians for", "knife Indians"};
}
