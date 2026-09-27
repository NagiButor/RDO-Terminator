#include "UIManager.hpp"

namespace YimMenu
{
    void UIManager::AddSubmenuImpl(const std::shared_ptr<Submenu>&& submenu)
    {
        if (!m_ActiveSubmenu)
            m_ActiveSubmenu = submenu;

        m_Submenus.push_back(std::move(submenu));
    }

    void UIManager::SetActiveSubmenuImpl(const std::shared_ptr<Submenu> Submenu)
    {
        m_ActiveSubmenu = Submenu;
    }

    void UIManager::DrawImpl()
    {
        // Left column: submenu selector. Height fills the remaining content
        // region; the right side flows naturally next to it, so nothing can
        // overlap regardless of fonts, DPI or window size.
        constexpr float kSubmenuColumnWidth = 80.f;
        if (ImGui::BeginChild("##submenus", ImVec2(kSubmenuColumnWidth, 0), true))
        {
            for (auto& submenu : m_Submenus)
            {
                if (ImGui::Selectable(submenu->m_Name.data(), (submenu == m_ActiveSubmenu)))
                {
                    SetActiveSubmenu(submenu);
                }
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginGroup(); // right side: categories row + options
        if (m_ActiveSubmenu)
            m_ActiveSubmenu->DrawCategorySelectors();

        if (ImGui::BeginChild("##options", ImVec2(0, 0), true))
        {
            if (m_OptionsFont)
                ImGui::PushFont(m_OptionsFont);

            if (m_ActiveSubmenu)
                m_ActiveSubmenu->Draw();

            if (m_OptionsFont)
                ImGui::PopFont();
        }
        ImGui::EndChild();
        ImGui::EndGroup();
    }

    std::shared_ptr<Submenu> UIManager::GetActiveSubmenuImpl()
    {
        if (m_ActiveSubmenu)
        {
            return m_ActiveSubmenu;
        }

        return nullptr;
    }

    std::shared_ptr<Category> UIManager::GetActiveCategoryImpl()
    {
        if (m_ActiveSubmenu)
        {
            return m_ActiveSubmenu->GetActiveCategory();
        }

        return nullptr;
    }
}