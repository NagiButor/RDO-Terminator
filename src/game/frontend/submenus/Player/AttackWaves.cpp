#include "AttackWaves.hpp"

#include "game/backend/Players.hpp"
#include "game/backend/ScriptMgr.hpp"
#include "util/Joaat.hpp"

#include <algorithm>
#include <cmath>
#include <random>

namespace YimMenu::Features::AttackWaves
{
    static std::mt19937& Rng()
    {
        static std::mt19937 rng{std::random_device{}()};
        return rng;
    }

    static const char* PickRandom(const std::vector<const char*>& list)
    {
        if (list.empty())
            return nullptr;

        std::uniform_int_distribution<std::size_t> dist(0, list.size() - 1);
        return list[dist(Rng())];
    }

    static bool IsAlive(int handle)
    {
        return handle && ENTITY::DOES_ENTITY_EXIST(handle) && !ENTITY::IS_ENTITY_DEAD(handle);
    }

    void SetUpAttacker(int handle, bool human, const Config& config, const char* weapon)
    {
        // NOTE: no SET_ENTITY_AS_MISSION_ENTITY: mission peds are garbage-collected
        // the moment they die, which made corpses vanish instantly.

        if (config.invincible)
            ENTITY::SET_ENTITY_INVINCIBLE(handle, true);

        if (config.health > 0.f)
        {
            // SET_ENTITY_HEALTH is clamped to the max, so raise the max first
            ENTITY::SET_ENTITY_MAX_HEALTH(handle, (int)config.health);
            ENTITY::SET_ENTITY_HEALTH(handle, (int)config.health, 0);
        }

        if (config.armour > 0)
            PED::ADD_ARMOUR_TO_PED(handle, config.armour);

        PED::SET_PED_COMBAT_ABILITY(handle, 3);  // max ability
        PED::SET_PED_COMBAT_MOVEMENT(handle, 3); // charge the target
        PED::SET_PED_COMBAT_RANGE(handle, 3);    // max range
        PED::SET_PED_MOVE_RATE_OVERRIDE(handle, config.move_rate);
        PED::SET_PED_ACCURACY(handle, (int)config.accuracy);
        PED::SET_PED_SEEING_RANGE(handle, 150.f);
        PED::SET_PED_HEARING_RANGE(handle, 150.f);
        PED::SET_PED_VISUAL_FIELD_PERIPHERAL_RANGE(handle, 180.f);
        PED::SET_PED_VISUAL_FIELD_CENTER_ANGLE(handle, 90.f);

        for (int attribute : {0, 1, 2, 3, 13, 20, 46, 50, 58, 61, 142, 188, 281})
            PED::SET_PED_COMBAT_ATTRIBUTES(handle, attribute, true);

        for (int flee_attribute : {0, 1, 2, 4, 8, 16, 32, 64, 128, 256})
            PED::SET_PED_FLEE_ATTRIBUTES(handle, flee_attribute, false);

        PED::SET_PED_CONFIG_FLAG(handle, 124, false); // AlwaysFleeNoMatterWhat
        PED::SET_PED_CONFIG_FLAG(handle, 56, true);   // Aggressive
        PED::SET_PED_CONFIG_FLAG(handle, (int)PedConfigFlag::CanAttackFriendly, true);
        PED::SET_PED_CONFIG_FLAG(handle, (int)PedConfigFlag::TreatDislikeAsHateWhenInCombat, true);
        PED::SET_PED_CONFIG_FLAG(handle, (int)PedConfigFlag::TreatNonFriendlyAsHateWhenInCombat, true);
        PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(handle, false);

        PED::SET_PED_LASSO_HOGTIE_FLAG(handle, (int)LassoFlags::LHF_CAN_BE_LASSOED, false);
        PED::SET_PED_LASSO_HOGTIE_FLAG(handle, (int)LassoFlags::LHF_CAN_BE_LASSOED_BY_FRIENDLY_PLAYERS, false);
        PED::SET_PED_LASSO_HOGTIE_FLAG(handle, (int)LassoFlags::LHF_DISABLE_IN_MP, true);

        // our attackers must always be visible, no matter the local visibility state
        ENTITY::SET_ENTITY_VISIBLE(handle, true);
        ENTITY::RESET_ENTITY_ALPHA(handle);

        if (!human)
            return;

        const char* weapon_name = weapon ? weapon : PickRandom(config.weapons);
        if (!weapon_name)
            return;

        bool give = weapon || config.weapon_chance >= 1.0f;
        if (!give && config.weapon_chance > 0.0f)
        {
            std::uniform_real_distribution<float> roll(0.f, 1.f);
            give = roll(Rng()) < config.weapon_chance;
        }
        if (!give)
            return;

        WEAPON::GIVE_WEAPON_TO_PED(handle, Joaat(weapon_name), 500, true, false, 0, false, 0.5f, 1.0f, 1, false, 0, false);
        if (config.force_equip || weapon)
            WEAPON::SET_CURRENT_PED_WEAPON(handle, Joaat(weapon_name), true, 0, false, false);
    }

