#include "core/commands/BoolCommand.hpp"
#include "core/commands/Commands.hpp"

namespace YimMenu::Features
{
	class GhostMode : public BoolCommand
	{
		using BoolCommand::BoolCommand;

		void OnEnable() override
		{
			if (auto invis = Commands::GetCommand<BoolCommand>("invis"_J))
				invis->SetState(true);
			if (auto radar = Commands::GetCommand<BoolCommand>("offtheradar"_J))
				radar->SetState(true);
		}

		void OnDisable() override
		{
			if (auto invis = Commands::GetCommand<BoolCommand>("invis"_J))
				invis->SetState(false);
			if (auto radar = Commands::GetCommand<BoolCommand>("offtheradar"_J))
				radar->SetState(false);
		}
	};

	static GhostMode _GhostMode{"ghostmode", "Ghost Mode", "Invisibility + off radar in one toggle. Note: your overhead nickname can still show to nearby players."};
}
