#include "Settings.hpp"

#include <fstream>
#include <filesystem>
#include <optional>
#include <Windows.h>

namespace YimMenu::Submenus
{
    static std::optional<std::string> g_SettingsFileOverride;

    void Settings::SetSettingsFileOverride(const std::string& path)
    {
        g_SettingsFileOverride = path;
    }

    void Settings::ClearSettingsFileOverride()
    {
        g_SettingsFileOverride.reset();
    }

    int Settings::GetWindowWidth() const { return m_WindowWidth; }
    int Settings::GetWindowHeight() const { return m_WindowHeight; }
    int Settings::GetWindowPosX() const { return m_WindowPosX; }
    int Settings::GetWindowPosY() const { return m_WindowPosY; }
    bool Settings::IsWindowMaximized() const { return m_WindowMaximized; }

    void Settings::SetWindowWidth(int w) { m_WindowWidth = w; }
    void Settings::SetWindowHeight(int h) { m_WindowHeight = h; }
    void Settings::SetWindowPosX(int x) { m_WindowPosX = x; }
    void Settings::SetWindowPosY(int y) { m_WindowPosY = y; }
    void Settings::SetWindowMaximized(bool maximized) { m_WindowMaximized = maximized; }

    std::string Settings::GetSettingsFilePath() const {
        if (g_SettingsFileOverride)
            return *g_SettingsFileOverride;
        char* appdata = std::getenv("appdata");
        if (!appdata) return "";
        std::filesystem::path path = std::filesystem::path(appdata) / "RDOTerminator" / "settings.json";
        return path.string();
    }

    void Settings::LoadWindowSettings(const nlohmann::json& j) {
        int defW = 800, defH = 600, defX = 100, defY = 100;
        bool defMax = false;
        if (j.contains("window") && j["window"].is_object()) {
            const auto& win = j["window"];
            m_WindowWidth = win.value("width", defW);
            m_WindowHeight = win.value("height", defH);
            m_WindowPosX = win.value("pos_x", defX);
            m_WindowPosY = win.value("pos_y", defY);
            m_WindowMaximized = win.value("is_maximized", defMax);
        } else {
            m_WindowWidth = defW; m_WindowHeight = defH; m_WindowPosX = defX; m_WindowPosY = defY; m_WindowMaximized = defMax;
        }

        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        if (m_WindowWidth < 300 || m_WindowWidth > screenW) m_WindowWidth = defW;
        if (m_WindowHeight < 200 || m_WindowHeight > screenH) m_WindowHeight = defH;
        if (m_WindowPosX < 0 || m_WindowPosX > screenW - 50) m_WindowPosX = defX;
        if (m_WindowPosY < 0 || m_WindowPosY > screenH - 50) m_WindowPosY = defY;

        m_LastSavedWidth = m_WindowWidth;
        m_LastSavedHeight = m_WindowHeight;
        m_LastSavedPosX = m_WindowPosX;
        m_LastSavedPosY = m_WindowPosY;
        m_LastSavedMaximized = m_WindowMaximized;
    }

    void Settings::SaveWindowSettings(nlohmann::json& j) const {
        nlohmann::json win;
        win["width"] = m_WindowWidth;
        win["height"] = m_WindowHeight;
        win["pos_x"] = m_WindowPosX;
        win["pos_y"] = m_WindowPosY;
        win["is_maximized"] = m_WindowMaximized;
        j["window"] = win;
    }

    void Settings::LoadSettings() {
        std::string path = GetSettingsFilePath();
        std::ifstream in(path);
        if (in) {
            try {
                nlohmann::json j;
                in >> j;
                LoadWindowSettings(j);
            } catch (...) {
                m_WindowWidth = 800; m_WindowHeight = 600; m_WindowPosX = 100; m_WindowPosY = 100; m_WindowMaximized = false;
            }
        } else {
            m_WindowWidth = 800; m_WindowHeight = 600; m_WindowPosX = 100; m_WindowPosY = 100; m_WindowMaximized = false;
        }
        m_LastSavedWidth = m_WindowWidth;
        m_LastSavedHeight = m_WindowHeight;
        m_LastSavedPosX = m_WindowPosX;
        m_LastSavedPosY = m_WindowPosY;
        m_LastSavedMaximized = m_WindowMaximized;
    }

    void Settings::SaveSettings() {
        std::string path = GetSettingsFilePath();
        std::filesystem::create_directories(std::filesystem::path(path).parent_path());
        nlohmann::json j;
        std::ifstream in(path);
        if (in) {
            try { in >> j; } catch (...) { j = nlohmann::json::object(); }
        }
        SaveWindowSettings(j);
        std::ofstream out(path);
        if (out) out << j.dump(4);
    }

    void Settings::SaveIfWindowChanged(int width, int height, int posX, int posY, bool maximized) {
        if (width != m_LastSavedWidth || height != m_LastSavedHeight ||
            posX != m_LastSavedPosX || posY != m_LastSavedPosY ||
            maximized != m_LastSavedMaximized) {

            m_WindowWidth = width;
            m_WindowHeight = height;
            m_WindowPosX = posX;
            m_WindowPosY = posY;
            m_WindowMaximized = maximized;

            SaveSettings();

            m_LastSavedWidth = width;
            m_LastSavedHeight = height;
            m_LastSavedPosX = posX;
            m_LastSavedPosY = posY;
            m_LastSavedMaximized = maximized;
        }
    }
}
