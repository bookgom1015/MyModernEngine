#pragma once

class TimeManager : public Singleton<TimeManager> {
	SINGLETON(TimeManager);

public:
	bool Initialize();
	bool Update();

public:
	__forceinline constexpr float GetDeltaTime() const noexcept;
	__forceinline constexpr float GetTime() const noexcept;

	float GetEngineDT();
	float GetEngineTime();

private:
	LARGE_INTEGER mFrequency;
	LARGE_INTEGER mPrevTime;
	LARGE_INTEGER mCurrTime;

	float mDeltaTime;
	float mTotalTime;
};

#include "TimeManager.inl"

#ifndef DT
#define DT TimeManager::GetInstance()->GetDeltaTime()
#endif

#ifndef Time
#define Time TimeManager::GetInstance()->GetTime()
#endif

#ifndef E_DT
#define E_DT TimeManager::GetInstance()->GetEngineDT()
#endif

#ifndef E_Time
#define E_Time TimeManager::GetInstance()->GetEngineTime()
#endif
