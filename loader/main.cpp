// Session loader: watches for the game process and injects the embedded DLL.
// Borderless, fully custom-drawn (GDI+) window in the same signature look as
// the in-game menu: near-black surfaces, cut corners, neon signal red.
//
// Reskinning for another game/product only needs the Config block below.

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <tlhelp32.h>
#include <objidl.h>
#include <gdiplus.h>

#include <cmath>
#include <string>

#include "res/resource.h"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "winmm.lib")

namespace Config
{
	const wchar_t* kTitlePrimary = L"RDO";
	const wchar_t* kTitleAccent  = L"TERMINATOR";
	const wchar_t* kSubtitle     = L"SESSION LOADER";
	const wchar_t* kWindowName   = L"RDO Terminator";
	const wchar_t* kVersion      = L"v2026.09.28";
	const wchar_t* kTagline      = L"BE AHEAD  /  STAY ALIVE";

	const wchar_t* kGameExe  = L"RDR2.exe";
	const wchar_t* kDllName  = L"RDOTerminator.dll";
	const wchar_t* kTempDir  = L"RDO_Terminator";
	const wchar_t* kMenuKeys = L"F5 / INSERT";

	// palette (mirrors game/frontend/Theme.hpp)
	const Gdiplus::Color kBg(255, 8, 9, 12);
	const Gdiplus::Color kPanel(255, 14, 16, 21);
	const Gdiplus::Color kAccent(255, 235, 26, 33);
	const Gdiplus::Color kAccentHi(255, 255, 69, 74);
	const Gdiplus::Color kText(255, 237, 237, 242);
	const Gdiplus::Color kMuted(255, 143, 148, 161);
	const Gdiplus::Color kOk(255, 76, 204, 102);
	const Gdiplus::Color kWarn(255, 235, 170, 60);
}

namespace
{
	using namespace Gdiplus;

	// ---- layout (logical pixels, scaled by the window DPI) -----------------
	const float kW       = 560.f;
	const float kH       = 380.f;
	const float kCut     = 18.f;
	const float kHeaderH = 88.f;

	const RectF kCloseRect(kW - 24.f - 30.f, 22.f, 30.f, 30.f);
	const RectF kMinRect(kW - 24.f - 64.f, 22.f, 30.f, 30.f);
	const RectF kStatusRect(24.f, 108.f, kW - 48.f, 86.f);
	const RectF kProgressRect(24.f, 210.f, kW - 48.f, 8.f);
	const RectF kButtonRect(24.f, 234.f, kW - 48.f, 54.f);

	const UINT kScanTimer    = 1;
	const UINT kScanPeriodMs = 1000;
	const UINT kAnimTimer    = 2;
	const UINT kAnimPeriodMs = 33;
	const UINT kWmInjectDone = WM_APP + 1;

	enum class LoaderState
	{
		NoGame,    // game not running
		Ready,     // found, can inject
		Injecting, // worker thread is injecting
		Loaded,    // injected into this PID
	};

	enum class Hot
	{
		None,
		Close,
		Minimize,
		Button,
	};

	HINSTANCE g_instance = nullptr;
	HWND g_window        = nullptr;
	float g_scale        = 1.f;

	FontFamily* g_display = nullptr; // Bahnschrift, falls back to Segoe UI
	FontFamily* g_body    = nullptr; // Segoe UI

	LoaderState g_state = LoaderState::NoGame;
	DWORD g_gamePid     = 0;
	DWORD g_injectedPid = 0;
	std::wstring g_error;
	ULONGLONG g_stateSince = 0;

	Hot g_hot     = Hot::None;
	Hot g_pressed = Hot::None;
	bool g_tracking = false;

	// ---- process / injection -----------------------------------------------

	DWORD FindGamePid()
	{
		HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (snap == INVALID_HANDLE_VALUE)
			return 0;

		PROCESSENTRY32W entry{};
		entry.dwSize = sizeof(entry);
		DWORD pid    = 0;
		if (Process32FirstW(snap, &entry))
		{
			do
			{
				if (_wcsicmp(entry.szExeFile, Config::kGameExe) == 0)
				{
					pid = entry.th32ProcessID;
					break;
				}
			} while (Process32NextW(snap, &entry));
		}
		CloseHandle(snap);
		return pid;
	}

