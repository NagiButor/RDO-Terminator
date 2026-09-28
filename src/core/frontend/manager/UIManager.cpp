#include "UIManager.hpp"
#include "game/frontend/Theme.hpp"

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
        // Left rail: section selector with an accent bar on the active entry.
        // The right side flows naturally next to it, so nothing can overlap
        // regardless of fonts, DPI or window size.
        constexpr float kSidebarWidth = 118.f;
        if (ImGui::BeginChild("##submenus", ImVec2(kSidebarWidth, 0), true))
        {
            for (auto& submenu : m_Submenus)
            {
                const bool active = (submenu == m_ActiveSubmenu);
                if (ImGui::Selectable(submenu->m_Name.data(), active))
                {
                    SetActiveSubmenu(submenu);
                }
                if (active)
                {
                    const ImVec2 mn = ImGui::GetItemRectMin();
                    const ImVec2 mx = ImGui::GetItemRectMax();
                    ImGui::GetWindowDrawList()->AddRectFilled({mn.x, mn.y}, {mn.x + 3.f, mx.y}, ImGui::ColorConvertFloat4ToU32(YimMenu::Theme::Accent()));
                }
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginGroup(); // right side: page header, categories row, options
        if (m_ActiveSubmenu)
            YimMenu::Theme::DrawSectionTitle(m_ActiveSubmenu->m_Name.data());

        if (m_ActiveSubmenu)
            m_ActiveSubmenu->DrawCategorySelectors();

        ImGui::Separator();

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