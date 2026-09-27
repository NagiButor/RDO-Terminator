#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/rdr/Natives.hpp"

namespace YimMenu::Features
{
	class AutoCock : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		virtual void OnTick() override
		{
			// forcing the cocked state while scoped kicks the player out of the
			// scope view (and replays the weapon animation), so stay out of the way
			if (Self::GetPed() && !PLAYER::_IS_PLAYER_IN_SCOPE(Self::GetPlayer().GetId()))
				WEAPON::_SET_FORCE_CURRENT_WEAPON_INTO_COCKED_STATE(Self::GetPed().GetHandle(), 0);
		}
	};

	static AutoCock _AutoCock{"autocock", "Auto Cock", "Automatically cock your weapon"};
}