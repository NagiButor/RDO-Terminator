// CavalryAttack.cpp - Mounted soldiers and gun wagons hunting the selected player

#include "core/commands/LoopedCommand.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/backend/Players.hpp"
#include "game/backend/ScriptMgr.hpp"
#include "game/frontend/submenus/Player/AttackWaves.hpp"
#include "game/frontend/submenus/Player/NpcAttack.hpp"
#include "game/rdr/Ped.hpp"
#include "game/rdr/Vehicle.hpp"
#include "util/Joaat.hpp"

#include <random>
#include <string>
#include <vector>

namespace YimMenu::Features
{
	static std::mt19937& CavalryRng()
	{
		static std::mt19937 rng{std::random_device{}()};
		return rng;
	}

	template<std::size_t N>
	static const char* Pick(const char* const (&list)[N])
	{
		std::uniform_int_distribution<std::size_t> dist(0, N - 1);
		return list[dist(CavalryRng())];
	}

	static AttackWaves::Config FighterConfig()
	{
		AttackWaves::Config config{};
		config.move_rate = 1.35f;
		config.accuracy  = 70.f;
		return config;
	}

	// ---- cavalry -----------------------------------------------------------

	static int SpawnCavalry(int target_handle, AttackWaves::WaveRunner& runner)
	{
		static constexpr const char* kHorses[] = {
		    "A_C_Horse_TennesseeWalker_Chestnut",
		    "A_C_Horse_AmericanStandardbred_Black",
		    "A_C_Horse_Mustang_GrulloDun",
		    "A_C_Horse_KentuckySaddle_Black",
		};
		static constexpr const char* kRiders[] = {"S_M_Y_Army_01", "S_M_M_Army_01", "U_M_M_ARMYTRN4_01"};
		static constexpr const char* kWeapons[] = {"WEAPON_REPEATER_CARBINE", "WEAPON_RIFLE_SPRINGFIELD", "WEAPON_REPEATER_WINCHESTER"};
		constexpr int kRiderCount = 4;

		std::vector<Hash> models;
		for (const char* m : kHorses)
			models.push_back(Joaat(m));
		for (const char* m : kRiders)
			models.push_back(Joaat(m));
		AttackWaves::RequestModels(models);

		const bool lasso = NpcAttackOptions::ConfiguredLasso();
		const auto config = FighterConfig();
		int riders = 0;
		for (int i = 0; i < kRiderCount; ++i)
		{
			ScriptMgr::Yield(); // spread networked spawns across ticks

			Vector3 pos;
			if (!AttackWaves::FindSpawnPos(target_handle, 55.f, AttackWaves::SpawnSide::Around, i, kRiderCount, pos))
				continue;

			Ped horse = Ped::Create(Joaat(Pick(kHorses)), pos, 0.f);
			if (!horse.IsValid() || !ENTITY::DOES_ENTITY_EXIST(horse.GetHandle()))
				continue;

			Ped rider = Ped::Create(Joaat(Pick(kRiders)), pos, 0.f);
			if (!rider.IsValid() || !ENTITY::DOES_ENTITY_EXIST(rider.GetHandle()))
			{
				// no rider for this horse: don't leave an untracked horse standing around
				int orphan = horse.GetHandle();
				PED::DELETE_PED(&orphan);
				continue;
			}

			const int horse_handle = horse.GetHandle();
			const int rider_handle = rider.GetHandle();
			AttackWaves::SetUpAttacker(horse_handle, false, config);
			AttackWaves::SetUpAttacker(rider_handle, true, config, Pick(kWeapons));
			rider.SetInMount(horse);
			AttackWaves::TaskAttack(rider_handle, target_handle, lasso);

			// the rider is the combatant; the horse is removed with the wave once riders are gone
			runner.m_Attackers.push_back(rider_handle);
			runner.AddExtra(horse_handle, false);
			++riders;
		}

		AttackWaves::ReleaseModels(models);
		return riders;
	}

	class CavalryAttack : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		AttackWaves::WaveRunner m_Runner;

		void OnTick() override
		{
			AttackWaves::WaveOptions options{};
			options.lasso         = NpcAttackOptions::ConfiguredLasso();
			options.wave_delay_ms = 6000;
			m_Runner.Tick(options, SpawnCavalry);
		}

		void OnEnable() override
		{
			Notifications::Show("Cavalry Attack", std::string("Sending mounted soldiers after ") + Players::GetSelected().GetName(), NotificationType::Info);
		}

