#include "Core/pch.h"
#include "Core/TaskQueue.hpp"

void TaskQueue::AddTask(const std::function<bool()>& task) {
	mTaskQueue.push(task);
}

bool TaskQueue::ExecuteTasks(unsigned numThreads) {
	std::vector<std::future<bool>> threads;

	for (unsigned i = 0; i < numThreads; ++i)
		threads.emplace_back(std::async(std::launch::async, &TaskQueue::ExecuteTask, this));

	bool status = true;
	for (unsigned i = 0; i < numThreads; ++i)
		status = status && threads[i].get();

	return status;
}

bool TaskQueue::ExecuteTask() {
	while (true) {
		std::function<bool()> task;
		{
			std::lock_guard<std::mutex> lock(mMutex);

			if (mTaskQueue.empty()) break;
			task = std::move(mTaskQueue.front());

			mTaskQueue.pop();
		}
		CheckReturn(task());
	}

	return true;
}