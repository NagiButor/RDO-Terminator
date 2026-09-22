// CavalryAttack.cpp - Mounted soldiers and a Maxim-gun wagon hunting the selected player

#include "core/commands/LoopedCommand.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/backend/Players.hpp"
#include "game/backend/ScriptMgr.hpp"
#include "game/frontend/submenus/Player/AttackWaves.hpp"
#include "game/frontend/submenus/Player/NpcAttack.hpp"
#include "game/rdr/Ped.hpp"
#include "game/rdr/Vehicle.hpp"
#include "util/Joaat.hpp"

#include <cmath>
#include <random>
#include <string>
#include <vector>

namespace YimMenu::Features
{
	static std::vector<int> cavalry_index; // riders + their horses
	static std::vector<int> maxim_crew_index;
	static int maxim_vehicle = 0;
	static std::vector<int> police_crew_index;
	static int police_vehicle = 0;

	static std::mt19937& CavalryRng()
	{
		static std::mt19937 rng{std::random_device{}()};
		return rng;
	}

	static void RequestModelBlocking(Hash model)
	{
		if (!STREAMING::IS_MODEL_VALID(model) || !STREAMING::IS_MODEL_IN_CDIMAGE(model))
			return;

		STREAMING::REQUEST_MODEL(model, false);
		int tries = 0;
		while (!STREAMING::HAS_MODEL_LOADED(model) && tries < 100)
		{
			ScriptMgr::Yield();
			++tries;
		}
	}

	static void SetupFighter(int handle, bool human, const char* weapon)
	{
		PED::SET_PED_COMBAT_ABILITY(handle, 3);
		PED::SET_PED_COMBAT_MOVEMENT(handle, 3);
		PED::SET_PED_COMBAT_RANGE(handle, 3);
		PED::SET_PED_MOVE_RATE_OVERRIDE(handle, 1.35f);
		PED::SET_PED_ACCURACY(handle, 70);
		PED::SET_PED_SEEING_RANGE(handle, 150.f);
		PED::SET_PED_HEARING_RANGE(handle, 150.f);

		PED::SET_PED_COMBAT_ATTRIBUTES(handle, 0, true);
		PED::SET_PED_COMBAT_ATTRIBUTES(handle, 1, true);
		PED::SET_PED_COMBAT_ATTRIBUTES(handle, 2, true);
		PED::SET_PED_COMBAT_ATTRIBUTES(handle, 3, true);
		PED::SET_PED_COMBAT_ATTRIBUTES(handle, 13, true);
		PED::SET_PED_COMBAT_ATTRIBUTES(handle, 20, true);
		PED::SET_PED_COMBAT_ATTRIBUTES(handle, 46, true);
		PED::SET_PED_COMBAT_ATTRIBUTES(handle, 50, true);
		PED::SET_PED_COMBAT_ATTRIBUTES(handle, 58, true);
		PED::SET_PED_COMBAT_ATTRIBUTES(handle, 61, true);
		PED::SET_PED_COMBAT_ATTRIBUTES(handle, 142, true);
		PED::SET_PED_COMBAT_ATTRIBUTES(handle, 188, true);
		PED::SET_PED_COMBAT_ATTRIBUTES(handle, 281, true);

		for (int flee_attribute : {0, 1, 2, 4, 8, 16, 32, 64, 128, 256})
			PED::SET_PED_FLEE_ATTRIBUTES(handle, flee_attribute, false);

		PED::SET_PED_CONFIG_FLAG(handle, 124, false);
		PED::SET_PED_CONFIG_FLAG(handle, 56, true);
		PED::SET_PED_CONFIG_FLAG(handle, (int)PedConfigFlag::CanAttackFriendly, true);
		PED::SET_PED_CONFIG_FLAG(handle, (int)PedConfigFlag::TreatDislikeAsHateWhenInCombat, true);
		PED::SET_PED_CONFIG_FLAG(handle, (int)PedConfigFlag::TreatNonFriendlyAsHateWhenInCombat, true);
		PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(handle, false);

		PED::SET_PED_LASSO_HOGTIE_FLAG(handle, (int)LassoFlags::LHF_CAN_BE_LASSOED, false);
		PED::SET_PED_LASSO_HOGTIE_FLAG(handle, (int)LassoFlags::LHF_CAN_BE_LASSOED_BY_FRIENDLY_PLAYERS, false);
		PED::SET_PED_LASSO_HOGTIE_FLAG(handle, (int)LassoFlags::LHF_DISABLE_IN_MP, true);

		if (human && weapon)
		{
			WEAPON::GIVE_WEAPON_TO_PED(handle, Joaat(weapon), 500, true, false, 0, false, 0.5f, 1.0f, 1, false, 0, false);
			WEAPON::SET_CURRENT_PED_WEAPON(handle, Joaat(weapon), true, 0, false, false);
		}
	}

