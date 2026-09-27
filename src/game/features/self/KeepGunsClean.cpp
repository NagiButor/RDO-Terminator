#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/rdr/Natives.hpp"

namespace YimMenu::Features
{
	class KeepGunsClean : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		int m_Tick = 0;

		virtual void OnTick() override
		{
			// don't poke the weapon object while scoped: same scope-kicking
			// risk as forcing the cocked state, dirt can wait a moment
			if (PLAYER::_IS_PLAYER_IN_SCOPE(Self::GetPlayer().GetId()))
				return;

			// dirt accumulates slowly; checking every tick only risks
			// rebuilding the weapon object under the player's hands
			if (++m_Tick < 60)
				return;
			m_Tick = 0;

			if (WEAPON::IS_WEAPON_A_GUN(WEAPON::_GET_PED_CURRENT_HELD_WEAPON(Self::GetPed().GetHandle())))
			{
				if (auto weaponObj = WEAPON::_GET_PED_WEAPON_OBJECT(Self::GetPed().GetHandle(), true))
				{
					if (WEAPON::_GET_WEAPON_DIRT(weaponObj) > 0.01f || WEAPON::_GET_WEAPON_SOOT(weaponObj) > 0.01f)
					{
						WEAPON::_SET_WEAPON_DEGRADATION(weaponObj, 0.0f);
						WEAPON::_SET_WEAPON_DIRT(weaponObj, 0.0f, true);
						WEAPON::_SET_WEAPON_SOOT(weaponObj, 0.0f, true);
					}
				}
			}
		}
	};

	static KeepGunsClean _KeepGunsClean{"keepgunsclean", "Keep Guns Clean", "Keeps your Guns in Pristine condition"};
}
