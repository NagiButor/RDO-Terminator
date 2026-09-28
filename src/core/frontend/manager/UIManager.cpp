#include "UIManager.hpp"
#include "game/frontend/Branding.hpp"
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

    void UIManager::DrawSidebar(float width)
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::PanelBg());
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 2.f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.f);
        ImGui::BeginChild("##sidebar", ImVec2(width, 0.f), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor();

        ImDrawList* dl  = ImGui::GetWindowDrawList();
        const ImVec2 wp = ImGui::GetWindowPos();
        const ImVec2 ws = ImGui::GetWindowSize();

        // emblem on top
        const float emblemH = 64.f;
        Theme::DrawIcon(dl, "emblem", ImVec2(wp.x + width * 0.5f, wp.y + emblemH * 0.5f + 6.f - ImGui::GetScrollY()), 34.f, Theme::Col(Theme::Accent()));
        ImGui::Dummy(ImVec2(width, emblemH));

        const float entryH = ImGui::GetFontSize() * 2.2f;
        for (auto& submenu : m_Submenus)
        {
            const std::string_view icon = submenu->m_Icon.empty() ? Theme::IconForName(submenu->m_Name) : std::string_view(submenu->m_Icon);
            if (Theme::SidebarEntry(submenu->m_Name.c_str(), icon, submenu == m_ActiveSubmenu, entryH))
                SetActiveSubmenu(submenu);
        }

        // decorative footer: diagonal slashes + tagline, only when there is room for it
        const float footerH  = 120.f;
        const float listEndY = ImGui::GetCursorScreenPos().y;
        const float bottom   = wp.y + ws.y;
        if (bottom - listEndY > footerH && ImGui::GetScrollMaxY() <= 0.f)
        {
            dl->PushClipRect(wp, ImVec2(wp.x + ws.x, bottom), true);
            for (int i = 0; i < 5; i++)
            {
                const float y0 = bottom - footerH + 8.f + i * 13.f;
                const float x0 = wp.x + width * (0.35f + i * 0.08f);
                const float a  = 0.55f - i * 0.09f;
                dl->AddQuadFilled(ImVec2(x0 + 70.f, y0), ImVec2(x0 + 76.f, y0), ImVec2(x0 + 6.f, y0 + 34.f), ImVec2(x0, y0 + 34.f), Theme::Col(Theme::Accent(), a));
            }
            dl->AddRectFilledMultiColor(ImVec2(wp.x, bottom - footerH), ImVec2(wp.x + ws.x, bottom),
                Theme::Col(Theme::Accent(), 0.f), Theme::Col(Theme::Accent(), 0.f), Theme::Col(Theme::Accent(), 0.12f), Theme::Col(Theme::Accent(), 0.12f));
            dl->AddLine(ImVec2(wp.x + 14.f, bottom - 58.f), ImVec2(wp.x + ws.x - 14.f, bottom - 72.f), Theme::Col(Theme::Accent(), 0.6f), 1.f);

            ImFont* sf        = Theme::g_SmallFont ? Theme::g_SmallFont : ImGui::GetFont();
            const float sfs   = sf->FontSize;
            const ImU32 muted = Theme::Col(Theme::MutedText());
            if (*Branding::kTaglineTop)
                Theme::DrawSpacedText(dl, ImVec2(wp.x + 22.f, bottom - 48.f), muted, Branding::kTaglineTop, 3.f, sf, sfs);
            if (*Branding::kTaglineBottom)
                Theme::DrawSpacedText(dl, ImVec2(wp.x + 22.f, bottom - 48.f + sfs + 4.f), muted, Branding::kTaglineBottom, 3.f, sf, sfs);
            dl->PopClipRect();
        }

        dl->AddRect(wp, ImVec2(wp.x + ws.x, wp.y + ws.y), Theme::Col(Theme::Accent(), 0.30f), 6.f, 0, 1.f);
        ImGui::EndChild();
    }

    void UIManager::DrawImpl()
    {
        // Left rail: submenus. Right: category tabs on top, the active category below.
        const float sidebarWidth = ImGui::GetFontSize() * 10.f;
        DrawSidebar(sidebarWidth);

        ImGui::SameLine(0.f, 12.f);

        ImGui::BeginGroup();
        if (m_ActiveSubmenu)
            m_ActiveSubmenu->DrawCategorySelectors();

        ImGui::Dummy(ImVec2(1.f, 4.f));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.f, 2.f));
        const bool open = ImGui::BeginChild("##options", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding);
        ImGui::PopStyleVar();
        if (open)
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
