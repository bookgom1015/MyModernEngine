#pragma once

#include "GameObject.hpp"

class TaskManager : public Singleton<TaskManager> {
	SINGLETON(TaskManager);

public:
	bool Update();

public:
	void AddTask(const TaskInfo& info);
	
private:
	std::vector<TaskInfo> mTasks;
	std::vector<Ptr<GameObject>> mDeadObjects;
};

#define MAKE_DEFERRED_TASK(__task, ...) \
	NEW std::function<void()>([__VA_ARGS__]() __task)

#define DWORD_PTR_DEFERRED_TASK(__task, ...) \
	reinterpret_cast<DWORD_PTR>(MAKE_DEFERRED_TASK(__task, __VA_ARGS__));

#ifndef TASK_MANAGER
#define TASK_MANAGER TaskManager::GetInstance()
#endif // TASK_MANAGER