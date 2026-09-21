#include "game/commands/PlayerCommand.hpp"
#include "game/backend/Players.hpp"
#include "core/frontend/Notifications.hpp"
#include "game/features/Features.hpp"
#include "game/rdr/Natives.hpp"
#include "game/rdr/Ped.hpp"
#include "util/network.hpp"

#include <string>


namespace YimMenu::Features
{
	class BuckOff : public PlayerCommand
	{
		using PlayerCommand::PlayerCommand;

		virtual void OnCall(Player player) override
		{
			if (!player.IsValid())
				return;

			Ped target_ped = player.GetPed();
			if (!target_ped.IsValid())
			{
				Notifications::Show("Buck Off", "Invalid target player.", NotificationType::Error);
				return;
			}

			Ped mount = target_ped.GetMount();
			if (!mount.IsValid() || !ENTITY::DOES_ENTITY_EXIST(mount.GetHandle()))
			{
				Notifications::Show("Buck Off", std::string(player.GetName()) + " is not mounted.", NotificationType::Error);
				return;
			}

			const int horse = mount.GetHandle();

			// someone else's horse is someone else's network object: ask for control first
			if (!Network::RequestControlOfEntity(horse, 50))
			{
				Notifications::Show("Buck Off", "No network control of the horse, try again.", NotificationType::Error);
				return;
			}

			PLAYER::SET_MIN_TIME_BEFORE_HORSE_BUCKING(horse, 0);
			PED::_HORSE_AGITATE(horse, true); // buck the rider off
			TASK::TASK_ANIMAL_FLEE(horse, target_ped.GetHandle(), 0);

			Notifications::Show("Buck Off", std::string(player.GetName()) + " got bucked off.", NotificationType::Success);
		}
	};

	static BuckOff _BuckOff{"buckoff", "Buck Off Horse", "Makes the selected player's horse buck them off and flee"};
}