	bool IsModuleLoaded(DWORD pid, const std::wstring& path)
	{
		HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
		if (snap == INVALID_HANDLE_VALUE)
			return false;

		// compare file names only: the temp path may be in 8.3 form on our side
		const size_t slash       = path.find_last_of(L"\\/");
		const std::wstring name  = slash == std::wstring::npos ? path : path.substr(slash + 1);

		MODULEENTRY32W entry{};
		entry.dwSize = sizeof(entry);
		bool found   = false;
		if (Module32FirstW(snap, &entry))
		{
			do
			{
				if (_wcsicmp(entry.szModule, name.c_str()) == 0)
				{
					found = true;
					break;
				}
			} while (Module32NextW(snap, &entry));
		}
		CloseHandle(snap);
		return found;
	}

	bool WriteFileAll(const std::wstring& path, const void* data, DWORD size)
	{
		HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE)
			return false;
		DWORD written = 0;
		const bool ok = WriteFile(file, data, size, &written, nullptr) && written == size;
		CloseHandle(file);
		return ok;
	}

	bool ExtractEmbeddedDll(DWORD pid, std::wstring& out_path, std::wstring& error)
	{
		wchar_t temp_dir[MAX_PATH]{};
		if (!GetTempPathW(MAX_PATH, temp_dir))
		{
			error = L"GetTempPath failed.";
			return false;
		}

		const std::wstring dir = std::wstring(temp_dir) + Config::kTempDir;
		CreateDirectoryW(dir.c_str(), nullptr); // ignore failure (may already exist)

		HRSRC res = FindResourceW(g_instance, MAKEINTRESOURCEW(IDR_HACKDLL), RT_RCDATA);
		if (!res)
		{
			error = L"Embedded DLL not found in resources.";
			return false;
		}

		const DWORD size = SizeofResource(g_instance, res);
		const void* data = LockResource(LoadResource(g_instance, res));
		if (!data || !size)
		{
			error = L"Embedded DLL is empty.";
			return false;
		}

		out_path = dir + L"\\" + Config::kDllName;
		if (WriteFileAll(out_path, data, size))
			return true;

		// the default file is still mapped by another game instance: use a per-PID copy
		out_path = dir + L"\\" + std::to_wstring(pid) + L"_" + Config::kDllName;
		if (WriteFileAll(out_path, data, size))
			return true;

		error = L"Cannot write the DLL to the temp folder.";
		return false;
	}

	bool InjectInto(DWORD pid, std::wstring& error)
	{
		std::wstring dll_path;
		if (!ExtractEmbeddedDll(pid, dll_path, error))
			return false;

		HANDLE process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid);
		if (!process)
		{
			error = L"Cannot open the game process. Run the loader as administrator.";
			return false;
		}

		const auto load_library = reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"));
		if (!load_library)
		{
			error = L"LoadLibraryW not found.";
			CloseHandle(process);
			return false;
		}

		const SIZE_T bytes = (dll_path.size() + 1) * sizeof(wchar_t);
		void* remote_mem   = VirtualAllocEx(process, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (!remote_mem)
		{
			error = L"VirtualAllocEx failed.";
			CloseHandle(process);
			return false;
		}

		bool ok          = false;
		bool may_free    = true;
		if (WriteProcessMemory(process, remote_mem, dll_path.c_str(), bytes, nullptr))
		{
			HANDLE thread = CreateRemoteThread(process, nullptr, 0, load_library, remote_mem, 0, nullptr);
			if (thread)
			{
				if (WaitForSingleObject(thread, 15000) == WAIT_OBJECT_0)
				{
					// the exit code is a truncated HMODULE; confirm via the module list
					DWORD exit_code = 0;
					GetExitCodeThread(thread, &exit_code);
					ok = exit_code != 0 || IsModuleLoaded(pid, dll_path);
					if (!ok)
						error = L"The game refused to load the DLL.";
				}
				else
				{
					// the remote thread may still read the path: never free it under it
					may_free = false;
					error    = L"Injection timed out.";
				}
				CloseHandle(thread);
			}
			else
			{
				error = L"CreateRemoteThread failed. Run the loader as administrator.";
			}
		}
		else
		{
			error = L"WriteProcessMemory failed.";
		}

		if (may_free)
			VirtualFreeEx(process, remote_mem, 0, MEM_RELEASE);
		CloseHandle(process);
		return ok;
	}

	struct InjectJob
	{
		HWND window;
		DWORD pid;
	};

	DWORD WINAPI InjectThreadProc(LPVOID param)
	{
		auto* job = static_cast<InjectJob*>(param);
		std::wstring error;
		const bool ok = InjectInto(job->pid, error);

		// each result owns its own error string; the UI thread deletes it
		auto* message = ok ? nullptr : new std::wstring(error.empty() ? L"Injection failed." : error);
		if (!PostMessageW(job->window, kWmInjectDone, ok ? 1 : 0, reinterpret_cast<LPARAM>(message)))
			delete message; // window already gone
		delete job;
		return 0;
	}

	void SetState(LoaderState state)
	{
		if (g_state != state)
			g_stateSince = GetTickCount64();
		g_state = state;
		InvalidateRect(g_window, nullptr, FALSE);
	}

	void StartInjection()
	{
		if (g_state != LoaderState::Ready)
			return;

		g_error.clear();
		SetState(LoaderState::Injecting);
		auto* job  = new InjectJob{g_window, g_gamePid};
		HANDLE thr = CreateThread(nullptr, 0, InjectThreadProc, job, 0, nullptr);
		if (thr)
		{
			CloseHandle(thr);
		}
		else
		{
			delete job;
			g_error = L"Cannot start the worker thread.";
			SetState(LoaderState::Ready);
		}
	}

	void ScanForGame()
	{
		const DWORD pid = FindGamePid();
		if (pid == 0)
		{
			// game gone: reset so the next launch can be injected again
			// (an injection in flight reports back on its own)
			if (g_state != LoaderState::NoGame && g_state != LoaderState::Injecting)
			{
				g_gamePid     = 0;
				g_injectedPid = 0;
				g_error.clear();
				SetState(LoaderState::NoGame);
			}
		}
		else if (g_state == LoaderState::NoGame || (g_state == LoaderState::Loaded && pid != g_injectedPid))
		{
			// fresh launch (or first sight): arm the button again
			g_gamePid = pid;
			g_error.clear();
			SetState(LoaderState::Ready);
		}
		else if (g_state == LoaderState::Ready && pid != g_gamePid)
		{
			g_gamePid = pid;
			g_error.clear();
			InvalidateRect(g_window, nullptr, FALSE);
		}
	}

	// ---- drawing -------------------------------------------------------------

	Color WithAlpha(const Color& c, float a)
	{
		return Color((BYTE)(c.GetA() * (a < 0.f ? 0.f : (a > 1.f ? 1.f : a))), c.GetR(), c.GetG(), c.GetB());
	}

	void ChamferPath(GraphicsPath& path, const RectF& r, float cut)
	{
		const PointF pts[8] = {
		    {r.X, r.Y + cut}, {r.X + cut, r.Y}, {r.GetRight() - cut, r.Y}, {r.GetRight(), r.Y + cut},
		    {r.GetRight(), r.GetBottom() - cut}, {r.GetRight() - cut, r.GetBottom()}, {r.X + cut, r.GetBottom()}, {r.X, r.GetBottom() - cut}};
		path.AddLines(pts, 8);
		path.CloseFigure();
	}

	void FillChamfered(Graphics& g, const RectF& r, float cut, const Brush& brush)
	{
		GraphicsPath path;
		ChamferPath(path, r, cut);
		g.FillPath(&brush, &path);
	}

	void NeonFrame(Graphics& g, const RectF& r, float cut, const Color& c, float glow)
	{
		GraphicsPath path;
		ChamferPath(path, r, cut);
		for (int i = 3; i >= 1; i--)
		{
			Pen pen(WithAlpha(c, 0.08f * glow * (4 - i)), 1.5f + i * 3.f);
			pen.SetLineJoin(LineJoinRound);
			g.DrawPath(&pen, &path);
		}
		Pen core(c, 1.5f);
		g.DrawPath(&core, &path);
	}

	void DrawEmblem(Graphics& g, PointF c, float size, const Color& col)
	{
		const float r = size * 0.5f;
		SolidBrush brush(col);
		auto wing = [&](float s, float dy) {
			const PointF left[4]  = {{c.X - r * s, c.Y + (-0.5f + dy) * r * s}, {c.X - 0.58f * r * s, c.Y + (-0.5f + dy) * r * s}, {c.X, c.Y + (0.08f + dy) * r * s}, {c.X, c.Y + (0.55f + dy) * r * s}};
			const PointF right[4] = {{c.X, c.Y + (0.55f + dy) * r * s}, {c.X, c.Y + (0.08f + dy) * r * s}, {c.X + 0.58f * r * s, c.Y + (-0.5f + dy) * r * s}, {c.X + r * s, c.Y + (-0.5f + dy) * r * s}};
			g.FillPolygon(&brush, left, 4);
			g.FillPolygon(&brush, right, 4);
		};
		wing(1.f, -0.25f);
		wing(0.62f, 0.55f);
	}

	float MeasureWidth(Graphics& g, const wchar_t* text, const Font& font)
	{
		RectF bounds;
		g.MeasureString(text, -1, &font, PointF(0.f, 0.f), StringFormat::GenericTypographic(), &bounds);
		return bounds.Width;
	}

	void DrawStr(Graphics& g, const wchar_t* text, const Font& font, PointF p, const Color& c)
	{
		SolidBrush brush(c);
		g.DrawString(text, -1, &font, p, StringFormat::GenericTypographic(), &brush);
	}

	void DrawSpaced(Graphics& g, const wchar_t* text, const Font& font, PointF p, const Color& c, float spacing)
	{
		SolidBrush brush(c);
		for (const wchar_t* ch = text; *ch; ch++)
		{
			g.DrawString(ch, 1, &font, p, StringFormat::GenericTypographic(), &brush);
			wchar_t one[2] = {*ch, 0};
			p.X += (*ch == L' ' ? font.GetSize() * 0.3f : MeasureWidth(g, one, font)) + spacing;
		}
	}

	void DrawCentered(Graphics& g, const wchar_t* text, const Font& font, const RectF& r, const Color& c)
	{
		StringFormat fmt;
		fmt.SetAlignment(StringAlignmentCenter);
		fmt.SetLineAlignment(StringAlignmentCenter);
		SolidBrush brush(c);
		g.DrawString(text, -1, &font, r, &fmt, &brush);
	}

	void DrawHeaderButton(Graphics& g, const RectF& r, bool close, bool hot)
	{
		if (hot)
		{
			SolidBrush bg(close ? WithAlpha(Config::kAccent, 0.35f) : Color(24, 255, 255, 255));
			g.FillRectangle(&bg, r);
		}
		Pen pen(hot ? Color(255, 255, 255, 255) : WithAlpha(Config::kText, 0.85f), 2.f);
		const float cx = r.X + r.Width * 0.5f, cy = r.Y + r.Height * 0.5f, s = r.Height * 0.2f;
		if (close)
		{
			g.DrawLine(&pen, cx - s, cy - s, cx + s, cy + s);
			g.DrawLine(&pen, cx - s, cy + s, cx + s, cy - s);
		}
		else
		{
			g.DrawLine(&pen, cx - s, cy + s * 0.5f, cx + s, cy + s * 0.5f);
		}
	}

	struct StatusView
	{
		std::wstring title;
		std::wstring detail;
		std::wstring button;
		Color dot;
		bool buttonEnabled;
	};

	StatusView DescribeState()
	{
		StatusView v{};
		switch (g_state)
		{
		case LoaderState::NoGame:
			v = {L"WAITING FOR GAME", std::wstring(L"Start ") + Config::kGameExe + L" - it will be detected automatically.", L"WAITING FOR GAME", Config::kMuted, false};
			break;
		case LoaderState::Ready:
			if (!g_error.empty())
				v = {L"INJECTION FAILED", g_error, L"RETRY", Config::kAccentHi, true};
			else
				v = {L"GAME DETECTED", std::wstring(Config::kGameExe) + L"  ·  PID " + std::to_wstring(g_gamePid) + L"  ·  wait for the main menu", L"LOAD", Config::kOk, true};
			break;
		case LoaderState::Injecting:
			v = {L"INJECTING", std::wstring(L"Loading ") + Config::kDllName + L" into PID " + std::to_wstring(g_gamePid) + L"...", L"INJECTING...", Config::kWarn, false};
			break;
		case LoaderState::Loaded:
			v = {L"LOADED", std::wstring(L"Open the menu in game with ") + Config::kMenuKeys + L".", L"LOADED", Config::kOk, false};
			break;
		}
		return v;
	}

	void Paint(Graphics& g)
	{
		const float t = (float)(GetTickCount64() % 100000ULL) / 1000.f;
		const StatusView view = DescribeState();

		g.SetSmoothingMode(SmoothingModeAntiAlias);
		g.SetPixelOffsetMode(PixelOffsetModeHalf);
		g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
		g.Clear(Color(255, 0, 0, 0));

		// body + top wash + neon frame
		const RectF body(1.f, 1.f, kW - 2.f, kH - 2.f);
		SolidBrush bg(Config::kBg);
		FillChamfered(g, body, kCut, bg);
		{
			LinearGradientBrush wash(RectF(0.f, 0.f, kW, kHeaderH), WithAlpha(Config::kAccent, 0.16f), WithAlpha(Config::kAccent, 0.f), LinearGradientModeForwardDiagonal);
			GraphicsPath path;
			ChamferPath(path, RectF(1.f, 1.f, kW - 2.f, kHeaderH), kCut);
			g.FillPath(&wash, &path);
		}
		NeonFrame(g, RectF(4.f, 4.f, kW - 8.f, kH - 8.f), kCut - 2.f, Config::kAccent, 1.f);

		// logo tile
		const RectF logo(24.f, 18.f, 52.f, 52.f);
		SolidBrush tile(Color(255, 16, 5, 7));
		FillChamfered(g, logo, 10.f, tile);
		DrawEmblem(g, PointF(logo.X + logo.Width * 0.5f, logo.Y + logo.Height * 0.5f + 2.f), 32.f, Config::kAccentHi);
		NeonFrame(g, logo, 10.f, Config::kAccent, 0.7f);

		// title: primary run + accent run with a soft glow
		Font title(g_display, 30.f, FontStyleBold, UnitPixel);
		Font smallFont(g_display, 12.f, FontStyleRegular, UnitPixel);
		const PointF tp(92.f, 16.f);
		DrawStr(g, Config::kTitlePrimary, title, tp, Config::kText);
		const float primaryW = MeasureWidth(g, Config::kTitlePrimary, title) + 9.f;
		for (int i = 0; i < 4; i++)
			DrawStr(g, Config::kTitleAccent, title, PointF(tp.X + primaryW + ((i & 1) ? 1.5f : -1.5f), tp.Y + ((i & 2) ? 1.5f : -1.5f)), WithAlpha(Config::kAccent, 0.16f));
		DrawStr(g, Config::kTitleAccent, title, PointF(tp.X + primaryW, tp.Y), Config::kAccent);
		DrawSpaced(g, Config::kSubtitle, smallFont,PointF(93.f, 54.f), WithAlpha(Config::kText, 0.72f), 2.5f);

		DrawHeaderButton(g, kMinRect, false, g_hot == Hot::Minimize);
		DrawHeaderButton(g, kCloseRect, true, g_hot == Hot::Close);

		// header divider, bright in the middle
		{
			LinearGradientBrush l(RectF(10.f, kHeaderH, kW * 0.5f - 10.f, 1.f), WithAlpha(Config::kAccent, 0.1f), WithAlpha(Config::kAccent, 0.85f), LinearGradientModeHorizontal);
			g.FillRectangle(&l, RectF(10.f, kHeaderH, kW * 0.5f - 10.f, 1.f));
			LinearGradientBrush r(RectF(kW * 0.5f, kHeaderH, kW * 0.5f - 10.f, 1.f), WithAlpha(Config::kAccent, 0.85f), WithAlpha(Config::kAccent, 0.1f), LinearGradientModeHorizontal);
			g.FillRectangle(&r, RectF(kW * 0.5f, kHeaderH, kW * 0.5f - 10.f, 1.f));
		}

		// status card
		{
			SolidBrush panel(Config::kPanel);
			GraphicsPath path;
			ChamferPath(path, kStatusRect, 8.f);
			g.FillPath(&panel, &path);
			Pen border(WithAlpha(Config::kAccent, 0.45f), 1.f);
			g.DrawPath(&border, &path);
			SolidBrush edge(Config::kAccentHi);
			g.FillRectangle(&edge, RectF(kStatusRect.X, kStatusRect.Y + 10.f, 2.f, kStatusRect.Height - 20.f));

			// slashes + title
			SolidBrush red(Config::kAccent);
			for (int k = 0; k < 2; k++)
			{
				const float x0 = kStatusRect.X + 16.f + k * 7.f, y0 = kStatusRect.Y + 16.f, h = 20.f;
				const PointF quad[4] = {{x0 + h * 0.32f, y0 + 2.f}, {x0 + h * 0.47f, y0 + 2.f}, {x0 + h * 0.15f, y0 + h - 2.f}, {x0, y0 + h - 2.f}};
				g.FillPolygon(&red, quad, 4);
			}
			Font state(g_display, 20.f, FontStyleBold, UnitPixel);
			DrawStr(g, view.title.c_str(), state, PointF(kStatusRect.X + 44.f, kStatusRect.Y + 15.f), Config::kText);

			// pulsing status dot, right aligned
			const float pulse = 0.55f + 0.45f * std::sin(t * 3.f);
			const PointF dot(kStatusRect.GetRight() - 24.f, kStatusRect.Y + 26.f);
			SolidBrush halo(WithAlpha(view.dot, 0.25f * pulse));
			const float hr = 5.f + 5.f * pulse;
			g.FillEllipse(&halo, dot.X - hr, dot.Y - hr, hr * 2.f, hr * 2.f);
			SolidBrush core(view.dot);
			g.FillEllipse(&core, dot.X - 4.5f, dot.Y - 4.5f, 9.f, 9.f);

			Font detail(g_body, 13.f, FontStyleRegular, UnitPixel);
			StringFormat fmt;
			fmt.SetTrimming(StringTrimmingEllipsisCharacter);
			SolidBrush muted(g_state == LoaderState::Ready && !g_error.empty() ? Config::kAccentHi : Config::kMuted);
			g.DrawString(view.detail.c_str(), -1, &detail, RectF(kStatusRect.X + 44.f, kStatusRect.Y + 46.f, kStatusRect.Width - 60.f, 34.f), &fmt, &muted);
		}

		// progress bar
		{
			SolidBrush track(Color(255, 22, 24, 30));
			g.FillRectangle(&track, kProgressRect);
			const float since = (float)(GetTickCount64() - g_stateSince) / 1000.f;
			RectF fill = kProgressRect;
			Color fillCol = Config::kAccent;
			switch (g_state)
			{
			case LoaderState::NoGame:
			{
				// slow scanning sweep
				const float span = kProgressRect.Width * 0.18f;
				const float pos  = std::fmod(t * 0.35f, 1.f) * (kProgressRect.Width + span) - span;
				fill.X     = kProgressRect.X + (pos < 0.f ? 0.f : pos);
				fill.Width = (pos < 0.f ? span + pos : span);
				if (fill.GetRight() > kProgressRect.GetRight())
					fill.Width = kProgressRect.GetRight() - fill.X;
				fillCol = WithAlpha(Config::kAccent, 0.55f);
				break;
			}
			case LoaderState::Ready:
				fill.Width = kProgressRect.Width * 0.35f;
				break;
			case LoaderState::Injecting:
				// eases toward 90% while the worker runs
				fill.Width = kProgressRect.Width * (0.35f + 0.55f * (1.f - std::exp(-since * 1.5f)));
				break;
			case LoaderState::Loaded:
				fillCol = Config::kOk;
				break;
			}
			if (fill.Width > 0.f)
			{
				SolidBrush glow(WithAlpha(fillCol, 0.25f));
				g.FillRectangle(&glow, RectF(fill.X, fill.Y - 3.f, fill.Width, fill.Height + 6.f));
				LinearGradientBrush bar(RectF(fill.X - 1.f, fill.Y, fill.Width + 2.f, fill.Height), WithAlpha(fillCol, 0.7f), fillCol, LinearGradientModeHorizontal);
				g.FillRectangle(&bar, fill);
			}
		}

		// main button
		{
			const bool enabled = view.buttonEnabled;
			const bool hot     = enabled && g_hot == Hot::Button;
			const bool down    = hot && g_pressed == Hot::Button;
			GraphicsPath path;
			ChamferPath(path, kButtonRect, 12.f);
			if (enabled)
			{
				const Color top    = down ? Color(255, 150, 16, 22) : (hot ? Config::kAccentHi : Config::kAccent);
				const Color bottom = down ? Color(255, 110, 10, 16) : Color(255, 150, 14, 20);
				LinearGradientBrush fill(kButtonRect, top, bottom, LinearGradientModeVertical);
				g.FillPath(&fill, &path);
				NeonFrame(g, kButtonRect, 12.f, Config::kAccentHi, hot ? 1.f : 0.6f);
			}
			else
			{
				SolidBrush fill(g_state == LoaderState::Loaded ? Color(255, 12, 30, 18) : Color(255, 20, 22, 28));
				g.FillPath(&fill, &path);
				Pen border(g_state == LoaderState::Loaded ? WithAlpha(Config::kOk, 0.7f) : Color(40, 255, 255, 255), 1.f);
				g.DrawPath(&border, &path);
			}

			Font label(g_display, 18.f, FontStyleBold, UnitPixel);
			const Color textCol = enabled ? Color(255, 255, 255, 255) : (g_state == LoaderState::Loaded ? Config::kOk : Config::kMuted);
			DrawCentered(g, view.button.c_str(), label, kButtonRect, textCol);
		}

		// hint
		{
			Font hint(g_body, 12.5f, FontStyleRegular, UnitPixel);
			SolidBrush muted(Config::kMuted);
			const std::wstring text = std::wstring(L"Inject at the main menu, not in-game. Menu key in game: ") + Config::kMenuKeys +
			                          L". Closing the game resets the loader for the next launch.";
			g.DrawString(text.c_str(), -1, &hint, RectF(24.f, 300.f, kW - 48.f, 36.f), nullptr, &muted);
		}

		// footer
		{
			Pen line(WithAlpha(Config::kAccent, 0.35f), 1.f);
			g.DrawLine(&line, 24.f, kH - 36.f, kW - 24.f, kH - 36.f);
			DrawSpaced(g, Config::kTagline, smallFont, PointF(26.f, kH - 28.f), WithAlpha(Config::kMuted, 0.9f), 2.f);
			const float vw = MeasureWidth(g, Config::kVersion, smallFont);
			DrawStr(g, Config::kVersion, smallFont,PointF(kW - 24.f - vw, kH - 28.f), WithAlpha(Config::kMuted, 0.9f));
		}
	}

	// ---- window plumbing -----------------------------------------------------

	PointF ToLogical(LPARAM lparam)
	{
		return PointF((float)GET_X_LPARAM(lparam) / g_scale, (float)GET_Y_LPARAM(lparam) / g_scale);
	}

	Hot HitTest(PointF p)
	{
		if (kCloseRect.Contains(p))
			return Hot::Close;
		if (kMinRect.Contains(p))
			return Hot::Minimize;
		if (kButtonRect.Contains(p))
			return Hot::Button;
		return Hot::None;
	}

	void ApplyWindowShape(HWND hwnd)
	{
		// real cut corners: clip the window to the chamfered outline
		const float s   = g_scale;
		const int w     = (int)std::lround(kW * s);
		const int h     = (int)std::lround(kH * s);
		const int c     = (int)std::lround(kCut * s);
		const POINT pts[8] = {{0, c}, {c, 0}, {w - c, 0}, {w, c}, {w, h - c}, {w - c, h}, {c, h}, {0, h - c}};
		HRGN rgn = CreatePolygonRgn(pts, 8, WINDING);
		SetWindowRgn(hwnd, rgn, TRUE); // the system owns rgn from here on
	}

	void UpdateScale(HWND hwnd)
	{
		const UINT dpi = GetDpiForWindow(hwnd);
		g_scale        = dpi ? dpi / 96.f : 1.f;
	}

	LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		switch (msg)
		{
		case WM_CREATE:
			g_window = hwnd;
			UpdateScale(hwnd);
			SetWindowPos(hwnd, nullptr, 0, 0, (int)std::lround(kW * g_scale), (int)std::lround(kH * g_scale), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
			ApplyWindowShape(hwnd);
			g_stateSince = GetTickCount64();
			SetTimer(hwnd, kScanTimer, kScanPeriodMs, nullptr);
			SetTimer(hwnd, kAnimTimer, kAnimPeriodMs, nullptr);
			ScanForGame();
			return 0;

		case WM_DPICHANGED:
		{
			g_scale             = HIWORD(wparam) / 96.f;
			const RECT* suggest = reinterpret_cast<const RECT*>(lparam);
			SetWindowPos(hwnd, nullptr, suggest->left, suggest->top, (int)std::lround(kW * g_scale), (int)std::lround(kH * g_scale), SWP_NOZORDER | SWP_NOACTIVATE);
			ApplyWindowShape(hwnd);
			InvalidateRect(hwnd, nullptr, FALSE);
			return 0;
		}

		case WM_TIMER:
			if (wparam == kScanTimer)
				ScanForGame();
			else if (wparam == kAnimTimer && !IsIconic(hwnd))
				InvalidateRect(hwnd, nullptr, FALSE);
			return 0;

		case WM_NCHITTEST:
		{
			POINT pt{GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
			ScreenToClient(hwnd, &pt);
			const PointF p((float)pt.x / g_scale, (float)pt.y / g_scale);
			if (HitTest(p) == Hot::None && p.Y < kHeaderH)
				return HTCAPTION; // drag the window by its header
			return HTCLIENT;
		}

		case WM_MOUSEMOVE:
		{
			if (!g_tracking)
			{
				TRACKMOUSEEVENT tme{sizeof(tme), TME_LEAVE, hwnd, 0};
				g_tracking = TrackMouseEvent(&tme) != FALSE;
			}
			const Hot hot = HitTest(ToLogical(lparam));
			if (hot != g_hot)
			{
				g_hot = hot;
				SetCursor(LoadCursorW(nullptr, hot == Hot::None ? IDC_ARROW : IDC_HAND));
				InvalidateRect(hwnd, nullptr, FALSE);
			}
			return 0;
		}

		case WM_SETCURSOR:
			if (LOWORD(lparam) == HTCLIENT)
			{
				SetCursor(LoadCursorW(nullptr, g_hot == Hot::None ? IDC_ARROW : IDC_HAND));
				return TRUE;
			}
			break;

		case WM_MOUSELEAVE:
			g_tracking = false;
			g_hot      = Hot::None;
			InvalidateRect(hwnd, nullptr, FALSE);
			return 0;

		case WM_LBUTTONDOWN:
			g_pressed = HitTest(ToLogical(lparam));
			if (g_pressed != Hot::None)
				SetCapture(hwnd);
			InvalidateRect(hwnd, nullptr, FALSE);
			return 0;

		case WM_LBUTTONUP:
		{
			const Hot released = HitTest(ToLogical(lparam));
			const Hot pressed  = g_pressed;
			g_pressed          = Hot::None;
			ReleaseCapture();
			if (released == pressed)
			{
				if (pressed == Hot::Close)
					DestroyWindow(hwnd);
				else if (pressed == Hot::Minimize)
					ShowWindow(hwnd, SW_MINIMIZE);
				else if (pressed == Hot::Button)
					StartInjection();
			}
			InvalidateRect(hwnd, nullptr, FALSE);
			return 0;
		}

		case WM_KEYDOWN:
			if (wparam == VK_RETURN || wparam == VK_SPACE)
				StartInjection();
			else if (wparam == VK_ESCAPE)
				DestroyWindow(hwnd);
			return 0;

		case WM_ERASEBKGND:
			return 1;

		case WM_PAINT:
		{
			PAINTSTRUCT ps{};
			HDC dc = BeginPaint(hwnd, &ps);
			RECT rc{};
			GetClientRect(hwnd, &rc);

			// double buffered: draw everything off-screen, then blit once
			HDC mem       = CreateCompatibleDC(dc);
			HBITMAP bmp   = CreateCompatibleBitmap(dc, rc.right, rc.bottom);
			HGDIOBJ old   = SelectObject(mem, bmp);
			{
				Graphics g(mem);
				g.ScaleTransform(g_scale, g_scale);
				Paint(g);
			}
			BitBlt(dc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
			SelectObject(mem, old);
			DeleteObject(bmp);
			DeleteDC(mem);

			EndPaint(hwnd, &ps);
			return 0;
		}

		case kWmInjectDone:
		{
			auto* message = reinterpret_cast<std::wstring*>(lparam);
			if (wparam == 1)
			{
				g_injectedPid = g_gamePid;
				g_error.clear();
				SetState(LoaderState::Loaded);
				PlaySoundW(MAKEINTRESOURCEW(IDR_SUCCESS), g_instance, SND_RESOURCE | SND_ASYNC | SND_NODEFAULT);
			}
			else
			{
				g_error = message ? *message : L"Injection failed.";
				// the game may have exited while we were injecting
				SetState(FindGamePid() == 0 ? LoaderState::NoGame : LoaderState::Ready);
				if (g_state == LoaderState::NoGame)
					g_error.clear();
			}
			delete message;
			return 0;
		}

		case WM_DESTROY:
			KillTimer(hwnd, kScanTimer);
			KillTimer(hwnd, kAnimTimer);
			PostQuitMessage(0);
			return 0;
		}
		return DefWindowProcW(hwnd, msg, wparam, lparam);
	}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int show)
{
	g_instance = instance;

	// crisp rendering on scaled displays; everything is laid out in logical px
	if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
		SetProcessDPIAware();

	GdiplusStartupInput gdiplusInput;
	ULONG_PTR gdiplusToken = 0;
	if (GdiplusStartup(&gdiplusToken, &gdiplusInput, nullptr) != Ok)
		return 1;

	g_display = new FontFamily(L"Bahnschrift");
	if (g_display->GetLastStatus() != Ok)
	{
		delete g_display;
		g_display = new FontFamily(L"Segoe UI");
	}
	g_body = new FontFamily(L"Segoe UI");

	WNDCLASSEXW cls{};
	cls.cbSize        = sizeof(cls);
	cls.style         = CS_DROPSHADOW;
	cls.lpfnWndProc   = WindowProc;
	cls.hInstance     = instance;
	cls.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
	cls.hbrBackground = nullptr;
	cls.lpszClassName = L"RdoTerminatorLoader";
	cls.hIcon         = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APPICON));
	cls.hIconSm       = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APPICON));
	RegisterClassExW(&cls);

	// centered on the primary work area; WM_CREATE resizes for the real DPI
	RECT work{0, 0, 1280, 720};
	SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
	const int x = work.left + (work.right - work.left - (int)kW) / 2;
	const int y = work.top + (work.bottom - work.top - (int)kH) / 2;

	// WS_POPUP: fully custom chrome. WS_MINIMIZEBOX + WS_SYSMENU keep taskbar
	// minimize/restore and Alt+F4 working.
	HWND window = CreateWindowExW(WS_EX_APPWINDOW, cls.lpszClassName, Config::kWindowName,
	    WS_POPUP | WS_MINIMIZEBOX | WS_SYSMENU,
	    x, y, (int)kW, (int)kH, nullptr, nullptr, instance, nullptr);
	if (!window)
		return 1;

	ShowWindow(window, show);
	UpdateWindow(window);

	MSG msg{};
	while (GetMessageW(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	delete g_display;
	delete g_body;
	GdiplusShutdown(gdiplusToken);
	return (int)msg.wParam;
}
