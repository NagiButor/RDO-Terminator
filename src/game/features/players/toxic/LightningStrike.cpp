#include "LightningStrike.hpp"

#include "game/commands/PlayerCommand.hpp"
#include "game/backend/Players.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/features/Features.hpp"
#include "game/pointers/Pointers.hpp"
#include "game/rdr/Enums.hpp"
#include "game/rdr/Natives.hpp"

#include <cmath>
#include <random>


namespace YimMenu::Features::LightningOptions
{
	static float s_distance = 10.f;

	float& StrikeDistance()
	{
		return s_distance;
	}

	static void StrikeAt(const rage::fvector3& pos)
	{
		static std::mt19937 rng{std::random_device{}()};
		std::uniform_real_distribution<float> angle_dist(0.f, 6.2831853f);

		const float angle = angle_dist(rng);
		const float dist  = std::max(0.f, s_distance);
		const float x     = pos.x + std::cos(angle) * dist;
		const float y     = pos.y + std::sin(angle) * dist;

		float z = pos.z + 1.f;
		float ground_z = z;
		if (MISC::GET_GROUND_Z_FOR_3D_COORD(x, y, pos.z + 5.f, &ground_z, false))
			z = ground_z + 1.f;

		// real bolt: flash + thunder at the coords (p3 = -1 like the game scripts use)...
		MISC::_FORCE_LIGHTNING_FLASH_AT_COORDS(x, y, z, -1.0f);
		// ...plus the lightning explosion tag for damage and sound
		FIRE::ADD_EXPLOSION(x, y, z, 33, 5.0f, true, false, 5.0f);
	}

	void StrikeNow()
	{
		Player target = Players::GetSelected();
		if (!target.IsValid())
		{
			Notifications::Show("Lightning", "Invalid target player.", NotificationType::Error);
			return;
		}

		Ped target_ped = target.GetPed();
		if (!target_ped.IsValid())
		{
			Notifications::Show("Lightning", "Invalid target player.", NotificationType::Error);
			return;
		}

		StrikeAt(target_ped.GetPosition());
		Notifications::Show("Lightning", std::string("Struck ") + target.GetName(), NotificationType::Success);
	}
}

namespace YimMenu::Features
{
	class LightningStrike : public PlayerCommand
	{
		using PlayerCommand::PlayerCommand;

		virtual void OnCall(Player player) override
		{
			if (!player.IsValid())
				return;

			Ped target_ped = player.GetPed();
			if (!target_ped.IsValid())
				return;

			LightningOptions::StrikeAt(target_ped.GetPosition());
		}
	};

	static LightningStrike _LightningStrike{"lightning", "Lightning Strike", "Strikes the player with a real lightning bolt"};
}
