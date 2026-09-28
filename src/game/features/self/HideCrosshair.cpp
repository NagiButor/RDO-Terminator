#include "core/commands/LoopedCommand.hpp"
#include "game/rdr/Natives.hpp"

namespace YimMenu::Features
{
	// HUD context that hides the aiming reticle (used by RedM scripts as well)
	constexpr Hash kHideReticleContext = 0xBB47198C;

	class HideCrosshair : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		virtual void OnEnable() override
		{
			HUD::_ENABLE_HUD_CONTEXT(kHideReticleContext);
		}

		virtual void OnTick() override
		{
			// scripts (aiming, dead eye, cutscenes) reset HUD contexts, so keep re-applying
			HUD::_ENABLE_HUD_CONTEXT_THIS_FRAME(kHideReticleContext);
		}

		virtual void OnDisable() override
		{
			HUD::_DISABLE_HUD_CONTEXT(kHideReticleContext);
		}
	};

	static HideCrosshair _HideCrosshair{"hidecrosshair", "Hide Crosshair", "Removes the aiming reticle from the screen"};
}
