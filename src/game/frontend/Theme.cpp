#include "Theme.hpp"

#include "imgui_internal.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>

namespace YimMenu::Theme
{
	namespace
	{
		constexpr float kPi = 3.14159265358979f;

		ImVec4 Lerp(const ImVec4& a, const ImVec4& b, float t)
		{
			return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
		}

		ImFont* HeadingFont()
		{
			return g_HeadingFont ? g_HeadingFont : ImGui::GetFont();
		}

		// smooth 0..1 animation value stored per widget id
		float Animate(ImGuiID id, float target, float speed = 14.f)
		{
			float* t = ImGui::GetStateStorage()->GetFloatRef(id, target);
			*t += (target - *t) * ImMin(1.f, ImGui::GetIO().DeltaTime * speed);
			if (std::fabs(target - *t) < 0.002f)
				*t = target;
			return *t;
		}

		// cut-corner polygon, clockwise; corners bitmask: 1 TL, 2 TR, 4 BR, 8 BL
		int ChamferPoints(ImVec2* out, ImVec2 mn, ImVec2 mx, float cut, int corners)
		{
			int n = 0;
			if (corners & 1) { out[n++] = {mn.x, mn.y + cut}; out[n++] = {mn.x + cut, mn.y}; }
			else out[n++] = mn;
			if (corners & 2) { out[n++] = {mx.x - cut, mn.y}; out[n++] = {mx.x, mn.y + cut}; }
			else out[n++] = {mx.x, mn.y};
			if (corners & 4) { out[n++] = {mx.x, mx.y - cut}; out[n++] = {mx.x - cut, mx.y}; }
			else out[n++] = mx;
			if (corners & 8) { out[n++] = {mn.x + cut, mx.y}; out[n++] = {mn.x, mx.y - cut}; }
			else out[n++] = {mn.x, mx.y};
			return n;
		}

		void DrawUser(ImDrawList* dl, ImVec2 c, float r, ImU32 col)
		{
			dl->AddCircleFilled({c.x, c.y - r * 0.45f}, r * 0.32f, col, 20);
			dl->PathArcTo({c.x, c.y + r * 0.95f}, r * 0.75f, kPi, 2.f * kPi, 20);
			dl->PathFillConvex(col);
		}

		bool StartsWord(const std::string& hay, const char* needle)
		{
			for (size_t pos = hay.find(needle); pos != std::string::npos; pos = hay.find(needle, pos + 1))
				if (pos == 0 || !std::isalpha((unsigned char)hay[pos - 1]))
					return true;
			return false;
		}
	}

	void ApplySignatureTheme()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4* colors    = style.Colors;

		style.WindowPadding    = ImVec2(16.f, 14.f);
		style.FramePadding     = ImVec2(10.f, 6.f);
		style.CellPadding      = ImVec2(14.f, 4.f);
		style.ItemSpacing      = ImVec2(10.f, 8.f);
		style.ItemInnerSpacing = ImVec2(8.f, 6.f);
		style.IndentSpacing    = 18.f;
		style.ScrollbarSize    = 10.f;
		style.GrabMinSize      = 10.f;

		style.WindowRounding    = 6.f;
		style.ChildRounding     = 6.f;
		style.FrameRounding     = 4.f;
		style.PopupRounding     = 6.f;
		style.ScrollbarRounding = 6.f;
		style.GrabRounding      = 4.f;
		style.TabRounding       = 4.f;
		style.WindowBorderSize  = 1.f;
		style.ChildBorderSize   = 1.f;
		style.FrameBorderSize   = 1.f;
		style.PopupBorderSize   = 1.f;
		style.WindowTitleAlign  = ImVec2(0.5f, 0.5f);

		const ImVec4 accent = Accent();
		const ImVec4 bright = AccentBright();
		const ImVec4 dim    = AccentDim();

