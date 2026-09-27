#pragma once

#include "game/frontend/esp/EspWorld.hpp"

#include <string>

namespace YimMenu
{
	struct MockCamera
	{
		rage::fvector3 m_Pos{0.f, -7.f, 3.f};
		float m_YawDeg   = 0.f; // manual orbit when tracking is off
		float m_PitchDeg = -12.f;
		float m_Fov      = 60.f;
		bool m_Track     = true; // aim at the actor's torso every frame
		int m_Width      = 1280;
		int m_Height     = 800;
	};

	// One scripted humanoid standing on the ground plane. Bone layout mirrors
	// the IDs used by ESP::DrawSkeleton so the overlay under test is identical.
	class MockActor : public IEspActor
	{
	public:
		rage::fvector3 m_Pos{0.f, 25.f, 0.f};
		int m_Health    = 200;
		int m_MaxHealth = 200;
		bool m_Dead     = false;
		bool m_Selected = false;
		bool m_Occluded = false; // simulates "behind a wall" for the visibility check
		bool m_ShowAsPlayer = true;
		bool m_Walking      = false;
		float m_WalkPhase   = 0.f;
		std::string m_Name  = "Test Subject";

		bool IsValid() const override
		{
			return true;
		}
		bool IsDead() const override
		{
			return m_Dead || m_Health <= 0;
		}
		bool IsAnimal() const override
		{
			return false;
		}
		bool IsHorse() const override
		{
			return false;
		}
		bool IsSelf() const override
		{
			return false;
		}
		bool IsSelected() const override
		{
			return m_Selected;
		}
		std::string GetName() const override
		{
			return m_Name;
		}
		std::string GetInfo() const override
		{
			return "MOCK_HUMAN (0x1A2B3C4D) mock_script ";
		}
		float GetDistance() const override;
		int GetHealth() const override
		{
			return m_Health;
		}
		int GetMaxHealth() const override
		{
			return m_MaxHealth;
		}
		bool IsVisible() const override
		{
			return !m_Occluded;
		}
		rage::fvector3 GetBone(int boneId) const override;

		void Respawn();
		void Damage(int amount);
		void Heal(int amount);
		void Kill();
		void Revive();
	};

	class MockEspWorld : public IEspWorld
	{
	public:
		MockActor m_Actor;
		rage::fvector3 m_Self{0.f, 0.f, 0.f};
		MockCamera m_Camera;

		bool IsAvailable() const override
		{
			return true;
		}
		void ForEachPlayer(const std::function<void(IEspActor&)>& callback) override;
		void ForEachPed(const std::function<void(IEspActor&)>& callback) override;
		ImVec2 Project(const rage::fvector3& world) const override;
		ImVec2 GetScreenSize() const override
		{
			return ImVec2((float)m_Camera.m_Width, (float)m_Camera.m_Height);
		}

		// Row-major view/projection shared by Project() and the test mesh pass,
		// so the rendered dummy lines up exactly with the ESP overlay.
		void ComputeForward(float& fx, float& fy, float& fz) const;
		void ComputeMatrices(float outView[16], float outProj[16]) const;

		void Update(float dt);
		ImVec2 ProjectHead() const;
	};
}
