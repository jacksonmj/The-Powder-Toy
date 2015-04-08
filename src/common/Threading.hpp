/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef Threading_h
#define Threading_h

#include <functional>
#include <thread>
#include <condition_variable>
#include <future>
#include <queue>
#include <vector>
#include "common/Compat.hpp"

class WorkerThreads
{
protected:
	std::queue<std::function<void()> > taskQueue;
	std::vector<std::thread> threads;
	std::condition_variable idleWaitCV;
	std::mutex mtx;
	bool shouldStop;
	TH_ENTRY_POINT void runWorker()
	{
		while (true)
		{
			std::function<void()> task;
			bool runTask = false;

			{
				std::unique_lock<std::mutex> l(mtx);
				if (taskQueue.empty() && !shouldStop)
					idleWaitCV.wait(l, [this](){ return (!taskQueue.empty() || shouldStop); });

				if (shouldStop)
				{
					return;
				}
				if (!taskQueue.empty())
				{
					runTask = true;
					task = taskQueue.front();
					taskQueue.pop();
				}
			}

			if (runTask)
				task();
		}
	}
	void stopThreads()
	{
		{
			std::lock_guard<std::mutex> l(mtx);
			shouldStop = true;
		}
		idleWaitCV.notify_all();

		for (auto it=threads.begin(); it!=threads.end(); ++it)
			it->join();
		threads.resize(0);
	}

public:
	template<class Function, class... Args>
	std::shared_future<typename std::result_of<Function(Args...)>::type> asyncTask(Function&& f, Args&&... args)
	{
		using taskpkg_t = std::packaged_task<typename std::result_of<Function(Args...)>::type()>;
		auto task = new taskpkg_t(std::bind(std::forward<Function>(f), std::forward<Args>(args)...));
		auto taskFuture = task->get_future().share();

		{
			std::lock_guard<std::mutex> l(mtx);
			taskQueue.emplace([task](){
				(*task)();
				delete task;
			});
		}

		idleWaitCV.notify_one();
		return taskFuture;
	}

	WorkerThreads(size_t threadCount=1) : shouldStop(false)
	{
		threads.reserve(threadCount);
		for (size_t i=0; i<threadCount; i++)
			threads.emplace_back(std::thread(&WorkerThreads::runWorker, this));
	}
	virtual ~WorkerThreads()
	{
		stopThreads();

		while (!taskQueue.empty())
		{
			std::function<void()> task = taskQueue.front();
			task();
			taskQueue.pop();
		}
	}
};


#endif
