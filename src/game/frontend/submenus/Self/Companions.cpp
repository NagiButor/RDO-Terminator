// Companions: spawns 10 bodyguards with their own horses that follow and protect the player.
// Each companion gets a random combat weapon, a lawman-style map blip and mounts its own
// horse when the player rides. Single manager fiber drives follow/combat/mount logic for all.
#include "game/backend/FiberPool.hpp"
#include "game/backend/Players.hpp"
#include "game/backend/ScriptMgr.hpp"
#include "game/backend/Self.hpp"
#include "game/frontend/items/Items.hpp"
#include "game/frontend/submenus/Player/NpcAttack.hpp"
#include "game/frontend/submenus/World/PedSpawner.hpp"
#include "game/rdr/Natives.hpp"
#include "game/rdr/Ped.hpp"
#include "game/rdr/Pools.hpp"
#include "game/rdr/data/PedModels.hpp"
#include "core/frontend/Notifications.hpp"
#include "util/Joaat.hpp"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <random>
#include <string>
#include <vector>

namespace YimMenu::Submenus
{
	struct CompanionRecord
	{
		int ped            = 0;
		int horse          = 0;
		int dismountTicks  = 0;
		Vector3 lastFollowTarget{};
		Vector3 lastPedPos{};
		int stuckTicks     = 0;
		bool hasFollowTask = false;
		float formX        = -1.5f; // personal follow offset: the squad fans out
		float formY        = -1.5f;
		int mountTicks     = 0; // mount task state: issue once, retry on timeout
		int vehTicks       = 0;
	};

	static std::vector<CompanionRecord> s_companions;
	static std::string s_model = "A_M_M_VALDEPUTYRESIDENT_01";
	static int s_aliveCount    = 0;

	static constexpr const char* kHorses[] = {
	    "A_C_Horse_TennesseeWalker_Chestnut",
	    "A_C_Horse_AmericanStandardbred_Black",
	    "A_C_Horse_Mustang_GrulloDun",
	    "A_C_Horse_KentuckySaddle_Black",
	};

	// curated combat pool only (no fishing rods, lanterns or cameras)
	static constexpr const char* kCombatWeapons[] = {
	    "WEAPON_PISTOL_M1899",
	    "WEAPON_PISTOL_MAUSER",
	    "WEAPON_PISTOL_SEMIAUTO",
	    "WEAPON_PISTOL_VOLCANIC",
	    "WEAPON_REVOLVER_CATTLEMAN",
	    "WEAPON_REVOLVER_DOUBLEACTION",
	    "WEAPON_REVOLVER_SCHOFIELD",
	    "WEAPON_REVOLVER_LEMAT",
	    "WEAPON_REVOLVER_NAVY",
	    "WEAPON_REPEATER_CARBINE",
	    "WEAPON_REPEATER_EVANS",
	    "WEAPON_REPEATER_HENRY",
	    "WEAPON_REPEATER_WINCHESTER",
	    "WEAPON_RIFLE_BOLTACTION",
	    "WEAPON_RIFLE_SPRINGFIELD",
	    "WEAPON_RIFLE_VARMINT",
	    "WEAPON_RIFLE_ELEPHANT",
	    "WEAPON_SHOTGUN_DOUBLEBARREL",
	    "WEAPON_SHOTGUN_PUMP",
	    "WEAPON_SHOTGUN_REPEATING",
	    "WEAPON_SHOTGUN_SEMIAUTO",
	    "WEAPON_SHOTGUN_SAWEDOFF",
	    "WEAPON_SNIPERRIFLE_CARCANO",
	    "WEAPON_SNIPERRIFLE_ROLLINGBLOCK",
	    "WEAPON_BOW",
	    "WEAPON_BOW_IMPROVED",
	};

	static std::mt19937& CompanionRng()
	{
		static std::mt19937 rng{std::random_device{}()};
		return rng;
	}

	static bool IsPedModelInList(const std::string& model)
	{
		return Data::g_PedModels.contains(Joaat(model));
	}

