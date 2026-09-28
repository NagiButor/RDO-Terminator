#include "Submenu.hpp"
#include "game/frontend/Theme.hpp"

namespace YimMenu
{
	void Submenu::SetActiveCategory(const std::shared_ptr<Category> category)
	{
		m_ActiveCategory = category;
	}

	void Submenu::AddCategory(std::shared_ptr<Category>&& category)
	{
		if (!m_ActiveCategory)
			m_ActiveCategory = category;

		m_Categories.push_back(std::move(category));
	}

	void Submenu::DrawCategorySelectors()
	{
		// tabs flow left to right and wrap onto a new row when the window is narrow
		const float right = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
		bool first        = true;
		for (auto& category : m_Categories)
		{
			if (!category)
				continue;

			if (!first)
			{
				ImGui::SameLine(0.f, 6.f);
				// rough width check; TabButton sizes itself from the heading font
				const float estimate = ImGui::CalcTextSize(category->m_Name.c_str()).x + 80.f;
				if (ImGui::GetCursorScreenPos().x + estimate > right)
					ImGui::NewLine();
			}
			first = false;

			if (Theme::TabButton(category->m_Name.c_str(), Theme::IconForName(category->m_Name), category == GetActiveCategory()))
				SetActiveCategory(category);
		}
	}

	void Submenu::Draw()
	{
		if (m_ActiveCategory)
		{
			m_ActiveCategory->Draw();
		}
	}
}
