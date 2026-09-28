#pragma once
#include <windows.h>

namespace YimMenu
{
	class GUI final
	{
	private:
		GUI();

	public:
		~GUI();

		GUI(const GUI&)                = delete;
		GUI(GUI&&) noexcept            = delete;
		GUI& operator=(const GUI&)     = delete;
		GUI& operator=(GUI&&) noexcept = delete;

		static void Init()
		{
			GetInstance();
		}

		static bool IsOpen()
		{
			return GetInstance().m_IsOpen;
		}

		static void Toggle()
		{
			GetInstance().m_IsOpen ^= true;
		}

		// Opens/closes the menu and keeps the mouse capture in sync
		// (used by the header close button).
		static void SetOpen(bool open)
		{
			if (GetInstance().m_IsOpen != open)
			{
				Toggle();
				ToggleMouse();
			}
		}

		static bool IsUsingKeyboard()
		{
			return ImGui::GetIO().WantTextInput;
		}

	private:
		void WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

		static void ToggleMouse();
		static GUI& GetInstance()
		{
			static GUI i{};

			return i;
		}

	private:
		bool m_IsOpen;
	};
}