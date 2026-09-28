#include "Items.hpp"
#include "game/frontend/Theme.hpp"

#include <algorithm>

namespace YimMenu
{
	Group::Group(const std::string& name, int items_per_row) :
	    m_Name(name),
	    m_ItemsPerRow(items_per_row)
	{
	}

	void Group::Draw()
	{
		// m_ItemsPerRow is "rows per column": after that many visible rows the
		// group continues in the next column. Only visible, non-inline items
		// count, so hidden conditional options no longer skew the columns.
		int rows = 0;
		for (auto& item : m_Items)
			if (item->IsVisible() && !item->IsInline())
				rows++;

		const int perColumn = m_ItemsPerRow > 0 ? m_ItemsPerRow : std::max(rows, 1);
		const int columns   = std::clamp((rows + perColumn - 1) / perColumn, 1, 4);

		const bool named = !m_Name.empty();
		if (named)
			Theme::BeginCard(m_Name.c_str(), this);
		else
			ImGui::PushID(this);

		const bool table = columns > 1 && ImGui::BeginTable("##columns", columns, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchSame);
		if (table)
			ImGui::TableNextColumn();
		else
			ImGui::BeginGroup();

		int rowInColumn = 0;
		for (auto& item : m_Items)
		{
			if (!item->IsVisible())
				continue;

			if (!item->IsInline())
			{
				if (table && rowInColumn == perColumn)
				{
					ImGui::TableNextColumn();
					rowInColumn = 0;
				}
				rowInColumn++;
			}
			item->Draw();
		}

		if (table)
			ImGui::EndTable();
		else
			ImGui::EndGroup();

		if (named)
			Theme::EndCard();
		else
			ImGui::PopID();
	}
}
