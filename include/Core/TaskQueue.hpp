#pragma once

class TaskQueue {
public:
	TaskQueue() = default;
	virtual ~TaskQueue() = default;

	DISALLOW_COPY_AND_MOVE(TaskQueue)

public:
	void AddTask(const std::function<bool()>& task);
	bool ExecuteTasks(unsigned numThreads);

private:
	bool ExecuteTask();

private:
	std::queue<std::function<bool()>> mTaskQueue;
	std::mutex mMutex;
};