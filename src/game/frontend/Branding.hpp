// Branding: every product-specific string the menu shell shows lives here.
// To reskin the menu for another game/product, edit this file (and the
// palette in Theme.hpp) - the shell, sidebar, tabs and cards stay untouched.
#pragma once

namespace YimMenu::Branding
{
	// header title is drawn as two runs: primary (text color) + accent (brand color)
	inline constexpr const char* kTitlePrimary = "RDO";
	inline constexpr const char* kTitleAccent  = "TERMINATOR";
	inline constexpr const char* kSubtitle     = "PREMIUM MENU FOR RED DEAD ONLINE";

	// status pill in the header (left of the build number)
	inline constexpr const char* kStatusText = "ONLINE";

	// two short lines at the bottom of the sidebar; set to "" to hide
	inline constexpr const char* kTaglineTop    = "BE AHEAD";
	inline constexpr const char* kTaglineBottom = "STAY ALIVE";
}
