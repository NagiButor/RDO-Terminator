// MenuVisualTest: standalone harness that renders the REAL menu shell and the
// REAL ESP draw code against a scripted fake scene. No RDR2, no injection,
// no game backend: entity data, camera projection and the task queue are faked.
#include "MockEspWorld.hpp"
#include "EspLabPanel.hpp"
#include "stubs/TestTasks.hpp"

#include "game/frontend/Menu.hpp"
#include "game/frontend/ESP.hpp"
#include "game/frontend/submenus/Settings.hpp"
#include "core/commands/BoolCommand.hpp"
#include "core/commands/Commands.hpp"
#include "core/commands/HotkeySystem.hpp"
#include "core/frontend/manager/UIManager.hpp"
#include "core/settings/Settings.hpp"
#include "game/backend/FiberPool.hpp"

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

#include <chrono>
#include <d3d11.h>
#include <filesystem>
#include <string>
#include <tchar.h>

using YimMenu::operator""_J;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace
{
	ID3D11Device* g_Device                = nullptr;
	ID3D11DeviceContext* g_Context       = nullptr;
	IDXGISwapChain* g_SwapChain          = nullptr;
	ID3D11RenderTargetView* g_MainRTV    = nullptr;
	bool g_SwapChainOccluded             = false;
	UINT g_ResizeWidth                   = 0;
	UINT g_ResizeHeight                  = 0;
	HWND g_Hwnd                          = nullptr;
	bool g_MenuOpen                      = false;
	bool g_GodMode                       = false;
	YimMenu::Submenus::Settings g_SettingsInstance;

	bool CreateDeviceD3D(HWND hwnd)
	{
		DXGI_SWAP_CHAIN_DESC sd{};
		sd.BufferCount                        = 2;
		sd.BufferDesc.Width                   = 0;
		sd.BufferDesc.Height                  = 0;
		sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator   = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow                       = hwnd;
		sd.SampleDesc.Count                   = 1;
		sd.SampleDesc.Quality                 = 0;
		sd.Windowed                           = TRUE;
		sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

		const D3D_FEATURE_LEVEL featureLevels[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
		D3D_FEATURE_LEVEL featureLevel           = {};
		if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevels, 2,
		        D3D11_SDK_VERSION, &sd, &g_SwapChain, &g_Device, &featureLevel, &g_Context)
		    != S_OK)
			return false;

		ID3D11Texture2D* backBuffer = nullptr;
		g_SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
		g_Device->CreateRenderTargetView(backBuffer, nullptr, &g_MainRTV);
		backBuffer->Release();
		return true;
	}

	void CleanupDeviceD3D()
	{
		if (g_MainRTV)
		{
			g_MainRTV->Release();
			g_MainRTV = nullptr;
		}
		if (g_SwapChain)
		{
			g_SwapChain->Release();
			g_SwapChain = nullptr;
		}
		if (g_Context)
		{
			g_Context->Release();
			g_Context = nullptr;
		}
		if (g_Device)
		{
			g_Device->Release();
			g_Device = nullptr;
		}
	}

	void CreateRenderTarget()
	{
		ID3D11Texture2D* backBuffer = nullptr;
		g_SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
		g_Device->CreateRenderTargetView(backBuffer, nullptr, &g_MainRTV);
		backBuffer->Release();
	}

	void CleanupRenderTarget()
	{
		if (g_MainRTV)
		{
			g_MainRTV->Release();
			g_MainRTV = nullptr;
		}
	}

	LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
			return true;

		switch (msg)
		{
		case WM_SIZE:
			if (wparam == SIZE_MINIMIZED)
				return 0;
			g_ResizeWidth  = (UINT)LOWORD(lparam);
			g_ResizeHeight = (UINT)HIWORD(lparam);
			return 0;
		case WM_SYSCOMMAND:
			if ((wparam & 0xfff0) == SC_KEYMENU) // disable ALT menu
				return 0;
			break;
		case WM_DESTROY:
			::PostQuitMessage(0);
			return 0;
		}
		return ::DefWindowProcW(hwnd, msg, wparam, lparam);
	}

	// Fake 3D backdrop: sky fill, horizon and a projected ground grid, plus a
	// faint body capsule where the scripted actor stands. The real ESP overlay
	// is drawn on top of this, exactly like over the game frame.
	void DrawFakeScene(YimMenu::MockEspWorld& world, ImDrawList* drawList)
	{
		const float w = (float)world.m_Camera.m_Width;
		const float h = (float)world.m_Camera.m_Height;

		drawList->AddRectFilled({0, 0}, {w, h * 0.55f}, IM_COL32(16, 22, 34, 255));
		drawList->AddRectFilled({0, h * 0.55f}, {w, h}, IM_COL32(24, 26, 20, 255));
		drawList->AddLine({0, h * 0.55f}, {w, h * 0.55f}, IM_COL32(90, 90, 90, 255));

		const ImU32 gridCol = IM_COL32(70, 80, 70, 160);
		for (float gy = 0.f; gy <= 60.f; gy += 5.f)
		{
			const ImVec2 a = world.Project({-40.f, gy, 0.f});
			const ImVec2 b = world.Project({40.f, gy, 0.f});
			if (a.x != 0 && b.x != 0)
				drawList->AddLine(a, b, gridCol);
		}
		for (float gx = -40.f; gx <= 40.f; gx += 5.f)
		{
			const ImVec2 a = world.Project({gx, 0.f, 0.f});
			const ImVec2 b = world.Project({gx, 60.f, 0.f});
			if (a.x != 0 && b.x != 0)
				drawList->AddLine(a, b, gridCol);
		}

		// faint capsule marking the scripted body behind the ESP skeleton
		const ImVec2 head = world.Project(world.m_Actor.GetBone(21030));
		const ImVec2 feet = world.Project({world.m_Actor.m_Pos.x, world.m_Actor.m_Pos.y, 0.f});
		if (head.x != 0 && feet.x != 0)
		{
			const float halfW = std::max(4.f, (feet.y - head.y) * 0.14f);
			drawList->AddRectFilled({head.x - halfW, head.y}, {head.x + halfW, feet.y}, IM_COL32(150, 130, 110, 70));
		}
	}
} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int)
{
	WNDCLASSEXW wc{sizeof(wc), CS_CLASSDC, WndProc, 0, 0, instance, nullptr, nullptr, nullptr, nullptr, L"Mi5hlenVisualTest", nullptr};
	::RegisterClassExW(&wc);
	g_Hwnd = ::CreateWindowW(wc.lpszClassName, L"RDO Terminator - Visual Test (fake scene)", WS_OVERLAPPEDWINDOW,
	    100, 100, 1280, 800, nullptr, nullptr, instance, nullptr);

	if (!CreateDeviceD3D(g_Hwnd))
	{
		CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, instance);
		return 1;
	}
	::ShowWindow(g_Hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(g_Hwnd);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui_ImplWin32_Init(g_Hwnd);
	ImGui_ImplDX11_Init(g_Device, g_Context);

	// isolated test config: never touches %appdata%/Mi5hlenMenu
	char* appdata = std::getenv("appdata");
	const std::filesystem::path testDir =
	    (appdata ? std::filesystem::path(appdata) : std::filesystem::temp_directory_path()) / "Mi5hlenMenuVisualTest";
	std::filesystem::create_directories(testDir);

	YimMenu::Submenus::Settings::SetSettingsFileOverride((testDir / "settings.json").string());
	YimMenu::Menu::SetupFonts((testDir / "imgui.ini").string());
	YimMenu::Menu::SetupStyle();

	YimMenu::Settings::Initialize(YimMenu::File((testDir / "settings.json").string()));
	YimMenu::g_HotkeySystem.RegisterCommands();
	YimMenu::UIManager::AddSubmenu(std::make_shared<YimMenu::Submenus::Settings>());
	g_SettingsInstance.LoadSettings();
	YimMenu::Submenus::MigrateThemeOnce();

	YimMenu::MockEspWorld world;
	world.m_Camera.m_Width  = 1280;
	world.m_Camera.m_Height = 800;

	auto lastTime = std::chrono::steady_clock::now();
	bool prevToggleKey = false;
	bool done          = false;
	while (!done)
	{
		MSG msg;
		while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessageW(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}
		if (done)
			break;

		if (g_SwapChainOccluded && g_SwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
		{
			::Sleep(10);
			continue;
		}
		g_SwapChainOccluded = false;

		if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
		{
			CleanupRenderTarget();
			g_SwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
			g_ResizeWidth = g_ResizeHeight = 0;
			CreateRenderTarget();
		}

		RECT rect{};
		::GetClientRect(g_Hwnd, &rect);
		world.m_Camera.m_Width  = rect.right - rect.left;
		world.m_Camera.m_Height = rect.bottom - rect.top;

		// menu toggle key, same rule as the in-game WndProc: Insert if ticked, else F5
		const auto useInsert = YimMenu::Commands::GetCommand<YimMenu::BoolCommand>("togglemenukey"_J);
		const int toggleVk   = (useInsert && useInsert->GetState()) ? VK_INSERT : VK_F5;
		const bool toggleDown = (::GetAsyncKeyState(toggleVk) & 0x8000) != 0;
		if (toggleDown && !prevToggleKey)
			g_MenuOpen = !g_MenuOpen;
		prevToggleKey = toggleDown;

		const auto now = std::chrono::steady_clock::now();
		const float dt = std::min(0.1f, std::chrono::duration<float>(now - lastTime).count());
		lastTime       = now;

		world.Update(dt);
		YimMenu::TestTasks::Pump();

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		if (ImDrawList* bg = ImGui::GetBackgroundDrawList())
			DrawFakeScene(world, bg);

		YimMenu::ESP::Draw(world);

		YimMenu::Menu::ShellContext ctx{};
		ctx.m_IsOpen       = g_MenuOpen;
		ctx.m_Logo         = 0;
		ctx.m_IsGodMode    = [] {
			return g_GodMode;
		};
		ctx.m_ToggleGodMode = [] {
			g_GodMode = !g_GodMode;
		};
		ctx.m_PushTask = [](std::function<void()> task) {
			YimMenu::FiberPool::Push(std::move(task));
		};
		ctx.m_Settings = &g_SettingsInstance;
		YimMenu::Menu::DrawShell(ctx);

		YimMenu::DrawEspLabPanel(world);

		ImGui::Render();
		const float clearCol[4] = {0.05f, 0.06f, 0.08f, 1.f};
		g_Context->OMSetRenderTargets(1, &g_MainRTV, nullptr);
		g_Context->ClearRenderTargetView(g_MainRTV, clearCol);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		HRESULT present = g_SwapChain->Present(1, 0);
		g_SwapChainOccluded = (present == DXGI_STATUS_OCCLUDED);

		YimMenu::Settings::Tick();
	}

	YimMenu::FiberPool::Destroy();
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	CleanupDeviceD3D();
	::DestroyWindow(g_Hwnd);
	::UnregisterClassW(wc.lpszClassName, instance);
	return 0;
}
