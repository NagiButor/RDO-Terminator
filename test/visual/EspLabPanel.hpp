#pragma once

namespace YimMenu
{
	class MockEspWorld;
	class DummyMesh;

	// Debug window driving the fake scene: respawn / damage / heal / kill,
	// actor placement, player-vs-ped presentation and the mock camera.
	// All ESP toggles stay in the real Settings submenu; this panel only
	// mutates the scripted actor, so overlay behavior can be inspected.
	void DrawEspLabPanel(MockEspWorld& world, DummyMesh& mesh);
}
