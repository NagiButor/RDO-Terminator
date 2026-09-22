#include "MapEditorMenu.hpp"
#include "NativeDrawing.hpp"
#include "../MapEditor.hpp"
#include "../Items/PedCategories.hpp"
#include "../Items/VehicleCategories.hpp"
#include "../Items/ObjectCategories.hpp"
#include "../Items/PedScenarios.hpp"
#include "../Items/PedExpressions.hpp"
#include "../Items/MapPed.hpp"
#include "../Items/MapObject.hpp"
#include "../Items/MapVehicle.hpp"
#include "game/backend/Players.hpp"
#include "game/backend/Self.hpp"
#include "game/rdr/data/PedModels.hpp"
#include "game/rdr/data/VehicleModels.hpp"
#include "game/rdr/data/WeaponTypes.hpp"
#include "game/rdr/Natives.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace YimMenu
{
	struct WeaponDef : ModelDefinition
	{
	};

	static const std::vector<WeaponDef>& GetWeaponList()
	{
		static const std::vector<WeaponDef> list = [] {
			std::vector<WeaponDef> v{{0, "None"}};
			for (const char* w : Data::g_WeaponTypes)
			{
				if (w)
					v.push_back({Joaat(w), w});
			}
			return v;
		}();
		return list;
	}

	static void SpawnRingAtCursor(const std::uint32_t* models, std::size_t model_count, int total, float radius)
	{
		const auto center = MapEditor::GetCursorWorldPos();
		for (int i = 0; i < total; ++i)
		{
			const float a = 6.2831853f * (float)i / (float)total;
			rage::fvector3 p;
			p.x = center.x + std::cos(a) * radius;
			p.y = center.y + std::sin(a) * radius;
			p.z = center.z + 0.5f;
			MapEditor::SpawnPedAt(models[i % model_count], p);
		}
	}
	std::unordered_map<MapEntity::Type, std::vector<NativeMenu::Submenu>> MapEditorMenu::sm_SubmenusByType = {
	    {MapEntity::Type::PED, {
			"Actors"sv,
		}},
		{MapEntity::Type::OBJECT, {
			"Props"sv,
		}},
		{MapEntity::Type::VEHICLE, {
			"Vehicles"sv,
		}},
	};

	// TODO: we're calling this a "mission creator" but this is actually just a glorified map editor
	MapEditorMenu::MapEditorMenu(MapInfo* info) :
	    NativeMenu::NativeMenu("Creator", 0.012499f, 0.07422201f, "Mission Creator"sv),
	    m_Info(info)
	{
	}

	void MapEditorMenu::UpdateView()
	{
		if (BeginSubmenu("Mission Creator"sv))
		{
			SubmenuButton("Mission Details", "Mission Details"sv, "Access Mission details");
			SubmenuButton("Placement", "Placement"sv, "Place items");
			{
				static float cam_speed = 0.02f;
				MapEditor::SetCameraSpeed(cam_speed);
				FloatOption("Camera Speed", cam_speed, 0.005f, 0.2f, 0.005f, "Fly speed of the editor camera");
			}
			if (Button("Clear All Entities", "Delete every placed actor, vehicle and prop"))
			{
				if (NativeUI::ShowAlertMessage("Alert", "Delete all placed entities?", "IB_YES"_J, "IB_NO"_J))
				{
					MapEditor::ClearEntities();
				}
			}
			BeginDisabled(m_Info->m_Name.empty(), "A Mission must have a name");
			if (Button("Save", "Save the mission to disk"))
			{
				MapEditor::SaveMap();
				NativeUI::ShowAlertMessage("Alert", "Your Mission has been saved", "IB_ACCEPT"_J);
			}
			EndDisabled();
			SubmenuButton("Load", "Load"sv, "Load Missions from disk");
			if (Button("Exit"sv))
			{
				if (NativeUI::ShowAlertMessage("Alert", "Are you sure you want to leave Mission Creator? Any unsaved data will be lost", "IB_YES"_J, "IB_NO"_J))
				{
					MapEditor::Toggle();
				}
			}
			EndSubmenu();
		}

		if (BeginSubmenu("Mission Details"sv))
		{
			char mission_name[26]; strncpy(mission_name, m_Info->m_Name.c_str(), sizeof(mission_name));
			char mission_desc[255]; strncpy(mission_desc, m_Info->m_Description.c_str(), sizeof(mission_desc));
			if (TextOption("Name", mission_name, sizeof(mission_name), "Sets the name of the mission"))
			{
				m_Info->m_Name = mission_name;
			}
			if (TextOption("Description", mission_desc, sizeof(mission_desc), "Sets the mission description"))
			{
				m_Info->m_Description = mission_desc;
			}
			EndSubmenu();
		}

		if (BeginSubmenu("Load"sv))
		{
			if (DidJustEnterSubmenu())
			{
				MapEditor::RefreshAvailableMaps();
			}

			std::optional<std::string> map_to_delete = std::nullopt;
			for (const auto& map : MapEditor::GetAvailableMaps())
			{
				if (Button(map, "Load the selected map"))
				{
					bool are_we_loading = false;

					// try saving the old map (this doesn't match GTA's behavior, but our creator doesn't separate the main menu from the editor menu)
					if (!m_Info->m_Name.empty())
					{
						MapEditor::SaveMap();
						are_we_loading = true;
					}
					else
					{
						are_we_loading = NativeUI::ShowAlertMessage("Alert", "Are you sure you want to load this Mission? Any unsaved data will be lost", "IB_YES"_J, "IB_NO"_J);
					}

					if (are_we_loading)
					{
						bool success = MapEditor::LoadMap(map);
						if (!success)
						{
							NativeUI::ShowAlertMessage("Alert", std::format("The Mission \"{}\" is corrupt and cannot be loaded", map), "IB_ACCEPT"_J);
						}
						else
						{
							NativeUI::ShowAlertMessage("Alert", std::format("The Mission \"{}\" has been loaded", map), "IB_ACCEPT"_J);
						}
					}

					NativeUI::FadeScreenIn(2000);
				}
			}

			if (map_to_delete)
				MapEditor::DeleteMap(*map_to_delete);

			EndSubmenu();
		}

		if (BeginSubmenu("Placement"sv))
		{
			SubmenuButton("Actors", "Actors"sv, "Place actors (peds) in the mission");
			SubmenuButton("Vehicles", "Vehicles"sv, "Place vehicles in the mission");
			SubmenuButton("Props", "Props"sv, "Place props in the mission");
			SubmenuButton("Angry of God", "Angry of God"sv, "Lightning, explosions and fire at the cursor");
			SubmenuButton("Quick Spawn", "Quick Spawn"sv, "Instant packs at the cursor");
			SubmenuButton("Weapon Spawn", "Weapon Spawn"sv, "Weapon pickups at the cursor");
			EndSubmenu();
		}

		if (BeginSubmenu("Angry of God"sv))
		{
			if (Button("Lightning at cursor", "Real bolt with damage where the cursor looks"))
			{
				const auto p = MapEditor::GetCursorWorldPos();
				MISC::_FORCE_LIGHTNING_FLASH_AT_COORDS(p.x, p.y, p.z, -1.0f);
				FIRE::ADD_EXPLOSION(p.x, p.y, p.z, 33, 5.0f, true, false, 5.0f);
			}
			if (Button("Explosion at cursor", "Dynamite blast where the cursor looks"))
			{
				const auto p = MapEditor::GetCursorWorldPos();
				FIRE::ADD_EXPLOSION(p.x, p.y, p.z, 25, 5.0f, true, false, 5.0f);
			}
			if (Button("Fire at cursor", "Start a lasting fire where the cursor looks"))
			{
				const auto p = MapEditor::GetCursorWorldPos();
				FIRE::START_SCRIPT_FIRE(p.x, p.y, p.z, 10, 0.0f, true, "", 0.0f, 0);
			}
			EndSubmenu();
		}

		if (BeginSubmenu("Quick Spawn"sv))
		{
			if (Button("Zombies x10", "Ring of cholera zombies at the cursor"))
			{
				static constexpr std::uint32_t kZombies[] = {"A_M_M_ARMCHOLERACORPSE_01"_J, "A_F_M_ARMCHOLERACORPSE_01"_J};
				SpawnRingAtCursor(kZombies, 2, 10, 6.f);
			}
			if (Button("Dutch Gang", "Core gang members ring at the cursor"))
			{
				static constexpr std::uint32_t kGang[] = {"cs_dutch"_J, "player_zero"_J, "player_three"_J, "cs_hoseamatthews"_J, "cs_mrsadler"_J, "cs_micahbell"_J, "cs_billwilliamson"_J, "cs_javierescuella"_J, "cs_sean"_J, "cs_lenny"_J, "cs_charlessmith"_J};
				SpawnRingAtCursor(kGang, 11, 11, 7.f);
			}
			EndSubmenu();
		}

		if (BeginSubmenu("Weapon Spawn"sv))
		{
			static std::uint32_t s_spawn_weapon = "WEAPON_REVOLVER_CATTLEMAN"_J;

			auto w_view = CategoryUtils::GetListView(GetWeaponList(), s_spawn_weapon);
			if (!w_view.m_CurrentEntry)
				w_view = CategoryUtils::GetListView(GetWeaponList(), "WEAPON_REVOLVER_CATTLEMAN"_J);
			if (w_view.m_CurrentEntry)
			{
				auto w_delta = ListOption("Weapon",
				    w_view.m_CurrentEntry->m_FriendlyName,
				    w_view.m_PreviousEntry ? std::optional<std::string_view>(w_view.m_PreviousEntry->m_FriendlyName) : std::nullopt,
				    w_view.m_NextEntry ? std::optional<std::string_view>(w_view.m_NextEntry->m_FriendlyName) : std::nullopt,
				    "Weapon to spawn, use left/right to browse all variants");
				if (w_delta < 0 && w_view.m_PreviousEntry)
					s_spawn_weapon = w_view.m_PreviousEntry->m_Hash;
				else if (w_delta > 0 && w_view.m_NextEntry)
					s_spawn_weapon = w_view.m_NextEntry->m_Hash;
			}

			if (Button("Spawn pickup at cursor", "Weapon pickup where the cursor looks"))
			{
				const auto p = MapEditor::GetCursorWorldPos();

				std::string wname;
				for (const auto& e : GetWeaponList())
				{
					if (e.m_Hash == s_spawn_weapon)
					{
						wname = e.m_FriendlyName;
						break;
					}
				}

				bool ok = false;
				if (!wname.empty() && s_spawn_weapon != 0)
				{
					const Object obj = OBJECT::CREATE_PORTABLE_PICKUP(Joaat("PICKUP_" + wname), p.x, p.y, p.z, true, 0);
					ok               = obj != 0 && ENTITY::DOES_ENTITY_EXIST(obj);
				}

				if (ok)
					NativeUI::ShowAlertMessage("Alert", "Weapon pickup spawned", "IB_ACCEPT"_J);
				else
					NativeUI::ShowAlertMessage("Alert", "No such pickup in game, given to you instead", "IB_ACCEPT"_J);

				if (!ok && s_spawn_weapon != 0)
				{
					const int ped = Self::GetPed().GetHandle();
					if (ENTITY::DOES_ENTITY_EXIST(ped))
					{
						WEAPON::GIVE_WEAPON_TO_PED(ped, s_spawn_weapon, 200, true, false, 0, true, 0.5f, 1.0f, 0x2CD419DC, true, 0.0f, false);
						WEAPON::SET_CURRENT_PED_WEAPON(ped, s_spawn_weapon, true, 0, false, false);
					}
				}
			}
			EndSubmenu();
		}

		if (BeginSubmenu("Actors"sv))
		{
			auto editing_ped = MapEditor::GetEditingEntity(MapEntity::Type::PED);
			if (editing_ped)
			{
				auto as_ped = (MapPed*)(&*editing_ped); // we do this to avoid a dynamic cast (which requires RTTI)
				auto sel_model_hash = as_ped->m_ModelHash;
				bool model_updated = false;

				auto cat_view = CategoryUtils::GetCategoryView(g_PedCategories, sel_model_hash);

				if (!cat_view.m_CurrentEntry)
				{
					LOG(WARNING) << "Failed to get category view of ped with hash " << HEX(sel_model_hash);
					sel_model_hash = "player_zero"_J;
					cat_view = CategoryUtils::GetCategoryView(g_PedCategories, sel_model_hash);
				}

				auto delta = ListOption("Category",
				    cat_view.m_CurrentEntry->first,
				    cat_view.m_PreviousEntry ? std::optional(cat_view.m_PreviousEntry->first) : std::nullopt,
				    cat_view.m_NextEntry ? std::optional(cat_view.m_NextEntry->first) : std::nullopt,
				    "Sets the category from which the Actor can be chosen");

				if (delta < 0)
				{
					cat_view.m_CurrentEntry = cat_view.m_PreviousEntry; // surely there's a better way to write this
					sel_model_hash = cat_view.m_CurrentEntry->second[0].m_Hash;
					model_updated = true;
				}
				else if (delta > 0)
				{
					cat_view.m_CurrentEntry = cat_view.m_NextEntry;
					sel_model_hash = cat_view.m_CurrentEntry->second[0].m_Hash;
					model_updated = true;
				}
				
				auto list_view = CategoryUtils::GetListView(cat_view.m_CurrentEntry->second, sel_model_hash);

				auto list_delta = ListOption("Type",
				    list_view.m_CurrentEntry->m_FriendlyName,
				    list_view.m_PreviousEntry ? std::optional(list_view.m_PreviousEntry->m_FriendlyName) : std::nullopt,
				    list_view.m_NextEntry ? std::optional(list_view.m_NextEntry->m_FriendlyName) : std::nullopt,
				    "Sets the Actor's type from the chosen category");
				
				if (list_delta < 0)
				{
					sel_model_hash = list_view.m_PreviousEntry->m_Hash;
					model_updated = true;
				}
				else if (list_delta > 0)
				{
					sel_model_hash = list_view.m_NextEntry->m_Hash;
					model_updated = true;
				}

				if (model_updated)
				{
					as_ped->ChangeModel(sel_model_hash);
				}

				int ui_variation = as_ped->m_Variation + 1;
				if (IntOption("Variation", ui_variation, 1, list_view.m_CurrentEntry->m_NumVariations, 1, "Select the variation"))
				{
					as_ped->ChangeVariation(ui_variation - 1);
				}

			auto scen_view = CategoryUtils::GetListView(g_PedScenarios, as_ped->m_Scenario);
			if (!scen_view.m_CurrentEntry)
			{
				LOG(WARNING) << "Failed to get scenario view with hash " << HEX(as_ped->m_Scenario);
				as_ped->ChangeScenario(0);
				scen_view = CategoryUtils::GetListView(g_PedScenarios, 0);
			}

			if (scen_view.m_CurrentEntry)
			{
			auto scen_delta = ListOption("Scenario",
			    scen_view.m_CurrentEntry->m_FriendlyName,
			    scen_view.m_PreviousEntry ? std::optional(scen_view.m_PreviousEntry->m_FriendlyName) : std::nullopt,
			    scen_view.m_NextEntry ? std::optional(scen_view.m_NextEntry->m_FriendlyName) : std::nullopt,
			    "Choose the ambient scenario for the Actor");

			if (scen_delta < 0)
			{
				as_ped->ChangeScenario(scen_view.m_PreviousEntry->m_Hash);
			}
			else if (scen_delta > 0)
			{
				as_ped->ChangeScenario(scen_view.m_NextEntry->m_Hash);
			}
			}

				auto exp_view = CategoryUtils::GetListView(g_PedExpressions, as_ped->m_Expression);
				if (!exp_view.m_CurrentEntry)
				{
					LOG(WARNING) << "Failed to get facial expression view with hash " << HEX(as_ped->m_Expression);
					exp_view = CategoryUtils::GetListView(g_PedExpressions, "MoodNone"_J);
				}
				auto exp_delta = ListOption("Expression",
				    exp_view.m_CurrentEntry->m_FriendlyName,
				    exp_view.m_PreviousEntry ? std::optional(exp_view.m_PreviousEntry->m_FriendlyName) : std::nullopt,
				    exp_view.m_NextEntry ? std::optional(exp_view.m_NextEntry->m_FriendlyName) : std::nullopt,
				    "Choose the facial expression for the Actor");
				if (exp_delta < 0)
					as_ped->m_Expression = exp_view.m_PreviousEntry->m_Hash;
				else if (exp_delta > 0)
					as_ped->m_Expression = exp_view.m_NextEntry->m_Hash;

			float ui_scale = as_ped->m_Scale;
			if (FloatOption("Scale", ui_scale, 0.50, 1.50, 0.02, "Adjust the scale factor for the Actor"))
			{
				as_ped->ChangeScale(ui_scale);
			}

			static constexpr const char* kBehaviors[] = {"Normal", "Friendly", "Aggressive"};
			{
				const int cur = std::clamp(as_ped->m_Behavior, 0, 2);
				auto delta = ListOption("Behavior",
				    kBehaviors[cur],
				    cur > 0 ? std::optional<std::string_view>(kBehaviors[cur - 1]) : std::nullopt,
				    cur < 2 ? std::optional<std::string_view>(kBehaviors[cur + 1]) : std::nullopt,
				    "Normal = scenario, Friendly = never fights, Aggressive = hunts the target");
				if (delta != 0)
				{
					as_ped->m_Behavior = std::clamp(cur + delta, 0, 2);
					if (as_ped->m_Behavior != 2 && as_ped->m_GameEntity)
						TASK::CLEAR_PED_TASKS_IMMEDIATELY(as_ped->m_GameEntity.GetHandle(), true, true);
				}
			}

			{
				auto w_view = CategoryUtils::GetListView(GetWeaponList(), as_ped->m_WeaponHash);
				if (!w_view.m_CurrentEntry)
					w_view = CategoryUtils::GetListView(GetWeaponList(), 0);
				if (w_view.m_CurrentEntry)
				{
					auto w_delta = ListOption("Weapon",
					    w_view.m_CurrentEntry->m_FriendlyName,
					    w_view.m_PreviousEntry ? std::optional<std::string_view>(w_view.m_PreviousEntry->m_FriendlyName) : std::nullopt,
					    w_view.m_NextEntry ? std::optional<std::string_view>(w_view.m_NextEntry->m_FriendlyName) : std::nullopt,
					    "Gun put into the Actor's hands");
					if (w_delta < 0 && w_view.m_PreviousEntry)
					{
						as_ped->m_WeaponHash = w_view.m_PreviousEntry->m_Hash;
						as_ped->ApplyWeapon();
					}
					else if (w_delta > 0 && w_view.m_NextEntry)
					{
						as_ped->m_WeaponHash = w_view.m_NextEntry->m_Hash;
						as_ped->ApplyWeapon();
					}
				}
			}

			int ui_hp = as_ped->m_Health;
			if (IntOption("HP", ui_hp, 0, 5000, 100, "Health of the Actor, 0 = game default"))
			{
				as_ped->m_Health = ui_hp;
				if (as_ped->m_GameEntity && ui_hp > 0)
					ENTITY::SET_ENTITY_HEALTH(as_ped->m_GameEntity.GetHandle(), ui_hp, 0);
			}

			if (as_ped->m_Behavior == 2)
			{
				std::vector<std::pair<std::uint64_t, std::string>> targets{{0, "Nearest"}};
				for (auto& [id, player] : Players::GetPlayers())
				{
					if (player.IsValid())
						targets.emplace_back(player.GetRID(), player.GetName());
				}
				std::size_t cur = 0;
				for (std::size_t i = 0; i < targets.size(); ++i)
				{
					if (targets[i].first == as_ped->m_TargetRID)
					{
						cur = i;
						break;
					}
				}
				auto t_delta = ListOption("Target",
				    targets[cur].second,
				    cur > 0 ? std::optional<std::string_view>(targets[cur - 1].second) : std::nullopt,
				    cur + 1 < targets.size() ? std::optional<std::string_view>(targets[cur + 1].second) : std::nullopt,
				    "Player the aggressive Actor hunts");
				if (t_delta < 0 && cur > 0)
					as_ped->m_TargetRID = targets[cur - 1].first;
				else if (t_delta > 0 && cur + 1 < targets.size())
					as_ped->m_TargetRID = targets[cur + 1].first;
			}
		}
		EndSubmenu();
		}

		if (BeginSubmenu("Props"sv))
		{
			auto editing_obj = MapEditor::GetEditingEntity(MapEntity::Type::OBJECT);
			if (editing_obj)
			{
				auto sel_model_hash = editing_obj->m_ModelHash;
				bool model_updated = false;

				auto cat_view = CategoryUtils::GetCategoryView(g_ObjectCategories, sel_model_hash);

				if (!cat_view.m_CurrentEntry)
				{
					LOG(WARNING) << "Failed to get category view of object with hash " << HEX(sel_model_hash);
					sel_model_hash = "p_chickenfence01x"_J;
					cat_view = CategoryUtils::GetCategoryView(g_ObjectCategories, sel_model_hash);
				}

				auto delta = ListOption("Category",
				    cat_view.m_CurrentEntry->first,
				    cat_view.m_PreviousEntry ? std::optional(cat_view.m_PreviousEntry->first) : std::nullopt,
				    cat_view.m_NextEntry ? std::optional(cat_view.m_NextEntry->first) : std::nullopt,
				    "Sets the category from which the Prop can be chosen");

				if (delta < 0)
				{
					cat_view.m_CurrentEntry = cat_view.m_PreviousEntry; // surely there's a better way to write this
					sel_model_hash = cat_view.m_CurrentEntry->second[0].m_Hash;
					model_updated = true;
				}
				else if (delta > 0)
				{
					cat_view.m_CurrentEntry = cat_view.m_NextEntry;
					sel_model_hash = cat_view.m_CurrentEntry->second[0].m_Hash;
					model_updated = true;
				}
				
				auto list_view = CategoryUtils::GetListView(cat_view.m_CurrentEntry->second, sel_model_hash);

				auto list_delta = ListOption("Type",
				    list_view.m_CurrentEntry->m_FriendlyName,
				    list_view.m_PreviousEntry ? std::optional(list_view.m_PreviousEntry->m_FriendlyName) : std::nullopt,
				    list_view.m_NextEntry ? std::optional(list_view.m_NextEntry->m_FriendlyName) : std::nullopt,
				    "Sets the Prop's type from the chosen category");
				
				if (list_delta < 0)
				{
					sel_model_hash = list_view.m_PreviousEntry->m_Hash;
					model_updated = true;
				}
				else if (list_delta > 0)
				{
					sel_model_hash = list_view.m_NextEntry->m_Hash;
					model_updated = true;
				}

				if (model_updated)
				{
					editing_obj->ChangeModel(sel_model_hash);
				}
			}
			EndSubmenu();
		}

		if (BeginSubmenu("Vehicles"sv))
		{
			auto editing_veh = MapEditor::GetEditingEntity(MapEntity::Type::VEHICLE);
			if (editing_veh)
			{
				auto sel_model_hash = editing_veh->m_ModelHash;
				bool model_updated = false;

				auto cat_view = CategoryUtils::GetCategoryView(g_VehicleCategories, sel_model_hash);

				if (!cat_view.m_CurrentEntry)
				{
					LOG(WARNING) << "Failed to get category view of vehicle with hash " << HEX(sel_model_hash);
					sel_model_hash = "wagoncircus01x"_J;
					cat_view = CategoryUtils::GetCategoryView(g_VehicleCategories, sel_model_hash);
				}

				auto delta = ListOption("Category",
				    cat_view.m_CurrentEntry->first,
				    cat_view.m_PreviousEntry ? std::optional(cat_view.m_PreviousEntry->first) : std::nullopt,
				    cat_view.m_NextEntry ? std::optional(cat_view.m_NextEntry->first) : std::nullopt,
				    "Sets the category from which the Vehicle can be chosen");

				if (delta < 0)
				{
					cat_view.m_CurrentEntry = cat_view.m_PreviousEntry; // surely there's a better way to write this
					sel_model_hash = cat_view.m_CurrentEntry->second[0].m_Hash;
					model_updated = true;
				}
				else if (delta > 0)
				{
					cat_view.m_CurrentEntry = cat_view.m_NextEntry;
					sel_model_hash = cat_view.m_CurrentEntry->second[0].m_Hash;
					model_updated = true;
				}
				
				auto list_view = CategoryUtils::GetListView(cat_view.m_CurrentEntry->second, sel_model_hash);

				auto list_delta = ListOption("Type",
				    list_view.m_CurrentEntry->m_FriendlyName,
				    list_view.m_PreviousEntry ? std::optional(list_view.m_PreviousEntry->m_FriendlyName) : std::nullopt,
				    list_view.m_NextEntry ? std::optional(list_view.m_NextEntry->m_FriendlyName) : std::nullopt,
				    "Sets the Vehicle's type from the chosen category");
				
				if (list_delta < 0)
				{
					sel_model_hash = list_view.m_PreviousEntry->m_Hash;
					model_updated = true;
				}
				else if (list_delta > 0)
				{
					sel_model_hash = list_view.m_NextEntry->m_Hash;
					model_updated = true;
				}

				if (model_updated)
				{
					editing_veh->ChangeModel(sel_model_hash);
				}
			}
			EndSubmenu();
		}
	}

	void MapEditorMenu::Setup()
	{
		// build the generated categories once: horses, animals, all humans, all vehicles
		static bool categories_built = false;
		if (categories_built)
			return;
		categories_built = true;

		auto sorted_by_name = [](const auto& a, const auto& b) {
			return std::string(a.m_FriendlyName) < std::string(b.m_FriendlyName);
		};

		{
			std::vector<PedModelDefinition> horses, animals, humans;
			for (const auto& [hash, model] : Data::g_PedModels)
			{
				if (!model)
					continue;
				const std::string name = model;
				if (name.rfind("A_C_Horse", 0) == 0)
					horses.push_back({hash, model, 1});
				else if (name.rfind("A_C_", 0) == 0)
					animals.push_back({hash, model, 1});
				else
					humans.push_back({hash, model, 1});
			}
			std::sort(horses.begin(), horses.end(), sorted_by_name);
			std::sort(animals.begin(), animals.end(), sorted_by_name);
			std::sort(humans.begin(), humans.end(), sorted_by_name);
			if (!horses.empty())
				g_PedCategories.emplace_back("Horses", std::move(horses));
			if (!animals.empty())
				g_PedCategories.emplace_back("Animals", std::move(animals));
			if (!humans.empty())
				g_PedCategories.emplace_back("All Humans", std::move(humans));
		}

		{
			std::vector<VehicleModelDefinition> all;
			for (const auto& v : Data::g_VehicleModels)
				all.push_back({Joaat(v.model), v.name.c_str()});
			std::sort(all.begin(), all.end(), sorted_by_name);
			if (!all.empty())
				g_VehicleCategories.emplace_back("All Vehicles", std::move(all));
		}
	}

	void MapEditorMenu::Update(MapControls& controls)
	{
		if (controls.GetDpadAccept().IsPressed(MapControl::Subsystem::UI))
			EnterPressed();
		if (controls.GetDpadCancel().IsPressed(MapControl::Subsystem::UI))
			BackPressed();
		if (controls.GetDpadUp().IsPressed(MapControl::Subsystem::UI))
			UpPressed();
		if (controls.GetDpadDown().IsPressed(MapControl::Subsystem::UI))
			DownPressed();
		if (controls.GetDpadLeft().IsPressed(MapControl::Subsystem::UI))
			LeftPressed();
		if (controls.GetDpadRight().IsPressed(MapControl::Subsystem::UI))
			RightPressed();

		Render();

		// TODO: this isn't right (should trigger on regular button hovers as well)
		if (IsSubmenuButtonHovered())
			controls.GetDpadAccept().SetExclusive(MapControl::Subsystem::UI); // prevent item selection when hovering over a submenu button

		if (!GetSubmenuType(GetView().m_Submenu))
			MapEditor::ClearEditingEntity();
	}

	void MapEditorMenu::Destroy()
	{
		ExitAllSubmenus();
	}

	NativeMenu::Submenu MapEditorMenu::GetDefaultSubmenu(MapEntity::Type type)
	{
		if (auto it = sm_SubmenusByType.find(type); it != sm_SubmenusByType.end())
		{
			return it->second[0];
		}

		LOG(WARNING) << "MapEditorMenu::GetDefaultSubmenu: unknown entity type " << (int)type << " passed in. Returning main menu";
		return "Mission Creator"sv;
	}

	std::optional<MapEntity::Type> MapEditorMenu::GetSubmenuType(const Submenu& menu)
	{
		for (auto& [type, entries] : sm_SubmenusByType)
		{
			for (auto& entry : entries)
			{
				if (entry == menu)
					return type;
			}
		}

		return std::nullopt;
	}

	void MapEditorMenu::OnItemSelected(MapEntity::Type type)
	{
		auto cur_type = GetSubmenuType(GetView().m_Submenu);
		if (cur_type && *cur_type == type)
			return; // do nothing, we're already in the right submenu for our entity type

		auto default_sub = GetDefaultSubmenu(type);
		ExitAllSubmenus();
		EnterSubmenu("Placement"sv);
		EnterSubmenu(default_sub);
	}
}