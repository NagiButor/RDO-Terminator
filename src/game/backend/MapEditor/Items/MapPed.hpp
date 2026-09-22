#pragma once
#include "MapEntity.hpp"
#include "util/Joaat.hpp"

namespace YimMenu
{
	class MapPed final : public MapEntity
	{
	protected:
		virtual bool CreateGameObjectImpl() override;
		virtual void CreateBlip() override;

	public:
		int m_Variation; // 0-indexed
		std::uint32_t m_Scenario;
		std::uint32_t m_Expression;
		float m_Scale;
		int m_Behavior = 0; // 0 = normal, 1 = friendly, 2 = aggressive
		std::uint32_t m_WeaponHash = 0; // 0 = unarmed
		int m_Health = 0; // 0 = game default
		std::uint64_t m_TargetRID = 0; // 0 = nearest player
		std::chrono::system_clock::time_point m_LastBehaviorTick{};

		MapPed() :
			MapEntity()
		{
			m_Variation = 0;
			m_Scenario = 0;
			m_Type = Type::PED;
			m_ModelHash = "player_zero"_J;
			m_Expression = "MoodNone"_J;
			m_Scale = 1.0; // technically all peds have a random (but minute) scale factor applied to them, but normalizing it to 1 isn't going to make a visible difference
		}

		MapPed(const MapPed& other) :
		    MapEntity(other),
		    m_Variation(other.m_Variation),
		    m_Scenario(other.m_Scenario),
		    m_Expression(other.m_Expression),
		    m_Scale(other.m_Scale),
		    m_Behavior(other.m_Behavior),
		    m_WeaponHash(other.m_WeaponHash),
		    m_Health(other.m_Health),
		    m_TargetRID(other.m_TargetRID)
		{
		}

		virtual void ChangeModel(std::uint32_t model) override;
		virtual std::shared_ptr<MapEntity> Clone() override;
		virtual void Update() override;

		void ChangeVariation(int new_variation);
		void ChangeScenario(std::uint32_t new_scenario);
		void ChangeScale(float scale);
		void ApplyWeapon();    // (re)equips m_WeaponHash on the live ped
		void UpdateBehavior(); // keeps aggressive actors on their target
		int ResolveHuntTarget();

		virtual void Serialize(nlohmann::json& object) override;
		virtual void Deserialize(nlohmann::json& object) override;
	};
}