		colors[ImGuiCol_Text]                  = Text();
		colors[ImGuiCol_TextDisabled]          = MutedText();
		colors[ImGuiCol_WindowBg]              = WindowBg();
		colors[ImGuiCol_ChildBg]               = ImVec4(0.f, 0.f, 0.f, 0.f);
		colors[ImGuiCol_PopupBg]               = ImVec4(0.043f, 0.047f, 0.063f, 0.98f);
		colors[ImGuiCol_Border]                = ImVec4(accent.x, accent.y, accent.z, 0.35f);
		colors[ImGuiCol_BorderShadow]          = ImVec4(0.f, 0.f, 0.f, 0.f);
		colors[ImGuiCol_FrameBg]               = ImVec4(0.075f, 0.082f, 0.102f, 1.f);
		colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.110f, 0.090f, 0.105f, 1.f);
		colors[ImGuiCol_FrameBgActive]         = ImVec4(0.160f, 0.070f, 0.085f, 1.f);
		colors[ImGuiCol_TitleBg]               = PanelBg();
		colors[ImGuiCol_TitleBgActive]         = ImVec4(0.12f, 0.03f, 0.04f, 1.f);
		colors[ImGuiCol_TitleBgCollapsed]      = PanelBg();
		colors[ImGuiCol_MenuBarBg]             = PanelBg();
		colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.f, 0.f, 0.f, 0.f);
		colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.20f, 0.21f, 0.25f, 1.f);
		colors[ImGuiCol_ScrollbarGrabHovered]  = dim;
		colors[ImGuiCol_ScrollbarGrabActive]   = accent;
		colors[ImGuiCol_CheckMark]             = bright;
		colors[ImGuiCol_SliderGrab]            = accent;
		colors[ImGuiCol_SliderGrabActive]      = bright;
		colors[ImGuiCol_Button]                = ImVec4(0.075f, 0.082f, 0.102f, 1.f);
		colors[ImGuiCol_ButtonHovered]         = ImVec4(0.30f, 0.05f, 0.07f, 1.f);
		colors[ImGuiCol_ButtonActive]          = ImVec4(0.55f, 0.07f, 0.09f, 1.f);
		colors[ImGuiCol_Header]                = ImVec4(accent.x, accent.y, accent.z, 0.22f);
		colors[ImGuiCol_HeaderHovered]         = ImVec4(accent.x, accent.y, accent.z, 0.32f);
		colors[ImGuiCol_HeaderActive]          = ImVec4(accent.x, accent.y, accent.z, 0.50f);
		colors[ImGuiCol_Separator]             = ImVec4(accent.x, accent.y, accent.z, 0.30f);
		colors[ImGuiCol_SeparatorHovered]      = ImVec4(accent.x, accent.y, accent.z, 0.60f);
		colors[ImGuiCol_SeparatorActive]       = accent;
		colors[ImGuiCol_ResizeGrip]            = ImVec4(accent.x, accent.y, accent.z, 0.25f);
		colors[ImGuiCol_ResizeGripHovered]     = ImVec4(accent.x, accent.y, accent.z, 0.60f);
		colors[ImGuiCol_ResizeGripActive]      = accent;
		colors[ImGuiCol_Tab]                   = ImVec4(0.075f, 0.082f, 0.102f, 1.f);
		colors[ImGuiCol_TabHovered]            = ImVec4(0.30f, 0.05f, 0.07f, 1.f);
		colors[ImGuiCol_TabSelected]           = ImVec4(0.40f, 0.05f, 0.07f, 1.f);
		colors[ImGuiCol_TabSelectedOverline]   = bright;
		colors[ImGuiCol_TabDimmed]             = ImVec4(0.06f, 0.065f, 0.08f, 1.f);
		colors[ImGuiCol_TabDimmedSelected]     = ImVec4(0.20f, 0.05f, 0.06f, 1.f);
		colors[ImGuiCol_PlotLines]             = accent;
		colors[ImGuiCol_PlotLinesHovered]      = bright;
		colors[ImGuiCol_PlotHistogram]         = accent;
		colors[ImGuiCol_PlotHistogramHovered]  = bright;
		colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.075f, 0.082f, 0.102f, 1.f);
		colors[ImGuiCol_TableBorderStrong]     = ImVec4(accent.x, accent.y, accent.z, 0.35f);
		colors[ImGuiCol_TableBorderLight]      = ImVec4(1.f, 1.f, 1.f, 0.07f);
		colors[ImGuiCol_TableRowBg]            = ImVec4(0.f, 0.f, 0.f, 0.f);
		colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.f, 1.f, 1.f, 0.025f);
		colors[ImGuiCol_TextSelectedBg]        = ImVec4(accent.x, accent.y, accent.z, 0.40f);
		colors[ImGuiCol_DragDropTarget]        = bright;
		colors[ImGuiCol_NavHighlight]          = ImVec4(accent.x, accent.y, accent.z, 0.80f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.f, 1.f, 1.f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.8f, 0.8f, 0.8f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.01f, 0.01f, 0.02f, 0.70f);
	}

	// ---- drawing primitives -------------------------------------------------

	void DrawIcon(ImDrawList* dl, std::string_view icon, ImVec2 c, float size, ImU32 col)
	{
		const float r = size * 0.5f;
		const float t = std::max(1.3f, size * 0.09f);
		auto P = [&](float x, float y) {
			return ImVec2(c.x + x * r, c.y + y * r);
		};

		if (icon == "gear")
		{
			for (int i = 0; i < 8; i++)
			{
				const float a = i * kPi / 4.f;
				dl->AddLine(P(std::cos(a) * 0.5f, std::sin(a) * 0.5f), P(std::cos(a) * 0.95f, std::sin(a) * 0.95f), col, r * 0.32f);
			}
			dl->AddCircleFilled(c, r * 0.68f, col, 24);
			dl->AddCircleFilled(c, r * 0.28f, IM_COL32(0, 0, 0, 200), 16);
		}
		else if (icon == "user")
		{
			DrawUser(dl, c, r, col);
		}
		else if (icon == "users")
		{
			DrawUser(dl, P(0.38f, -0.05f), r * 0.72f, (col & 0x00FFFFFF) | (((col >> 24) * 6 / 10) << 24));
			DrawUser(dl, P(-0.25f, 0.05f), r * 0.8f, col);
		}
		else if (icon == "eye")
		{
			dl->AddBezierQuadratic(P(-1.f, 0.f), P(0.f, -0.95f), P(1.f, 0.f), col, t, 16);
			dl->AddBezierQuadratic(P(-1.f, 0.f), P(0.f, 0.95f), P(1.f, 0.f), col, t, 16);
			dl->AddCircleFilled(c, r * 0.3f, col, 16);
		}
		else if (icon == "crosshair")
		{
			dl->AddCircle(c, r * 0.62f, col, 24, t);
			dl->AddLine(P(0.f, -1.f), P(0.f, -0.3f), col, t);
			dl->AddLine(P(0.f, 0.3f), P(0.f, 1.f), col, t);
			dl->AddLine(P(-1.f, 0.f), P(-0.3f, 0.f), col, t);
			dl->AddLine(P(0.3f, 0.f), P(1.f, 0.f), col, t);
			dl->AddCircleFilled(c, r * 0.12f, col, 10);
		}
		else if (icon == "shield")
		{
			dl->PathLineTo(P(0.f, -0.95f));
			dl->PathLineTo(P(0.8f, -0.62f));
			dl->PathLineTo(P(0.75f, 0.1f));
			dl->PathBezierCubicCurveTo(P(0.7f, 0.55f), P(0.3f, 0.8f), P(0.f, 0.98f));
			dl->PathBezierCubicCurveTo(P(-0.3f, 0.8f), P(-0.7f, 0.55f), P(-0.75f, 0.1f));
			dl->PathLineTo(P(-0.8f, -0.62f));
			dl->PathStroke(col, ImDrawFlags_Closed, t);
			const ImVec2 check[3] = {P(-0.32f, 0.02f), P(-0.06f, 0.3f), P(0.36f, -0.26f)};
			dl->AddPolyline(check, 3, col, ImDrawFlags_None, t);
		}
		else if (icon == "globe")
		{
			dl->AddCircle(c, r * 0.9f, col, 28, t);
			dl->AddEllipse(c, ImVec2(r * 0.4f, r * 0.9f), col, 0.f, 24, t);
			dl->AddLine(P(-0.9f, 0.f), P(0.9f, 0.f), col, t);
			dl->AddLine(P(-0.72f, -0.5f), P(0.72f, -0.5f), col, t * 0.8f);
			dl->AddLine(P(-0.72f, 0.5f), P(0.72f, 0.5f), col, t * 0.8f);
		}
		else if (icon == "wheel")
		{
			dl->AddCircle(c, r * 0.85f, col, 28, t * 1.2f);
			dl->AddCircleFilled(c, r * 0.2f, col, 12);
			for (int i = 0; i < 6; i++)
			{
				const float a = i * kPi / 3.f;
				dl->AddLine(P(std::cos(a) * 0.2f, std::sin(a) * 0.2f), P(std::cos(a) * 0.85f, std::sin(a) * 0.85f), col, t * 0.8f);
			}
		}
		else if (icon == "coin")
		{
			dl->AddCircle(c, r * 0.88f, col, 28, t);
			dl->AddCircle(c, r * 0.58f, col, 24, t * 0.8f);
			dl->AddLine(P(0.f, -0.35f), P(0.f, 0.35f), col, t * 1.2f);
		}
		else if (icon == "info")
		{
			dl->AddCircle(c, r * 0.9f, col, 28, t);
			dl->AddCircleFilled(P(0.f, -0.42f), r * 0.12f, col, 10);
			dl->AddLine(P(0.f, -0.12f), P(0.f, 0.5f), col, t * 1.3f);
		}
		else if (icon == "bug")
		{
			for (float y : {-0.1f, 0.22f, 0.54f})
			{
				dl->AddLine(P(-0.35f, y), P(-0.85f, y - 0.18f), col, t);
				dl->AddLine(P(0.35f, y), P(0.85f, y - 0.18f), col, t);
			}
			dl->AddEllipseFilled(P(0.f, 0.2f), ImVec2(r * 0.42f, r * 0.62f), col, 0.f, 20);
			dl->AddCircleFilled(P(0.f, -0.58f), r * 0.26f, col, 14);
		}
		else if (icon == "monitor")
		{
			dl->AddRect(P(-0.92f, -0.75f), P(0.92f, 0.42f), col, r * 0.12f, 0, t);
			dl->AddLine(P(0.f, 0.42f), P(0.f, 0.78f), col, t);
			dl->AddLine(P(-0.45f, 0.8f), P(0.45f, 0.8f), col, t);
		}
		else if (icon == "network")
		{
			const ImVec2 a = P(0.f, -0.55f), b = P(-0.62f, 0.52f), d = P(0.62f, 0.52f);
			dl->AddLine(a, b, col, t);
			dl->AddLine(a, d, col, t);
			dl->AddLine(b, d, col, t);
			dl->AddCircleFilled(a, r * 0.24f, col, 12);
			dl->AddCircleFilled(b, r * 0.24f, col, 12);
			dl->AddCircleFilled(d, r * 0.24f, col, 12);
		}
		else if (icon == "layers")
		{
			dl->AddQuadFilled(P(0.f, -0.85f), P(0.95f, -0.38f), P(0.f, 0.1f), P(-0.95f, -0.38f), col);
			const ImVec2 mid[3] = {P(-0.95f, 0.05f), P(0.f, 0.52f), P(0.95f, 0.05f)};
			dl->AddPolyline(mid, 3, col, ImDrawFlags_None, t * 1.2f);
			const ImVec2 low[3] = {P(-0.95f, 0.45f), P(0.f, 0.92f), P(0.95f, 0.45f)};
			dl->AddPolyline(low, 3, col, ImDrawFlags_None, t * 1.2f);
		}
		else if (icon == "keyboard")
		{
			dl->AddRect(P(-0.95f, -0.55f), P(0.95f, 0.55f), col, r * 0.12f, 0, t);
			for (int row = 0; row < 2; row++)
				for (int k = 0; k < 4; k++)
					dl->AddRectFilled(P(-0.62f + k * 0.4f - 0.1f, -0.28f + row * 0.3f - 0.08f), P(-0.62f + k * 0.4f + 0.1f, -0.28f + row * 0.3f + 0.08f), col);
			dl->AddLine(P(-0.45f, 0.3f), P(0.45f, 0.3f), col, t);
		}
		else if (icon == "emblem")
		{
			// two stacked chevrons, built from convex quads
			auto wing = [&](float s, float dy, ImU32 cc) {
				const ImVec2 left[4]  = {P(-1.f * s, (-0.5f + dy) * s), P(-0.58f * s, (-0.5f + dy) * s), P(0.f, (0.08f + dy) * s), P(0.f, (0.55f + dy) * s)};
				const ImVec2 right[4] = {P(0.f, (0.55f + dy) * s), P(0.f, (0.08f + dy) * s), P(0.58f * s, (-0.5f + dy) * s), P(1.f * s, (-0.5f + dy) * s)};
				dl->AddConvexPolyFilled(left, 4, cc);
				dl->AddConvexPolyFilled(right, 4, cc);
			};
			wing(1.f, -0.25f, col);
			wing(0.62f, 0.55f, col);
		}
		else
		{
			dl->AddCircleFilled(c, r * 0.26f, col, 12);
		}
	}

	std::string_view IconForName(std::string_view name)
	{
		std::string s(name);
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) {
			return (char)std::tolower(ch);
		});

		static constexpr std::pair<const char*, const char*> table[] = {
		    {"players", "users"}, {"ped", "users"}, {"player", "user"}, {"self", "user"}, {"companion", "users"},
		    {"visual", "eye"}, {"esp", "eye"}, {"weapon", "crosshair"}, {"combat", "crosshair"}, {"aim", "crosshair"},
		    {"vehicle", "wheel"}, {"train", "wheel"}, {"world", "globe"}, {"teleport", "globe"}, {"network", "network"},
		    {"session", "network"}, {"protect", "shield"}, {"recovery", "coin"}, {"money", "coin"}, {"interface", "monitor"},
		    {"gui", "monitor"}, {"overlay", "layers"}, {"context", "layers"}, {"setting", "gear"}, {"misc", "gear"},
		    {"hotkey", "keyboard"}, {"about", "info"}, {"info", "info"}, {"debug", "bug"},
		};
		for (const auto& [key, icon] : table)
			if (StartsWord(s, key))
				return icon;
		return {};
	}

	void FillChamfered(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float cut, ImU32 col)
	{
		ImVec2 pts[8];
		const int n = ChamferPoints(pts, mn, mx, cut, 15);
		dl->AddConvexPolyFilled(pts, n, col);
	}

	void DrawNeonFrame(ImDrawList* dl, ImVec2 mn, ImVec2 mx, float cut, ImU32 col, float glow)
	{
		ImVec2 pts[8];
		const int n       = ChamferPoints(pts, mn, mx, cut, 15);
		const ImVec4 base = ImGui::ColorConvertU32ToFloat4(col);
		if (glow > 0.f)
		{
			for (int i = 3; i >= 1; i--)
				dl->AddPolyline(pts, n, Col(base, 0.07f * glow * (4 - i)), ImDrawFlags_Closed, 1.5f + i * 3.f);
		}
		dl->AddPolyline(pts, n, col, ImDrawFlags_Closed, 1.5f);
	}

	void DrawSlashes(ImDrawList* dl, ImVec2 pos, float h, ImU32 col)
	{
		for (int k = 0; k < 2; k++)
		{
			const float x0 = pos.x + k * h * 0.34f;
			dl->AddQuadFilled({x0 + h * 0.32f, pos.y + h * 0.1f}, {x0 + h * 0.47f, pos.y + h * 0.1f}, {x0 + h * 0.15f, pos.y + h * 0.9f}, {x0, pos.y + h * 0.9f}, col);
		}
	}

	float CalcSpacedTextWidth(const char* text, float spacing, ImFont* font, float fontSize)
	{
		if (!font)
			font = ImGui::GetFont();
		if (fontSize <= 0.f)
			fontSize = font->FontSize;
		float w = 0.f;
		for (const char* p = text; *p; p++)
			w += font->CalcTextSizeA(fontSize, FLT_MAX, 0.f, p, p + 1).x + (p[1] ? spacing : 0.f);
		return w;
	}

	void DrawSpacedText(ImDrawList* dl, ImVec2 pos, ImU32 col, const char* text, float spacing, ImFont* font, float fontSize)
	{
		if (!font)
			font = ImGui::GetFont();
		if (fontSize <= 0.f)
			fontSize = font->FontSize;
		for (const char* p = text; *p; p++)
		{
			dl->AddText(font, fontSize, pos, col, p, p + 1);
			pos.x += font->CalcTextSizeA(fontSize, FLT_MAX, 0.f, p, p + 1).x + spacing;
		}
	}

	// ---- widgets ------------------------------------------------------------

	bool Switch(const char* label, bool* v)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g       = *GImGui;
		const ImGuiID id      = window->GetID(label);
		const char* labelEnd  = ImGui::FindRenderedTextEnd(label);
		const ImVec2 textSize = ImGui::CalcTextSize(label, labelEnd);
		const float h         = IM_FLOOR(g.FontSize * 1.05f);
		const float w         = IM_FLOOR(h * 1.9f);
		const float gap       = textSize.x > 0.f ? g.Style.ItemInnerSpacing.x * 1.6f : 0.f;
		const float rowH      = ImMax(h, textSize.y);

		const ImVec2 pos = window->DC.CursorPos;
		const ImRect bb(pos, ImVec2(pos.x + w + gap + textSize.x, pos.y + rowH));
		ImGui::ItemSize(bb);
		if (!ImGui::ItemAdd(bb, id))
			return false;

		bool hovered = false, held = false;
		const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
		if (pressed)
		{
			*v = !*v;
			ImGui::MarkItemEdited(id);
		}

		const float t = Animate(id, *v ? 1.f : 0.f);
		ImDrawList* dl = window->DrawList;
		const ImVec2 tMin(pos.x, pos.y + (rowH - h) * 0.5f);
		const ImVec2 tMax(tMin.x + w, tMin.y + h);

		if (t > 0.f)
			dl->AddRectFilled({tMin.x - 3.f, tMin.y - 3.f}, {tMax.x + 3.f, tMax.y + 3.f}, Col(Accent(), 0.16f * t), h);
		const ImVec4 offTrack(0.15f, 0.16f, 0.19f, 1.f);
		dl->AddRectFilled(tMin, tMax, Col(Lerp(offTrack, Accent(), t)), h * 0.5f);
		dl->AddRect(tMin, tMax, Col(Lerp(ImVec4(1.f, 1.f, 1.f, hovered ? 0.18f : 0.08f), AccentBright(), t), t > 0.f ? 0.8f : 1.f), h * 0.5f, 0, 1.f);

		const float knobR = h * 0.5f - 2.5f + (hovered ? 0.6f : 0.f);
		const float cx    = ImLerp(tMin.x + h * 0.5f, tMax.x - h * 0.5f, t);
		const ImVec2 kc(cx, tMin.y + h * 0.5f);
		dl->AddCircleFilled({kc.x, kc.y + 1.f}, knobR, IM_COL32(0, 0, 0, 90), 20);
		dl->AddCircleFilled(kc, knobR, Col(Lerp(ImVec4(0.62f, 0.63f, 0.67f, 1.f), ImVec4(1.f, 1.f, 1.f, 1.f), t)), 20);

		if (textSize.x > 0.f)
			ImGui::RenderText(ImVec2(tMax.x + gap, pos.y + (rowH - textSize.y) * 0.5f), label, labelEnd);
		return pressed;
	}

	void SameLineValueColumn()
	{
		const float start = ImGui::GetItemRectMin().x;
		ImGui::SameLine();
		const float target = start + ImGui::GetFontSize() * 11.5f;
		const ImVec2 cur   = ImGui::GetCursorScreenPos();
		if (cur.x < target)
			ImGui::SetCursorScreenPos(ImVec2(target, cur.y));
	}

	bool ColorSwatch(const char* id, float* rgba)
	{
		const float sz = IM_FLOOR(ImGui::GetFontSize() * 1.1f);
		bool changed   = false;

		ImGui::PushID(id);
		if (ImGui::InvisibleButton("##swatch", ImVec2(sz, sz)))
			ImGui::OpenPopup("##picker");
		const bool hovered = ImGui::IsItemHovered();
		const ImVec2 mn    = ImGui::GetItemRectMin();
		const ImVec2 mx    = ImGui::GetItemRectMax();
		ImDrawList* dl     = ImGui::GetWindowDrawList();
		const ImVec4 c(rgba[0], rgba[1], rgba[2], 1.f);
		dl->AddRect({mn.x - 1.f, mn.y - 1.f}, {mx.x + 1.f, mx.y + 1.f}, Col(c, 0.30f), 5.f, 0, 2.f);
		dl->AddRectFilled(mn, mx, Col(c), 4.f);
		dl->AddRect(mn, mx, IM_COL32(255, 255, 255, hovered ? 150 : 40), 4.f, 0, 1.f);
		if (hovered)
			ImGui::SetTooltip("Click to change color");

		if (ImGui::BeginPopup("##picker"))
		{
			ImGui::SetNextItemWidth(ImGui::GetFontSize() * 12.f);
			changed = ImGui::ColorPicker4("##p", rgba, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_PickerHueBar);
			ImGui::EndPopup();
		}
		ImGui::PopID();
		return changed;
	}

	bool Stepper(const char* label, const std::vector<std::pair<int, const char*>>& list, int* value)
	{
		if (list.empty())
			return false;

		ImGui::PushID(label);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label, ImGui::FindRenderedTextEnd(label));
		SameLineValueColumn();

		int idx = 0;
		for (int i = 0; i < (int)list.size(); i++)
			if (list[i].first == *value)
				idx = i;

		const float h      = ImGui::GetFrameHeight();
		const float arrowW = h;
		float textW        = 0.f;
		for (const auto& el : list)
			textW = ImMax(textW, ImGui::CalcTextSize(el.second).x);
		const float w = ImMax(textW + arrowW * 2.f + 24.f, ImGui::GetFontSize() * 9.f);

		const ImVec2 p     = ImGui::GetCursorScreenPos();
		const bool clicked = ImGui::InvisibleButton("##stepper", ImVec2(w, h));
		const bool hovered = ImGui::IsItemHovered();
		const float mx     = ImGui::GetIO().MousePos.x;
		const int zone     = !hovered ? 0 : (mx < p.x + arrowW ? -1 : (mx > p.x + w - arrowW ? 1 : 2));

		bool changed = false;
		if (clicked)
		{
			if (zone == -1 || zone == 1)
			{
				idx     = (idx + zone + (int)list.size()) % (int)list.size();
				*value  = list[idx].first;
				changed = true;
			}
			else
			{
				ImGui::OpenPopup("##list");
			}
		}

		ImDrawList* dl = ImGui::GetWindowDrawList();
		const ImVec2 q(p.x + w, p.y + h);
		dl->AddRectFilled(p, q, Col(ImVec4(0.06f, 0.065f, 0.085f, 1.f)), 4.f);
		dl->AddRect(p, q, hovered ? Col(Accent(), 0.8f) : IM_COL32(255, 255, 255, 40), 4.f, 0, 1.f);

		auto chevron = [&](float cx, float dir, bool hot) {
			const float s = h * 0.16f;
			const ImVec2 pts[3] = {{cx + s * 0.5f * dir, p.y + h * 0.5f - s}, {cx - s * 0.5f * dir, p.y + h * 0.5f}, {cx + s * 0.5f * dir, p.y + h * 0.5f + s}};
			dl->AddPolyline(pts, 3, hot ? Col(AccentBright()) : Col(Text(), 0.85f), ImDrawFlags_None, 1.8f);
		};
		chevron(p.x + arrowW * 0.5f, 1.f, zone == -1);
		chevron(q.x - arrowW * 0.5f, -1.f, zone == 1);

		const char* cur    = list[idx].second;
		const ImVec2 ts    = ImGui::CalcTextSize(cur);
		dl->AddText(ImVec2(p.x + (w - ts.x) * 0.5f, p.y + (h - ts.y) * 0.5f), zone == 2 ? Col(AccentBright()) : Col(Text()), cur);

		ImGui::SetNextWindowPos(ImVec2(p.x, q.y + 2.f));
		ImGui::SetNextWindowSizeConstraints(ImVec2(w, 0.f), ImVec2(FLT_MAX, FLT_MAX));
		if (ImGui::BeginPopup("##list"))
		{
			for (int i = 0; i < (int)list.size(); i++)
			{
				if (ImGui::Selectable(list[i].second, i == idx))
				{
					*value  = list[i].first;
					changed = true;
				}
				if (i == idx)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndPopup();
		}

		ImGui::PopID();
		return changed;
	}

	bool BeginCard(const char* title, const void* id)
	{
		ImGui::PushID(id ? id : (const void*)title);
		ImGui::PushStyleColor(ImGuiCol_ChildBg, CardBg());
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.f);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.f, 12.f));
		const bool visible = ImGui::BeginChild("##card", ImVec2(0.f, 0.f),
		    ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding,
		    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor();

		if (title && *title)
		{
			ImDrawList* dl   = ImGui::GetWindowDrawList();
			ImFont* hf       = HeadingFont();
			const float hs   = hf->FontSize;
			const ImVec2 p   = ImGui::GetCursorScreenPos();
			const char* end  = ImGui::FindRenderedTextEnd(title);

			DrawSlashes(dl, p, hs, Col(Accent()));
			float x = p.x + hs * 0.95f;
			const std::string_view icon = IconForName(std::string_view(title, end - title));
			if (!icon.empty())
			{
				DrawIcon(dl, icon, ImVec2(x + hs * 0.45f, p.y + hs * 0.5f), hs * 0.82f, Col(Text()));
				x += hs * 1.25f;
			}
			dl->AddText(hf, hs, ImVec2(x, p.y), Col(Text()), title, end);
			ImGui::Dummy(ImVec2(1.f, hs));

			// full-bleed divider under the header, fading out to the right
			ImGuiWindow* win = ImGui::GetCurrentWindow();
			const float y    = ImGui::GetCursorScreenPos().y;
			dl->PushClipRect(win->Pos, ImVec2(win->Pos.x + win->Size.x, win->Pos.y + win->Size.y), true);
			dl->AddRectFilledMultiColor(ImVec2(win->Pos.x + 1.f, y), ImVec2(win->Pos.x + win->Size.x - 1.f, y + 1.f),
			    Col(Accent(), 0.55f), Col(Accent(), 0.04f), Col(Accent(), 0.04f), Col(Accent(), 0.55f));
			dl->PopClipRect();
			ImGui::Dummy(ImVec2(1.f, 6.f));
		}
		return visible;
	}

	void EndCard()
	{
		ImGuiWindow* win = ImGui::GetCurrentWindow();
		ImDrawList* dl   = win->DrawList;
		const ImVec2 mn(win->Pos.x + 0.5f, win->Pos.y + 0.5f);
		const ImVec2 mx(win->Pos.x + win->Size.x - 0.5f, win->Pos.y + win->Size.y - 0.5f);
		dl->PushClipRect(win->Pos, ImVec2(win->Pos.x + win->Size.x, win->Pos.y + win->Size.y), true);
		dl->AddRect(mn, mx, Col(Accent(), 0.45f), 6.f, 0, 1.f);
		// brighter accent along the left edge
		dl->AddRectFilledMultiColor(ImVec2(mn.x, mn.y + 6.f), ImVec2(mn.x + 2.f, mx.y - 6.f),
		    Col(AccentBright(), 0.9f), Col(AccentBright(), 0.9f), Col(Accent(), 0.1f), Col(Accent(), 0.1f));
		dl->PopClipRect();

		ImGui::EndChild();
		ImGui::PopID();
		ImGui::Dummy(ImVec2(1.f, 4.f));
	}

	bool SidebarEntry(const char* label, std::string_view icon, bool active, float height)
	{
		ImGui::PushID(label);
		const ImVec2 p     = ImGui::GetCursorScreenPos();
		const float w      = ImGui::GetContentRegionAvail().x;
		const bool clicked = ImGui::InvisibleButton("##entry", ImVec2(w, height));
		const bool hovered = ImGui::IsItemHovered();
		const float t      = Animate(ImGui::GetItemID(), active ? 1.f : (hovered ? 0.35f : 0.f), 16.f);
		ImGui::PopID();

		ImDrawList* dl = ImGui::GetWindowDrawList();
		const ImVec2 q(p.x + w, p.y + height);
		if (t > 0.f)
		{
			dl->AddRectFilledMultiColor(p, q, Col(Accent(), 0.34f * t), Col(Accent(), 0.f), Col(Accent(), 0.f), Col(Accent(), 0.34f * t));
			if (active)
			{
				dl->AddRectFilled(p, ImVec2(p.x + 3.f, q.y), Col(AccentBright()));
				dl->AddRectFilled(p, ImVec2(p.x + 9.f, q.y), Col(Accent(), 0.25f));
			}
		}

		const ImU32 col = active ? Col(AccentBright()) : Col(Text(), hovered ? 1.f : 0.78f);
		ImFont* hf      = HeadingFont();
		const float hs  = hf->FontSize;
		DrawIcon(dl, icon.empty() ? "dot" : icon, ImVec2(p.x + 30.f, p.y + height * 0.5f), hs * 1.05f, col);
		dl->AddText(hf, hs, ImVec2(p.x + 56.f, p.y + (height - hs) * 0.5f), col, label, ImGui::FindRenderedTextEnd(label));
		return clicked;
	}

	bool TabButton(const char* label, std::string_view icon, bool active)
	{
		ImFont* hf         = HeadingFont();
		const float hs     = hf->FontSize;
		const char* end    = ImGui::FindRenderedTextEnd(label);
		const ImVec2 ts    = hf->CalcTextSizeA(hs, FLT_MAX, 0.f, label, end);
		const float iconW  = icon.empty() ? 0.f : hs * 1.35f;
		const float h      = IM_FLOOR(hs + 20.f);
		const float w      = IM_FLOOR(ts.x + iconW + 44.f);
		const float cut    = 8.f;

		ImGui::PushID(label);
		const ImVec2 p     = ImGui::GetCursorScreenPos();
		const bool clicked = ImGui::InvisibleButton("##tab", ImVec2(w, h));
		const bool hovered = ImGui::IsItemHovered();
		const float t      = Animate(ImGui::GetItemID(), active ? 1.f : 0.f, 16.f);
		ImGui::PopID();

		ImDrawList* dl = ImGui::GetWindowDrawList();
		const ImVec2 q(p.x + w, p.y + h);
		FillChamfered(dl, p, q, cut, Col(Lerp(hovered ? ImVec4(0.09f, 0.08f, 0.10f, 1.f) : ImVec4(0.065f, 0.072f, 0.09f, 1.f), ImVec4(0.20f, 0.025f, 0.035f, 1.f), t)));
		if (t > 0.f)
		{
			dl->AddRectFilledMultiColor(ImVec2(p.x + 1.f, p.y + h * 0.35f), ImVec2(q.x - 1.f, q.y - 1.f),
			    Col(Accent(), 0.f), Col(Accent(), 0.f), Col(Accent(), 0.30f * t), Col(Accent(), 0.30f * t));
			DrawNeonFrame(dl, p, q, cut, Col(Accent(), t), 0.6f * t);
			dl->AddRectFilled(ImVec2(p.x + cut, q.y - 3.f), ImVec2(q.x - cut, q.y), Col(AccentBright(), t));
			dl->AddRectFilled(ImVec2(p.x + cut, q.y), ImVec2(q.x - cut, q.y + 4.f), Col(Accent(), 0.35f * t));
		}
		else
		{
			ImVec2 pts[8];
			const int n = ChamferPoints(pts, p, q, cut, 15);
			dl->AddPolyline(pts, n, hovered ? Col(Accent(), 0.55f) : IM_COL32(255, 255, 255, 22), ImDrawFlags_Closed, 1.f);
		}

		const ImU32 col = active ? IM_COL32(255, 255, 255, 255) : Col(Text(), hovered ? 0.95f : 0.72f);
		float x         = p.x + 22.f;
		if (!icon.empty())
		{
			DrawIcon(dl, icon, ImVec2(x + hs * 0.5f, p.y + h * 0.5f), hs * 1.0f, active ? Col(AccentBright()) : col);
			x += iconW;
		}
		dl->AddText(hf, hs, ImVec2(x, p.y + (h - ts.y) * 0.5f), col, label, end);
		return clicked;
	}

	bool HeaderButton(const char* id, std::string_view glyph, ImVec2 size, bool danger)
	{
		const ImVec2 p     = ImGui::GetCursorScreenPos();
		const bool clicked = ImGui::InvisibleButton(id, size);
		const bool hovered = ImGui::IsItemHovered();
		ImDrawList* dl     = ImGui::GetWindowDrawList();
		const ImVec2 q(p.x + size.x, p.y + size.y);
		const ImVec2 c((p.x + q.x) * 0.5f, (p.y + q.y) * 0.5f);
		if (hovered)
			dl->AddRectFilled(p, q, danger ? Col(Accent(), 0.35f) : IM_COL32(255, 255, 255, 18), 4.f);

		const ImU32 col = hovered ? IM_COL32(255, 255, 255, 255) : Col(Text(), 0.85f);
		const float s   = size.y * 0.22f;
		if (glyph == "close")
		{
			dl->AddLine({c.x - s, c.y - s}, {c.x + s, c.y + s}, col, 2.2f);
			dl->AddLine({c.x - s, c.y + s}, {c.x + s, c.y - s}, col, 2.2f);
		}
		else if (glyph == "minimize")
		{
			dl->AddLine({c.x - s, c.y + s * 0.4f}, {c.x + s, c.y + s * 0.4f}, col, 2.2f);
		}
		else if (glyph == "restore")
		{
			dl->AddRect({c.x - s, c.y - s}, {c.x + s, c.y + s}, col, 1.f, 0, 1.8f);
		}
		else
		{
			DrawIcon(dl, glyph, c, size.y * 0.62f, col);
		}
		return clicked;
	}

	bool Chip(const char* label, bool active)
	{
		const char* end    = ImGui::FindRenderedTextEnd(label);
		const ImVec2 ts    = ImGui::CalcTextSize(label, end);
		const float h      = IM_FLOOR(ts.y + 10.f);
		const float w      = IM_FLOOR(ts.x + 30.f);
		const ImVec2 p     = ImGui::GetCursorScreenPos();
		const bool clicked = ImGui::InvisibleButton(label, ImVec2(w, h));
		const bool hovered = ImGui::IsItemHovered();
		ImDrawList* dl     = ImGui::GetWindowDrawList();
		const ImVec2 q(p.x + w, p.y + h);

		if (active)
		{
			dl->AddRectFilled({p.x - 3.f, p.y - 3.f}, {q.x + 3.f, q.y + 3.f}, Col(Accent(), 0.18f), h);
			dl->AddRectFilled(p, q, Col(hovered ? AccentBright() : Accent()), h * 0.5f);
		}
		else
		{
			dl->AddRectFilled(p, q, hovered ? Col(Accent(), 0.20f) : Col(Accent(), 0.06f), h * 0.5f);
			dl->AddRect(p, q, Col(Accent(), 0.8f), h * 0.5f, 0, 1.f);
		}
		dl->AddCircleFilled({p.x + 11.f, p.y + h * 0.5f}, 3.f, active ? IM_COL32(255, 255, 255, 255) : Col(Accent()), 10);
		dl->AddText({p.x + 19.f, p.y + (h - ts.y) * 0.5f}, active ? IM_COL32(255, 255, 255, 255) : Col(Text()), label, end);
		return clicked;
	}

	void DrawSectionTitle(const char* title, const char* desc)
	{
		const float h = ImGui::GetFontSize();
		DrawSlashes(ImGui::GetWindowDrawList(), ImGui::GetCursorScreenPos(), h, Col(Accent()));
		ImGui::Dummy(ImVec2(h * 0.85f, h));
		ImGui::SameLine(0.f, 4.f);
		ImGui::TextUnformatted(title);
		if (desc)
		{
			ImGui::SameLine(0.f, 8.f);
			ImGui::PushStyleColor(ImGuiCol_Text, MutedText());
			ImGui::TextUnformatted(desc);
			ImGui::PopStyleColor();
		}
	}
}
