#include "Items.hpp"
#include "core/commands/ColorCommand.hpp"
#include "core/commands/Command.hpp"
#include "core/commands/Commands.hpp"
#include "game/frontend/Theme.hpp"

namespace YimMenu
{
	ColorCommandItem::ColorCommandItem(joaat_t id, std::optional<std::string> labelOverride) :
	    m_Command(Commands::GetCommand<ColorCommand>(id)),
	    m_LabelOverride(labelOverride)
	{
	}

	void ColorCommandItem::Draw()
	{
		if (!m_Command)
		{
			ImGui::TextDisabled("Unknown color picker!");
			return;
		}

		// swatch sits on the row of the toggle it belongs to, in the shared value column
		Theme::SameLineValueColumn();

		auto color = m_Command->GetState();
		// the command hash keeps popups unique even when two pickers share a label
		ImGui::PushID((int)m_Command->GetHash());
		if (Theme::ColorSwatch(m_LabelOverride.value_or(m_Command->GetLabel()).c_str(), (float*)&color))
			m_Command->SetState(color);
		ImGui::PopID();
	}
}
