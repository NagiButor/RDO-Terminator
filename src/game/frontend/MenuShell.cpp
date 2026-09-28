// MenuShell: the actual cheat-menu window, shared by the in-game DLL and the
// visual-test harness. It only depends on UIManager, the Settings submenu
// (window geometry), Theme/Branding and an injected ShellContext, so it can
// run without Renderer, GUI, MapEditor, FiberPool or any game backend.
#include "game/frontend/Menu.hpp"
#include "game/frontend/Branding.hpp"
#include "game/frontend/Theme.hpp"

#include "core/frontend/manager/UIManager.hpp"
#include "game/frontend/fonts/Fonts.hpp"
#include "game/frontend/submenus/Settings.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>
#include <Windows.h>

namespace YimMenu::Menu
{
	namespace
	{
		constexpr float kHeaderHeight = 84.f;
		constexpr float kFrameCut     = 18.f;
		constexpr int kMinWidth       = 760;
		constexpr int kMinHeight      = 480;

		// Loads a Windows system font when present, otherwise the embedded one,
		// so the shell looks the same on machines without the system font.
		ImFont* LoadDisplayFont(const char* systemFile, float size)
		{
			auto& io = ImGui::GetIO();
			char* windir = std::getenv("WINDIR");
			const std::filesystem::path file = std::filesystem::path(windir ? windir : "C:\\Windows") / "Fonts" / systemFile;
			std::error_code ec;
			if (std::filesystem::exists(file, ec))
				if (ImFont* font = io.Fonts->AddFontFromFileTTF(file.string().c_str(), size))
					return font;

			ImFontConfig cfg{};
			cfg.FontDataOwnedByAtlas = false;
			return io.Fonts->AddFontFromMemoryTTF(const_cast<std::uint8_t*>(Fonts::MainFont), sizeof(Fonts::MainFont), size, &cfg);
		}

		void DrawLogo(ImDrawList* dl, ImVec2 p, float size, ImTextureID logo)
		{
			const ImVec2 q(p.x + size, p.y + size);
			// soft red halo behind the logo
			const ImVec2 c(p.x + size * 0.5f, p.y + size * 0.5f);
			for (int i = 4; i >= 1; i--)
				dl->AddCircleFilled(c, size * (0.45f + i * 0.12f), Theme::Col(Theme::Accent(), 0.035f), 32);

			Theme::FillChamfered(dl, p, q, 10.f, Theme::Col(ImVec4(0.06f, 0.02f, 0.03f, 1.f)));
			if (logo)
				dl->AddImage(logo, ImVec2(p.x + 4.f, p.y + 4.f), ImVec2(q.x - 4.f, q.y - 4.f));
			else
				Theme::DrawIcon(dl, "emblem", ImVec2(c.x, c.y + 2.f), size * 0.62f, Theme::Col(Theme::AccentBright()));
			Theme::DrawNeonFrame(dl, p, q, 10.f, Theme::Col(Theme::Accent()), 0.7f);
		}