	static void RequestModelBlocking(Hash model)
	{
		if (!STREAMING::IS_MODEL_VALID(model) || !STREAMING::IS_MODEL_IN_CDIMAGE(model))
			return;

		STREAMING::REQUEST_MODEL(model, false);
		int tries = 0;
		while (!STREAMING::HAS_MODEL_LOADED(model) && tries < 100)
		{
			ScriptMgr::Yield();
			++tries;
		}
	}

	static void RemovePedBlip(int handle)
	{
		Blip blip = MAP::GET_BLIP_FROM_ENTITY(handle);
		if (MAP::DOES_BLIP_EXIST(blip))
			MAP::REMOVE_BLIP(&blip);
	}

	static void DismissCompanions()
	{
		for (auto& rec : s_companions)
		{
			if (ENTITY::DOES_ENTITY_EXIST(rec.ped))
			{
				RemovePedBlip(rec.ped);
				YimMenu::Ped ped(rec.ped);
				if (ped.IsValid())
				{
					ped.ForceControl();
					ped.Delete();
				}
			}
			if (ENTITY::DOES_ENTITY_EXIST(rec.horse))
			{
				YimMenu::Ped horse(rec.horse);
				if (horse.IsValid())
				{
					horse.ForceControl();
					horse.Delete();
				}
			}
		}
		s_companions.clear();
		s_aliveCount = 0;
	}

	static int FindAttackerOnPlayer(int playerHandle, int selfHandle, Vector3 playerPos, float radius)
	{
		int attacker = 0;
		if (!PED::GET_CLOSEST_PED(playerPos.x, playerPos.y, playerPos.z, radius, 0, 1, &attacker, 1, 0, 0, -1) || attacker == 0)
			return 0;
		if (attacker == selfHandle || attacker == playerHandle)
			return 0;
		if (!ENTITY::DOES_ENTITY_EXIST(attacker))
			return 0;
		if (!PED::IS_PED_IN_COMBAT(attacker, playerHandle))
			return 0;
		return attacker;
	}

	static float DistBetween(Vector3 a, Vector3 b)
	{
		return MISC::GET_DISTANCE_BETWEEN_COORDS(a.x, a.y, a.z, b.x, b.y, b.z, true);
	}

	static bool IsCompanionHandle(int handle)
	{
		for (const auto& rec : s_companions)
			if (rec.ped == handle || rec.horse == handle)
				return true;
		return false;
	}

	// whoever the player is fighting (NPC or another player) becomes the squad's target
	static int FindPlayerVictim(int playerHandle, int selfId, Vector3 playerPos, float radius)
	{
		for (auto ped : Pools::GetPeds())
		{
			const int h = ped.GetHandle();
			if (!ENTITY::DOES_ENTITY_EXIST(h) || ENTITY::IS_ENTITY_DEAD(h))
				continue;
			if (h == playerHandle || IsCompanionHandle(h))
				continue;
			if (DistBetween(playerPos, ped.GetPosition()) > radius)
				continue;
			if (PED::IS_PED_IN_COMBAT(playerHandle, h))
				return h;
		}

		for (int i = 0; i < 32; ++i)
		{
			if (i == selfId || !NETWORK::NETWORK_IS_PLAYER_ACTIVE(i))
				continue;
			const int h = PLAYER::GET_PLAYER_PED(i);
			if (!ENTITY::DOES_ENTITY_EXIST(h) || ENTITY::IS_ENTITY_DEAD(h) || h == playerHandle)
				continue;
			if (DistBetween(playerPos, ENTITY::GET_ENTITY_COORDS(h, true, true)) > radius)
				continue;
			if (PED::IS_PED_IN_COMBAT(playerHandle, h))
				return h;
		}

		return 0;
	}

