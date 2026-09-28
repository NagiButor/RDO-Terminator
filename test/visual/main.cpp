// MenuVisualTest: standalone harness that renders the REAL menu shell and the
// REAL ESP draw code against a scripted fake scene. No RDR2, no injection,
// no game backend: entity data, camera projection and the task queue are faked.
//
// Keys:   F5 / INSERT  toggle the menu (same rule as in game)
//         F12          save a screenshot to %appdata%\RDOTerminatorVisualTest\screenshots
//
// Command line (for scripted checks, all optional):
//   --screenshot <file.png>   render, save the frame and exit
//   --frames <n>              frames to render before the screenshot (default 45)
//   --size <w>x<h>            client size
//   --submenu <name>          open this sidebar entry        (e.g. Visuals)
//   --category <name>         open this tab inside it        (e.g. "Player ESP")
//   --set <command>=<value>   set a bool/int/list command    (e.g. espdrawplayers=1)
//   --crowd                   spawn the extra actors
//   --distance <m>            main actor distance from the camera (default 25)
//   --lab                     expand the ESP Lab panel
//   --menu-closed             start with the menu hidden
#include "MockEspWorld.hpp"
#include "EspLabPanel.hpp"
#include "DummyMesh.hpp"
#include "stubs/TestTasks.hpp"

#include "game/frontend/Menu.hpp"
#include "game/frontend/ESP.hpp"
#include "game/frontend/submenus/Interface.hpp"
#include "game/frontend/submenus/Visuals.hpp"
#include "game/frontend/items/Items.hpp"
#include "core/commands/BoolCommand.hpp"
#include "core/commands/Commands.hpp"
#include "core/commands/HotkeySystem.hpp"
#include "core/commands/IntCommand.hpp"
#include "core/commands/ListCommand.hpp"
#include "core/frontend/manager/UIManager.hpp"
#include "core/settings/Settings.hpp"
#include "game/backend/FiberPool.hpp"

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

#include <chrono>
#include <d3d11.h>
#include <filesystem>
#include <shellapi.h>
#include <string>
#include <vector>
#include <wincodec.h>

