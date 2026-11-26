#ifndef __TIMEMANAGER_INL__
#define __TIMEMANAGER_INL__

constexpr float TimeManager::GetDeltaTime() const noexcept { return mDeltaTime; }

constexpr float TimeManager::GetTime() const noexcept { return mTotalTime; }

#endif // __TIMEMANAGER_INL__