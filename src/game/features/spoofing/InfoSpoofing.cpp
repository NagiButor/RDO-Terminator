// InfoSpoofing: the consumer behind Network -> Spoofing -> Info Spoofing.
// The menu only fills g_SpoofingStorage; this looped applier writes the values
// into our local gamer info (which is what other clients see on join/sync).
// Done "cleanly" where possible: RID is patched in BOTH gamer handles so the
// standard handle-mismatch check doesn't trip; plain names without '~' don't
// trip the name check either. Color/icon prefixes and IP spoofing WILL still
// look spoofed to strict menus - that tradeoff is on the user.
#include "core/commands/LoopedCommand.hpp"
#include "game/backend/Self.hpp"
#include "util/Storage/Spoofing.hpp"

#include <network/rlGamerInfo.hpp>

#include <cstring>

namespace YimMenu::Features
{
	class InfoSpoof : public LoopedCommand
	{
		using LoopedCommand::LoopedCommand;

		char m_OrigName[21]{};
		int64_t m_OrigRID  = 0;
		int64_t m_OrigRID2 = 0;
		uint32_t m_OrigExt   = 0;
		uint32_t m_OrigRelay = 0;
		bool m_Cached        = false;

		void CacheOriginals(rage::rlGamerInfo* info)
		{
			memcpy(m_OrigName, info->m_Name, sizeof(m_OrigName));
			m_OrigRID   = info->m_GamerHandle.m_RockstarId;
			m_OrigRID2  = info->m_GamerHandle2.m_RockstarId;
			m_OrigExt   = info->m_ExternalAddress.m_packed;
			m_OrigRelay = info->m_RelayAddress.m_packed;
			m_Cached    = true;
		}

		virtual void OnTick() override
		{
			const bool wantName = g_SpoofingStorage.spoofName;
			const bool wantIP   = g_SpoofingStorage.spoofIP;
			const bool wantRID  = g_SpoofingStorage.spoofRID;
			if (!wantName && !wantIP && !wantRID)
				return;

			auto self = Self::GetPlayer();
			if (!self.IsValid())
				return;
			auto info = self.GetGamerInfo();
			if (!info)
				return;
			if (!m_Cached)
				CacheOriginals(info);

			if (wantName)
			{
				strncpy(info->m_Name, g_SpoofingStorage.spoofedName.c_str(), 20);
				info->m_Name[20] = '\0';
			}
			else
			{
				memcpy(info->m_Name, m_OrigName, sizeof(m_OrigName));
			}

			if (wantRID)
			{
				// patch both handles identically: a mismatch is exactly what
				// other menus flag as SPOOFING_ROCKSTAR_ID
				info->m_GamerHandle.m_RockstarId  = g_SpoofingStorage.spoofedRID;
				info->m_GamerHandle2.m_RockstarId = g_SpoofingStorage.spoofedRID;
			}
			else
			{
				info->m_GamerHandle.m_RockstarId  = m_OrigRID;
				info->m_GamerHandle2.m_RockstarId = m_OrigRID2;
			}

			if (wantIP)
			{
				const auto& ip = g_SpoofingStorage.spoofedIP;
				const uint32_t packed =
				    (uint32_t)(ip[0] & 0xFF) << 24 | (uint32_t)(ip[1] & 0xFF) << 16 | (uint32_t)(ip[2] & 0xFF) << 8 | (uint32_t)(ip[3] & 0xFF);
				info->m_ExternalAddress.m_packed = packed;
				info->m_RelayAddress.m_packed    = packed;
			}
			else
			{
				info->m_ExternalAddress.m_packed = m_OrigExt;
				info->m_RelayAddress.m_packed    = m_OrigRelay;
			}
		}

		virtual void OnDisable() override
		{
			m_Cached = false; // re-cache fresh originals next time (new session, etc.)
		}

		virtual void LoadState(nlohmann::json&) override
		{
			SetState(true); // always running: the storage checkboxes are the real switches
		}

	public:
		InfoSpoof() :
		    LoopedCommand("infospoof", "Info Spoof Applier", "Applies Network info spoofing")
		{
			SetState(true);
		}
	};

	static InfoSpoof _InfoSpoof{};
}
