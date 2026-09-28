// Production IEspWorld: maps live RDR2 objects (Players/Pools/Self/Pointers)
// to the shared ESP view. Only this file (plus the mock in the visual
// test) knows about natives, pools and pattern-scanned pointers.
#include "game/frontend/ESP.hpp"
#include "game/frontend/esp/EspWorld.hpp"
#include "core/commands/BoolCommand.hpp"
#include "game/backend/Players.hpp"
#include "game/backend/Self.hpp"
#include "game/features/Features.hpp"
#include "game/pointers/Pointers.hpp"
#include "game/rdr/Natives.hpp"
#include "game/rdr/Pools.hpp"
#include "game/rdr/Scripts.hpp"
#include "game/rdr/data/PedModels.hpp"
#include "game/rdr/invoker/Invoker.hpp"
#include "util/Joaat.hpp"
#include "util/Math.hpp"

#include <format>
#include <string>
#include <chrono>
#include <unordered_map>

namespace YimMenu
{
	namespace Features
	{
		// ESP toggles owned by ESP.cpp; read here to build ped info lines.
		extern BoolCommand _ESPModelPeds;
		extern BoolCommand _ESPNetworkInfoPeds;
		extern BoolCommand _ESPScriptInfoPeds;
		extern BoolCommand _ESPVisibilityCheck;
	}

	// Line-of-sight checks are native calls: throttle per handle so a crowd
	// of actors doesn't stall the frame. Stale entries are dropped in bulk.
	static bool CheckVisibleThrottled(int selfHandle, int targetHandle)
	{
		using Clock = std::chrono::steady_clock;
		struct Entry
		{
			bool visible = true;
			Clock::time_point at{};
		};
		static std::unordered_map<int, Entry> cache;

		if (!Features::_ESPVisibilityCheck.GetState())
			return true;

		const auto now = Clock::now();
		if (auto it = cache.find(targetHandle); it != cache.end() && now - it->second.at < std::chrono::milliseconds(300))
			return it->second.visible;

		const bool visible = ENTITY::HAS_ENTITY_CLEAR_LOS_TO_ENTITY(selfHandle, targetHandle, 17) != 0;
		cache[targetHandle] = {visible, now};
		if (cache.size() > 512)
			cache.clear();
		return visible;
	}
	// Projects with the camera that actually renders the picture (spectator/freecam/script cams).
	// Returns x == 0 when the point can't be projected (no cam / behind it).
	static ImVec2 ProjectFromRenderCam(const rage::fvector3& bone)
	{
		const Cam cam = CAM::GET_RENDERING_CAM();
		if (!cam || !CAM::DOES_CAM_EXIST(cam))
			return ImVec2(0, 0);

		const Vector3 campos = CAM::GET_CAM_COORD(cam);
		const Vector3 camrot = CAM::GET_CAM_ROT(cam, 2);
		const float fov      = CAM::GET_CAM_FOV(cam);
		if (fov <= 1.f)
			return ImVec2(0, 0);

		const float yaw   = Math::DegToRad(camrot.z);
		const float pitch = Math::DegToRad(camrot.x);
		const float cp    = std::cos(pitch);

		// camera basis (same convention as Math::RotationToDirection)
		const float fwd_x = -std::sin(yaw) * cp;
		const float fwd_y = std::cos(yaw) * cp;
		const float fwd_z = std::sin(pitch);
		const float right_x = std::cos(yaw);
		const float right_y = std::sin(yaw);
		const float right_z = 0.f;
		// up = right x fwd
		const float up_x = right_y * fwd_z - right_z * fwd_y;
		const float up_y = right_z * fwd_x - right_x * fwd_z;
		const float up_z = right_x * fwd_y - right_y * fwd_x;

		const float rx = bone.x - campos.x;
		const float ry = bone.y - campos.y;
		const float rz = bone.z - campos.z;

		const float cx = rx * right_x + ry * right_y + rz * right_z;
		const float cy = rx * up_x + ry * up_y + rz * up_z;
		const float cz = rx * fwd_x + ry * fwd_y + rz * fwd_z;

		if (cz <= 0.1f)
			return ImVec2(0, 0); // behind the camera

		const float tan_half = std::tan(Math::DegToRad(fov) * 0.5f);
		const float aspect   = (float)(*Pointers.ScreenResX) / (float)(*Pointers.ScreenResY);

		return ImVec2(
		    (0.5f + 0.5f * cx / (cz * tan_half * aspect)) * (float)(*Pointers.ScreenResX),
		    (0.5f - 0.5f * cy / (cz * tan_half)) * (float)(*Pointers.ScreenResY));
	}

	class RdrPlayerActor : public IEspActor
	{
	public:
		explicit RdrPlayerActor(Player player, bool visible) :
		    m_Player(player),
		    m_Ped(player.GetPed()),
		    m_Visible(visible)
		{
		}