	static Vector3 RingPosAround(int target_handle, float distance)
	{
		std::uniform_real_distribution<float> angle_dist(0.f, 6.2831853f);
		std::uniform_real_distribution<float> radius_dist(distance * 0.6f, distance);

		const float angle  = angle_dist(CavalryRng());
		const float radius = radius_dist(CavalryRng());
		Vector3 pos = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(target_handle, std::cos(angle) * radius, std::sin(angle) * radius, 0.f);

		float ground_z = 0.f;
		if (MISC::GET_GROUND_Z_FOR_3D_COORD(pos.x, pos.y, pos.z + 5.f, &ground_z, false))
			pos.z = ground_z + 0.5f;

		return pos;
	}

	static void SpawnCavalry(int target_handle, float distance)
	{
		static constexpr const char* kHorses[] = {
			"A_C_Horse_TennesseeWalker_Chestnut",
			"A_C_Horse_AmericanStandardbred_Black",
			"A_C_Horse_Mustang_GrulloDun",
			"A_C_Horse_KentuckySaddle_Black",
		};
		static constexpr const char* kRiders[] = {
			"S_M_Y_Army_01",
			"S_M_M_Army_01",
			"U_M_M_ARMYTRN4_01",
		};
		static constexpr const char* kWeapons[] = {
			"WEAPON_REPEATER_CARBINE",
			"WEAPON_RIFLE_SPRINGFIELD",
			"WEAPON_REPEATER_WINCHESTER",
		};

		for (const char* model : kHorses)
			RequestModelBlocking(Joaat(model));
		for (const char* model : kRiders)
			RequestModelBlocking(Joaat(model));

		std::uniform_int_distribution<std::size_t> horse_dist(0, 3);
		std::uniform_int_distribution<std::size_t> rider_dist(0, 2);
		std::uniform_int_distribution<std::size_t> weapon_dist(0, 2);

		for (int i = 0; i < 4; ++i)
		{
			ScriptMgr::Yield(); // spread networked spawns across ticks

			Vector3 pos = RingPosAround(target_handle, distance);

			Ped horse = Ped::Create(Joaat(kHorses[horse_dist(CavalryRng())]), pos, 0.f);
			if (!horse.IsValid())
				continue;

			Ped rider = Ped::Create(Joaat(kRiders[rider_dist(CavalryRng())]), pos, 0.f);
			if (!rider.IsValid())
				continue;

			const int horse_handle = horse.GetHandle();
			const int rider_handle = rider.GetHandle();
			if (!ENTITY::DOES_ENTITY_EXIST(horse_handle) || !ENTITY::DOES_ENTITY_EXIST(rider_handle))
				continue;

			SetupFighter(horse_handle, false, nullptr);
			SetupFighter(rider_handle, true, kWeapons[weapon_dist(CavalryRng())]);

			rider.SetInMount(horse);

			if (NpcAttackOptions::ConfiguredLasso())
			{
				WEAPON::GIVE_WEAPON_TO_PED(rider_handle, Joaat("WEAPON_LASSO"), 1, true, false, 0, false, 0.5f, 1.0f, 1, false, 0, false);
				WEAPON::SET_CURRENT_PED_WEAPON(rider_handle, Joaat("WEAPON_LASSO"), true, 0, false, false);
				TASK::TASK_LASSO_PED(rider_handle, target_handle);
			}
			else
			{
				TASK::TASK_COMBAT_PED(rider_handle, target_handle, 0, 16);
			}

			cavalry_index.push_back(horse_handle);
			cavalry_index.push_back(rider_handle);
		}

		for (const char* model : kHorses)
			STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(Joaat(model));
		for (const char* model : kRiders)
			STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(Joaat(model));
	}

