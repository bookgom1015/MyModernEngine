#pragma once

#include "Asset.hpp"
#include "Assets.hpp"

#include "PathManager.hpp"

class AssetManager : public Singleton<AssetManager> {
	SINGLETON(AssetManager);

public:
	using FileStamp = std::pair<std::chrono::steady_clock::time_point, std::wstring>;
	using LoadFunc = std::function<bool(const std::wstring&)>;

public:
	bool Initialize();
	bool Update();

public:
	bool IsChanged();

	void WatchDirectory(const std::wstring& folderPath);

	bool AddAsset(const std::wstring& key, Ptr<Asset> asset);
	bool GetAssetNames(EAsset::Type type, std::vector<std::wstring>& names) const;

	Ptr<Asset> FindAsset(EAsset::Type type, const std::wstring& key) const;

	template <typename T>
	Ptr<T> Find(const std::wstring& key) const;

	template <typename T>
	Ptr<T> Load(const std::wstring& key, const std::wstring& filePath);

	template <typename T>
	Ptr<T> ForceLoad(const std::wstring& key, const std::wstring& filePath);

private:
	void CreateStamp(const std::wstring& fileName);

	bool LoadAssets(
		const std::wstring& folder,
		const std::unordered_set<std::string>& extensions,
		const LoadFunc& func);

	bool CreateBasicGeometries();
	bool CreateDefaultMaterial();

	bool LoadTextures();
	bool LoadMeshes();
	bool LoadGltfAssetBundles();
	bool LoadLevels();

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

#include "AssetManager.inl"

#ifndef ASSET_MANAGER
#define ASSET_MANAGER AssetManager::GetInstance()
#endif // ASSET_MANAGER

#define FIND(__type, __key) ASSET_MANAGER->Find<__type>(__key)
#define LOAD(__type, __path) ASSET_MANAGER->Load<__type>(__path, __path)
#define FORCE_LOAD(__type, __path) ASSET_MANAGER->ForceLoad<__type>(__path, __path)