	static void ManageCompanions()
	{
		while (!s_companions.empty())
		{
			YimMenu::Ped playerPed = Self::GetPed();
			if (!playerPed.IsValid())
			{
				ScriptMgr::Yield(std::chrono::milliseconds(500));
				continue;
			}

			const int playerHandle = playerPed.GetHandle();
			const Vector3 playerPos = playerPed.GetPosition();
			const bool playerOnMount = PED::IS_PED_ON_MOUNT(playerHandle);
			const int selfId = Self::GetPlayer().GetId();
			int alive = 0;

			// squad-wide targets, computed once per tick: the player's victim first,
			// then whoever is fighting the player
			int squadTarget = FindPlayerVictim(playerHandle, selfId, playerPos, 60.0f);
			if (squadTarget == 0)
			{
			for (const auto& rec : s_companions)
			{
				if (!ENTITY::DOES_ENTITY_EXIST(rec.ped) || ENTITY::IS_ENTITY_DEAD(rec.ped))
					continue;
				squadTarget = FindAttackerOnPlayer(playerHandle, rec.ped, playerPos, 50.0f);
				if (squadTarget != 0 && !IsCompanionHandle(squadTarget))
					break;
				squadTarget = 0;
			}
			}

			for (auto it = s_companions.begin(); it != s_companions.end();)
			{
				const int pedHandle = it->ped;
				if (!ENTITY::DOES_ENTITY_EXIST(pedHandle) || ENTITY::IS_ENTITY_DEAD(pedHandle))
				{
					// leave the corpse to the game, drop the record and its blip
					if (ENTITY::DOES_ENTITY_EXIST(pedHandle))
						RemovePedBlip(pedHandle);
					it = s_companions.erase(it);
					continue;
				}
				++alive;

				const int horseHandle   = it->horse;
				const bool horseUsable  = ENTITY::DOES_ENTITY_EXIST(horseHandle) && !ENTITY::IS_ENTITY_DEAD(horseHandle);
				const bool pedOnMount   = PED::IS_PED_ON_MOUNT(pedHandle);
				const bool pedInVehicle = PED::IS_PED_IN_ANY_VEHICLE(pedHandle, false);
				const bool playerInVehicle =
				    !playerOnMount && PED::IS_PED_IN_ANY_VEHICLE(playerHandle, false);

				if (squadTarget != 0)
				{
					Hash attackerGroup = PED::GET_PED_RELATIONSHIP_GROUP_HASH(squadTarget);
					if (attackerGroup == Joaat("REL_COP"))
					{
						Hash ownGroup = PED::GET_PED_RELATIONSHIP_GROUP_HASH(pedHandle);
						PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, ownGroup, Joaat("REL_COP"));
					}
					TASK::TASK_COMBAT_PED(pedHandle, squadTarget, 0, 16);
					it->dismountTicks  = 0;
					it->hasFollowTask  = false;
					it->stuckTicks     = 0;
					it->mountTicks     = 0;
					it->vehTicks       = 0;
				}
				else if (playerOnMount && !pedOnMount && horseUsable && !PED::IS_PED_ON_MOUNT(horseHandle))
				{
					// issue the mount order once and let it play out: reissuing
					// every tick restarts the approach, so they never get on
					if (++it->mountTicks == 1 || it->mountTicks >= 8)
					{
						TASK::TASK_MOUNT_ANIMAL(pedHandle, horseHandle, -1, -1, 1.0f, 1, 0, 0);
						if (it->mountTicks >= 8)
							it->mountTicks = 1;
					}
					it->dismountTicks = 0;
					it->hasFollowTask = false;
					it->stuckTicks    = 0;
					it->vehTicks      = 0;
				}
				else if (!playerOnMount && pedOnMount)
				{
					it->hasFollowTask = false;
					it->stuckTicks    = 0;
					it->mountTicks    = 0;
					it->vehTicks      = 0;
					if (++it->dismountTicks >= 2)
					{
						TASK::TASK_DISMOUNT_ANIMAL(pedHandle, 0, 0, 0, 0, 0);
						it->dismountTicks = 0;
					}
				}
				else if (playerInVehicle && !pedInVehicle && !pedOnMount)
				{
					// ride along in the player's wagon/vehicle, or trail it on foot
					it->mountTicks    = 0;
					it->hasFollowTask = false;
					it->stuckTicks    = 0;
					const int playerVeh = PED::GET_VEHICLE_PED_IS_USING(playerHandle);
					if (ENTITY::DOES_ENTITY_EXIST(playerVeh) && VEHICLE::IS_VEHICLE_SEAT_FREE(playerVeh, -2))
					{
						TASK::TASK_ENTER_VEHICLE(pedHandle, playerVeh, -1, -2, 1.0f, 1, 0);
						it->vehTicks = 0;
					}
					else
					{
						TASK::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(pedHandle, playerHandle, it->formX, it->formY, 0.0f, 3.0f, -1, 2.0f, true, false, false, true, false, false);
						it->lastFollowTarget = playerPos;
						it->hasFollowTask    = true;
					}
				}
				else if (!playerInVehicle && pedInVehicle)
				{
					it->mountTicks    = 0;
					it->hasFollowTask = false;
					it->stuckTicks    = 0;
					if (++it->vehTicks >= 2)
					{
						const int pedVeh = PED::GET_VEHICLE_PED_IS_USING(pedHandle);
						if (ENTITY::DOES_ENTITY_EXIST(pedVeh))
							TASK::TASK_LEAVE_VEHICLE(pedHandle, pedVeh, 0, 0);
						it->vehTicks = 0;
					}
				}
				else if (!pedOnMount && !pedInVehicle)
				{
					// reissue follow only when needed: every reissue restarts the
					// path and makes followers stutter, so keep a standing order
					const Vector3 pedPos   = ENTITY::GET_ENTITY_COORDS(pedHandle, true, true);
					const float distPlayer = DistBetween(pedPos, playerPos);
					const bool playerMoved = !it->hasFollowTask || DistBetween(playerPos, it->lastFollowTarget) > 5.0f;

					if (distPlayer > 4.0f)
					{
						if (DistBetween(pedPos, it->lastPedPos) < 0.5f)
							++it->stuckTicks;
						else
							it->stuckTicks = 0;

						if (playerMoved || it->stuckTicks >= 4)
						{
							if (it->stuckTicks >= 4)
							{
								// wedged on geometry: drop the stale task and start over
								TASK::CLEAR_PED_TASKS_IMMEDIATELY(pedHandle, false, true);
								it->stuckTicks = 0;
							}
							TASK::TASK_FOLLOW_TO_OFFSET_OF_ENTITY(pedHandle, playerHandle, it->formX, it->formY, 0.0f, 3.0f, -1, 2.0f, true, false, false, true, false, false);
							it->lastFollowTarget = playerPos;
							it->hasFollowTask    = true;
						}
					}
					else
					{
						it->stuckTicks = 0;
					}
					it->lastPedPos    = pedPos;
					it->dismountTicks = 0;
					it->mountTicks    = 0;
					it->vehTicks      = 0;
				}
				else
				{
					// both mounted: the player group AI rides along, no explicit task needed
					it->dismountTicks = 0;
					it->hasFollowTask = false;
					it->stuckTicks    = 0;
					it->mountTicks    = 0;
					it->vehTicks      = 0;
				}
				++it;
			}

			s_aliveCount = alive;
			ScriptMgr::Yield(std::chrono::milliseconds(500));
		}
		s_aliveCount = 0;
	}

	static void SpawnCompanions(int count, bool withHorses)
	{
		DismissCompanions();

		Hash modelHash = 0;
		if (s_model.rfind("0x", 0) == 0)
		{
			try
			{
				modelHash = (Hash)std::stoul(s_model, nullptr, 16);
			}
			catch (...)
			{
				Notifications::Show("Companions", "Invalid model hash: " + s_model, NotificationType::Error);
				return;
			}
		}
		else
		{
			modelHash = Joaat(Features::NpcAttackOptions::FixRuKeyboardLayout(s_model));
		}

		if (!STREAMING::IS_MODEL_VALID(modelHash) || !IsPedModelInList(s_model))
		{
			Notifications::Show("Companions", "Unknown ped model: " + s_model, NotificationType::Error);
			return;
		}

		RequestModelBlocking(modelHash);
		if (!STREAMING::HAS_MODEL_LOADED(modelHash))
		{
			Notifications::Show("Companions", "Model failed to load", NotificationType::Error);
			return;
		}
		for (const char* horseModel : kHorses)
		{
			if (!withHorses)
				break;
			RequestModelBlocking(Joaat(horseModel));
		}

		YimMenu::Ped playerPed = Self::GetPed();
		if (!playerPed.IsValid())
		{
			Notifications::Show("Companions", "Player ped not available", NotificationType::Error);
			return;
		}
		const int playerHandle = playerPed.GetHandle();
		const Vector3 playerPos = playerPed.GetPosition();

		std::uniform_int_distribution<std::size_t> horseDist(0, 3);
		std::uniform_int_distribution<std::size_t> weaponDist(0, sizeof(kCombatWeapons) / sizeof(kCombatWeapons[0]) - 1);

		int spawned = 0;
		for (int i = 0; i < count; ++i)
		{
			ScriptMgr::Yield(); // spread networked spawns across ticks

			const float angle = 6.2831853f * (float)i / (float)(count > 0 ? count : 1); // ring around the player
			Vector3 pos = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(playerHandle, std::cos(angle) * 5.0f, std::sin(angle) * 5.0f, 0.0f);
			float groundZ = 0.0f;
			if (MISC::GET_GROUND_Z_FOR_3D_COORD(pos.x, pos.y, pos.z + 5.0f, &groundZ, false))
				pos.z = groundZ + 1.0f;
			else
				pos.z = playerPos.z + 1.0f;

			YimMenu::Ped companion = YimMenu::Ped::Create(modelHash, pos, 0.0f);
			if (!companion.IsValid() || !ENTITY::DOES_ENTITY_EXIST(companion.GetHandle()))
				continue;
			const int pedHandle = companion.GetHandle();

			ENTITY::SET_ENTITY_VISIBLE(pedHandle, true);
			ENTITY::RESET_ENTITY_ALPHA(pedHandle);

			int horseHandle = 0;
			if (withHorses)
			{
				Vector3 horsePos{pos.x - std::sin(angle) * 2.0f, pos.y + std::cos(angle) * 2.0f, pos.z};
				YimMenu::Ped horse = YimMenu::Ped::Create(Joaat(kHorses[horseDist(CompanionRng())]), horsePos, 0.0f);
				if (!horse.IsValid() || !ENTITY::DOES_ENTITY_EXIST(horse.GetHandle()))
				{
					companion.ForceControl();
					companion.Delete();
					continue;
				}
				horseHandle = horse.GetHandle();

				ENTITY::SET_ENTITY_VISIBLE(horseHandle, true);
				ENTITY::RESET_ENTITY_ALPHA(horseHandle);

				// the horse stands by until the player rides: no fleeing, no wandering off
				PED::SET_PED_FLEE_ATTRIBUTES(horseHandle, 0, false);
				PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(horseHandle, true);
			}

			const char* weapon = kCombatWeapons[weaponDist(CompanionRng())];
			WEAPON::GIVE_WEAPON_TO_PED(pedHandle, Joaat(weapon), 500, true, false, 0, false, 0.5f, 1.0f, 1, false, 0, false);
			WEAPON::SET_CURRENT_PED_WEAPON(pedHandle, Joaat(weapon), true, 0, false, false);

			ApplyCompanionSettings(companion);

			// never shoot the boss, even on stray hits: enemies are unaffected
			companion.SetConfigFlag(PedConfigFlag::CanAttackFriendly, false);
			// keep up with a sprinting player instead of lagging behind
			PED::SET_PED_MOVE_RATE_OVERRIDE(pedHandle, 1.25f);

			// companions protect the player: hate whoever hunts them
			Hash ownGroup = PED::GET_PED_RELATIONSHIP_GROUP_HASH(pedHandle);
			PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, ownGroup, Joaat("REL_CRIMINALS"));
			PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, ownGroup, Joaat("REL_GANG"));

			// lawman-style map icon
			Blip blip = MAP::GET_BLIP_FROM_ENTITY(pedHandle);
			if (MAP::DOES_BLIP_EXIST(blip))
			{
				MAP::SET_BLIP_SPRITE(blip, "BLIP_AMBIENT_LAW"_J, true);
				MAP::_SET_BLIP_NAME(blip, "Companion");
			}

			s_companions.push_back({pedHandle, horseHandle, 0, {}, {}, 0, false, std::cos(angle) * 3.0f, std::sin(angle) * 3.0f, 0, 0});
			++spawned;
		}

		STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(modelHash);
		if (withHorses)
		{
			for (const char* horseModel : kHorses)
				STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(Joaat(horseModel));
		}

		if (spawned > 0)
		{
			s_aliveCount = spawned;
			FiberPool::Push([] {
				ManageCompanions();
			});
			Notifications::Show("Companions", "Spawned " + std::to_string(spawned) + " companion" + (spawned > 1 ? "s" : ""), NotificationType::Success);
		}
		else
		{
			Notifications::Show("Companions", "Failed to spawn companions", NotificationType::Error);
		}
	}

	static std::string ToLowerCopy(std::string value)
	{
		std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return (char)std::tolower(c); });
		return value;
	}

	void RenderCompanionsCategory()
	{
		// model input with a live "closest matches" dropdown, same as World -> Spawner -> Ped
		InputTextWithHint("##compmodel", "model, e.g. A_M_M_VALDEPUTYRESIDENT_01 (click / type to see matches)", &s_model,
		    ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackEdit, nullptr,
		    Features::NpcAttackOptions::ModelInputCallback)
		    .Draw();

		const bool is_input_active = ImGui::IsItemActive();
		if (!s_model.empty() && !IsPedModelInList(s_model))
		{
			if (is_input_active)
				ImGui::OpenPopup("##comp_search_popup");
		}

		const ImVec2 input_pos  = ImGui::GetItemRectMin();
		const ImVec2 input_size = ImGui::GetItemRectSize();

		float item_height         = ImGui::GetTextLineHeightWithSpacing();
		float max_dropdown_height = (item_height * 10.0f) + (ImGui::GetStyle().WindowPadding.y * 2.0f);

		ImGui::SetNextWindowPos({input_pos.x, input_pos.y + input_size.y});
		ImGui::SetNextWindowSize({input_size.x, 0});
		ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, max_dropdown_height));

		if (ImGui::BeginPopup("##comp_search_popup", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_ChildWindow))
		{
			const std::string buffer_lower = ToLowerCopy(Features::NpcAttackOptions::FixRuKeyboardLayout(s_model));
			for (const auto& [hash, model] : Data::g_PedModels)
			{
				if (!model)
					continue;

				std::string model_lower = model;
				std::transform(model_lower.begin(), model_lower.end(), model_lower.begin(), [](unsigned char c) { return (char)std::tolower(c); });
				if (model_lower.find(buffer_lower) == std::string::npos)
					continue;

				if (ImGui::Selectable(model))
				{
					s_model = model;
					ImGui::CloseCurrentPopup();
				}
			}
			if (!is_input_active && !ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
				ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}

		if (ImGui::Button("Spawn 10 Companions"))
		{
			FiberPool::Push([] {
				SpawnCompanions(10, true);
			});
		}
		ImGui::SameLine();
		if (ImGui::Button("Spawn 10 (no horses)"))
		{
			FiberPool::Push([] {
				SpawnCompanions(10, false);
			});
		}
		if (ImGui::Button("Spawn 1 Companion"))
		{
			FiberPool::Push([] {
				SpawnCompanions(1, false);
			});
		}
		ImGui::SameLine();
		if (ImGui::Button("Spawn 1 + Horse"))
		{
			FiberPool::Push([] {
				SpawnCompanions(1, true);
			});
		}
		ImGui::SameLine();
		if (ImGui::Button("Dismiss All"))
		{
			FiberPool::Push([] {
				DismissCompanions();
				Notifications::Show("Companions", "Dismissed all companions", NotificationType::Info);
			});
		}
		ImGui::Text("Active: %d", s_aliveCount);
		ImGui::TextWrapped("Companions follow you, fight whoever you fight or whoever fights you, and mount their own horses when you ride.");
	}
}
