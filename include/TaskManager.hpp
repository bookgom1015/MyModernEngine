#pragma once

#include "GameObject.hpp"

class TaskManager : public Singleton<TaskManager> {
	SINGLETON(TaskManager);

public:
	bool Update();
};