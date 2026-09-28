#pragma once
#include "core/filemgr/FileMgr.hpp"

#include <chrono>
#include <ctime>
#include <fstream>
#include <mutex>
#include <string>

namespace YimMenu::AttackLog
{
	// Appends one line to %appdata%/RDOTerminator/attacks.log:
	// [time] [category] [sender] text. Never throws, safe to call from hooks.
	inline void Write(const std::string& category, const std::string& sender, const std::string& text)
	{
		static std::mutex mutex;
		std::lock_guard lock(mutex);

		try
		{
			const std::filesystem::path path = FileMgr::GetProjectFile("./attacks.log");
			std::ofstream out(path, std::ios::app);
			if (!out)
				return;

			const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			std::tm tm{};
			localtime_s(&tm, &now);
			char stamp[32]{};
			std::strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &tm);

			out << "[" << stamp << "] [" << category << "] [" << sender << "] " << text << "\n";
		}
		catch (...)
		{
		}
	}
}
