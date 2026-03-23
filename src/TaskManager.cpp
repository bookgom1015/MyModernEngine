#include "pch.h"
#include "TaskManager.hpp"

#include "LevelManager.hpp"
#include "AssetManager.hpp"

TaskManager::TaskManager() {}

TaskManager::~TaskManager() {}

bool TaskManager::Update() {
	mDeadObjects.clear();

	for (size_t i = 0; i < mTasks.size(); ++i) {
		const auto& task = mTasks[i];
		switch (task.Type) {
		case ETask::E_CreateObject: {
			Ptr<GameObject> obj = reinterpret_cast<GameObject*>(task.Param_0);

			Ptr<ALevel> currLevel = LEVEL_MANAGER->GetCurrentLevel();
			if (currLevel == nullptr) break;

			currLevel->AddGameObject(static_cast<int>(mTasks[i].Param_1), obj);
			currLevel->Change();

			if (LEVEL_MANAGER->GetCurrentLevelState() == ELevelState::E_Playing)
				obj->Begin();
		}
			break;
		case ETask::E_DestroyObject: {
			Ptr<GameObject> obj = reinterpret_cast<GameObject*>(task.Param_0);

			if (!obj->IsDead()) {
				obj->mbIsDead = true;
				mDeadObjects.push_back(obj);

				Ptr<ALevel> currLevel = LEVEL_MANAGER->GetCurrentLevel();
				currLevel->Change();
			}

			obj->Destroy();
			mDeadObjects.push_back(obj);
		}
			break;
		case ETask::E_ChangeLevel: {
			decltype(auto) levelName = reinterpret_cast<const wchar_t*>(mTasks[i].Param_0);
			Ptr<ALevel> level = ASSET_MANAGER->Find<ALevel>(levelName);
			LEVEL_MANAGER->ChangeLevel(level);
		}
			break;
		case ETask::E_ChangeNewLevel: {
			Ptr<ALevel> level = reinterpret_cast<ALevel*>(mTasks[i].Param_0);			
			LEVEL_MANAGER->ChangeLevel(level);
		}
			break;
		case ETask::E_ChangeLevelState: {
			auto NextState = static_cast<ELevelState::Type>(mTasks[i].Param_0);
			LEVEL_MANAGER->ChangeLevelState(NextState);
		}
			break;
		case ETask::E_DeferredProcessing: {
			auto* func = reinterpret_cast<std::function<void()>*>(mTasks[i].Param_0);
			(*func)();
			delete func;
		}
			break;
		default:
			break;
		}
	}

	mTasks.clear();

	return true;
}

void TaskManager::AddTask(const TaskInfo& info) {
	mTasks.push_back(info);
}