		void OnDisable() override
		{
			m_Runner.Clear();
			Notifications::Show("Cavalry Attack", "Cleared cavalry", NotificationType::Info);
		}
	};

	static CavalryAttack _CavalryAttack{"cavalryattack", "Cavalry Attack", "Spawns 4 soldiers on horseback with rifles that hunt down the selected player."};

	// ---- gun wagons --------------------------------------------------------

	static int FirstFreeSeat(int vehicle, int from_seat = -1)
	{
		const int seats = VEHICLE::GET_VEHICLE_MODEL_NUMBER_OF_SEATS(ENTITY::GET_ENTITY_MODEL(vehicle));
		for (int seat = from_seat; seat < seats; ++seat)
			if (VEHICLE::IS_VEHICLE_SEAT_FREE(vehicle, seat))
				return seat;
		return -2; // none free
	}

	struct GunWagonSpec
	{
		const char* wagon;
		const char* turret;
		const char* driver;
		const char* gunner;
		const char* name;
	};

	static int SpawnGunWagon(int target_handle, AttackWaves::WaveRunner& runner, const GunWagonSpec& spec)
	{
		const Hash wagon_model  = Joaat(spec.wagon);
		const Hash driver_model = Joaat(spec.driver);
		const Hash gunner_model = Joaat(spec.gunner);
		const std::vector<Hash> models{wagon_model, driver_model, gunner_model};
		AttackWaves::RequestModels(models);

		int crew = 0;
		Vector3 pos;
		if (AttackWaves::FindSpawnPos(target_handle, 60.f, AttackWaves::SpawnSide::Around, 0, 1, pos))
		{
			Vehicle wagon = Vehicle::Create(wagon_model, pos, 0.f);
			const int vehicle = wagon.IsValid() ? wagon.GetHandle() : 0;
			if (vehicle && ENTITY::DOES_ENTITY_EXIST(vehicle))
			{
				runner.AddExtra(vehicle, true); // deleted with the wave if the crew fails / dies
				WEAPON::_SET_AMMO_IN_TURRET(vehicle, Joaat(spec.turret), 999);
				const auto config = FighterConfig();

				Ped driver = Ped::Create(driver_model, pos, 0.f);
				if (driver.IsValid() && ENTITY::DOES_ENTITY_EXIST(driver.GetHandle()))
				{
					const int h = driver.GetHandle();
					AttackWaves::SetUpAttacker(h, true, config, "WEAPON_REPEATER_CARBINE");
					if (const int seat = FirstFreeSeat(vehicle, -1); seat >= -1)
						PED::SET_PED_INTO_VEHICLE(h, vehicle, seat);
					AttackWaves::TaskAttack(h, target_handle, false);
					runner.m_Attackers.push_back(h);
					++crew;
				}

				Ped gunner = Ped::Create(gunner_model, pos, 0.f);
				if (gunner.IsValid() && ENTITY::DOES_ENTITY_EXIST(gunner.GetHandle()))
				{
					const int h = gunner.GetHandle();
					AttackWaves::SetUpAttacker(h, true, config, "WEAPON_REPEATER_WINCHESTER");

					// prefer the turret seat so the gunner works the gun, otherwise any free seat
					int seat = -2, turret_seat = -2;
					if (VEHICLE::_GET_VEHICLE_TURRET_SEAT(vehicle, &turret_seat) && VEHICLE::IS_VEHICLE_SEAT_FREE(vehicle, turret_seat))
						seat = turret_seat;
					else
						seat = FirstFreeSeat(vehicle, 0);

					if (seat >= -1)
					{
						PED::SET_PED_INTO_VEHICLE(h, vehicle, seat);
						TASK::TASK_VEHICLE_SHOOT_AT_PED(h, target_handle, 0.5f);
					}
					else
					{
						AttackWaves::TaskAttack(h, target_handle, false);
					}
					runner.m_Attackers.push_back(h);
					++crew;
				}
			}
		}

		AttackWaves::ReleaseModels(models);
		if (crew == 0)
			Notifications::Show(spec.name, "Could not spawn the wagon crew, retrying shortly.", NotificationType::Error);
		return crew;
	}

	class GunWagonAttack : public LoopedCommand
	{
	public:
		GunWagonAttack(std::string name, std::string label, std::string description, GunWagonSpec spec) :
		    LoopedCommand(std::move(name), std::move(label), std::move(description)),
		    m_Spec(spec)
		{
		}

	private:
		GunWagonSpec m_Spec;
		AttackWaves::WaveRunner m_Runner;

		void OnTick() override
		{
			AttackWaves::WaveOptions options{};
			options.wave_delay_ms = 8000;
			options.fail_retry_ms = 15000;
			options.leash         = 350.f; // wagons take the long way round
			m_Runner.Tick(options, [this](int target, AttackWaves::WaveRunner& runner) {
				return SpawnGunWagon(target, runner, m_Spec);
			});
		}

		void OnEnable() override
		{
			Notifications::Show(m_Spec.name, std::string("Rolling out on ") + Players::GetSelected().GetName(), NotificationType::Info);
		}

		void OnDisable() override
		{
			m_Runner.Clear();
			Notifications::Show(m_Spec.name, "Cleared the wagon", NotificationType::Info);
		}
	};

	static GunWagonAttack _MaximWagonAttack{"maximwagonattack", "Maxim Wagon", "Spawns a Maxim-gun wagon with driver and gunner that hunts down the selected player.",
	    {"gatlingMaxim02", "WEAPON_TURRET_MAXIM", "S_M_Y_Army_01", "U_M_M_ARMYTRN4_01", "Maxim Wagon"}};

	static GunWagonAttack _PoliceGatlingAttack{"policewagonattack", "Police Gatling Wagon", "Spawns a police Gatling wagon with driver and gunner that hunts down the selected player.",
	    {"policeWagongatling01x", "WEAPON_TURRET_GATLING", "S_M_M_DispatchPolice_01", "S_M_M_DispatchLeaderPolice_01", "Police Gatling"}};
}
