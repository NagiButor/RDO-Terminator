#include "Items.hpp"
#include "core/commands/Command.hpp"
#include "core/commands/Commands.hpp"
#include "core/commands/ListCommand.hpp"
#include "game/frontend/Theme.hpp"

namespace YimMenu
{
	ListCommandItem::ListCommandItem(joaat_t id, std::optional<std::string> label_override) :
	    m_Command(Commands::GetCommand<ListCommand>(id)),
	    m_LabelOverride(label_override)
	{
	}

	void ListCommandItem::Draw()
	{
		if (!m_Command)
		{
			ImGui::TextDisabled("Unknown list!");
			return;
		}

		// state is read every frame, so hotkeys / config reloads are reflected immediately
		int value = m_Command->GetState();
		if (Theme::Stepper(m_LabelOverride.value_or(m_Command->GetLabel()).c_str(), m_Command->GetList(), &value))
			m_Command->SetState(value);
	}
}
