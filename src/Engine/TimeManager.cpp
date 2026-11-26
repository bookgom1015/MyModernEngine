#include "Engine/pch.h"
#include "Engine/TimeManager.hpp"

#include "Engine/LevelManager.hpp"

namespace {
	float gEngineDeltaTime = 0.f;
	float gEngineTotalTime = 0.f;
}

TimeManager::TimeManager() 
	: mFrequency{}
	, mPrevTime{}
	, mCurrTime{}
	, mDeltaTime{}
	, mTotalTime{} {}

TimeManager::~TimeManager() {}

bool TimeManager::Initialize() {
	// 1초당 카운터 틱 수입니다.
	QueryPerformanceFrequency(&mFrequency);

	// 초기 카운터 값을 저장합니다.
	QueryPerformanceCounter(&mCurrTime);
	mPrevTime = mCurrTime;

	return true;
}

bool TimeManager::Update() {
	// 현재 카운터 값을 가져옵니다.
	QueryPerformanceCounter(&mCurrTime);

	// 이전 값과 현재 값의 차이를 초당 틱 수로 나누어 프레임 경과 시간을 구합니다.
	mDeltaTime = static_cast<float>(mCurrTime.QuadPart - mPrevTime.QuadPart) 
		/ (float)mFrequency.QuadPart;

	// 다음 프레임을 위해 이전 카운터 값을 현재 값으로 갱신합니다.
	mPrevTime = mCurrTime;

	// 누적 시간을 계산합니다.
	mTotalTime += mDeltaTime;

	// 게임 엔진이 사용하는 시간입니다.
	gEngineDeltaTime = mDeltaTime;
	gEngineTotalTime += mDeltaTime;

	// 레벨이 일시 정지 또는 정지 상태인 경우입니다.
	if (LevelManager::GetInstance()->GetCurrentLevelState() != ELevelState::E_Playing) {
		mDeltaTime = 0.f;
		mTotalTime = 0.f;
	}
	// 레벨이 재생 상태인 경우입니다.
	else {
		// 게임 콘텐츠가 사용하는 시간입니다.
		mDeltaTime = mDeltaTime;
		mTotalTime += mDeltaTime;
	}

	return true;
}

float TimeManager::GetEngineDT() { return gEngineDeltaTime; }

float TimeManager::GetEngineTime() { return gEngineTotalTime; }