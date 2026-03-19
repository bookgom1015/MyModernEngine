#pragma once

#include "ALevel.hpp"

class LevelManager : public Singleton<LevelManager> {
	SINGLETON(LevelManager);

public:
	bool Initialize();
	bool Update();

public:
	Ptr<GameObject> FindObjectByName(const std::wstring& name);

public:
	__forceinline Ptr<ALevel> GetCurrentLevel() const noexcept;

	__forceinline ELevelState::Type GetLevelState() const noexcept;

private:
	Ptr<ALevel> mCurrentLevel;
	Ptr<ALevel> mSharedLevel;

	ELevelState::Type mLevelState;

	bool mbLevelResetRequested;
};

#include "LevelManager.inl"

#ifndef LEVEL_MANAGER
#define LEVEL_MANAGER LevelManager::GetInstance()
#endif // LEVEL_MANAGER