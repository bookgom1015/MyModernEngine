#include "pch.h"
#include "AssetManager.hpp"

AssetManager::AssetManager() 
	: mAssets{}
	, mbChanged{}
	, mbQuit{}
	, mDelay{ 250 } {}

AssetManager::~AssetManager() {
	// Watcher thread의 ReadDirectoryChangesW 함수 pending 풀어주기
	CancelIoEx(mhDirectory, nullptr);

	mbQuit = true;
	mWatcherThread.join();
}

bool AssetManager::Initlaize() {
	//mWatcherThread = std::thread(
	//	&AssetManager::WatchDirectory, this
	//	, std::format(L"{}", CONTENT_PATH));

	return true;
}

bool AssetManager::Update() {
	return true;
}