using YimMenu::operator""_J;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace
{
	ID3D11Device* g_Device                = nullptr;
	ID3D11DeviceContext* g_Context       = nullptr;
	IDXGISwapChain* g_SwapChain          = nullptr;
	ID3D11RenderTargetView* g_MainRTV    = nullptr;
	ID3D11Texture2D* g_DepthStencil      = nullptr;
	ID3D11DepthStencilView* g_DepthView  = nullptr;
	ID3D11ShaderResourceView* g_LogoSrv  = nullptr;
	bool g_SwapChainOccluded             = false;
	UINT g_ResizeWidth                   = 0;
	UINT g_ResizeHeight                  = 0;
	HWND g_Hwnd                          = nullptr;
	bool g_MenuOpen                      = true; // the harness exists to look at the menu
	bool g_GodMode                       = false;
	YimMenu::Submenus::Settings g_SettingsInstance;

	struct Options
	{
		std::wstring m_Screenshot;
		int m_Frames = 45;
		int m_Width  = 1280;
		int m_Height = 800;
		std::string m_Submenu;
		std::string m_Category;
		std::vector<std::pair<std::string, std::string>> m_Sets;
		bool m_Crowd      = false;
		float m_Distance  = 0.f;
		bool m_Lab        = false;
		bool m_MenuClosed = false;
	};

	std::string Narrow(const std::wstring& w)
	{
		if (w.empty())
			return {};
		const int n = ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
		std::string s((size_t)n, '\0');
		::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), s.data(), n, nullptr, nullptr);
		return s;
	}

	Options ParseOptions()
	{
		Options o;
		int argc     = 0;
		LPWSTR* argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
		for (int i = 1; argv && i < argc; i++)
		{
			const std::wstring a = argv[i];
			const bool hasNext   = i + 1 < argc;
			if (a == L"--screenshot" && hasNext)
				o.m_Screenshot = argv[++i];
			else if (a == L"--frames" && hasNext)
				o.m_Frames = std::max(2, _wtoi(argv[++i]));
			else if (a == L"--size" && hasNext)
				swscanf_s(argv[++i], L"%dx%d", &o.m_Width, &o.m_Height);
			else if (a == L"--submenu" && hasNext)
				o.m_Submenu = Narrow(argv[++i]);
			else if (a == L"--category" && hasNext)
				o.m_Category = Narrow(argv[++i]);
			else if (a == L"--set" && hasNext)
			{
				const std::string kv = Narrow(argv[++i]);
				if (const size_t eq = kv.find('='); eq != std::string::npos)
					o.m_Sets.emplace_back(kv.substr(0, eq), kv.substr(eq + 1));
			}
			else if (a == L"--distance" && hasNext)
				o.m_Distance = (float)_wtof(argv[++i]);
			else if (a == L"--crowd")
				o.m_Crowd = true;
			else if (a == L"--lab")
				o.m_Lab = true;
			else if (a == L"--menu-closed")
				o.m_MenuClosed = true;
		}
		if (argv)
			::LocalFree(argv);
		return o;
	}

	void ApplySet(const std::string& name, const std::string& value)
	{
		auto* cmd = YimMenu::Commands::GetCommand(YimMenu::Joaat(name));
		if (!cmd)
			return;
		const int v = std::atoi(value.c_str());
		if (auto* b = dynamic_cast<YimMenu::BoolCommand*>(cmd))
			b->SetState(v != 0);
		else if (auto* l = dynamic_cast<YimMenu::ListCommand*>(cmd))
			l->SetState(v);
		else if (auto* n = dynamic_cast<YimMenu::IntCommand*>(cmd))
			n->SetState(v);
	}

	// sidebar order, identical to Menu::Init() in the DLL (UIManager keeps its own list private)
	std::vector<std::shared_ptr<YimMenu::Submenu>> g_TestSubmenus;

	void OpenPage(const std::string& submenu, const std::string& category)
	{
		for (auto& sm : g_TestSubmenus)
		{
			if (_stricmp(sm->m_Name.c_str(), submenu.c_str()) != 0)
				continue;
			YimMenu::UIManager::SetActiveSubmenu(sm);
			for (auto& cat : sm->m_Categories)
				if (!category.empty() && _stricmp(cat->m_Name.c_str(), category.c_str()) == 0)
					sm->SetActiveCategory(cat);
		}
	}

	void CreateRenderTarget();

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
		HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevels, 2,
		    D3D11_SDK_VERSION, &sd, &g_SwapChain, &g_Device, &featureLevel, &g_Context);
		if (hr == DXGI_ERROR_UNSUPPORTED) // no usable GPU (VM, RDP): fall back to the software rasterizer
			hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, featureLevels, 2,
			    D3D11_SDK_VERSION, &sd, &g_SwapChain, &g_Device, &featureLevel, &g_Context);
		if (hr != S_OK)
			return false;

		// render target + depth buffer: the crash-dummy pass needs depth from frame one
		CreateRenderTarget();
		return true;
	}

	template<typename T>
	void SafeRelease(T*& p)
	{
		if (p)
		{
			p->Release();
			p = nullptr;
		}
	}

	void CleanupRenderTarget()
	{
		SafeRelease(g_MainRTV);
		SafeRelease(g_DepthView);
		SafeRelease(g_DepthStencil);
	}

	void CleanupDeviceD3D()
	{
		CleanupRenderTarget();
		SafeRelease(g_LogoSrv);
		SafeRelease(g_SwapChain);
		SafeRelease(g_Context);
		SafeRelease(g_Device);
	}

	void CreateRenderTarget()
	{
		ID3D11Texture2D* backBuffer = nullptr;
		g_SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
		g_Device->CreateRenderTargetView(backBuffer, nullptr, &g_MainRTV);

		D3D11_TEXTURE2D_DESC backDesc{};
		backBuffer->GetDesc(&backDesc);
		backBuffer->Release();

		D3D11_TEXTURE2D_DESC dsDesc{};
		dsDesc.Width          = backDesc.Width;
		dsDesc.Height         = backDesc.Height;
		dsDesc.MipLevels      = 1;
		dsDesc.ArraySize      = 1;
		dsDesc.Format         = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsDesc.SampleDesc     = {1, 0};
		dsDesc.Usage          = D3D11_USAGE_DEFAULT;
		dsDesc.BindFlags      = D3D11_BIND_DEPTH_STENCIL;
		if (SUCCEEDED(g_Device->CreateTexture2D(&dsDesc, nullptr, &g_DepthStencil)))
			g_Device->CreateDepthStencilView(g_DepthStencil, nullptr, &g_DepthView);
	}

	// Same header logo as the DLL (resources/terminator.png), decoded with WIC.
	ID3D11ShaderResourceView* LoadPngTexture(const std::wstring& path)
	{
		IWICImagingFactory* factory = nullptr;
		if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))))
			return nullptr;

		ID3D11ShaderResourceView* srv  = nullptr;
		IWICBitmapDecoder* decoder     = nullptr;
		IWICBitmapFrameDecode* frame   = nullptr;
		IWICFormatConverter* converter = nullptr;
		UINT w = 0, h = 0;
		if (SUCCEEDED(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder))
		    && SUCCEEDED(decoder->GetFrame(0, &frame)) && SUCCEEDED(factory->CreateFormatConverter(&converter))
		    && SUCCEEDED(converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom))
		    && SUCCEEDED(converter->GetSize(&w, &h)) && w && h)
		{
			std::vector<uint8_t> pixels((size_t)w * h * 4);
			if (SUCCEEDED(converter->CopyPixels(nullptr, w * 4, (UINT)pixels.size(), pixels.data())))
			{
				D3D11_TEXTURE2D_DESC desc{};
				desc.Width      = w;
				desc.Height     = h;
				desc.MipLevels  = 1;
				desc.ArraySize  = 1;
				desc.Format     = DXGI_FORMAT_R8G8B8A8_UNORM;
				desc.SampleDesc = {1, 0};
				desc.Usage      = D3D11_USAGE_DEFAULT;
				desc.BindFlags  = D3D11_BIND_SHADER_RESOURCE;
				D3D11_SUBRESOURCE_DATA data{pixels.data(), w * 4, 0};
				ID3D11Texture2D* tex = nullptr;
				if (SUCCEEDED(g_Device->CreateTexture2D(&desc, &data, &tex)))
				{
					g_Device->CreateShaderResourceView(tex, nullptr, &srv);
					tex->Release();
				}
			}
		}
		SafeRelease(converter);
		SafeRelease(frame);
		SafeRelease(decoder);
		factory->Release();
		return srv;
	}

	// Copies the current back buffer into a PNG (call before Present).
	bool SaveBackBufferPng(const std::wstring& path)
	{
		ID3D11Texture2D* backBuffer = nullptr;
		if (FAILED(g_SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))))
			return false;
		D3D11_TEXTURE2D_DESC desc{};
		backBuffer->GetDesc(&desc);
		desc.Usage          = D3D11_USAGE_STAGING;
		desc.BindFlags      = 0;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		desc.MiscFlags      = 0;
		ID3D11Texture2D* staging = nullptr;
		bool ok                  = SUCCEEDED(g_Device->CreateTexture2D(&desc, nullptr, &staging));
		if (ok)
			g_Context->CopyResource(staging, backBuffer);
		backBuffer->Release();

		D3D11_MAPPED_SUBRESOURCE mapped{};
		ok = ok && SUCCEEDED(g_Context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped));
		if (!ok)
		{
			SafeRelease(staging);
			return false;
		}

		std::error_code ec;
		std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);

		IWICImagingFactory* factory = nullptr;
		IWICStream* stream          = nullptr;
		IWICBitmapEncoder* encoder  = nullptr;
		IWICBitmapFrameEncode* frame = nullptr;
		ok = SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)))
		    && SUCCEEDED(factory->CreateStream(&stream)) && SUCCEEDED(stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE))
		    && SUCCEEDED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder))
		    && SUCCEEDED(encoder->Initialize(stream, WICBitmapEncoderNoCache)) && SUCCEEDED(encoder->CreateNewFrame(&frame, nullptr))
		    && SUCCEEDED(frame->Initialize(nullptr)) && SUCCEEDED(frame->SetSize(desc.Width, desc.Height));
		if (ok)
		{
			// back buffer is RGBA, the PNG encoder wants BGRA; force alpha opaque
			std::vector<uint8_t> rows((size_t)desc.Width * desc.Height * 4);
			for (UINT y = 0; y < desc.Height; y++)
			{
				const uint8_t* src = (const uint8_t*)mapped.pData + (size_t)y * mapped.RowPitch;
				uint8_t* dst       = rows.data() + (size_t)y * desc.Width * 4;
				memcpy(dst, src, (size_t)desc.Width * 4);
				for (UINT x = 0; x < desc.Width; x++)
				{
					std::swap(dst[x * 4], dst[x * 4 + 2]);
					dst[x * 4 + 3] = 255;
				}
			}
			WICPixelFormatGUID fmt = GUID_WICPixelFormat32bppBGRA;
			ok = SUCCEEDED(frame->SetPixelFormat(&fmt)) && IsEqualGUID(fmt, GUID_WICPixelFormat32bppBGRA)
			    && SUCCEEDED(frame->WritePixels(desc.Height, desc.Width * 4, (UINT)rows.size(), rows.data()))
			    && SUCCEEDED(frame->Commit()) && SUCCEEDED(encoder->Commit());
		}
		g_Context->Unmap(staging, 0);
		SafeRelease(staging);
		SafeRelease(frame);
		SafeRelease(encoder);
		SafeRelease(stream);
		SafeRelease(factory);
		return ok;
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

	// Fake 3D backdrop: sky gradient, horizon and a projected ground grid. Drawn
	// into its own list and rendered BEFORE the crash-dummy mesh; the ESP overlay
	// and the menu are rendered after it, like over a real game frame.
	void DrawFakeScene(YimMenu::MockEspWorld& world, ImDrawList* drawList, bool meshLoaded)
	{
		const float w       = (float)world.m_Camera.m_Width;
		const float h       = (float)world.m_Camera.m_Height;
		const ImVec2 horizonL = world.Project({-2000.f, 2000.f, 0.f});
		const float horizon = horizonL.x != 0 ? std::clamp(world.Project({0.f, 2000.f, 0.f}).y, 0.f, h) : h * 0.55f;

		drawList->AddRectFilledMultiColor({0, 0}, {w, horizon}, IM_COL32(24, 30, 46, 255), IM_COL32(24, 30, 46, 255),
		    IM_COL32(62, 58, 60, 255), IM_COL32(62, 58, 60, 255));
		drawList->AddRectFilledMultiColor({0, horizon}, {w, h}, IM_COL32(44, 40, 32, 255), IM_COL32(44, 40, 32, 255),
		    IM_COL32(22, 22, 18, 255), IM_COL32(22, 22, 18, 255));
		drawList->AddLine({0, horizon}, {w, horizon}, IM_COL32(110, 100, 90, 255));

		const ImU32 gridCol = IM_COL32(80, 86, 72, 120);
		for (float gy = 0.f; gy <= 100.f; gy += 5.f)
		{
			const ImVec2 a = world.Project({-50.f, gy, 0.f});
			const ImVec2 b = world.Project({50.f, gy, 0.f});
			if (a.x != 0 && b.x != 0)
				drawList->AddLine(a, b, gridCol);
		}
		for (float gx = -50.f; gx <= 50.f; gx += 5.f)
		{
			const ImVec2 a = world.Project({gx, 1.f, 0.f});
			const ImVec2 b = world.Project({gx, 100.f, 0.f});
			if (a.x != 0 && b.x != 0)
				drawList->AddLine(a, b, gridCol);
		}

		// stand-ins for bodies the mesh does not cover (animals always, humans when the mesh failed to load)
		auto body = [&](const YimMenu::MockActor& a) {
			const ImVec2 top  = world.Project(a.GetBone(21030));
			const ImVec2 feet = world.Project({a.m_Pos.x, a.m_Pos.y, 0.f});
			if (top.x == 0 || feet.x == 0)
				return;
			const float bodyH = feet.y - top.y;
			if (a.m_Animal)
			{
				const ImVec2 c{(top.x + feet.x) * 0.5f, feet.y - bodyH * 0.45f};
				drawList->AddEllipseFilled(c, ImVec2(bodyH * 1.1f, bodyH * 0.42f), IM_COL32(92, 86, 78, 230));
			}
			else if (!meshLoaded)
			{
				const float halfW = std::max(4.f, bodyH * 0.14f);
				drawList->AddRectFilled({top.x - halfW, top.y}, {top.x + halfW, feet.y}, IM_COL32(150, 130, 110, 110), halfW);
			}
		};
		body(world.m_Actor);
		for (const auto& a : world.m_Crowd)
			body(a);
	}

	void RenderDrawList(ImDrawList* list, float w, float h)
	{
		ImDrawData data;
		data.Valid            = true;
		data.DisplayPos       = ImVec2(0.f, 0.f);
		data.DisplaySize      = ImVec2(w, h);
		data.FramebufferScale = ImVec2(1.f, 1.f);
		data.AddDrawList(list);
		ImGui_ImplDX11_RenderDrawData(&data);
	}
} // namespace