		// Title (two color runs), subtitle, status and header buttons.
		// Returns true when the minimize button was pressed.
		bool DrawHeader(const ShellContext& ctx, ImVec2 wp, float width, bool minimized)
		{
			ImDrawList* dl = ImGui::GetWindowDrawList();
			const float x0 = wp.x + 22.f;
			const float y0 = wp.y + 14.f;

			const float logoSize = kHeaderHeight - 28.f;
			DrawLogo(dl, ImVec2(x0, y0), logoSize, ctx.m_Logo);

			ImFont* tf        = Theme::g_TitleFont ? Theme::g_TitleFont : ImGui::GetFont();
			ImFont* sf        = Theme::g_SmallFont ? Theme::g_SmallFont : ImGui::GetFont();
			const float tfs   = tf->FontSize;
			const float sfs   = sf->FontSize;
			const float textX = x0 + logoSize + 18.f;
			const float titleY = y0 + 2.f;

			// primary run, faux-bold
			const ImU32 white = IM_COL32(245, 245, 248, 255);
			dl->AddText(tf, tfs, ImVec2(textX, titleY), white, Branding::kTitlePrimary);
			dl->AddText(tf, tfs, ImVec2(textX + 1.f, titleY), white, Branding::kTitlePrimary);
			const float primaryW = tf->CalcTextSizeA(tfs, FLT_MAX, 0.f, Branding::kTitlePrimary).x + tfs * 0.3f;

			// accent run with a soft glow
			const ImVec2 ap(textX + primaryW, titleY);
			for (int i = 0; i < 4; i++)
			{
				const float ox = (i & 1) ? 1.5f : -1.5f;
				const float oy = (i & 2) ? 1.5f : -1.5f;
				dl->AddText(tf, tfs, ImVec2(ap.x + ox, ap.y + oy), Theme::Col(Theme::Accent(), 0.18f), Branding::kTitleAccent);
			}
			dl->AddText(tf, tfs, ap, Theme::Col(Theme::Accent()), Branding::kTitleAccent);
			dl->AddText(tf, tfs, ImVec2(ap.x + 1.f, ap.y), Theme::Col(Theme::Accent()), Branding::kTitleAccent);

			Theme::DrawSpacedText(dl, ImVec2(textX + 1.f, titleY + tfs + 2.f), Theme::Col(Theme::Text(), 0.72f), Branding::kSubtitle, 2.2f, sf, sfs);
			const float textEnd = textX + std::max(primaryW + tf->CalcTextSizeA(tfs, FLT_MAX, 0.f, Branding::kTitleAccent).x,
			                                  Theme::CalcSpacedTextWidth(Branding::kSubtitle, 2.2f, sf, sfs));

			// right side, laid out right-to-left. Header actions (pills) sit next to
			// the status when there is room, otherwise on a second row under it.
			const float btn   = 30.f;
			const float right = wp.x + width - 26.f;
			float x           = right;
			bool minimize     = false;

			const std::string build = std::string("Build: ") + kMenuVersion;
			const ImVec2 bs         = ImGui::CalcTextSize(build.c_str());
			const ImVec2 ss         = ImGui::CalcTextSize(Branding::kStatusText);
			const float statusRowW  = (ctx.m_OnClose ? btn + 4.f : 0.f) + btn + 22.f + bs.x + 36.f + ss.x + 30.f;
			const float chipH       = ImGui::GetTextLineHeight() + 10.f;
			float chipsW            = 0.f;
			for (const auto& action : ctx.m_Actions)
				chipsW += ImGui::CalcTextSize(action.m_Label.c_str()).x + 40.f;
			const bool twoRows = !ctx.m_Actions.empty() && right - statusRowW - chipsW < textEnd + 24.f;
			const float midY   = twoRows ? wp.y + 27.f : wp.y + kHeaderHeight * 0.5f;
			const float chipY  = twoRows ? wp.y + kHeaderHeight - chipH * 0.5f - 10.f : midY;

			if (ctx.m_OnClose)
			{
				x -= btn;
				ImGui::SetCursorScreenPos(ImVec2(x, midY - btn * 0.5f));
				if (Theme::HeaderButton("##close", "close", ImVec2(btn, btn), true))
					ctx.m_OnClose();
				x -= 4.f;
			}
			x -= btn;
			ImGui::SetCursorScreenPos(ImVec2(x, midY - btn * 0.5f));
			if (Theme::HeaderButton("##minimize", minimized ? "restore" : "minimize", ImVec2(btn, btn), false))
				minimize = true;
			x -= 22.f;

			x -= bs.x;
			dl->AddText(ImVec2(x, midY - bs.y * 0.5f), Theme::Col(Theme::Text(), 0.85f), build.c_str());
			x -= 18.f;
			dl->AddLine(ImVec2(x, midY - bs.y * 0.5f), ImVec2(x, midY + bs.y * 0.5f), Theme::Col(Theme::Text(), 0.35f), 1.f);
			x -= 18.f;

			x -= ss.x;
			dl->AddText(ImVec2(x, midY - ss.y * 0.5f), Theme::Col(Theme::Text()), Branding::kStatusText);
			const float pulse = 0.55f + 0.45f * std::sin((float)ImGui::GetTime() * 3.f);
			dl->AddCircleFilled(ImVec2(x - 11.f, midY), 7.f * pulse + 2.f, Theme::Col(Theme::Accent(), 0.25f * pulse), 16);
			dl->AddCircleFilled(ImVec2(x - 11.f, midY), 4.f, Theme::Col(Theme::AccentBright()), 16);
			x -= 30.f;

			if (twoRows)
				x = right;
			for (auto it = ctx.m_Actions.rbegin(); it != ctx.m_Actions.rend(); ++it)
			{
				const float w = ImGui::CalcTextSize(it->m_Label.c_str()).x + 30.f;
				x -= w;
				if (x < textEnd + 24.f)
					break; // window too narrow even for the second row
				ImGui::SetCursorScreenPos(ImVec2(x, chipY - chipH * 0.5f));
				ImGui::PushID(&*it);
				if (Theme::Chip(it->m_Label.c_str(), it->m_IsActive && it->m_IsActive()) && it->m_OnClick)
				{
					if (ctx.m_PushTask)
						ctx.m_PushTask(it->m_OnClick);
					else
						it->m_OnClick();
				}
				ImGui::PopID();
				x -= 10.f;
			}

			// header divider with a bright center
			const float hy = wp.y + kHeaderHeight;
			dl->AddRectFilledMultiColor(ImVec2(wp.x + 10.f, hy), ImVec2(wp.x + width * 0.5f, hy + 1.f),
			    Theme::Col(Theme::Accent(), 0.1f), Theme::Col(Theme::Accent(), 0.8f), Theme::Col(Theme::Accent(), 0.8f), Theme::Col(Theme::Accent(), 0.1f));
			dl->AddRectFilledMultiColor(ImVec2(wp.x + width * 0.5f, hy), ImVec2(wp.x + width - 10.f, hy + 1.f),
			    Theme::Col(Theme::Accent(), 0.8f), Theme::Col(Theme::Accent(), 0.1f), Theme::Col(Theme::Accent(), 0.1f), Theme::Col(Theme::Accent(), 0.8f));
			return minimize;
		}
	}