	class CavalryAttack : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		void OnTick() override
		{
			Player target_player = Players::GetSelected();
			Ped player_ped       = target_player.GetPed();

			if (!player_ped.IsValid() || player_ped.GetHealth() <= 0)
			{
				AttackWaves::Cleanup(cavalry_index);
				return;
			}

			if (!cavalry_index.empty())
			{
				AttackWaves::Cleanup(cavalry_index);
				return;
			}

			SpawnCavalry(player_ped.GetHandle(), 55.f);
			AttackWaves::Cleanup(cavalry_index);
		}

		void OnEnable() override
		{
			Notifications::Show("Cavalry Attack", std::string("Sending mounted soldiers after ") + Players::GetSelected().GetName(), NotificationType::Info);
		}

		void OnDisable() override
		{
			AttackWaves::Cleanup(cavalry_index, true);
			Notifications::Show("Cavalry Attack", std::string("Cleared cavalry (target: ") + Players::GetSelected().GetName() + ")", NotificationType::Info);
		}
	};

	static CavalryAttack _CavalryAttack{"cavalryattack", "Cavalry Attack", "Spawns 4 soldiers on horseback with rifles that hunt down the selected player."};

	static int FirstFreeSeat(int vehicle, int from_seat = -1)
	{
		const int seats = VEHICLE::GET_VEHICLE_MODEL_NUMBER_OF_SEATS(ENTITY::GET_ENTITY_MODEL(vehicle));
		for (int seat = from_seat; seat < seats; ++seat)
		{
			if (VEHICLE::IS_VEHICLE_SEAT_FREE(vehicle, seat))
				return seat;
		}
		return -2; // none free
	}

	static void SpawnGunWagon(int target_handle, Hash wagon_model, Hash turret_weapon, const char* driver_model, const char* gunner_model, const char* feature_name, std::vector<int>& crew_index, int& vehicle_handle)
	{
		const Hash driver_model_hash = Joaat(driver_model);
		const Hash gunner_model_hash = Joaat(gunner_model);

		RequestModelBlocking(wagon_model);
		RequestModelBlocking(driver_model_hash);
		RequestModelBlocking(gunner_model_hash);

		Vector3 pos = RingPosAround(target_handle, 60.f);

		Vehicle wagon = Vehicle::Create(wagon_model, pos, 0.f);
		if (!wagon.IsValid())
		{
			Notifications::Show(feature_name, "Could not spawn the wagon.", NotificationType::Error);
			return;
		}

		vehicle_handle = wagon.GetHandle();
		if (!ENTITY::DOES_ENTITY_EXIST(vehicle_handle))
		{
			vehicle_handle = 0;
			return;
		}

		WEAPON::_SET_AMMO_IN_TURRET(vehicle_handle, turret_weapon, 999);

		Ped driver = Ped::Create(driver_model_hash, pos, 0.f);
		Ped gunner = Ped::Create(gunner_model_hash, pos, 0.f);

		if (driver.IsValid() && ENTITY::DOES_ENTITY_EXIST(driver.GetHandle()))
		{
			const int driver_handle = driver.GetHandle();
			SetupFighter(driver_handle, true, "WEAPON_REPEATER_CARBINE");

			const int seat = FirstFreeSeat(vehicle_handle, -1); // -1 = driver first
			if (seat >= -1)
				PED::SET_PED_INTO_VEHICLE(driver_handle, vehicle_handle, seat);

			TASK::TASK_COMBAT_PED(driver_handle, target_handle, 0, 16);
			crew_index.push_back(driver_handle);
		}

		if (gunner.IsValid() && ENTITY::DOES_ENTITY_EXIST(gunner.GetHandle()))
		{
			const int gunner_handle = gunner.GetHandle();
			SetupFighter(gunner_handle, true, "WEAPON_REPEATER_WINCHESTER");

			// prefer the turret seat so the gunner works the gun, otherwise any free seat
			int turret_seat = -2;
			int seat       = -2;
			if (VEHICLE::_GET_VEHICLE_TURRET_SEAT(vehicle_handle, &turret_seat) && VEHICLE::IS_VEHICLE_SEAT_FREE(vehicle_handle, turret_seat))
				seat = turret_seat;
			else
				seat = FirstFreeSeat(vehicle_handle, 0);

			if (seat >= -1)
			{
				PED::SET_PED_INTO_VEHICLE(gunner_handle, vehicle_handle, seat);
				TASK::TASK_VEHICLE_SHOOT_AT_PED(gunner_handle, target_handle, 0.5f);
			}
			else
			{
				TASK::TASK_COMBAT_PED(gunner_handle, target_handle, 0, 16);
			}
			crew_index.push_back(gunner_handle);
		}

		STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(wagon_model);
		STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(driver_model_hash);
		STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(gunner_model_hash);
	}

	static void DeleteGunWagon(int& vehicle_handle)
	{
		if (vehicle_handle != 0 && ENTITY::DOES_ENTITY_EXIST(vehicle_handle))
		{
			auto handle = vehicle_handle;
			VEHICLE::DELETE_VEHICLE(&handle);
		}
		vehicle_handle = 0;
	}

	class MaximWagonAttack : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		void OnTick() override
		{
			Player target_player = Players::GetSelected();
			Ped player_ped       = target_player.GetPed();

			if (!player_ped.IsValid() || player_ped.GetHealth() <= 0)
			{
				AttackWaves::Cleanup(maxim_crew_index);
				return;
			}

			if (!maxim_crew_index.empty())
			{
				AttackWaves::Cleanup(maxim_crew_index);
				if (maxim_crew_index.empty())
					DeleteGunWagon(maxim_vehicle); // crew wiped out: remove the empty wagon too
				return;
			}

			DeleteGunWagon(maxim_vehicle);
			SpawnGunWagon(player_ped.GetHandle(), Joaat("gatlingMaxim02"), Joaat("WEAPON_TURRET_MAXIM"),
			    "S_M_Y_Army_01", "U_M_M_ARMYTRN4_01", "Maxim Wagon", maxim_crew_index, maxim_vehicle);
			AttackWaves::Cleanup(maxim_crew_index);
		}

		void OnEnable() override
		{
			Notifications::Show("Maxim Wagon", std::string("Rolling out the Maxim wagon on ") + Players::GetSelected().GetName(), NotificationType::Info);
		}

		void OnDisable() override
		{
			AttackWaves::Cleanup(maxim_crew_index, true);
			DeleteGunWagon(maxim_vehicle);
			Notifications::Show("Maxim Wagon", std::string("Cleared the wagon (target: ") + Players::GetSelected().GetName() + ")", NotificationType::Info);
		}
	};

	static MaximWagonAttack _MaximWagonAttack{"maximwagonattack", "Maxim Wagon", "Spawns a Maxim-gun wagon with driver and gunner that hunts down the selected player."};

	class PoliceGatlingAttack : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		void OnTick() override
		{
			Player target_player = Players::GetSelected();
			Ped player_ped       = target_player.GetPed();

			if (!player_ped.IsValid() || player_ped.GetHealth() <= 0)
			{
				AttackWaves::Cleanup(police_crew_index);
				return;
			}

			if (!police_crew_index.empty())
			{
				AttackWaves::Cleanup(police_crew_index);
				if (police_crew_index.empty())
					DeleteGunWagon(police_vehicle); // crew wiped out: remove the empty wagon too
				return;
			}

			DeleteGunWagon(police_vehicle);
			SpawnGunWagon(player_ped.GetHandle(), Joaat("policeWagongatling01x"), Joaat("WEAPON_TURRET_GATLING"),
			    "S_M_M_DispatchPolice_01", "S_M_M_DispatchLeaderPolice_01", "Police Gatling", police_crew_index, police_vehicle);
			AttackWaves::Cleanup(police_crew_index);
		}

		void OnEnable() override
		{
			Notifications::Show("Police Gatling", std::string("Rolling out the police Gatling wagon on ") + Players::GetSelected().GetName(), NotificationType::Info);
		}

		void OnDisable() override
		{
			AttackWaves::Cleanup(police_crew_index, true);
			DeleteGunWagon(police_vehicle);
			Notifications::Show("Police Gatling", std::string("Cleared the wagon (target: ") + Players::GetSelected().GetName() + ")", NotificationType::Info);
		}
	};

	static PoliceGatlingAttack _PoliceGatlingAttack{"policewagonattack", "Police Gatling Wagon", "Spawns a police Gatling wagon with driver and gunner that hunts down the selected player."};
}
