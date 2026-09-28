#pragma once
#include "game/rdr/Natives.hpp"
#include "game/rdr/Ped.hpp"

#include <chrono>
#include <functional>
#include <vector>

namespace YimMenu::Features::AttackWaves
{
    // Hard limits: every spawned ped is a networked object, so a burst of
    // clones is exactly what kicks the local player into a solo session.
    inline constexpr int kMaxLiveAttackers = 10;
    inline constexpr int kMaxSpawnPerCall  = 10;
    // pile-up waves (zombies) ignore kMaxLiveAttackers but never exceed this
    inline constexpr int kMaxPileUp = 30;
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
        bool unlimited = false;   // pile-up: waves keep coming on a timer (capped by kMaxPileUp)
    };

    // Combat attributes, flee/lasso flags, HP/armour and weapon for one attacker.
    // `weapon` overrides the random pick from config.weapons when set.
    void SetUpAttacker(int handle, bool human, const Config& config, const char* weapon = nullptr);

    // Sends one attacker after the target (combat, or lasso for lasso duty).
    void TaskAttack(int handle, int target_handle, bool lasso);

    // Ground-snapped point on a ring around the target. Returns false when no
    // ground was found (open water / unloaded collision): skip that spot.
    bool FindSpawnPos(int target_handle, float distance, SpawnSide side, int index, int total, Vector3& out);

    // Requests all models at once and waits (bounded) until they stream in.
    void RequestModels(const std::vector<Hash>& models);
    void ReleaseModels(const std::vector<Hash>& models);

    // Spawns a wave of attackers around the target ped and sends them after it.
    // Returns the amount of peds that were actually spawned.
    int SpawnWave(int target_handle, const Config& config, std::vector<int>& spawned);

    // Forgets despawned/dead attackers. Corpses are NOT deleted: they stay in
    // the world like typical RDO corpses (lootable / skinnable).
    // When delete_all is set, living attackers are removed as well
    // (used when the feature gets turned off).
    void Cleanup(std::vector<int>& spawned, bool delete_all = false);

    // Auto-cleanup: deletes the oldest living attacker(s) so a new spawn
    // never pushes the session over the networked-object budget.
    void RemoveOldest(std::vector<int>& spawned, int count = 1);

    struct WaveOptions
    {
        bool pile_up        = false; // spawn on a timer even while attackers are alive
        int wave_delay_ms   = 4000;  // pause after a wave is wiped out before the next one
        int pile_interval_ms = 10000; // pile-up: gap between waves
        int fail_retry_ms   = 8000;  // spawn produced nothing: back off instead of retrying every tick
        float leash         = 250.f; // living attackers further than this from the target are removed
        bool lasso          = false; // attackers run TASK_LASSO_PED instead of combat
    };

    // Shared loop for every "attack waves" feature: owns the attacker list,
    // re-targets them when the selected player / their ped changes, removes
    // stragglers, paces the spawns and cleans up mounts / wagons.
    class WaveRunner
    {
    public:
        // spawn(target) must push combatants into m_Attackers (and mounts /
        // vehicles into AddExtra) and return how many combatants it created
        using SpawnFn = std::function<int(int target_handle, WaveRunner& runner)>;

        std::vector<int> m_Attackers;

        void Tick(const WaveOptions& options, const SpawnFn& spawn);
        // feature off: delete the living attackers and their mounts / wagons
        void Clear();

        void AddExtra(int handle, bool is_vehicle);
        int LiveCount() const
        {
            return (int)m_Attackers.size();
        }

    private:
        using Clock = std::chrono::steady_clock;

        void Maintain(int target, const WaveOptions& options);
        void DeleteExtras();

        struct Extra
        {
            int handle;
            bool vehicle;
        };
        std::vector<Extra> m_Extras;
        int m_Target = 0;
        bool m_HadWave = false;
        Clock::time_point m_NextSpawn{};
        Clock::time_point m_NextMaintain{};
    };
}
