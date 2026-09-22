#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/rdr/Natives.hpp"

namespace YimMenu::Features
{
	class Invis : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		virtual void OnTick() override
		{
			if (Self::GetPed())
			{
				Self::GetPed().SetVisible(false);
				// false = fully invisible, even to yourself
				// (true would force-override and keep you visible locally)
				NETWORK::SET_PLAYER_VISIBLE_LOCALLY(Self::GetPlayer().GetId(), false);

				// hide what you're riding too, otherwise the horse gives you away
				if (auto mount = Self::GetPed().GetMount(); mount.IsValid())
					mount.SetVisible(false);
				if (auto vehicle = Self::GetPed().GetVehicle(); vehicle.IsValid())
					vehicle.SetVisible(false);
			}
		}

		virtual void OnDisable() override
		{
			if (Self::GetPed())
			{
				Self::GetPed().SetVisible(true);
				NETWORK::SET_PLAYER_VISIBLE_LOCALLY(Self::GetPlayer().GetId(), false);

				if (auto mount = Self::GetPed().GetMount(); mount.IsValid())
					mount.SetVisible(true);
				if (auto vehicle = Self::GetPed().GetVehicle(); vehicle.IsValid())
					vehicle.SetVisible(true);
			}
		}
	};

	static Invis _Invis{"invis", "Invisibility", "Be invisible"};
}