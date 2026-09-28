#include "game/commands/PlayerCommand.hpp"
#include "core/frontend/Notifications.hpp"

namespace YimMenu::Features
{
	class KillHorse : public PlayerCommand
	{
		using PlayerCommand::PlayerCommand;

		virtual void OnCall(Player player) override
		{
			auto ped = player.GetPed();
			if (!ped.IsValid())
				return;

			// the horse they are riding, otherwise the last one they rode
			auto horse = ped.GetMount();
			if (!horse.IsValid())
				horse = ped.GetLastMount();

			if (!horse.IsValid() || horse.IsDead())
			{
				Notifications::Show("Kill Horse", "Player has no living horse nearby", NotificationType::Error);
				return;
			}

			horse.Kill();
		}
	};

	static KillHorse _KillHorse{"killhorse", "Kill Horse", "Kills the player's horse (current or last ridden)"};
}
