#include "AttackWaves.hpp"

#include "util/Joaat.hpp"

#include <chrono>
#include <cmath>
#include <random>
#include <unordered_map>

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
        ENTITY::SET_ENTITY_AS_MISSION_ENTITY(handle, true, true);

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
            WEAPON::GIVE_WEAPON_TO_PED(handle, Joaat(weapon_name), 500, true, false, 0, false, 0.5f, 1.0f, 1, false, 0, false);
    }

    int SpawnWave(int target_handle, const Config& config, std::vector<int>& spawned)
    {
        if (!ENTITY::DOES_ENTITY_EXIST(target_handle) || config.models.empty() || config.count <= 0)
            return 0;

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

        return spawn_count;
    }

    // Corpses are kept around for a while after death so they don't vanish the moment they die.
    static constexpr float corpse_lifetime = 45.f;
    static std::unordered_map<int, std::chrono::steady_clock::time_point> s_corpses;

    void Cleanup(std::vector<int>& spawned, bool delete_all)
    {
        const auto now = std::chrono::steady_clock::now();

        // purge corpses that outlived their keep-alive time (everything when delete_all is set)
        for (auto it = s_corpses.begin(); it != s_corpses.end();)
        {
            if (delete_all || now >= it->second)
            {
                int handle = it->first;
                if (ENTITY::DOES_ENTITY_EXIST(handle) && ENTITY::IS_ENTITY_DEAD(handle))
                    PED::DELETE_PED(&handle);
                it = s_corpses.erase(it);
            }
            else
                ++it;
        }

        auto it = spawned.begin();
        while (it != spawned.end())
        {
            const bool exists = ENTITY::DOES_ENTITY_EXIST(*it);
            const bool remove = delete_all || !exists || ENTITY::IS_ENTITY_DEAD(*it);

            if (remove)
            {
                if (delete_all)
                {
                    if (exists)
                        PED::DELETE_PED(&(*it));
                }
                else if (exists)
                {
                    // dead attacker: keep the corpse visible for a while instead of deleting it instantly
                    s_corpses[*it] = now + std::chrono::seconds((int)corpse_lifetime);
                }
                // a ped that simply despawned has nothing left to clean up

                it = spawned.erase(it);
            }
            else
                ++it;
        }
    }
}
