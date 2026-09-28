#pragma once

namespace YimMenu::Features
{
	// Persistent script that animates meteors and puts out their fires.
	// Must be registered once with ScriptMgr (see main.cpp): meteors never
	// occupy FiberPool fibers, so other features are not delayed.
	void MeteorDirector();
}
