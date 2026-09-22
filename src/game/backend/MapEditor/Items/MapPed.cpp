#include "MapPed.hpp"
#include "game/backend/Players.hpp"
#include "game/backend/Self.hpp"
#include "game/rdr/Ped.hpp"
#include "game/rdr/Natives.hpp"
#include "PedScenarios.hpp"

namespace YimMenu
{
	bool MapPed::CreateGameObjectImpl()
	{
		if (m_GameEntity)
			DestroyGameObject();

		m_GameEntity = Ped::Create(m_ModelHash, m_Position, 0.0);
		if (!m_GameEntity)
			return false;

		m_GameEntity.PreventMigration();
		m_GameEntity.SetRotation(m_Rotation);
		m_GameEntity.SetInvincible(true);
		m_GameEntity.As<Ped>().SetVariation(m_Variation);
		if (m_Health > 0)
			ENTITY::SET_ENTITY_HEALTH(m_GameEntity.GetHandle(), m_Health, 0);
		ChangeScenario(m_Scenario);
		ChangeScale(m_Scale);
		ApplyWeapon();

		if (m_Hidden)
			Hide();
		if (m_Selected)
			Select();
		return true;
	}

	void MapPed::CreateBlip()
	{
		if (m_Blip || !m_GameEntity)
			return;

		m_Blip = MAP::BLIP_ADD_FOR_ENTITY("BLIP_STYLE_CREATOR_DEFAULT"_J, m_GameEntity.GetHandle());
		MAP::BLIP_ADD_MODIFIER(m_Blip, "BLIP_MODIFIER_MP_COLOR_10"_J);
		MAP::SET_BLIP_SPRITE(m_Blip, "BLIP_AMBIENT_PED_SMALL"_J, true);
		MAP::_SET_BLIP_NAME(m_Blip, "Actor"sv.data());
	}
	
	void MapPed::ChangeModel(std::uint32_t model)
	{
		if (m_ModelHash == model)
			return;

		DestroyGameObject();
		m_ModelHash = model;
		m_Variation = 0;
		CreateGameObject(); // run this again to recreate ped
	}

	std::shared_ptr<MapEntity> MapPed::Clone()
	{
		return std::make_shared<MapPed>(*this);
	}

	void MapPed::Update()
	{
		MapEntity::Update();
		if (auto entity = m_GameEntity)
			PED::_REQUEST_PED_FACIAL_MOOD_THIS_FRAME(entity.GetHandle(), m_Expression, 6);
		UpdateBehavior();
	}

	void MapPed::ApplyWeapon()
	{
		if (!m_GameEntity || m_WeaponHash == 0)
			return;

		const int handle = m_GameEntity.GetHandle();
		if (!ENTITY::DOES_ENTITY_EXIST(handle) || !PED::IS_PED_HUMAN(handle))
			return;

		WEAPON::GIVE_WEAPON_TO_PED(handle, m_WeaponHash, 500, true, false, 0, false, 0.5f, 1.0f, 1, false, 0, false);
		WEAPON::SET_CURRENT_PED_WEAPON(handle, m_WeaponHash, true, 0, false, false);
	}

	int MapPed::ResolveHuntTarget()
	{
		if (m_TargetRID != 0)
		{
			Player target = Players::GetByRID(m_TargetRID);
			if (target.IsValid() && target.GetPed().IsValid() && !target.GetPed().IsDead())
				return target.GetPed().GetHandle();
			return 0;
		}

		// nearest player ped, including our own
		const auto from = m_GameEntity.GetPosition();
		int best        = 0;
		float best_d    = 3.402823466e+38F;

		if (Self::GetPed().IsValid() && !Self::GetPed().IsDead())
		{
			best   = Self::GetPed().GetHandle();
			best_d = from.GetDistance(Self::GetPed().GetPosition());
		}
		for (auto& [id, player] : Players::GetPlayers())
		{
			if (!player.IsValid() || !player.GetPed().IsValid() || player.GetPed().IsDead())
				continue;
			const float d = from.GetDistance(player.GetPed().GetPosition());
			if (d < best_d)
			{
				best_d = d;
				best   = player.GetPed().GetHandle();
			}
		}
		return best;
	}

	void MapPed::UpdateBehavior()
	{
		if (m_Behavior == 0 || !m_GameEntity)
			return;

		const int handle = m_GameEntity.GetHandle();
		if (!ENTITY::DOES_ENTITY_EXIST(handle) || !PED::IS_PED_HUMAN(handle))
			return;

		const auto now = std::chrono::system_clock::now();
		if (now - m_LastBehaviorTick < std::chrono::seconds(2))
			return;
		m_LastBehaviorTick = now;

		if (m_Behavior == 2) // aggressive: hunt the target
		{
			if (const int target = ResolveHuntTarget())
				TASK::TASK_COMBAT_PED(handle, target, 0, 16);
		}
	}

	void MapPed::ChangeVariation(int new_variation)
	{
		m_Variation = new_variation;

		if (m_GameEntity)
			m_GameEntity.As<Ped>().SetVariation(m_Variation);

		ChangeScale(m_Scale); // you need to do this or the scale reverts back
	}

	void MapPed::ChangeScenario(std::uint32_t new_scenario)
	{
		m_Scenario = new_scenario;

		if (m_GameEntity)
		{
			auto handle = m_GameEntity.GetHandle();
			TASK::CLEAR_PED_TASKS_IMMEDIATELY(handle, true, true);
			if (m_Scenario == 0)
				return; // "None": no scenario task (hash 0 crashes the task system)
			if (DoesScenarioUseChair(m_Scenario))
			{
				auto dimensions = GetDimensions();
				auto pos_to_use = m_Position;
				pos_to_use.z -= std::abs(dimensions.m_Min.z);
				TASK::TASK_START_SCENARIO_AT_POSITION(handle, m_Scenario, pos_to_use.x, pos_to_use.y, pos_to_use.z, m_Rotation.z, -1, false, true, nullptr, 1.0f, 0); // locks the ped during selection, but
				                                                                                                                                                      // TASK_START_SCENARIO_IN_PLACE_HASH does that for these task types too
			}
			else
			{
				TASK::TASK_START_SCENARIO_IN_PLACE_HASH(handle, m_Scenario, -1, false, 0, -1, false);
			}
		}
	}

	void MapPed::ChangeScale(float scale)
	{
		m_Scale = scale;
		if (m_GameEntity)
			m_GameEntity.As<Ped>().SetScale(m_Scale);
	}

	void MapPed::Serialize(nlohmann::json& object)
	{
		MapEntity::Serialize(object);
		object["variation"] = m_Variation;
		object["scenario"] = m_Scenario;
		object["expression"] = m_Expression;
		object["scale"] = m_Scale;
		object["behavior"] = m_Behavior;
		object["weapon"] = m_WeaponHash;
		object["health"] = m_Health;
		object["target_rid"] = m_TargetRID;
	}

	void MapPed::Deserialize(nlohmann::json& object)
	{
		MapEntity::Deserialize(object);
		m_Variation = object["variation"];
		m_Scenario = object["scenario"];
		if (auto it = object.find("expression"); it != object.end())
			m_Expression = *it;
		if (auto it = object.find("scale"); it != object.end())
			m_Scale = *it;
		if (auto it = object.find("behavior"); it != object.end())
			m_Behavior = *it;
		if (auto it = object.find("weapon"); it != object.end())
			m_WeaponHash = *it;
		if (auto it = object.find("health"); it != object.end())
			m_Health = *it;
		if (auto it = object.find("target_rid"); it != object.end())
			m_TargetRID = *it;
	}
}