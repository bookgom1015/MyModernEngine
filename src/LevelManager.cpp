#include "pch.h"
#include "LevelManager.hpp"

#include "TimeManager.hpp"
#include "EditorManager.hpp"

#include "Inspector.hpp"

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

			accumulatedTime -= gFixedDT;

			if (++steps >= gMaxSteps) {
				accumulatedTime = 0.f;
				break;
			}
		}

		CheckReturn(mCurrentLevel->LateUpdate(DT));
	}

	CheckReturn(mCurrentLevel->Final());

	return true;
}

Ptr<GameObject> LevelManager::FindObjectByName(const std::wstring& name) {
	if (mCurrentLevel != nullptr) return mCurrentLevel->FindObjectByName(name);

	return nullptr;
}

void LevelManager::ChangeLevel(Ptr<ALevel> newLevel) {
	mCurrentLevel = mSharedLevel = newLevel;
	mLevelState = ELevelState::E_Stopped;

	mCurrentLevel->Change();
}

void LevelManager::ChangeLevelState(ELevelState::Type newState) {
	if (mLevelState == newState) return;

	// Stop -> Play
	if (mLevelState == ELevelState::E_Playing && mbLevelResetRequested) {
		mbLevelResetRequested = false;

		// 원본 에셋 레벨의 복제본 레벨을 만들어서 현재 레벨로 가리킨다.
		mCurrentLevel = mSharedLevel->Clone();
		mCurrentLevel->Change();
		mCurrentLevel->Begin();

		auto ui = EDITOR_MANAGER->FindUI("Inspector");
		auto inspector = static_cast<Inspector*>(ui.Get());
		inspector->NeedToResetTarget();
	}
	else if (mLevelState == ELevelState::E_Stopped) {
		mbLevelResetRequested = true;

		mCurrentLevel = mSharedLevel;
		mCurrentLevel->Change();

		auto ui = EDITOR_MANAGER->FindUI("Inspector");
		auto inspector = static_cast<Inspector*>(ui.Get());
		inspector->NeedToResetTarget();
	}

	mLevelState = newState;
}