    void TaskAttack(int handle, int target_handle, bool lasso)
    {
        if (lasso && PED::IS_PED_HUMAN(handle))
        {
            // hogtie duty: lasso in hands + lasso task instead of a gunfight
            WEAPON::GIVE_WEAPON_TO_PED(handle, "WEAPON_LASSO"_J, 1, true, false, 0, false, 0.5f, 1.0f, 1, false, 0, false);
            WEAPON::SET_CURRENT_PED_WEAPON(handle, "WEAPON_LASSO"_J, true, 0, false, false);
            TASK::TASK_LASSO_PED(handle, target_handle);
            return;
        }

        PED::REGISTER_TARGET(handle, target_handle, true);
        TASK::TASK_COMBAT_PED(handle, target_handle, 0, 16);
    }

    bool FindSpawnPos(int target_handle, float distance, SpawnSide side, int index, int total, Vector3& out)
    {
        std::uniform_real_distribution<float> angle_dist(0.f, 6.2831853f);
        std::uniform_real_distribution<float> radius_dist(distance * 0.6f, distance);
        std::uniform_real_distribution<float> jitter(-1.5f, 1.5f);

        // a few attempts: a random spot can land in water or on unloaded collision
        for (int attempt = 0; attempt < 4; ++attempt)
        {
            float offset_x = 0.f, offset_y = 0.f;
            if (side == SpawnSide::Around)
            {
                const float angle  = angle_dist(Rng());
                const float radius = radius_dist(Rng());
                offset_x           = std::cos(angle) * radius;
                offset_y           = std::sin(angle) * radius;
            }
            else
            {
                // a line across the front / back instead of everyone on one point
                const float spread = 2.5f;
                const float lateral = (index - (total - 1) * 0.5f) * spread;
                offset_x            = lateral + jitter(Rng());
                offset_y            = (side == SpawnSide::InFront ? distance : -distance) + jitter(Rng());
            }

            Vector3 pos = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(target_handle, offset_x, offset_y, 0.f);

            // probe from well above: from +5 m the ray starts inside any hill higher than the target
            float ground_z = 0.f;
            if (!MISC::GET_GROUND_Z_FOR_3D_COORD(pos.x, pos.y, pos.z + 50.f, &ground_z, false))
                continue;

            pos.z = ground_z + 0.5f;
            out   = pos;
            return true;
        }
        return false;
    }

    void RequestModels(const std::vector<Hash>& models)
    {
        for (Hash model : models)
            if (STREAMING::IS_MODEL_VALID(model) && STREAMING::IS_MODEL_IN_CDIMAGE(model))
                STREAMING::REQUEST_MODEL(model, false);

        // all requests stream in parallel; one bounded wait for the lot
        for (int tries = 0; tries < 150; ++tries)
        {
            bool all = true;
            for (Hash model : models)
                if (STREAMING::IS_MODEL_VALID(model) && !STREAMING::HAS_MODEL_LOADED(model))
                    all = false;
            if (all)
                return;
            ScriptMgr::Yield();
        }
    }

