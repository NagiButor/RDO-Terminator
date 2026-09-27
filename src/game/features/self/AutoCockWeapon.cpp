#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Self.hpp"
#include "game/rdr/Natives.hpp"
#include "util/Joaat.hpp"

#include <chrono>

namespace YimMenu::Features
{
	class AutoCock : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		Hash m_LastWeapon = 0;
		int m_LastClip    = -1;
		std::chrono::steady_clock::time_point m_CooldownUntil{};

		virtual void OnTick() override
		{
			auto ped = Self::GetPed();
			if (!ped)
				return;

			// forcing the cocked state while scoped kicks the player out of the
			// scope view (and replays weapon animations), so stay out of the way
			if (PLAYER::_IS_PLAYER_IN_SCOPE(Self::GetPlayer().GetId()))
			{
				m_LastClip = -1;
				return;
			}

			const Hash weapon = WEAPON::_GET_PED_CURRENT_HELD_WEAPON(ped.GetHandle());
			if (weapon == 0 || weapon == "WEAPON_UNARMED"_J)
			{
				m_LastWeapon = 0;
				m_LastClip   = -1;
				return;
			}

			int clip = 0;
			if (!WEAPON::GET_AMMO_IN_CLIP(ped.GetHandle(), &clip, weapon))
			{
				m_LastClip = -1;
				return;
			}

			const auto now = std::chrono::steady_clock::now();
			// cock exactly once per fired shot, then leave the weapon alone:
			// hammering the state every tick is what broke scopes and replayed anims
			if (m_LastWeapon == weapon && m_LastClip >= 0 && clip < m_LastClip && now >= m_CooldownUntil)
			{
				WEAPON::_SET_FORCE_CURRENT_WEAPON_INTO_COCKED_STATE(ped.GetHandle(), 0);
				m_CooldownUntil = now + std::chrono::seconds(2);
			}
			m_LastWeapon = weapon;
			m_LastClip   = clip;
		}
	};

	static AutoCock _AutoCock{"autocock", "Auto Cock", "Automatically cock your weapon"};
}
