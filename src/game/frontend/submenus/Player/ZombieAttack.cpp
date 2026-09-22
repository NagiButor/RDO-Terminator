// ZombieAttack.cpp - Spawns 10 cholera-corpse zombies that hunt down the selected player.
// Half of them (coin flip each) get a knife forced into their hands.

#include "core/commands/LoopedCommand.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/backend/Players.hpp"
#include "game/frontend/submenus/Player/AttackWaves.hpp"
#include "game/rdr/Ped.hpp"

#include <string>

namespace YimMenu::Features
{
    static std::vector<int> zombie_index;
    static int zombie_cooldown = 0;

    static AttackWaves::Config GetZombieConfig()
    {
        AttackWaves::Config config{};
        config.models = {"A_M_M_ARMCHOLERACORPSE_01", "A_F_M_ARMCHOLERACORPSE_01"};
        config.weapons = {"WEAPON_MELEE_KNIFE"};
        config.weapon_chance = 0.5f; // every zombie flips a coin for the knife
        config.unlimited = true; // the game reads them as corpses: waves pile into a real horde
        config.count = 10;
        config.distance = 45.f;
        config.move_rate = 1.1f; // shambling horde, still catches walking targets
        config.accuracy = 50.f;
        config.force_equip = true; // knife straight into their hands
        return config;
    }

    class ZombieAttack : public LoopedCommand
    {
        using LoopedCommand::LoopedCommand;

        void OnTick() override
        {
            Player target_player = Players::GetSelected();
            Ped player_ped = target_player.GetPed();

            if (!player_ped.IsValid() || player_ped.GetHealth() <= 0)
            {
                AttackWaves::Cleanup(zombie_index);
                return;
            }

            // only forget the dead (their corpses stay in the world) -- never gate on the living
            AttackWaves::Cleanup(zombie_index);

            if (zombie_cooldown > 0)
            {
                --zombie_cooldown;
                return;
            }

            if (AttackWaves::SpawnWave(player_ped.GetHandle(), GetZombieConfig(), zombie_index) == 0)
            {
                zombie_cooldown = 300;
                return;
            }

            zombie_cooldown = 600; // next wave of 10 piles onto the horde
            AttackWaves::Cleanup(zombie_index);
        }

        void OnEnable() override
        {
            zombie_cooldown = 0;
            Notifications::Show("Zombie Attack", std::string("Raising a horde on ") + Players::GetSelected().GetName(), NotificationType::Info);
        }

        void OnDisable() override
        {
            AttackWaves::Cleanup(zombie_index, true);
            Notifications::Show("Zombie Attack", std::string("Cleared zombies (target: ") + Players::GetSelected().GetName() + ")", NotificationType::Info);
        }
    };

    static ZombieAttack _ZombieAttack{"zombieattack", "Zombie Attack", "Endless waves of 10 cholera zombies (half armed with knives) piling onto the selected player."};
}
