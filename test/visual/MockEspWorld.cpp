#include "MockEspWorld.hpp"

#include <cmath>

namespace YimMenu
{
	namespace
	{
		constexpr float kPi = 3.14159265358979323846f;

		float DegToRad(float deg)
		{
			return deg * (kPi / 180.f);
		}

		// Same bone IDs as ESP::DrawSkeleton.
		rage::fvector3 BoneOffset(int boneId, float swing)
		{
			switch (boneId)
			{
			case 21030: return {0.f, 0.f, 1.75f}; // head
			case 14283: return {0.f, 0.f, 1.50f}; // neck
			case 14410: return {0.f, 0.f, 1.15f}; // torso
			case 44903: return {-0.23f, 0.f, 1.48f}; // left shoulder
			case 4312: return {0.23f, 0.f, 1.48f}; // right shoulder
			case 22711: return {-0.30f, 0.f, 1.18f}; // left elbow
			case 2992: return {0.30f, 0.f, 1.18f}; // right elbow
			case 34606: return {-0.32f, swing, 0.92f}; // left hand
			case 22798: return {0.32f, -swing, 0.92f}; // right hand
			case 22173: return {-0.13f, -swing * 0.5f, 0.55f}; // left knee
			case 63133: return {0.13f, swing * 0.5f, 0.55f}; // right knee
			case 45454: return {-0.14f, swing, 0.08f}; // left foot
			case 33646: return {0.14f, -swing, 0.08f}; // right foot
			default: return {0.f, 0.f, 1.15f};
			}
		}
	}

	float MockActor::GetDistance() const
	{
		const float dx = m_Pos.x;
		const float dy = m_Pos.y;
		const float dz = m_Pos.z;
		return std::sqrt(dx * dx + dy * dy + dz * dz);
	}

	rage::fvector3 MockActor::GetBone(int boneId) const
	{
		const float swing = m_Walking ? std::sin(m_WalkPhase) * 0.25f : 0.f;
		const rage::fvector3 off = BoneOffset(boneId, swing);
		return rage::fvector3{m_Pos.x + off.x, m_Pos.y + off.y, m_Pos.z + off.z};
	}

	void MockActor::Respawn()
	{
		m_Pos       = {0.f, 25.f, 0.f};
		m_Health    = m_MaxHealth;
		m_Dead      = false;
		m_WalkPhase = 0.f;
	}

	void MockActor::Damage(int amount)
	{
		m_Health -= amount;
		if (m_Health <= 0)
		{
			m_Health = 0;
			m_Dead   = true;
		}
	}

	void MockActor::Heal(int amount)
	{
		m_Health += amount;
		if (m_Health > m_MaxHealth)
			m_Health = m_MaxHealth;
		if (m_Health > 0)
			m_Dead = false;
	}

	void MockActor::Kill()
	{
		m_Health = 0;
		m_Dead   = true;
	}

	void MockActor::Revive()
	{
		m_Dead = false;
		if (m_Health <= 0)
			m_Health = m_MaxHealth;
	}

	void MockEspWorld::ForEachPlayer(const std::function<void(IEspActor&)>& callback)
	{
		if (m_Actor.m_ShowAsPlayer)
			callback(m_Actor);
	}

	void MockEspWorld::ForEachPed(const std::function<void(IEspActor&)>& callback)
	{
		if (!m_Actor.m_ShowAsPlayer)
			callback(m_Actor);
	}

	ImVec2 MockEspWorld::Project(const rage::fvector3& world) const
	{
		float yaw = m_Camera.m_YawDeg;
		float pitch = m_Camera.m_PitchDeg;
		if (m_Camera.m_Track)
		{
			const rage::fvector3 torso{m_Actor.m_Pos.x, m_Actor.m_Pos.y, m_Actor.m_Pos.z + 1.15f};
			const float dx = torso.x - m_Camera.m_Pos.x;
			const float dy = torso.y - m_Camera.m_Pos.y;
			const float dz = torso.z - m_Camera.m_Pos.z;
			const float len = std::sqrt(dx * dx + dy * dy + dz * dz);
			if (len > 0.001f)
			{
				yaw   = std::atan2(-dx, dy) * 180.f / kPi;
				pitch = std::asin(dz / len) * 180.f / kPi;
			}
		}

		const float yawR   = DegToRad(yaw);
		const float pitchR = DegToRad(pitch);
		const float cp     = std::cos(pitchR);

		const float fwd_x = -std::sin(yawR) * cp;
		const float fwd_y = std::cos(yawR) * cp;
		const float fwd_z = std::sin(pitchR);
		const float right_x = std::cos(yawR);
		const float right_y = std::sin(yawR);
		const float up_x = right_y * fwd_z;
		const float up_y = -right_x * fwd_z;
		const float up_z = right_x * fwd_y - right_y * fwd_x;

		const float rx = world.x - m_Camera.m_Pos.x;
		const float ry = world.y - m_Camera.m_Pos.y;
		const float rz = world.z - m_Camera.m_Pos.z;

		const float cx = rx * right_x + ry * right_y;
		const float cy = rx * up_x + ry * up_y + rz * up_z;
		const float cz = rx * fwd_x + ry * fwd_y + rz * fwd_z;

		if (cz <= 0.1f)
			return ImVec2(0, 0);

		const float tan_half = std::tan(DegToRad(m_Camera.m_Fov) * 0.5f);
		const float aspect   = (float)m_Camera.m_Width / (float)m_Camera.m_Height;

		return ImVec2(
		    (0.5f + 0.5f * cx / (cz * tan_half * aspect)) * (float)m_Camera.m_Width,
		    (0.5f - 0.5f * cy / (cz * tan_half)) * (float)m_Camera.m_Height);
	}

	void MockEspWorld::Update(float dt)
	{
		if (m_Actor.m_Walking && !m_Actor.IsDead())
			m_Actor.m_WalkPhase += dt * 6.f;
	}

	ImVec2 MockEspWorld::ProjectHead() const
	{
		return Project(m_Actor.GetBone(21030));
	}
}
