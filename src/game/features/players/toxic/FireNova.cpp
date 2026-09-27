#include "game/commands/PlayerCommand.hpp"
#include "game/backend/ScriptMgr.hpp"
#include "game/rdr/Natives.hpp"
#include "game/rdr/Pools.hpp"
#include "game/rdr/Object.hpp"
#include "core/frontend/Notifications.hpp"
#include "util/Joaat.hpp"

#include <chrono>
#include <cmath>
#include <random>
#include <utility>
#include <vector>

namespace YimMenu::Features
{
	// Rules: the blast itself never kills us or the called-on player (NPCs may
	// die from it), but the fire afterwards burns everyone, us included.
	static constexpr float kFireRadius    = 25.0f;
	static constexpr float kFireGridStep  = 3.0f;
	static constexpr int kRainStrikes     = 12;
	static constexpr int kRainIntervalMs  = 800;

	static float GroundZ(float x, float y, float ref_z, float fallback_z)
	{
		float z = fallback_z;
		if (MISC::GET_GROUND_Z_FOR_3D_COORD(x, y, ref_z, &z, false))
			return z;
		return fallback_z;
	}

	static void ProtectBlast(std::vector<std::pair<int, bool>>& out, int handle)
	{
		if (!ENTITY::DOES_ENTITY_EXIST(handle))
			return;
		for (auto& e : out)
			if (e.first == handle)
				return;
		out.emplace_back(handle, ENTITY::_GET_ENTITY_CAN_BE_DAMAGED(handle));
		ENTITY::SET_ENTITY_CAN_BE_DAMAGED(handle, false);
	}

	static void ProtectWithMount(int handle, std::vector<std::pair<int, bool>>& out)
	{
		ProtectBlast(out, handle);
		if (PED::IS_PED_ON_MOUNT(handle))
			ProtectBlast(out, PED::GET_MOUNT(handle));
		if (PED::IS_PED_IN_ANY_VEHICLE(handle, false))
			ProtectBlast(out, PED::GET_VEHICLE_PED_IS_USING(handle));
	}

	static void Ignite(int handle, int& count)
	{
		if (!ENTITY::DOES_ENTITY_EXIST(handle))
			return;
		if (FIRE::IS_ENTITY_ON_FIRE(handle))
			return;
		FIRE::START_ENTITY_FIRE(handle, 0, 0, 0);
		++count;
	}

	// one meteor strike: falling rock, damageless-for-us blast, full-area fire
	static void FireNovaStrike(rage::fvector3 center, int targetPed, int selfPed, int& outFires, int& outIgnited)
	{
		const float groundZ = GroundZ(center.x, center.y, center.z + 2.0f, center.z);

		// 1. a burning meteor falls out of the sky onto the point
		static constexpr uint32_t kMeteorModels[] = {
			"s_meteoriteshard01x"_J,
			"p_gen_rockcluster01x"_J,
			"s_rock01x"_J,
		};

		Object meteor{nullptr};
		for (uint32_t model : kMeteorModels)
		{
			if (!STREAMING::IS_MODEL_IN_CDIMAGE(model) || !STREAMING::IS_MODEL_VALID(model))
				continue;
			meteor = Object::Create(model, {center.x, center.y, center.z + 55.0f});
			if (meteor)
				break;
		}

		if (meteor)
		{
			meteor.SetVelocity({0.0f, 0.0f, -40.0f});
			FIRE::START_ENTITY_FIRE(meteor.GetHandle(), 0, 0, 0);

			const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(4);
			while (std::chrono::steady_clock::now() < deadline)
			{
				ScriptMgr::Yield(std::chrono::milliseconds(16));
				if (!meteor || !ENTITY::DOES_ENTITY_EXIST(meteor.GetHandle()))
					break;
				if (meteor.GetPosition().z <= groundZ + 1.0f)
					break;
				meteor.SetVelocity({0.0f, 0.0f, -40.0f}); // keep it falling
			}
		}

		// 2. shield only us and the called-on player (+ mounts/vehicles) from the blast
		std::vector<std::pair<int, bool>> protectedEntities;
		ProtectWithMount(targetPed, protectedEntities);
		if (selfPed != targetPed)
			ProtectWithMount(selfPed, protectedEntities);

		// 3. boom (volatile dynamite tag), then remove the meteor
		FIRE::ADD_EXPLOSION(center.x, center.y, groundZ + 0.5f, 27, 8.0f, true, false, 0.8f);

		if (meteor && ENTITY::DOES_ENTITY_EXIST(meteor.GetHandle()))
			meteor.Delete();

		// 4. dense ground grid so the whole area burns, no patchy gaps
		for (float dx = -kFireRadius; dx <= kFireRadius; dx += kFireGridStep)
		{
			for (float dy = -kFireRadius; dy <= kFireRadius; dy += kFireGridStep)
			{
				if (dx * dx + dy * dy > kFireRadius * kFireRadius)
					continue;
				const float x = center.x + dx;
				const float y = center.y + dy;
				const float z = GroundZ(x, y, groundZ + 10.0f, groundZ) + 0.1f;
				FIRE::START_SCRIPT_FIRE(x, y, z, 25, 0.0f, true, "", 0.0f, 0);
				++outFires;
			}
		}

		// 5. everything in range catches fire, us and the target included
		for (auto ped : Pools::GetPeds())
		{
			if (center.GetDistance(ped.GetPosition()) <= kFireRadius)
				Ignite(ped.GetHandle(), outIgnited);
		}
		for (auto veh : Pools::GetVehicles())
		{
			if (center.GetDistance(veh.GetPosition()) <= kFireRadius)
				Ignite(veh.GetHandle(), outIgnited);
		}
		for (auto obj : Pools::GetObjects())
		{
			if (center.GetDistance(obj.GetPosition()) <= kFireRadius)
				Ignite(obj.GetHandle(), outIgnited);
		}
		Ignite(targetPed, outIgnited);
		Ignite(selfPed, outIgnited);

		// 6. let the blast pass, then drop the shields so the fire burns
		ScriptMgr::Yield(std::chrono::milliseconds(1500));
		for (auto& [handle, was_damagable] : protectedEntities)
		{
			if (ENTITY::DOES_ENTITY_EXIST(handle))
				ENTITY::SET_ENTITY_CAN_BE_DAMAGED(handle, was_damagable);
		}
	}

