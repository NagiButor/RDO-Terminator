#include "EspLabPanel.hpp"
#include "MockEspWorld.hpp"
#include "DummyMesh.hpp"

#include "core/commands/BoolCommand.hpp"
#include "core/commands/Commands.hpp"
#include "core/commands/ListCommand.hpp"
#include "imgui.h"

#include <string>
#include <utility>
#include <vector>

namespace YimMenu
{
	namespace
	{
		std::string g_MeshStatus = "not loaded";

		void SetBool(const char* name, bool v)
		{
			if (auto* c = Commands::GetCommand<BoolCommand>(Joaat(name)))
				c->SetState(v);
		}

		void SetList(const char* name, int v)
		{
			if (auto* c = Commands::GetCommand<ListCommand>(Joaat(name)))
				c->SetState(v);
		}

		// Every player + ped option on, so one glance covers the whole overlay.
		void PresetFull()
		{
			for (const char* n : {"espdrawplayers", "espdrawdeadplayers", "espnameplayers", "esphealthplayers", "espdistanceplayers",
			         "espskeletonplayers", "espsnaplineplayers", "espdrawpeds", "espdrawdeadpeds", "espanimalspeds", "espmodelspeds",
			         "espnetinfopeds", "espscriptinfopeds", "esphealthpeds", "espdistancepeds", "espskeletonpeds"})
				SetBool(n, true);
			SetList("espboxplayers", 1);
			SetList("espboxpeds", 3);
		}

		// What most people run: boxes, names, health, distance.
		void PresetClean()
		{
			PresetFull();
			for (const char* n : {"espskeletonplayers", "espsnaplineplayers", "espnetinfopeds", "espscriptinfopeds", "espskeletonpeds",
			         "espsnaplinepeds", "espdrawdeadpeds"})
				SetBool(n, false);
		}

		void PresetOff()
		{
			SetBool("espdrawplayers", false);
			SetBool("espdrawpeds", false);
		}
	}

	void SetEspLabMeshStatus(const std::string& status)
	{
		g_MeshStatus = status;
	}

	void DrawEspLabPanel(MockEspWorld& world, DummyMesh& mesh, bool expandOnFirstUse)
	{
		MockActor& actor = world.m_Actor;

		// dock to the right edge so it never sits under the menu
		const ImVec2 display = ImGui::GetIO().DisplaySize;
		ImGui::SetNextWindowPos(ImVec2(display.x - 310.f, 20.f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(290.f, 560.f), ImGuiCond_FirstUseEver);
		// --lab forces it open once; otherwise it starts collapsed and imgui.ini remembers the rest
		static bool first = true;
		if (first && expandOnFirstUse)
			ImGui::SetNextWindowCollapsed(false, ImGuiCond_Always);
		else
			ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
		first = false;
		if (!ImGui::Begin("ESP Lab (fake scene)"))
		{
			ImGui::End();
			return;
		}

		ImGui::SeparatorText("ESP presets");
		if (ImGui::Button("Full"))
			PresetFull();
		ImGui::SameLine();
		if (ImGui::Button("Clean"))
			PresetClean();
		ImGui::SameLine();
		if (ImGui::Button("Off"))
			PresetOff();
		ImGui::TextDisabled("Flip the real Visuals options.");

		ImGui::SeparatorText("Scene");
		if (ImGui::Button("Spawn crowd"))
			world.SpawnCrowd();
		ImGui::SameLine();
		if (ImGui::Button("Clear crowd"))
			world.m_Crowd.clear();
		ImGui::SameLine();
		ImGui::TextDisabled("%d extra", (int)world.m_Crowd.size());

		ImGui::SeparatorText("Main actor");
		if (ImGui::Button("Respawn"))
			actor.Respawn();
		ImGui::SameLine();
		if (ImGui::Button("-25 HP"))
			actor.Damage(25);
		ImGui::SameLine();
		if (ImGui::Button("+25 HP"))
			actor.Heal(25);
		ImGui::SameLine();
		if (ImGui::Button(actor.IsDead() ? "Revive" : "Kill"))
			actor.IsDead() ? actor.Revive() : actor.Kill();

		if (ImGui::SliderInt("Health", &actor.m_Health, 0, actor.m_MaxHealth))
			actor.m_Dead = actor.m_Health <= 0;

		int mode = actor.m_ShowAsPlayer ? 0 : 1;
		ImGui::RadioButton("Player", &mode, 0);
		ImGui::SameLine();
		ImGui::RadioButton("Ped", &mode, 1);
		actor.m_ShowAsPlayer = (mode == 0);
		ImGui::SameLine();
		ImGui::Checkbox("Animal", &actor.m_Animal);

		ImGui::Checkbox("Walking", &actor.m_Walking);
		ImGui::SameLine();
		ImGui::Checkbox("Selected", &actor.m_Selected);
		ImGui::SameLine();
		ImGui::Checkbox("Occluded", &actor.m_Occluded);

		ImGui::SliderFloat("Distance", &actor.m_Pos.y, 3.f, 400.f, "%.0f m");
		ImGui::SliderFloat("Lateral", &actor.m_Pos.x, -30.f, 30.f, "%.1f m");

		ImGui::SeparatorText("Camera");
		ImGui::Checkbox("Track actor", &world.m_Camera.m_Track);
		if (!world.m_Camera.m_Track)
		{
			ImGui::SliderFloat("Yaw", &world.m_Camera.m_YawDeg, -180.f, 180.f, "%.0f deg");
			ImGui::SliderFloat("Pitch", &world.m_Camera.m_PitchDeg, -80.f, 80.f, "%.0f deg");
		}
		ImGui::SliderFloat("Height", &world.m_Camera.m_Pos.z, 0.5f, 20.f, "%.1f m");
		ImGui::SliderFloat("FOV", &world.m_Camera.m_Fov, 20.f, 110.f, "%.0f");

		ImGui::SeparatorText("Readout");
		const ImVec2 head = world.ProjectHead();
		ImGui::Text("HP %d / %d%s   dist %dm", actor.GetHealth(), actor.GetMaxHealth(), actor.IsDead() ? " (dead)" : "", (int)actor.GetDistance());
		ImGui::Text("Head on screen: %.0f, %.0f %s", head.x, head.y, head.x == 0 ? "(culled)" : "");

		ImGui::SeparatorText("Crash dummy mesh");
		if (!mesh.IsLoaded())
		{
			ImGui::TextColored(ImVec4(1.f, 0.55f, 0.4f, 1.f), "Mesh: %s", g_MeshStatus.c_str());
			ImGui::TextDisabled("Put crash_dummy\scene.gltf next to the exe");
			ImGui::TextDisabled("or set RDO_DUMMY_MESH.");
		}
		else
		{
			ImGui::SliderFloat("Scale", &mesh.m_Scale, 0.5f, 2.f, "%.2f");
			ImGui::SliderFloat("Yaw##mesh", &mesh.m_YawDeg, -180.f, 180.f, "%.0f deg");
			ImGui::SliderFloat("Z offset", &mesh.m_ZOffset, -0.5f, 0.5f, "%.2f");
			ImGui::Checkbox("Wireframe", &mesh.m_Wireframe);
			ImGui::SameLine();
			ImGui::Checkbox("Textured", &mesh.m_Textured);
		}
		ImGui::TextDisabled("Crash Dummy by robomax (CC-BY-NC-4.0)");

		ImGui::Separator();
		static bool showMetrics = false;
		ImGui::Checkbox("ImGui Metrics (layout debug)", &showMetrics);
		if (showMetrics)
			ImGui::ShowMetricsWindow(&showMetrics);

		ImGui::End();
	}
}
