#pragma once

#include "Asset.hpp"

class AssetManager : public Singleton<AssetManager> {
	SINGLETON(AssetManager);

public:
	using FileStamp = std::pair<std::chrono::steady_clock::time_point, std::wstring>;

public:
	bool Initlaize();
	bool Update();

private:
	std::map<std::wstring, Ptr<Asset>> mAssets[EAsset::Count];
	bool mbChanged;

	std::thread mWatcherThread;
	std::mutex mWatcherMutex;
	bool mbQuit;

	std::vector<std::wstring> mLogs;
	std::vector<FileStamp> mFileStamps;

	HANDLE mhDirectory;

	long long mDelay;
};