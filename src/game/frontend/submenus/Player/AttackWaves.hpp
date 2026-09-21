#pragma once
#include "game/rdr/Natives.hpp"
#include "game/rdr/Ped.hpp"

#include <vector>

namespace YimMenu::Features::AttackWaves
{
    // Where the attackers should spawn relative to the target
    enum class SpawnSide
    {
        Around = 0, // random ring around the target
        InFront = 1,
        Behind = 2,
    };

    struct Config
    {
        std::vector<const char*> models{};  // one model is picked at random per ped
        std::vector<const char*> weapons{}; // one weapon is picked at random per ped (humans only)
        int count = 10;                     // attackers per wave
        float distance = 50.f;              // spawn distance from the target (metres)
        SpawnSide side = SpawnSide::Around; // where the attackers appear
        bool invincible = false;
        float health = 0.f; // 0 = keep the game default
        int armour = 0;     // 0 = no extra armour
        float move_rate = 1.35f;
        float accuracy = 70.f;
    };

    // Spawns a wave of attackers around the target ped and sends them after it.
    // Returns the amount of peds that were actually spawned.
    int SpawnWave(int target_handle, const Config& config, std::vector<int>& spawned);

    // Deletes dead/despawned attackers and removes them from the list.
    // When delete_all is set, living attackers are removed as well (used when the feature gets turned off).
    void Cleanup(std::vector<int>& spawned, bool delete_all = false);
}
