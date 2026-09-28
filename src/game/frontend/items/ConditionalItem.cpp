#include "Items.hpp"
#include "core/commands/Command.hpp"
#include "core/commands/Commands.hpp"
#include "core/commands/BoolCommand.hpp"

namespace YimMenu
{
	ConditionalItem::ConditionalItem(joaat_t bool_cmd_id, std::shared_ptr<UIItem> to_draw) :
	    m_Condition(Commands::GetCommand<BoolCommand>(bool_cmd_id)),
	    m_Item(to_draw)
	{
	}

	void ConditionalItem::Draw()
	{
		if (IsVisible())
			m_Item->Draw();
	}

	bool ConditionalItem::IsVisible() const
	{
		return m_Condition && m_Item && m_Condition->GetState() && m_Item->IsVisible();
	}

	bool ConditionalItem::IsInline() const
	{
		return m_Item && m_Item->IsInline();
	}
}