	void SetupStyle()
	{
		YimMenu::Theme::ApplySignatureTheme();
	}

	void SetupFonts(const std::string& iniPath)
	{
		auto& IO = ImGui::GetIO();
		static std::string path;
		if (iniPath.empty())
		{
			char* appdata  = std::getenv("appdata");
			auto file_path = (appdata ? std::filesystem::path(appdata) : std::filesystem::temp_directory_path()) / "RDOTerminator" / "imgui.ini";
			path           = file_path.string();
		}
		else
		{
			path = iniPath;
		}
		IO.IniFilename = path.c_str();
		IO.LogFilename = NULL;
		ImFontConfig FontCfg{};
		FontCfg.FontDataOwnedByAtlas = false;

		Menu::Font::g_DefaultFont = IO.Fonts->AddFontFromMemoryTTF(
		    const_cast<std::uint8_t*>(Fonts::MainFont), sizeof(Fonts::MainFont), 19.0f, &FontCfg);
		Menu::Font::g_OptionsFont = IO.Fonts->AddFontFromMemoryTTF(
		    const_cast<std::uint8_t*>(Fonts::MainFont), sizeof(Fonts::MainFont), 19.0f, &FontCfg);
		Menu::Font::g_ChildTitleFont = IO.Fonts->AddFontFromMemoryTTF(
		    const_cast<std::uint8_t*>(Fonts::MainFont), sizeof(Fonts::MainFont), 19.0f, &FontCfg);
		Menu::Font::g_ChatFont = IO.Fonts->AddFontFromMemoryTTF(
		    const_cast<std::uint8_t*>(Fonts::MainFont), sizeof(Fonts::MainFont), 22.0f, &FontCfg);
		Menu::Font::g_OverlayFont = IO.Fonts->AddFontFromMemoryTTF(
		    const_cast<std::uint8_t*>(Fonts::MainFont), sizeof(Fonts::MainFont), 16.0f, &FontCfg);
		Menu::Font::g_EspFont = IO.Fonts->AddFontDefault();

		// display faces for the shell: condensed DIN-like Bahnschrift ships with Windows 10+
		Theme::g_TitleFont   = LoadDisplayFont("bahnschrift.ttf", 38.0f);
		Theme::g_HeadingFont = LoadDisplayFont("bahnschrift.ttf", 21.0f);
		Theme::g_SmallFont   = LoadDisplayFont("bahnschrift.ttf", 14.0f);

		UIManager::SetOptionsFont(Menu::Font::g_OptionsFont);
	}

