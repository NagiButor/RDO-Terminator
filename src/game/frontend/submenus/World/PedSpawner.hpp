#pragma once
#include <unordered_map>

namespace YimMenu
{
	class Ped;
}

namespace YimMenu::Submenus
{
	void RenderPedSpawnerMenu();
	void ApplyCompanionSettings(YimMenu::Ped& ped);

	inline std::unordered_map<int, const char*> groupFormations = {
        {0, "Default"}, 
        {1, "Circle Around Leader"}, 
        {2, "Alternative Circle Around Leader"}, 
        {3, "Line, with Leader at center"}
    };
}