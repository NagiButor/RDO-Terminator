// Test-only FiberPool: production code posts work via FiberPool::Push.
// Outside the game there are no script fibers, so tasks are queued here
// and executed by the visual-test host once per frame.
#include "game/backend/FiberPool.hpp"

#include <functional>
#include <mutex>
#include <queue>

namespace YimMenu
{
	namespace
	{
		std::queue<std::function<void()>> g_TestTasks;
		std::mutex g_TestTasksMutex;
	}

	void FiberPool::InitImpl(int)
	{
	}

	void FiberPool::DestroyImpl()
	{
		std::lock_guard lock(g_TestTasksMutex);
		while (!g_TestTasks.empty())
			g_TestTasks.pop();
	}

	void FiberPool::PushImpl(std::function<void()> callback)
	{
		std::lock_guard lock(g_TestTasksMutex);
		g_TestTasks.push(std::move(callback));
	}

	namespace TestTasks
	{
		void Pump()
		{
			for (;;)
			{
				std::function<void()> task;
				{
					std::lock_guard lock(g_TestTasksMutex);
					if (g_TestTasks.empty())
						return;
					task = std::move(g_TestTasks.front());
					g_TestTasks.pop();
				}
				if (task)
					task();
			}
		}
	}
}