	void DrawShell(const ShellContext& ctx)
	{
		if (!ctx.m_IsOpen || !ctx.m_Settings)
			return;

		ImGui::PushFont(Menu::Font::g_DefaultFont);

		static bool firstFrame      = true;
		static bool minimized       = false;
		static float restoreHeight  = 0.f;
		static int lastPosX         = ctx.m_Settings->GetWindowPosX();
		static int lastPosY         = ctx.m_Settings->GetWindowPosY();
		static int lastWidth        = ctx.m_Settings->GetWindowWidth();
		static int lastHeight       = ctx.m_Settings->GetWindowHeight();
		static bool lastMaximized   = ctx.m_Settings->IsWindowMaximized();

		const int screenW = GetSystemMetrics(SM_CXSCREEN);
		const int screenH = GetSystemMetrics(SM_CYSCREEN);
		const int maxW    = std::max(screenW, kMinWidth);
		const int maxH    = std::max(screenH, kMinHeight);

		if (firstFrame)
		{
			// clamp saved geometry to the current screen (hi >= lo even on tiny screens)
			const int width  = std::clamp(ctx.m_Settings->GetWindowWidth(), kMinWidth, maxW);
			const int height = std::clamp(ctx.m_Settings->GetWindowHeight(), kMinHeight, maxH);
			const int posX   = std::clamp(ctx.m_Settings->GetWindowPosX(), 0, std::max(0, screenW - width));
			const int posY   = std::clamp(ctx.m_Settings->GetWindowPosY(), 0, std::max(0, screenH - height));
			ImGui::SetNextWindowPos(ImVec2((float)posX, (float)posY), ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2((float)width, (float)height), ImGuiCond_Always);
		}

		const float minH = minimized ? kHeaderHeight + 8.f : (float)kMinHeight;
		ImGui::SetNextWindowSizeConstraints(ImVec2((float)kMinWidth, minH), ImVec2((float)maxW, minimized ? minH : (float)maxH));

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.f, 16.f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
		const bool window_visible = ImGui::Begin("##RDOTerminator", nullptr,
		    ImGuiWindowFlags_NoTitleBar |
		    ImGuiWindowFlags_NoCollapse |
		    ImGuiWindowFlags_NoScrollbar |
		    ImGuiWindowFlags_NoScrollWithMouse |
		    ImGuiWindowFlags_NoBackground |
		    ImGuiWindowFlags_NoSavedSettings);
		ImGui::PopStyleVar(2);

		if (window_visible)
		{
			ImDrawList* dl   = ImGui::GetWindowDrawList();
			const ImVec2 wp  = ImGui::GetWindowPos();
			const ImVec2 ws  = ImGui::GetWindowSize();
			const ImVec2 wmx(wp.x + ws.x, wp.y + ws.y);

			// body: dark fill, faint red wash at the top, neon cut-corner frame
			Theme::FillChamfered(dl, ImVec2(wp.x + 3.f, wp.y + 3.f), ImVec2(wmx.x - 3.f, wmx.y - 3.f), kFrameCut, Theme::Col(Theme::WindowBg()));
			dl->AddRectFilledMultiColor(ImVec2(wp.x + kFrameCut, wp.y + 4.f), ImVec2(wmx.x - kFrameCut, wp.y + kHeaderHeight),
			    Theme::Col(Theme::Accent(), 0.10f), Theme::Col(Theme::Accent(), 0.02f), Theme::Col(Theme::Accent(), 0.f), Theme::Col(Theme::Accent(), 0.f));
			Theme::DrawNeonFrame(dl, ImVec2(wp.x + 4.f, wp.y + 4.f), ImVec2(wmx.x - 4.f, wmx.y - 4.f), kFrameCut, Theme::Col(Theme::Accent()), 1.f);

			const bool toggleMin = DrawHeader(ctx, wp, ws.x, minimized);

			if (!minimized)
			{
				ImGui::SetCursorScreenPos(ImVec2(wp.x + 20.f, wp.y + kHeaderHeight + 14.f));
				UIManager::Draw();
			}

			if (toggleMin)
			{
				minimized = !minimized;
				if (minimized)
				{
					restoreHeight = ws.y;
					ImGui::SetWindowSize(ImVec2(ws.x, kHeaderHeight + 8.f));
				}
				else
				{
					ImGui::SetWindowSize(ImVec2(ws.x, std::max(restoreHeight, (float)kMinHeight)));
				}
			}

			const int curPosX   = (int)wp.x;
			const int curPosY   = (int)wp.y;
			const int curWidth  = (int)ws.x;
			const int curHeight = minimized ? (int)restoreHeight : (int)ws.y;
			const bool maximized = false;

			if ((firstFrame ||
			    curPosX != lastPosX || curPosY != lastPosY ||
			    curWidth != lastWidth || curHeight != lastHeight ||
			    maximized != lastMaximized)
			    && curWidth >= kMinWidth && curHeight >= kMinHeight)
			{
				// skip degenerate geometry (e.g. while the game is minimized)
				// so reopening the menu doesn't restore a collapsed slit
				ctx.m_Settings->SaveIfWindowChanged(curWidth, curHeight, curPosX, curPosY, maximized);
				lastPosX      = curPosX;
				lastPosY      = curPosY;
				lastWidth     = curWidth;
				lastHeight    = curHeight;
				lastMaximized = maximized;
			}
			firstFrame = false;
		}

		// End() must always be called, even when Begin() returned false (window clipped
		// or not visible). Skipping it corrupts the ImGui stack and every subsequent
		// frame renders broken.
		ImGui::End();

		ImGui::PopFont();
	}
}