    void ReleaseModels(const std::vector<Hash>& models)
    {
        for (Hash model : models)
            if (STREAMING::IS_MODEL_VALID(model))
                STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
    }

    static float HeadingTowards(const Vector3& from, const Vector3& to)
    {
        return std::atan2(-(to.x - from.x), to.y - from.y) * 57.2957795f;
    }

    int SpawnWave(int target_handle, const Config& config, std::vector<int>& spawned)
    {
        if (!ENTITY::DOES_ENTITY_EXIST(target_handle) || config.models.empty() || config.count <= 0)
            return 0;

        // Network safety: every ped is a replicated object. Forget dead handles
        // first, then clamp the wave so one call can never flood the session.
        Cleanup(spawned);

        const int cap = config.unlimited ? kMaxPileUp : kMaxLiveAttackers;
        const int wanted = std::min({config.count, kMaxSpawnPerCall, cap - (int)spawned.size()});
        if (wanted <= 0)
            return 0;

        std::vector<Hash> models;
        for (const char* name : config.models)
        {
            if (!name)
                continue;
            const Hash model = Joaat(name);
            if (STREAMING::IS_MODEL_VALID(model) && STREAMING::IS_MODEL_IN_CDIMAGE(model)
                && std::find(models.begin(), models.end(), model) == models.end())
                models.push_back(model);
        }
        if (models.empty())
            return 0;

        // Ped::Create alone only waits ~30 ticks per model; never-loaded models
        // (e.g. the knife Indians) used to spawn 0 peds that way
        RequestModels(models);

        const Vector3 target_pos = ENTITY::GET_ENTITY_COORDS(target_handle, true, true);
        std::uniform_int_distribution<std::size_t> model_dist(0, models.size() - 1);

        int spawn_count = 0;
        for (int i = 0; i < wanted; ++i)
        {
            ScriptMgr::Yield(); // spread networked spawns across ticks, never a single-tick burst
            if (!ENTITY::DOES_ENTITY_EXIST(target_handle))
                break;

            const Hash model = models[model_dist(Rng())];
            if (!STREAMING::HAS_MODEL_LOADED(model))
                continue;

            Vector3 pos;
            if (!FindSpawnPos(target_handle, config.distance, config.side, i, wanted, pos))
                continue;

            Ped attacker = Ped::Create(model, pos, HeadingTowards(pos, target_pos));
            if (!attacker.IsValid())
                continue;

            const int handle = attacker.GetHandle();
            if (!ENTITY::DOES_ENTITY_EXIST(handle))
                continue;

            const bool human = PED::IS_PED_HUMAN(handle) != 0;
            SetUpAttacker(handle, human, config);
            TaskAttack(handle, target_handle, config.use_lasso);

            spawned.push_back(handle);
            ++spawn_count;
        }

        ReleaseModels(models);
        return spawn_count;
    }

    void Cleanup(std::vector<int>& spawned, bool delete_all)
    {
        auto it = spawned.begin();
        while (it != spawned.end())
        {
            const bool exists = ENTITY::DOES_ENTITY_EXIST(*it);

            if (delete_all)
            {
                // feature turned off: remove only the living ones (so they stop
                // attacking); dead ones stay as normal corpses
                if (exists && !ENTITY::IS_ENTITY_DEAD(*it))
                    PED::DELETE_PED(&(*it));
                it = spawned.erase(it);
            }
            else if (!exists || ENTITY::IS_ENTITY_DEAD(*it))
            {
                // never DELETE_PED a dead attacker: the corpse stays lootable
                it = spawned.erase(it);
            }
            else
                ++it;
        }
    }

    void RemoveOldest(std::vector<int>& spawned, int count)
    {
        for (int n = 0; n < count && !spawned.empty(); ++n)
        {
            auto it = spawned.begin();
            while (it != spawned.end() && !IsAlive(*it))
                it = spawned.erase(it);

            if (it == spawned.end())
                return;

            PED::DELETE_PED(&(*it));
            spawned.erase(it);
        }
    }

