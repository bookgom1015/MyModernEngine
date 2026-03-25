#pragma once

#include "ALevel.hpp"

class LevelManager : public Singleton<LevelManager> {
	SINGLETON(LevelManager);

public:
	bool Initialize();
	bool Update();

public:
	Ptr<GameObject> FindObjectByName(const std::wstring& name);

	void ChangeLevel(Ptr<ALevel> newLevel);
	void ChangeLevelState(ELevelState::Type newState);

	UINT GetLightCount() const;
	const struct LightData* GetLightData(size_t idx) const;

public:
	__forceinline Ptr<ALevel> GetCurrentLevel() const noexcept;

	__forceinline ELevelState::Type GetCurrentLevelState() const noexcept;

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