#include "pch.h"
#include "TimeManager.hpp"

#include "LevelManager.hpp"

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
	// 1 초동안 가능한 카운팅 횟수
	QueryPerformanceFrequency(&mFrequency);

	// 초기 카운트
	QueryPerformanceCounter(&mCurrTime);
	mPrevTime = mCurrTime;

	return true;
}

bool TimeManager::Update() {
	// 현재 카운팅 가져오기
	QueryPerformanceCounter(&mCurrTime);

	// 이전과 현재 카운팅 차이를 Frequency 로 나눠서 1 프레임동안 진행한 시간값을 구하기
	mDeltaTime = static_cast<float>(mCurrTime.QuadPart - mPrevTime.QuadPart) 
		/ (float)mFrequency.QuadPart;

	// Prev 카운팅을 다시 현재카운팅으로 맞추기
	mPrevTime = mCurrTime;

	// 누적 시간 계산
	mTotalTime += mDeltaTime;

	// Game Engine용 Time
	gEngineDeltaTime = mDeltaTime;
	gEngineTotalTime += mDeltaTime;

	// Level 이 Pause 나 Stop 상태라면
	if (LevelManager::GetInstance()->GetCurrentLevelState() != ELevelState::E_Playing) {
		gEngineDeltaTime = mDeltaTime = 0.f;
		gEngineTotalTime = 0.f;
	}
	// Level 이 Play 상태
	else {
		// Game Content 용 Time
		gEngineDeltaTime = mDeltaTime;
		gEngineTotalTime += mDeltaTime;
	}

	return true;
}

float TimeManager::GetEngineDT() { return gEngineDeltaTime; }

float TimeManager::GetEngineTime() { return gEngineTotalTime; }