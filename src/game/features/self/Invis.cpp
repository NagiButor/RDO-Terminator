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
				// the dedicated invisibility native (covers the ped + its vehicle)
				NETWORK::SET_PLAYER_INVISIBLE_LOCALLY(Self::GetPlayer().GetId(), true);
				Self::GetPed().SetVisible(false);

				// hide the horse too, otherwise it gives you away
				if (auto mount = Self::GetPed().GetMount(); mount.IsValid())
					mount.SetVisible(false);
			}
		}

		virtual void OnDisable() override
		{
			if (Self::GetPed())
			{
				NETWORK::SET_PLAYER_VISIBLE_LOCALLY(Self::GetPlayer().GetId(), true);
				Self::GetPed().SetVisible(true);

				if (auto mount = Self::GetPed().GetMount(); mount.IsValid())
					mount.SetVisible(true);
			}
		}
	};

	static Invis _Invis{"invis", "Invisibility", "Be invisible"};
}