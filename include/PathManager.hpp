#pragma once

class PathManager : public Singleton<PathManager> {
	SINGLETON(PathManager);

public:
	bool Initialize();

	__forceinline const wchar_t* GetContentPath() const noexcept;

private:
	wchar_t mContentPath[255];
};

#include "PathManager.inl"

#ifndef CONTENT_PATH
#define CONTENT_PATH PathManager::GetInstance()->GetContentPath()
#endif