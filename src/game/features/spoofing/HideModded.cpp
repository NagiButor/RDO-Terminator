// HideModded: one switch that minimizes how modded we look to foreign menus.
// Foreign menus flag us for: godmode (probed via damage/health sync),
// spectating, spoofed blips and spoofed name/RID/IP (handle/address mismatch
// checks). This enables our hiders and turns the detectable spoofs off.
// It deliberately does NOT touch our own detections of others.
#include "core/commands/BoolCommand.hpp"
#include "core/commands/Commands.hpp"
#include "util/Joaat.hpp"
#include "util/Storage/Spoofing.hpp"

namespace YimMenu::Features
{
	class HideModded : public BoolCommand
	{
		using BoolCommand::BoolCommand;

		static void SetHidden(const char* id, bool state)
		{
			if (auto cmd = Commands::GetCommand<BoolCommand>(Joaat(id)))
				cmd->SetState(state);
		}

		virtual void OnEnable() override
		{
			SetHidden("hidegod", true);
			SetHidden("hidespectate", true);
			SetHidden("spoofblip", false);
			SetHidden("spoofdiscriminator", false);
			g_SpoofingStorage.spoofName = false;
			g_SpoofingStorage.spoofIP   = false;
			g_SpoofingStorage.spoofRID  = false;
		}

		virtual void OnDisable() override
		{
			// stays as-is on purpose: re-enabling traces requires a conscious click
		}
	};

	static HideModded _HideModded{"hidemodded", "Hide Modded", "Hides cheat traces from other players: enables Hide God/Spectate, disables blip/name/IP/RID spoofs"};
}
