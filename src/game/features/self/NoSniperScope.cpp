#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/rdr/Natives.hpp"

namespace YimMenu::Features
{
	// Strips scopes from the rifle object in hands only. Touching the ped's
	// weapon inventory (_REMOVE_WEAPON_COMPONENT_FROM_PED / giving
	// COMPONENT_SCOPE_NONE) makes the inventory script see a mismatch with the
	// saved customization, holster the rifle and redraw it with the scope back.
	class NoSniperScope : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		static constexpr joaat_t kScopes[] = {
		    "COMPONENT_RIFLE_SCOPE02"_J,
		    "COMPONENT_RIFLE_SCOPE03"_J,
		    "COMPONENT_RIFLE_SCOPE04"_J,
		};

		int m_Tick = 0;

		virtual void OnTick() override
		{
			if (++m_Tick < 10)
				return;
			m_Tick = 0;

			if (PLAYER::_IS_PLAYER_IN_SCOPE(Self::GetPlayer().GetId()))
				return;

			const int ped = Self::GetPed().GetHandle();
			const Hash weapon = WEAPON::_GET_PED_CURRENT_HELD_WEAPON(ped);
			if (weapon != "WEAPON_SNIPERRIFLE_CARCANO"_J && weapon != "WEAPON_SNIPERRIFLE_ROLLINGBLOCK"_J)
				return;

			// the object is rebuilt on every draw, so this re-applies after holstering
			auto weaponObj = WEAPON::_GET_PED_WEAPON_OBJECT(ped, true);
			if (!weaponObj || !ENTITY::DOES_ENTITY_EXIST(weaponObj))
				return;

			for (joaat_t scope : kScopes)
				if (WEAPON::HAS_WEAPON_GOT_WEAPON_COMPONENT(weaponObj, scope))
					WEAPON::REMOVE_WEAPON_COMPONENT_FROM_WEAPON_OBJECT(weaponObj, scope);
		}
	};

	static NoSniperScope _NoSniperScope{"nosniperscope", "No Sniper Scope", "Keeps the Carcano / Rolling Block on iron sights (scope removed from the rifle in hands only)"};
}
