#pragma once

#include <string>

namespace YimMenu
{
	class MockEspWorld;
	class DummyMesh;

	// Debug window driving the fake scene: respawn / damage / heal / kill,
	// actor placement, player-vs-ped presentation, crowd, ESP presets and the
	// mock camera. ESP options themselves live in the real Visuals submenu;
	// presets only flip those same commands.
	void DrawEspLabPanel(MockEspWorld& world, DummyMesh& mesh, bool expandOnFirstUse = false);

	// shown in the panel so a missing crash_dummy folder is obvious
	void SetEspLabMeshStatus(const std::string& status);
}
