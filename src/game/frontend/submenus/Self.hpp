#pragma once
#include "core/frontend/manager/UIManager.hpp"
#include "game/backend/AnimationDict.hpp"
#include "game/backend/MusicDict.hpp"

namespace YimMenu::Submenus
{
	void RenderAnimationsCategory();
	void RenderCompanionsCategory();

	void LoadMusicHistory();
	void SaveMusicHistory();

	// one-time init for the animation/music history backing stores
	// (used by the Player submenu, which now owns those categories)
	void EnsureSelfHelpersLoaded();
}