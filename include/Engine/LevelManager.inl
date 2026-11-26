#ifndef __LEVELMANAGER_INL__
#define __LEVELMANAGER_INL__

Ptr<ALevel> LevelManager::GetCurrentLevel() const noexcept { return mCurrentLevel; }

ELevelState::Type LevelManager::GetCurrentLevelState() const noexcept { return mLevelState; }

#endif // __LEVELMANAGER_INL__