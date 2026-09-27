#include "EspLabPanel.hpp"
#include "MockEspWorld.hpp"
#include "DummyMesh.hpp"

#include "imgui.h"

#include <string>

namespace YimMenu
{
	void DrawEspLabPanel(MockEspWorld& world, DummyMesh& mesh)
	{
		MockActor& actor = world.m_Actor;

		ImGui::SetNextWindowPos(ImVec2(980.f, 20.f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(280.f, 480.f), ImGuiCond_FirstUseEver);
		if (!ImGui::Begin("ESP Lab (fake scene)"))
		{
			ImGui::End();
			return;
		}

		ImGui::TextUnformatted("Actor");
		if (ImGui::Button("Respawn"))
			actor.Respawn();
		ImGui::SameLine();
		if (ImGui::Button("Damage -25"))
			actor.Damage(25);
		ImGui::SameLine();
		if (ImGui::Button("Heal +25"))
			actor.Heal(25);
		if (ImGui::Button("Kill"))
			actor.Kill();
		ImGui::SameLine();
		if (ImGui::Button("Revive"))
			actor.Revive();

		ImGui::Checkbox("Walking", &actor.m_Walking);
		ImGui::Checkbox("Selected (blue name)", &actor.m_Selected);
		ImGui::Checkbox("Occluded (behind wall)", &actor.m_Occluded);
		int mode = actor.m_ShowAsPlayer ? 0 : 1;
		ImGui::RadioButton("Show as Player", &mode, 0);
		ImGui::SameLine();
		ImGui::RadioButton("Show as Ped", &mode, 1);
		actor.m_ShowAsPlayer = (mode == 0);

		ImGui::SliderFloat("Distance", &actor.m_Pos.y, 3.f, 300.f, "%.0fm");
		ImGui::SliderFloat("Lateral", &actor.m_Pos.x, -30.f, 30.f, "%.1f");

		ImGui::Separator();
		ImGui::TextUnformatted("Camera");
		ImGui::Checkbox("Track actor", &world.m_Camera.m_Track);
		if (!world.m_Camera.m_Track)
		{
			ImGui::SliderFloat("Yaw", &world.m_Camera.m_YawDeg, -180.f, 180.f, "%.0f deg");
			ImGui::SliderFloat("Pitch", &world.m_Camera.m_PitchDeg, -80.f, 80.f, "%.0f deg");
		}
		ImGui::SliderFloat("FOV", &world.m_Camera.m_Fov, 20.f, 110.f, "%.0f");

		ImGui::Separator();
		const ImVec2 head = world.ProjectHead();
		ImGui::Text("HP: %d / %d%s", actor.GetHealth(), actor.GetMaxHealth(), actor.IsDead() ? " (dead)" : "");
		ImGui::Text("Distance: %dm", (int)actor.GetDistance());
		ImGui::Text("Head: %.0f, %.0f %s", head.x, head.y, head.x == 0 ? "(culled)" : "");

		ImGui::Separator();
		ImGui::TextUnformatted("Crash dummy mesh");
		if (!mesh.IsLoaded())
		{
			ImGui::TextDisabled("scene.gltf not loaded");
		}
		else
		{
			ImGui::SliderFloat("Mesh scale", &mesh.m_Scale, 0.5f, 2.f, "%.2f");
			ImGui::SliderFloat("Mesh yaw", &mesh.m_YawDeg, -180.f, 180.f, "%.0f deg");
			ImGui::SliderFloat("Mesh Z", &mesh.m_ZOffset, -0.5f, 0.5f, "%.2f");
			ImGui::Checkbox("Wireframe", &mesh.m_Wireframe);
			ImGui::SameLine();
			ImGui::Checkbox("Textured", &mesh.m_Textured);
		}
		ImGui::TextDisabled("Mesh: Crash Dummy by robomax (CC-BY-NC-4.0)");

		ImGui::Separator();
		static bool showMetrics = false;
		ImGui::Checkbox("ImGui Metrics (layout debug)", &showMetrics);
		if (showMetrics)
			ImGui::ShowMetricsWindow(&showMetrics);

		ImGui::End();
	}
}
