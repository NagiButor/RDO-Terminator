#include "game/commands/PlayerCommand.hpp"
#include "game/backend/ScriptMgr.hpp"
#include "game/rdr/Natives.hpp"
#include "game/rdr/Pools.hpp"
#include "game/rdr/Object.hpp"
#include "core/frontend/Notifications.hpp"
#include "util/Joaat.hpp"

#include <chrono>
#include <cmath>
#include <utility>
#include <vector>

namespace YimMenu::Features
{
	class FireNova : public PlayerCommand
	{
		using PlayerCommand::PlayerCommand;

		static constexpr float kFireRadius    = 25.0f;
		static constexpr float kProtectRadius = 30.0f;

		static float GroundZ(float x, float y, float ref_z, float fallback_z)
		{
			float z = fallback_z;
			if (MISC::GET_GROUND_Z_FOR_3D_COORD(x, y, ref_z, &z, false))
				return z;
			return fallback_z;
		}

		virtual void OnCall(Player player) override
		{
			auto targetPed = player.GetPed();
			if (!targetPed || !targetPed.IsValid() || !ENTITY::DOES_ENTITY_EXIST(targetPed.GetHandle()))
			{
				Notifications::Show("Fire Nova", "Player ped not found!", NotificationType::Error);
				return;
			}

			const auto targetPos = targetPed.GetPosition();
			const float groundZ  = GroundZ(targetPos.x, targetPos.y, targetPos.z + 2.0f, targetPos.z);

			// 1. a burning meteor falls out of the sky right onto the target
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
				meteor = Object::Create(model, {targetPos.x, targetPos.y, targetPos.z + 55.0f});
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

			// 2. protect everyone around the blast so the explosion itself deals no damage
			std::vector<std::pair<int, bool>> protectedEntities;
			auto protect = [&protectedEntities](int handle) {
				if (!ENTITY::DOES_ENTITY_EXIST(handle))
					return;
				for (auto& e : protectedEntities)
					if (e.first == handle)
						return;
				protectedEntities.emplace_back(handle, ENTITY::_GET_ENTITY_CAN_BE_DAMAGED(handle));
				ENTITY::SET_ENTITY_CAN_BE_DAMAGED(handle, false);
			};

			protect(targetPed.GetHandle());
			for (auto ped : Pools::GetPeds())
			{
				if (targetPos.GetDistance(ped.GetPosition()) <= kProtectRadius)
					protect(ped.GetHandle());
			}
			for (auto veh : Pools::GetVehicles())
			{
				if (targetPos.GetDistance(veh.GetPosition()) <= kProtectRadius)
					protect(veh.GetHandle());
			}

			// 3. boom (volatile dynamite tag), then remove the meteor
			FIRE::ADD_EXPLOSION(targetPos.x, targetPos.y, groundZ + 0.5f, 27, 8.0f, true, false, 0.8f);

			if (meteor && ENTITY::DOES_ENTITY_EXIST(meteor.GetHandle()))
				meteor.Delete();

			// 4. everything in 25m catches fire: ground grid + entities
			int fires = 0;
			static constexpr float kRings[] = {0.0f, 8.0f, 16.0f, 24.0f};
			for (float r : kRings)
			{
				const int count = (r < 0.5f) ? 1 : static_cast<int>(r);
				for (int i = 0; i < count; ++i)
				{
					const float ang = 6.2831853f * static_cast<float>(i) / static_cast<float>(count);
					const float x   = targetPos.x + std::cos(ang) * r;
					const float y   = targetPos.y + std::sin(ang) * r;
					const float z   = GroundZ(x, y, groundZ + 10.0f, groundZ) + 0.1f;
					FIRE::START_SCRIPT_FIRE(x, y, z, 25, 0.0f, true, "", 0.0f, 0);
					++fires;
				}
			}

			int ignited = 0;
			auto ignite = [&ignited](int handle) {
				if (!ENTITY::DOES_ENTITY_EXIST(handle))
					return;
				if (FIRE::IS_ENTITY_ON_FIRE(handle))
					return;
				FIRE::START_ENTITY_FIRE(handle, 0, 0, 0);
				++ignited;
			};

			for (auto ped : Pools::GetPeds())
			{
				if (targetPos.GetDistance(ped.GetPosition()) <= kFireRadius)
					ignite(ped.GetHandle());
			}
			for (auto veh : Pools::GetVehicles())
			{
				if (targetPos.GetDistance(veh.GetPosition()) <= kFireRadius)
					ignite(veh.GetHandle());
			}
			for (auto obj : Pools::GetObjects())
			{
				if (targetPos.GetDistance(obj.GetPosition()) <= kFireRadius)
					ignite(obj.GetHandle());
			}

			// 5. let the blast pass, then restore damage flags so the fire burns
			ScriptMgr::Yield(std::chrono::milliseconds(1500));
			for (auto& [handle, was_damagable] : protectedEntities)
			{
				if (ENTITY::DOES_ENTITY_EXIST(handle))
					ENTITY::SET_ENTITY_CAN_BE_DAMAGED(handle, was_damagable);
			}

			Notifications::Show("Fire Nova",
			    std::string("Meteor hit! ") + std::to_string(fires) + " ground fires, " + std::to_string(ignited) + " entities burning.",
			    NotificationType::Success);
		}
	};

	static FireNova _FireNova{"firenova", "Fire Nova", "Drops a burning meteor on the player, then sets everything in 25m on fire"};
}
