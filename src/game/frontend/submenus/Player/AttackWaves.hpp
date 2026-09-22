#pragma once
#include "game/rdr/Natives.hpp"
#include "game/rdr/Ped.hpp"

#include <vector>

namespace YimMenu::Features::AttackWaves
{
    // Hard limits: every spawned ped is a networked object, so a burst of
    // clones is exactly what kicks the local player into a solo session.
    // Kept at the same scale as the other attacks (Indian/Army/Police use 10).
    inline constexpr int kMaxLiveAttackers = 10;
    inline constexpr int kMaxSpawnPerCall  = 10;
    // Single-spawn buffer: at most this many living manual NPCs.
    // When full, the oldest one is auto-removed to make room.
    inline constexpr int kMaxManualAttackers = 5;

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
        bool force_equip = false; // force the given weapon into the ped's hands (melee-only attackers)
        bool use_lasso = false;   // humans get WEAPON_LASSO + TASK_LASSO_PED on the target instead of combat
        float weapon_chance = 1.0f; // probability (0-1) a human actually receives the configured weapon
    };

    // Spawns a wave of attackers around the target ped and sends them after it.
    // Returns the amount of peds that were actually spawned.
    int SpawnWave(int target_handle, const Config& config, std::vector<int>& spawned);

    // Deletes despawned/dead attackers from the list (same logic as IndianAttack).
    // Their corpses are NOT deleted: the peds are regular entities, so the
    // corpses stay in the world like typical RDO corpses until the game cleans
    // them up on its own (other players can loot/skin them).
    // When delete_all is set, living attackers are removed as well
    // (used when the feature gets turned off).
    void Cleanup(std::vector<int>& spawned, bool delete_all = false);

    // Auto-cleanup: deletes the oldest living attacker(s) so a new spawn
    // never pushes the session over the networked-object budget.
    // Dead/despawned handles are just forgotten (corpses stay in the world).
    void RemoveOldest(std::vector<int>& spawned, int count = 1);
}
