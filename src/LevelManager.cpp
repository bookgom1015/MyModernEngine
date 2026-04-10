#include "pch.h"
#include "LevelManager.hpp"

#include "Engine.hpp"

#include "TimeManager.hpp"
#include "EditorManager.hpp"
#include "ScriptManager.hpp"
#include "TaskManager.hpp"
#include "PhysicsManager.hpp"

#include "Outliner.hpp"

#include "CLight.hpp"

#include "Script/CMoveFreeCameraScript.hpp"

namespace {
	const float gFixedDT = 1.f / 60.f;
	const int gMaxSteps = 8; // 폭주 방지
}

LevelManager::LevelManager() 
	: mCurrentLevel{}
	, mSharedLevel{}
	, mLevelState{ELevelState::E_Stopped}
	, mbLevelResetRequested{ true } {}

LevelManager::~LevelManager() {}

bool LevelManager::Initialize() {
	return true;
}

bool LevelManager::Update() {
	if (mCurrentLevel == nullptr) return true;

	CheckReturn(mCurrentLevel->Deregister());

	if (mLevelState == ELevelState::E_Playing) {
		CheckReturn(mCurrentLevel->Update(DT));
			
		static float accumulatedTime = 0.f;
		accumulatedTime += DT;

		int steps = 0;
		while (accumulatedTime >= gFixedDT) {
			CheckReturn(mCurrentLevel->FixedUpdate(gFixedDT));
			CheckReturn(PHYSICS_MANAGER->FixedUpdate(gFixedDT));

			accumulatedTime -= gFixedDT;

			if (++steps >= gMaxSteps) {
				accumulatedTime = 0.f;
				break;
			}
		}

		CheckReturn(mCurrentLevel->LateUpdate(DT));
	}

	CheckReturn(mCurrentLevel->Final());
	CheckReturn(PHYSICS_MANAGER->Final());

	return true;
}

Ptr<GameObject> LevelManager::FindObjectByName(const std::wstring& name) {
	if (mCurrentLevel != nullptr) return mCurrentLevel->FindObjectByName(name);

	return nullptr;
}

void LevelManager::ChangeLevel(Ptr<ALevel> newLevel) {
	mCurrentLevel = mSharedLevel = newLevel->Clone();
	mLevelState = ELevelState::E_Stopped;

	mCurrentLevel->Change();
}

void LevelManager::ChangeLevelState(ELevelState::Type newState) {
	if (mLevelState == newState) return;

	// Stop -> Play
	if (newState == ELevelState::E_Playing && mbLevelResetRequested) {
		mbLevelResetRequested = false;

		MakeDefaultCameraIfNecessary();

		mCurrentLevel->OnUnloaded();

		// 원본 에셋 레벨의 복제본 레벨을 만들어서 현재 레벨로 가리킨다.
		mCurrentLevel = mSharedLevel->Clone();
		mCurrentLevel->Change();
		mCurrentLevel->Begin();

		mCurrentLevel->OnLoaded();

		auto ui = EDITOR_MANAGER->FindUI("Outliner");
		auto outliner = static_cast<Outliner*>(ui.Get());
		outliner->RestoreSelection();
	}
	else if (newState == ELevelState::E_Stopped) {
		mbLevelResetRequested = true;

		mCurrentLevel->OnUnloaded();

		mCurrentLevel = mSharedLevel;
		mCurrentLevel->Change();

		mCurrentLevel->OnLoaded();

		auto ui = EDITOR_MANAGER->FindUI("Outliner");
		auto outliner = static_cast<Outliner*>(ui.Get());
		outliner->RestoreSelection();
	}

	mLevelState = newState;
}

void LevelManager::MakeDefaultCameraIfNecessary() {
	auto layer = mCurrentLevel->GetLayer(ELevelLayer::E_Camera);
	if (layer->GetAllObjects().size() == 0) {
		TaskInfo info{};
		info.Type = ETask::E_DeferredProcessing;
		info.Param_0 = DWORD_PTR_DEFERRED_TASK({
			TaskInfo info{};

			auto object = NEW GameObject;
			object->SetName(L"Default Camera");

			object->AddComponent(NEW CTransform);
			object->AddComponent(NEW CCamera);
			object->AddComponent(GET_SCRIPT(CMoveFreeCameraScript));

			object->Camera()->LayerCheckAll();

			object->Camera()->SetProjectionType(EProjection::E_Perspective);
			object->Camera()->SetFar(10000.f);
			object->Camera()->SetFovY(PITwo);
			object->Camera()->SetOrthoScale(1.f);

			auto resolution = ENGINE->GetResolution();
			object->Camera()->SetAspectRatio(
				static_cast<FLOAT>(resolution.x) / static_cast<FLOAT>(resolution.y));
			object->Camera()->SetWidth(static_cast<FLOAT>(resolution.x));

			CreateGameObject(object, ELevelLayer::E_Camera);
			}, &);
		TASK_MANAGER->AddTask(info);
	}
}