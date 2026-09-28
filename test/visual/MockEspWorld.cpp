#include "MockEspWorld.hpp"

#include "core/commands/BoolCommand.hpp"

#include <cmath>
#include <cstdio>

namespace YimMenu
{
	namespace Features
	{
		extern BoolCommand _ESPModelPeds;
		extern BoolCommand _ESPNetworkInfoPeds;
		extern BoolCommand _ESPScriptInfoPeds;
	}

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

	std::string MockActor::GetInfo() const
	{
		std::string info;
		char buf[96];
		if (Features::_ESPModelPeds.GetState())
		{
			snprintf(buf, sizeof(buf), "%s (0x%08X)\n", m_Model.c_str(), m_Hash);
			info += buf;
		}
		if (Features::_ESPNetworkInfoPeds.GetState() && m_NetId)
		{
			snprintf(buf, sizeof(buf), "net %d | Host\n", m_NetId);
			info += buf;
		}
		if (Features::_ESPScriptInfoPeds.GetState() && !m_Script.empty())
			info += m_Script + "\n";
		return info;
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
		rage::fvector3 off = BoneOffset(boneId, swing);
		if (m_Animal) // four-legged stand-in: same bones, squashed to shoulder height
			off = {off.x * 0.8f, off.y, off.z * 0.5f};
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
		for (auto& actor : m_Crowd)
			if (actor.m_ShowAsPlayer)
				callback(actor);
	}

	void MockEspWorld::ForEachPed(const std::function<void(IEspActor&)>& callback)
	{
		if (!m_Actor.m_ShowAsPlayer)
			callback(m_Actor);
		for (auto& actor : m_Crowd)
			if (!actor.m_ShowAsPlayer)
				callback(actor);
	}

	void MockEspWorld::SpawnCrowd()
	{
		struct Spec
		{
			float x, y;
			bool player, walking, animal;
			int hp;
			const char* name;
			const char* model;
			unsigned hash;
			int netId;
			const char* script;
		};
		static const Spec kSpecs[] = {
		    {-4.5f, 18.f, true, true, false, 200, "Arthur_M", "MP_MALE", 0xF5C1611E, 0, ""},
		    {5.5f, 34.f, true, false, false, 70, "sadie.adler", "MP_FEMALE", 0x0D7114C9, 0, ""},
		    {9.f, 16.f, false, true, false, 150, "", "A_M_M_VALTOWNFOLK_01", 0x0F6DA2A1, 412, "net_ambient_peds"},
		    {-10.f, 45.f, false, false, false, 30, "", "S_M_M_VALDEPUTY_01", 0x9D71E3F0, 0, "mission_controller"},
		    {-2.f, 70.f, true, false, false, 200, "far_away_rider", "MP_MALE", 0xF5C1611E, 0, ""},
		    {3.f, 12.f, false, false, true, 90, "", "A_C_WOLF", 0x9BD6E7F5, 0, ""},
		};

		m_Crowd.clear();
		for (const auto& spec : kSpecs)
		{
			MockActor a;
			a.m_Pos          = {spec.x, spec.y, 0.f};
			a.m_ShowAsPlayer = spec.player;
			a.m_Walking      = spec.walking;
			a.m_Animal       = spec.animal;
			a.m_Health       = spec.hp;
			a.m_Name         = spec.name;
			a.m_Model        = spec.model;
			a.m_Hash         = spec.hash;
			a.m_NetId        = spec.netId;
			a.m_Script       = spec.script;
			a.m_WalkPhase    = spec.x; // desync the walk cycles
			m_Crowd.push_back(std::move(a));
		}
	}

	ImVec2 MockEspWorld::Project(const rage::fvector3& world) const
	{
		float view[16], proj[16];
		ComputeMatrices(view, proj);

		const float tx = world.x * view[0] + world.y * view[4] + world.z * view[8] + view[12];
		const float ty = world.x * view[1] + world.y * view[5] + world.z * view[9] + view[13];
		const float tz = world.x * view[2] + world.y * view[6] + world.z * view[10] + view[14];
		const float tw = world.x * view[3] + world.y * view[7] + world.z * view[11] + view[15];

		const float cx = tx * proj[0] + ty * proj[4] + tz * proj[8] + tw * proj[12];
		const float cy = tx * proj[1] + ty * proj[5] + tz * proj[9] + tw * proj[13];
		const float cw = tx * proj[3] + ty * proj[7] + tz * proj[11] + tw * proj[15];

		if (cw <= 0.01f)
			return ImVec2(0, 0); // behind the camera

		const float ndcX = cx / cw;
		const float ndcY = cy / cw;
		return ImVec2((ndcX * 0.5f + 0.5f) * (float)m_Camera.m_Width, (0.5f - ndcY * 0.5f) * (float)m_Camera.m_Height);
	}