    // ---- WaveRunner -------------------------------------------------------

    void WaveRunner::AddExtra(int handle, bool is_vehicle)
    {
        if (handle)
            m_Extras.push_back({handle, is_vehicle});
    }

    void WaveRunner::DeleteExtras()
    {
        for (auto& extra : m_Extras)
        {
            if (!ENTITY::DOES_ENTITY_EXIST(extra.handle))
                continue;
            int handle = extra.handle;
            if (extra.vehicle)
                VEHICLE::DELETE_VEHICLE(&handle);
            else if (!ENTITY::IS_ENTITY_DEAD(handle)) // dead horses stay as lootable corpses
                PED::DELETE_PED(&handle);
        }
        m_Extras.clear();
    }

    void WaveRunner::Maintain(int target, const WaveOptions& options)
    {
        Cleanup(m_Attackers);

        const bool target_changed = target != m_Target;
        m_Target                  = target;
        const Vector3 target_pos  = ENTITY::GET_ENTITY_COORDS(target, true, true);

        for (auto it = m_Attackers.begin(); it != m_Attackers.end();)
        {
            const Vector3 pos = ENTITY::GET_ENTITY_COORDS(*it, true, true);
            const float dx = pos.x - target_pos.x, dy = pos.y - target_pos.y, dz = pos.z - target_pos.z;

            // stuck / left behind (target teleported, fell off a cliff...): replace it
            if (dx * dx + dy * dy + dz * dz > options.leash * options.leash)
            {
                PED::DELETE_PED(&(*it));
                it = m_Attackers.erase(it);
                continue;
            }

            // new selected player or their ped respawned: the old task points at a stale handle.
            // Idle fighters get kicked back into combat; lasso duty only on a real target change
            // (re-issuing TASK_LASSO_PED would restart the throw every time).
            if (target_changed || (!options.lasso && !PED::IS_PED_IN_COMBAT(*it, target)))
                TaskAttack(*it, target, options.lasso);
            ++it;
        }

        // every rider / crew member is gone: riderless horses and empty wagons go too
        if (m_Attackers.empty() && !m_Extras.empty())
            DeleteExtras();
    }

    void WaveRunner::Tick(const WaveOptions& options, const SpawnFn& spawn)
    {
        Ped target_ped = Players::GetSelected().GetPed();
        if (!target_ped.IsValid())
        {
            Cleanup(m_Attackers);
            return;
        }

        const int target = target_ped.GetHandle();
        const auto now   = Clock::now();

        if (now >= m_NextMaintain || target != m_Target)
        {
            m_NextMaintain = now + std::chrono::milliseconds(1500);
            Maintain(target, options);
        }

        // wave wiped out: short breather before the next one instead of an instant respawn
        if (m_HadWave && m_Attackers.empty())
        {
            m_HadWave   = false;
            m_NextSpawn = now + std::chrono::milliseconds(options.wave_delay_ms);
        }

        // never spawn onto a corpse; the retask above catches the respawn
        if (target_ped.GetHealth() <= 0 || ENTITY::IS_ENTITY_DEAD(target))
            return;
        if (now < m_NextSpawn)
            return;
        if (!options.pile_up && !m_Attackers.empty())
            return;

        const int created = spawn(target, *this);
        if (created > 0)
        {
            m_HadWave   = true;
            m_NextSpawn = options.pile_up ? now + std::chrono::milliseconds(options.pile_interval_ms) : Clock::time_point{};
        }
        else
        {
            // model not streamed / cap reached / no ground: back off, don't hammer every tick
            m_NextSpawn = Clock::now() + std::chrono::milliseconds(options.fail_retry_ms);
            if (m_Attackers.empty())
                DeleteExtras(); // half-built wagon / orphaned horses
        }
    }

    void WaveRunner::Clear()
    {
        Cleanup(m_Attackers, true);
        DeleteExtras();
        m_Target    = 0;
        m_HadWave   = false;
        m_NextSpawn = {};
        m_NextMaintain = {};
    }
}