		bool IsValid() const override
		{
			return m_Player.IsValid() && m_Ped.IsValid();
		}
		bool IsDead() const override
		{
			return m_Ped.IsDead();
		}
		bool IsAnimal() const override
		{
			return m_Ped.IsAnimal();
		}
		bool IsHorse() const override
		{
			return false;
		}
		bool IsSelf() const override
		{
			return m_Player == Self::GetPlayer();
		}
		bool IsSelected() const override
		{
			return m_Player == Players::GetSelected();
		}
		std::string GetName() const override
		{
			const char* name = m_Player.GetName();
			return name ? name : "";
		}
		std::string GetInfo() const override
		{
			return {};
		}
		float GetDistance() const override
		{
			return Self::GetPed().GetPosition().GetDistance(m_Ped.GetBonePosition(14410));
		}
		int GetHealth() const override
		{
			return m_Ped.GetHealth();
		}
		int GetMaxHealth() const override
		{
			return ENTITY::GET_ENTITY_MAX_HEALTH(m_Ped.GetHandle(), true);
		}
		bool IsVisible() const override
		{
			return m_Visible;
		}
		rage::fvector3 GetBone(int boneId) const override
		{
			return m_Ped.GetBonePosition(boneId);
		}

	private:
		mutable Player m_Player;
		mutable Ped m_Ped;
		bool m_Visible = true;
	};

	class RdrPedActor : public IEspActor
	{
	public:
		explicit RdrPedActor(Ped ped, bool visible) :
		    m_Ped(ped),
		    m_Visible(visible)
		{
		}

		bool IsValid() const override
		{
			return m_Ped.IsValid();
		}
		bool IsDead() const override
		{
			return m_Ped.IsDead();
		}
		bool IsAnimal() const override
		{
			return m_Ped.IsAnimal();
		}
		bool IsHorse() const override
		{
			return PED::_IS_THIS_MODEL_A_HORSE(m_Ped.GetModel()) != 0;
		}
		bool IsSelf() const override
		{
			return m_Ped.IsPlayer() || m_Ped == Self::GetPlayer().GetPed();
		}
		bool IsSelected() const override
		{
			return false;
		}
		std::string GetName() const override
		{
			return {};
		}
		std::string GetInfo() const override
		{
			std::string info;
			if (Features::_ESPModelPeds.GetState())
			{
				if (auto it = Data::g_PedModels.find(m_Ped.GetModel()); it != Data::g_PedModels.end())
					info += std::format("{} (0x{:08X})\n", it->second, static_cast<joaat_t>(m_Ped.GetModel())); // Show both name and hash
				else
					info += std::format("0x{:08X}\n", static_cast<joaat_t>(m_Ped.GetModel())); // Show only hash if name not found
			}
			if (Features::_ESPNetworkInfoPeds.GetState() && m_Ped.IsNetworked())
			{
				auto owner = Player(m_Ped.GetOwner());
				auto id    = m_Ped.GetNetworkObjectId();
				info += std::format("net {} | {}\n", id, owner.GetName());
			}
			if (Features::_ESPScriptInfoPeds.GetState())
			{
				if (auto script = ENTITY::_GET_ENTITY_SCRIPT(m_Ped.GetHandle(), nullptr))
				{
					if (auto name = Scripts::GetScriptName(script))
					{
						info += std::format("{}\n", name);
					}
				}
			}
			return info;
		}
		float GetDistance() const override
		{
			return Self::GetPed().GetPosition().GetDistance(m_Ped.GetBonePosition(14410));
		}
		int GetHealth() const override
		{
			return m_Ped.GetHealth();
		}
		int GetMaxHealth() const override
		{
			return ENTITY::GET_ENTITY_MAX_HEALTH(m_Ped.GetHandle(), true);
		}
		bool IsVisible() const override
		{
			return m_Visible;
		}
		rage::fvector3 GetBone(int boneId) const override
		{
			return m_Ped.GetBonePosition(boneId);
		}

	private:
		mutable Ped m_Ped;
		bool m_Visible = true;
	};

	class RdrEspWorld : public IEspWorld
	{
	public:
		static RdrEspWorld& Get()
		{
			static RdrEspWorld instance;
			return instance;
		}

		bool IsAvailable() const override
		{
			return NativeInvoker::AreHandlersCached() && !CAM::IS_SCREEN_FADED_OUT();
		}
		void ForEachPlayer(const std::function<void(IEspActor&)>& callback) override
		{
			const int selfHandle = Self::GetPed().GetHandle();
			for (auto& [id, player] : Players::GetPlayers())
			{
				RdrPlayerActor actor(player, CheckVisibleThrottled(selfHandle, player.GetPed().GetHandle()));
				callback(actor);
			}
		}
		void ForEachPed(const std::function<void(IEspActor&)>& callback) override
		{
			if (!GetPedPool())
				return;
			const int selfHandle = Self::GetPed().GetHandle();
			for (Ped ped : Pools::GetPeds())
			{
				if (ped.IsValid() || ped.GetPointer<void*>())
				{
					RdrPedActor actor(ped, CheckVisibleThrottled(selfHandle, ped.GetHandle()));
					callback(actor);
				}
			}
		}
		ImVec2 Project(const rage::fvector3& bone) const override
		{
			// In spectator mode the picture comes from the render camera at the target,
			// but the engine WorldToScreen still projects from our own gameplay camera.
			if (g_Spectating)
				return ProjectFromRenderCam(bone);

			float screen_x, screen_y;
			float boneCoords[3] = { bone.x, bone.y, bone.z };

			Pointers.WorldToScreen(boneCoords, &screen_x, &screen_y);

			return ImVec2(screen_x * (*Pointers.ScreenResX), screen_y * (*Pointers.ScreenResY));
		}
		ImVec2 GetScreenSize() const override
		{
			return ImVec2((float)(*Pointers.ScreenResX), (float)(*Pointers.ScreenResY));
		}
	};

	void ESP::Draw()
	{
		Draw(RdrEspWorld::Get());
	}
}