namespace
{
	// Pages that need the live game: same name, icon and tabs as the DLL, with a note instead of options.
	std::shared_ptr<YimMenu::Submenu> Placeholder(const char* name, std::initializer_list<const char*> tabs)
	{
		auto sm = std::make_shared<YimMenu::Submenu>(name);
		for (const char* tab : tabs)
		{
			auto cat   = std::make_shared<YimMenu::Category>(tab);
			auto group = std::make_shared<YimMenu::Group>(tab);
			group->AddItem(std::make_shared<YimMenu::ImGuiItem>([] {
				ImGui::TextDisabled("Needs the running game - not available in the visual test.");
				ImGui::TextDisabled("Visuals and Interface are the real in-game pages.");
			}));
			cat->AddItem(std::move(group));
			sm->AddCategory(std::move(cat));
		}
		return sm;
	}

	void RegisterSubmenus()
	{
		g_TestSubmenus = {
		    Placeholder("Player", {"Main", "Companions", "Teleport", "Saved", "Animations"}),
		    std::make_shared<YimMenu::Submenus::Visuals>(),
		    Placeholder("Weapons", {"Weapons"}),
		    Placeholder("Vehicle", {"Horse", "Vehicle"}),
		    Placeholder("World", {"Main", "Weather", "Shows", "Time", "Spawners"}),
		    Placeholder("Players", {"Info", "Helpful", "Pranks", "Griefing", "Attack", "NPC Attacks", "Kick"}),
		    Placeholder("Network", {"Session", "Spoofing", "Player Database", "Voice"}),
		    Placeholder("Protection", {"Protection"}),
		    Placeholder("Recovery", {"Recovery"}),
		    std::make_shared<YimMenu::Submenus::Interface>(),
		    Placeholder("About", {"About"}),
		    Placeholder("Debug", {"Logging/Misc", "Globals", "Locals", "Scripts"}),
		};
		for (auto sm : g_TestSubmenus)
			YimMenu::UIManager::AddSubmenu(std::move(sm));
	}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int)
{
	const Options opts = ParseOptions();

	::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	WNDCLASSEXW wc{sizeof(wc), CS_CLASSDC, WndProc, 0, 0, instance, nullptr, nullptr, nullptr, nullptr, L"RDOTerminatorVisualTest", nullptr};
	::RegisterClassExW(&wc);

	// requested client size, shrunk to the work area on small / high-DPI-scaled screens
	RECT work{0, 0, opts.m_Width, opts.m_Height};
	::SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
	RECT frame{0, 0, opts.m_Width, opts.m_Height};
	::AdjustWindowRect(&frame, WS_OVERLAPPEDWINDOW, FALSE);
	const int winW = std::min((int)(frame.right - frame.left), (int)(work.right - work.left));
	const int winH = std::min((int)(frame.bottom - frame.top), (int)(work.bottom - work.top));
	g_Hwnd = ::CreateWindowW(wc.lpszClassName, L"RDO Terminator - Visual Test (fake scene)", WS_OVERLAPPEDWINDOW,
	    work.left + (work.right - work.left - winW) / 2, work.top + (work.bottom - work.top - winH) / 2, winW, winH, nullptr, nullptr, instance, nullptr);

	if (!CreateDeviceD3D(g_Hwnd))
	{
		CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, instance);
		return 1;
	}
	// scripted screenshot runs stay out of the way (no focus steal)
	::ShowWindow(g_Hwnd, opts.m_Screenshot.empty() ? SW_SHOWDEFAULT : SW_SHOWNOACTIVATE);
	::UpdateWindow(g_Hwnd);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui_ImplWin32_Init(g_Hwnd);
	ImGui_ImplDX11_Init(g_Device, g_Context);

	// isolated test config: never touches %appdata%/RDOTerminator
	char* appdata = std::getenv("appdata");
	const std::filesystem::path testDir =
	    (appdata ? std::filesystem::path(appdata) : std::filesystem::temp_directory_path()) / "RDOTerminatorVisualTest";
	std::filesystem::create_directories(testDir);

	YimMenu::Submenus::Settings::SetSettingsFileOverride((testDir / "settings.json").string());
	YimMenu::Menu::SetupFonts((testDir / "imgui.ini").string());
	YimMenu::Menu::SetupStyle();

	YimMenu::Settings::Initialize(YimMenu::File((testDir / "settings.json").string()));
	YimMenu::g_HotkeySystem.RegisterCommands();
	RegisterSubmenus();
	g_SettingsInstance.LoadSettings();

	for (const auto& [name, value] : opts.m_Sets)
		ApplySet(name, value);
	if (!opts.m_Submenu.empty())
		OpenPage(opts.m_Submenu, opts.m_Category);
	if (opts.m_MenuClosed)
		g_MenuOpen = false;

	// header logo: next to the exe first, then the source tree
	{
		wchar_t exePath[MAX_PATH]{};
		::GetModuleFileNameW(nullptr, exePath, MAX_PATH);
		std::filesystem::path logo = std::filesystem::path(exePath).parent_path() / L"terminator.png";
		std::error_code ec;
#ifdef RDO_LOGO_PATH
		if (!std::filesystem::exists(logo, ec))
			logo = RDO_LOGO_PATH;
#endif
		if (std::filesystem::exists(logo, ec))
			g_LogoSrv = LoadPngTexture(logo.wstring());
	}

	YimMenu::MockEspWorld world;
	world.m_Camera.m_Width  = opts.m_Width;
	world.m_Camera.m_Height = opts.m_Height;
	if (opts.m_Crowd)
		world.SpawnCrowd();
	if (opts.m_Distance > 0.f)
		world.m_Actor.m_Pos.y = opts.m_Distance;

	// crash-dummy mesh: RDO_DUMMY_MESH env var, else crash_dummy\scene.gltf next to the exe,
	// else E:\Downloads\crash_dummy\scene.gltf
	YimMenu::DummyMesh dummyMesh;
	std::wstring meshStatus = L"not found";
	{
		std::filesystem::path meshPath;
		if (const wchar_t* env = _wgetenv(L"RDO_DUMMY_MESH"); env && *env)
		{
			meshPath = env;
		}
		else
		{
			wchar_t exePath[MAX_PATH]{};
			::GetModuleFileNameW(nullptr, exePath, MAX_PATH);
			meshPath = std::filesystem::path(exePath).parent_path() / L"crash_dummy" / L"scene.gltf";
		}
		std::error_code ec;
		if (!std::filesystem::exists(meshPath, ec))
			meshPath = L"E:\\Downloads\\crash_dummy\\scene.gltf";
		if (std::filesystem::exists(meshPath, ec))
			meshStatus = dummyMesh.Load(g_Device, meshPath.wstring()) ? meshPath.wstring() : L"failed to load " + meshPath.wstring();
	}
	YimMenu::SetEspLabMeshStatus(Narrow(meshStatus));

	static ImDrawList sceneList(ImGui::GetDrawListSharedData());

	auto lastTime      = std::chrono::steady_clock::now();
	bool prevToggleKey = false;
	bool prevShotKey   = false;
	int frameIndex     = 0;
	std::string shotNote;
	float shotNoteTime = 0.f;
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

		if (g_SwapChainOccluded && opts.m_Screenshot.empty() && g_SwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
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
		world.m_Camera.m_Width  = std::max(1L, rect.right - rect.left);
		world.m_Camera.m_Height = std::max(1L, rect.bottom - rect.top);

		// only react while the harness has focus, not to key presses in other apps
		const bool focused = ::GetForegroundWindow() == g_Hwnd;

		// menu toggle key, same rule as the in-game WndProc: Insert if ticked, else F5
		const auto useInsert = YimMenu::Commands::GetCommand<YimMenu::BoolCommand>("togglemenukey"_J);
		const int toggleVk   = (useInsert && useInsert->GetState()) ? VK_INSERT : VK_F5;
		const bool toggleDown = focused && (::GetAsyncKeyState(toggleVk) & 0x8000) != 0;
		if (toggleDown && !prevToggleKey)
			g_MenuOpen = !g_MenuOpen;
		prevToggleKey = toggleDown;

		const bool shotDown = focused && (::GetAsyncKeyState(VK_F12) & 0x8000) != 0;
		const bool takeShot = shotDown && !prevShotKey;
		prevShotKey         = shotDown;

		const auto now = std::chrono::steady_clock::now();
		const float dt = std::min(0.1f, std::chrono::duration<float>(now - lastTime).count());
		lastTime       = now;

		world.Update(dt);
		YimMenu::TestTasks::Pump();

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		sceneList._ResetForNewFrame();
		sceneList.PushTextureID(io.Fonts->TexID);
		sceneList.PushClipRectFullScreen();
		DrawFakeScene(world, &sceneList, dummyMesh.IsLoaded());

		YimMenu::ESP::Draw(world);

		static YimMenu::Menu::ShellContext ctx = [] {
			YimMenu::Menu::ShellContext c{};
			c.m_Actions.push_back({"God Mode",
			    [] {
				    return g_GodMode;
			    },
			    [] {
				    g_GodMode = !g_GodMode;
			    }});
			c.m_OnClose = [] {
				g_MenuOpen = false;
			};
			c.m_PushTask = [](std::function<void()> task) {
				YimMenu::FiberPool::Push(std::move(task));
			};
			c.m_Settings = &g_SettingsInstance;
			return c;
		}();
		ctx.m_IsOpen = g_MenuOpen;
		ctx.m_Logo   = (ImTextureID)g_LogoSrv;
		YimMenu::Menu::DrawShell(ctx);

		if (!g_MenuOpen)
		{
			ImGui::SetNextWindowPos(ImVec2(16.f, 16.f));
			ImGui::Begin("##hint", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs);
			const auto insert = YimMenu::Commands::GetCommand<YimMenu::BoolCommand>("togglemenukey"_J);
			ImGui::Text("Press %s to open the menu  |  F12 = screenshot", (insert && insert->GetState()) ? "INSERT" : "F5");
			ImGui::End();
		}

		YimMenu::DrawEspLabPanel(world, dummyMesh, opts.m_Lab);

		if (!shotNote.empty() && ImGui::GetTime() - shotNoteTime < 2.5f)
		{
			ImGui::GetForegroundDrawList()->AddText(ImVec2(16.f, io.DisplaySize.y - 28.f), IM_COL32(120, 230, 140, 255), shotNote.c_str());
		}

		ImGui::Render();

		float view[16], proj[16];
		world.ComputeMatrices(view, proj);
		g_Context->OMSetRenderTargets(1, &g_MainRTV, g_DepthView);
		const float clearCol[4] = {0.05f, 0.06f, 0.08f, 1.f};
		g_Context->ClearRenderTargetView(g_MainRTV, clearCol);
		if (g_DepthView)
			g_Context->ClearDepthStencilView(g_DepthView, D3D11_CLEAR_DEPTH, 1.f, 0);

		// 1) backdrop  2) crash dummies with depth  3) ESP overlay + menu
		RenderDrawList(&sceneList, io.DisplaySize.x, io.DisplaySize.y);
		// the ImGui backend restores whatever viewport was bound before it (none on a
		// fresh context), so without this the mesh pass rasterized nothing at all
		const D3D11_VIEWPORT vp{0.f, 0.f, io.DisplaySize.x, io.DisplaySize.y, 0.f, 1.f};
		g_Context->RSSetViewports(1, &vp);
		g_Context->OMSetRenderTargets(1, &g_MainRTV, g_DepthView);
		if (!world.m_Actor.m_Animal)
			dummyMesh.Render(g_Context, view, proj, world.m_Actor.m_Pos.x, world.m_Actor.m_Pos.y, world.m_Actor.m_Pos.z);
		for (const auto& a : world.m_Crowd)
			if (!a.m_Animal)
				dummyMesh.Render(g_Context, view, proj, a.m_Pos.x, a.m_Pos.y, a.m_Pos.z);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		frameIndex++;
		if (takeShot)
		{
			SYSTEMTIME st{};
			::GetLocalTime(&st);
			wchar_t name[64];
			swprintf_s(name, L"shot_%04d%02d%02d_%02d%02d%02d.png", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
			const std::filesystem::path out = testDir / L"screenshots" / name;
			shotNote     = SaveBackBufferPng(out.wstring()) ? "Saved " + Narrow(out.wstring()) : "Screenshot failed";
			shotNoteTime = (float)ImGui::GetTime();
		}
		if (!opts.m_Screenshot.empty() && frameIndex >= opts.m_Frames)
		{
			SaveBackBufferPng(opts.m_Screenshot);
			done = true;
		}

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
	::CoUninitialize();
	return 0;
}