	void MockEspWorld::ComputeForward(float& fx, float& fy, float& fz) const
	{
		if (m_Camera.m_Track)
		{
			const float dx = m_Actor.m_Pos.x - m_Camera.m_Pos.x;
			const float dy = m_Actor.m_Pos.y - m_Camera.m_Pos.y;
			const float dz = (m_Actor.m_Pos.z + 1.15f) - m_Camera.m_Pos.z;
			const float len = std::sqrt(dx * dx + dy * dy + dz * dz);
			if (len > 0.001f)
			{
				fx = dx / len;
				fy = dy / len;
				fz = dz / len;
				return;
			}
		}

		const float yawR   = DegToRad(m_Camera.m_YawDeg);
		const float pitchR = DegToRad(m_Camera.m_PitchDeg);
		const float cp     = std::cos(pitchR);
		fx                 = -std::sin(yawR) * cp;
		fy                 = std::cos(yawR) * cp;
		fz                 = std::sin(pitchR);
	}

	void MockEspWorld::ComputeMatrices(float outView[16], float outProj[16]) const
	{
		float fx, fy, fz;
		ComputeForward(fx, fy, fz);

		// left-handed lookAt, row-major (mul(v, M) convention)
		float rx = -fy, ry = fx, rz = 0.f; // cross((0,0,1), f)
		float rl = std::sqrt(rx * rx + ry * ry);
		if (rl < 0.0001f)
		{
			rx = 1.f;
			ry = 0.f;
			rl = 1.f;
		}
		rx /= rl;
		ry /= rl;
		const float ux = fy * rz - fz * ry;
		const float uy = fz * rx - fx * rz;
		const float uz = fx * ry - fy * rx;

		const float ex = m_Camera.m_Pos.x, ey = m_Camera.m_Pos.y, ez = m_Camera.m_Pos.z;
		outView[0] = rx;
		outView[1] = ux;
		outView[2] = fx;
		outView[3] = 0.f;
		outView[4] = ry;
		outView[5] = uy;
		outView[6] = fy;
		outView[7] = 0.f;
		outView[8] = rz;
		outView[9] = uz;
		outView[10] = fz;
		outView[11] = 0.f;
		outView[12] = -(rx * ex + ry * ey + rz * ez);
		outView[13] = -(ux * ex + uy * ey + uz * ez);
		outView[14] = -(fx * ex + fy * ey + fz * ez);
		outView[15] = 1.f;

		const float tanHalf = std::tan(DegToRad(m_Camera.m_Fov) * 0.5f);
		const float aspect  = (float)m_Camera.m_Width / (float)m_Camera.m_Height;
		const float zn = 0.1f, zf = 1000.f;
		const float xs = 1.f / (tanHalf * aspect);
		const float ys = 1.f / tanHalf;
		outProj[0] = xs;
		outProj[1] = 0.f;
		outProj[2] = 0.f;
		outProj[3] = 0.f;
		outProj[4] = 0.f;
		outProj[5] = ys;
		outProj[6] = 0.f;
		outProj[7] = 0.f;
		outProj[8] = 0.f;
		outProj[9] = 0.f;
		outProj[10] = zf / (zf - zn);
		outProj[11] = 1.f;
		outProj[12] = 0.f;
		outProj[13] = 0.f;
		outProj[14] = -zn * zf / (zf - zn);
		outProj[15] = 0.f;
	}

	void MockEspWorld::Update(float dt)
	{
		if (m_Actor.m_Walking && !m_Actor.IsDead())
			m_Actor.m_WalkPhase += dt * 6.f;
		for (auto& actor : m_Crowd)
			if (actor.m_Walking && !actor.IsDead())
				actor.m_WalkPhase += dt * 6.f;
	}

	ImVec2 MockEspWorld::ProjectHead() const
	{
		return Project(m_Actor.GetBone(21030));
	}
}