	class FireNova : public PlayerCommand
	{
		using PlayerCommand::PlayerCommand;

		virtual void OnCall(Player player) override
		{
			auto targetPed = player.GetPed();
			if (!targetPed || !targetPed.IsValid() || !ENTITY::DOES_ENTITY_EXIST(targetPed.GetHandle()))
			{
				Notifications::Show("Fire Nova", "Player ped not found!", NotificationType::Error);
				return;
			}

			const auto targetPos = targetPed.GetPosition();
			const int selfPed    = PLAYER::PLAYER_PED_ID();

			int fires = 0, ignited = 0;
			FireNovaStrike(targetPos, targetPed.GetHandle(), selfPed, fires, ignited);

			Notifications::Show("Fire Nova",
			    std::string("Meteor hit! ") + std::to_string(fires) + " ground fires, " + std::to_string(ignited) + " entities burning.",
			    NotificationType::Success);
		}
	};

	class FireRain : public PlayerCommand
	{
		using PlayerCommand::PlayerCommand;

		virtual void OnCall(Player player) override
		{
			auto targetPed = player.GetPed();
			if (!targetPed || !targetPed.IsValid() || !ENTITY::DOES_ENTITY_EXIST(targetPed.GetHandle()))
			{
				Notifications::Show("Meteor Rain", "Player ped not found!", NotificationType::Error);
				return;
			}

			const auto targetPos = targetPed.GetPosition();
			const int selfPed    = PLAYER::PLAYER_PED_ID();

			static std::mt19937 rng{std::random_device{}()};
			std::uniform_real_distribution<float> angDist(0.0f, 6.2831853f);
			std::uniform_real_distribution<float> radDist(0.0f, kFireRadius);

			int fires = 0, ignited = 0;
			for (int i = 0; i < kRainStrikes; ++i)
			{
				const float ang = angDist(rng);
				const float rad = radDist(rng);
				auto strike     = targetPos;
				strike.x += std::cos(ang) * rad;
				strike.y += std::sin(ang) * rad;
				FireNovaStrike(strike, targetPed.GetHandle(), selfPed, fires, ignited);
				if (i + 1 < kRainStrikes)
					ScriptMgr::Yield(std::chrono::milliseconds(kRainIntervalMs));
			}

			Notifications::Show("Meteor Rain",
			    std::string("Rain over! ") + std::to_string(fires) + " ground fires, " + std::to_string(ignited) + " entities burning.",
			    NotificationType::Success);
		}
	};

	static FireNova _FireNova{"firenova", "Fire Nova", "Drops a burning meteor on the player, then sets everything in 25m on fire"};
	static FireRain _FireRain{"firerain", "Meteor Rain", "Rains 12 meteors over 25m around the player"};
}
