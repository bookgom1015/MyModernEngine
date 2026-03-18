#include "pch.h"
#include "LevelManager.hpp"

#include "TimeManager.hpp"

namespace {
	const float gFixedDT = 1.f / 60.f;
	const int gMaxSteps = 8; // 폭주 방지
}

LevelManager::LevelManager() 
	: mCurrentLevel{}
	, mSharedLevel{}
	, mLevelState{ELevelState::E_Playing}
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