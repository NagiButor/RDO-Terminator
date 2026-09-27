#pragma once

namespace YimMenu::TestTasks
{
	// Runs everything queued via FiberPool::Push since the last call.
	void Pump();
}
