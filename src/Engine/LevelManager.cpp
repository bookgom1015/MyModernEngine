#include "Engine/pch.h"
#include "Engine/LevelManager.hpp"

#include "Engine/Engine.hpp"

#include "Engine/TimeManager.hpp"
#include "Engine/EditorManager.hpp"
#include "Engine/ScriptManager.hpp"
#include "Engine/TaskManager.hpp"
#include "Engine/PhysicsManager.hpp"

#include "Engine/Outliner.hpp"

#include "Engine/CLight.hpp"

#include "Engine/Script/CMoveFreeCameraScript.hpp"

namespace {
	const float gFixedDT = 1.f / 60.f;
	const int gMaxSteps = 8;
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
			CheckReturn(PHYSICS_MANAGER->FixedUpdate(gFixedDT));
			CheckReturn(mCurrentLevel->FixedUpdate(gFixedDT));

			accumulatedTime -= gFixedDT;

			// 최대 스텝 수를 초과하면 루프를 종료하고 누적 시간을 초기화하여
			// 다음 프레임에서 다시 시도하도록 합니다.
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
	if (mCurrentLevel) return mCurrentLevel->FindObjectByName(name);

	return nullptr;
}

void LevelManager::ChangeLevel(Ptr<ALevel> newLevel) {
	if (mCurrentLevel) mCurrentLevel->OnUnloaded();

	mCurrentLevel = mSharedLevel = newLevel->Clone();
	mLevelState = ELevelState::E_Stopped;
	mCurrentLevel->Change();

	mCurrentLevel->OnLoaded();
}

void LevelManager::ChangeLevelState(ELevelState::Type newState) {
	if (mLevelState == newState) return;

	// 정지 상태에서 재생 상태로 전환합니다.
	if (newState == ELevelState::E_Playing && mbLevelResetRequested) {
		mbLevelResetRequested = false;

		// 현재 레벨에 카메라가 존재하지 않으면 기본 카메라를 생성합니다.
		MakeDefaultCameraIfNecessary();

		mCurrentLevel->OnUnloaded();
			
		// 원본 에셋레벨을 보존하고, 현재 레벨을 복제하여 플레이 상태로 전환합니다.
		mCurrentLevel = mSharedLevel->Clone();
		mCurrentLevel->Change();
		mCurrentLevel->Begin();

		mCurrentLevel->OnLoaded();

		auto ui = EDITOR_MANAGER->FindUI("Outliner");
		auto outliner = static_cast<Outliner*>(ui.Get());
		outliner->RestoreSelection();
	}
	// 일시 정지 또는 재생 상태에서 정지 상태로 전환합니다.
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