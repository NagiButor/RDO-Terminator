#include "Items.hpp"
#include "core/commands/Commands.hpp"
#include "core/commands/Command.hpp"
#include "core/commands/LoopedCommand.hpp"
#include "game/frontend/Theme.hpp"

namespace YimMenu
{
	BoolCommandItem::BoolCommandItem(joaat_t id, std::optional<std::string> label_override) :
	    m_Command(Commands::GetCommand<BoolCommand>(id)),
	    m_LabelOverride(label_override)
	{
	}

	void BoolCommandItem::Draw()
	{
		if (!m_Command)
		{
			ImGui::TextDisabled("Unknown!");
			return;
		}

		const std::string& label = m_LabelOverride.has_value() ? m_LabelOverride.value() : m_Command->GetLabel();
		bool enabled             = m_Command->GetState();
		// hash-scoped ID: two toggles sharing a label (e.g. "Snapline") must not share state
		ImGui::PushID((int)m_Command->GetHash());
		if (Theme::Switch(label.c_str(), &enabled))
			m_Command->SetState(enabled);
		ImGui::PopID();

		const auto popupLabel = std::format("{} Hotkey##{}", m_Command->GetLabel(), m_Command->GetHash());

		if (ImGui::IsItemHovered())
		{
			// descriptions may contain '%', never pass them as a format string
			if (!m_Command->GetDescription().empty())
				ImGui::SetTooltip("%s", m_Command->GetDescription().c_str());
			if (GetAsyncKeyState(VK_OEM_3) & 0x8000)
				ImGui::OpenPopup(popupLabel.c_str());
		}

		ImGui::SetNextWindowSize(ImVec2(500, 0));
		if (ImGui::BeginPopupModal(popupLabel.c_str(), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar))
		{
			ImGui::BulletText("Hover over the command name to change its hotkey");
			ImGui::BulletText("Press any registered key to remove");
			ImGui::Separator();

			HotkeySetter(m_Command->GetHash()).Draw();

			ImGui::Spacing();
			if (ImGui::Button("Close") || ((!ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) && ImGui::IsMouseClicked(ImGuiMouseButton_Left)))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
	}
}
