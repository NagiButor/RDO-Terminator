#include "AttackWaves.hpp"

#include "game/backend/ScriptMgr.hpp"
#include "util/Joaat.hpp"

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

    static void SetUpAttacker(int handle, bool human, const Config& config)
    {
        // NOTE: no SET_ENTITY_AS_MISSION_ENTITY here -- same as IndianAttack.
        // Mission peds are garbage-collected by the engine the moment they die,
        // which made corpses vanish instantly. Leaving the ped as a regular
        // entity keeps the corpse around like any normal RDO corpse (lootable
        // / skinnable by players).

        if (config.invincible)
            ENTITY::SET_ENTITY_INVINCIBLE(handle, true);

        if (config.health > 0.f)
            ENTITY::SET_ENTITY_HEALTH(handle, (int)config.health, 0);

        if (config.armour > 0)
            PED::ADD_ARMOUR_TO_PED(handle, config.armour);

        // combat setup
        PED::SET_PED_COMBAT_ABILITY(handle, 3);  // max ability
        PED::SET_PED_COMBAT_MOVEMENT(handle, 3); // charge the target
        PED::SET_PED_COMBAT_RANGE(handle, 3);    // max range
        PED::SET_PED_MOVE_RATE_OVERRIDE(handle, config.move_rate);
        PED::SET_PED_ACCURACY(handle, (int)config.accuracy);
        PED::SET_PED_SEEING_RANGE(handle, 150.f);
        PED::SET_PED_HEARING_RANGE(handle, 150.f);
        PED::SET_PED_VISUAL_FIELD_PERIPHERAL_RANGE(handle, 180.f);
        PED::SET_PED_VISUAL_FIELD_CENTER_ANGLE(handle, 90.f);

        // aggression (same recipe the other attack features use)
        PED::SET_PED_COMBAT_ATTRIBUTES(handle, 0, true);   // use cover
        PED::SET_PED_COMBAT_ATTRIBUTES(handle, 1, true);   // use vehicles
        PED::SET_PED_COMBAT_ATTRIBUTES(handle, 2, true);   // can fight armed peds
        PED::SET_PED_COMBAT_ATTRIBUTES(handle, 3, true);   // always fight
        PED::SET_PED_COMBAT_ATTRIBUTES(handle, 13, true);  // can chase the target on foot
        PED::SET_PED_COMBAT_ATTRIBUTES(handle, 20, true);  // can use melee
        PED::SET_PED_COMBAT_ATTRIBUTES(handle, 46, true);  // aggressive animal / fight while unarmed
        PED::SET_PED_COMBAT_ATTRIBUTES(handle, 50, true);  // always attack
        PED::SET_PED_COMBAT_ATTRIBUTES(handle, 58, true);  // ignore danger
        PED::SET_PED_COMBAT_ATTRIBUTES(handle, 61, true);  // ignore dead peds
        PED::SET_PED_COMBAT_ATTRIBUTES(handle, 142, true); // attack when threatened
        PED::SET_PED_COMBAT_ATTRIBUTES(handle, 188, true); // animals can attack
        PED::SET_PED_COMBAT_ATTRIBUTES(handle, 281, true); // ignore when outnumbered

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

        if (!human)
            return;

        const char* weapon_name = PickRandom(config.weapons);
        if (weapon_name)
        {
            WEAPON::GIVE_WEAPON_TO_PED(handle, Joaat(weapon_name), 500, true, false, 0, false, 0.5f, 1.0f, 1, false, 0, false);

            if (config.force_equip)
                WEAPON::SET_CURRENT_PED_WEAPON(handle, Joaat(weapon_name), true, 0, false, false);
        }
    }

    int SpawnWave(int target_handle, const Config& config, std::vector<int>& spawned)
    {
        if (!ENTITY::DOES_ENTITY_EXIST(target_handle) || config.models.empty() || config.count <= 0)
            return 0;

        // Pre-request the models the same way IndianAttack does. Ped::Create on
        // its own only waits ~30 ticks for a model to stream in, which is not
        // enough for models that were never loaded before (e.g. the knife-wave
        // Indians) -- such a wave silently spawned 0 peds, the list stayed
        // empty and a new wave was attempted every tick, which broke the
        // "next wave only after the last ped of the group dies" flow.
        for (const char* model_name : config.models)
        {
            if (!model_name)
                continue;

            const Hash model = Joaat(model_name);
            if (!STREAMING::IS_MODEL_VALID(model) || !STREAMING::IS_MODEL_IN_CDIMAGE(model))
                continue;

            STREAMING::REQUEST_MODEL(model, false);
            int tries = 0;
            while (!STREAMING::HAS_MODEL_LOADED(model) && tries < 100)
            {
                ScriptMgr::Yield();
                ++tries;
            }
        }

        int spawn_count = 0;

        std::uniform_real_distribution<float> angle_dist(0.f, 6.2831853f);
        std::uniform_real_distribution<float> radius_dist(config.distance * 0.6f, config.distance);

        for (int i = 0; i < config.count; ++i)
        {
            const char* model_name = PickRandom(config.models);
            if (!model_name)
                continue;

            Hash model = Joaat(model_name);
            if (!STREAMING::IS_MODEL_VALID(model) || !STREAMING::IS_MODEL_IN_CDIMAGE(model))
                continue;

            float offset_x = 0.f;
            float offset_y = 0.f;

            switch (config.side)
            {
            case SpawnSide::InFront:
                offset_y = config.distance;
                break;
            case SpawnSide::Behind:
                offset_y = -config.distance;
                break;
            case SpawnSide::Around:
            default:
            {
                float angle = angle_dist(Rng());
                float radius = radius_dist(Rng());
                offset_x = std::cos(angle) * radius;
                offset_y = std::sin(angle) * radius;
                break;
            }
            }

            Vector3 spawn_pos = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(target_handle, offset_x, offset_y, 0.f);

            float ground_z = 0.f;
            if (MISC::GET_GROUND_Z_FOR_3D_COORD(spawn_pos.x, spawn_pos.y, spawn_pos.z + 5.f, &ground_z, false))
                spawn_pos.z = ground_z + 0.5f;

            Ped attacker = Ped::Create(model, spawn_pos, 0.f);
            if (!attacker.IsValid())
                continue;

            int handle = attacker.GetHandle();
            if (!ENTITY::DOES_ENTITY_EXIST(handle))
                continue;

            SetUpAttacker(handle, PED::IS_PED_HUMAN(handle) != 0, config);
            TASK::TASK_COMBAT_PED(handle, target_handle, 0, 16);

            spawned.push_back(handle);
            ++spawn_count;
        }

        // release the model references once the wave is out (same as PedSpawner)
        for (const char* model_name : config.models)
        {
            if (!model_name)
                continue;

            const Hash model = Joaat(model_name);
            if (STREAMING::IS_MODEL_VALID(model))
                STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
        }

        return spawn_count;
    }

    // Same tracking as IndianAttack: dead/despawned attackers are only removed
    // from the list -- their corpses are never deleted, so they stay in the
    // world like typical RDO corpses (lootable / skinnable) until the game
    // cleans them up on its own.
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
                // NOTE: never DELETE_PED a dead attacker -- that is exactly what
                // made corpses vanish instantly. Just stop tracking it; the
                // corpse stays in the world like a typical RDO corpse
                // (lootable / skinnable) until the game cleans it up itself.
                it = spawned.erase(it);
            }
            else
                ++it;
        }
    }
}
