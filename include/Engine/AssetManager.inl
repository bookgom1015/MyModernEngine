#ifndef __ASSET_MANAGER_INL__
#define __ASSET_MANAGER_INL__

template<typename T>
EAsset::Type GetAssetType() {
	if constexpr (std::is_same_v<T, AMesh>)
		return EAsset::E_Mesh;
	else if constexpr (std::is_same_v<T, AMaterial>)
		return EAsset::E_Material;
	else if constexpr (std::is_same_v<T, ATexture>)
		return EAsset::E_Texture;
	else if constexpr (std::is_same_v<T, ASprite>)
		return EAsset::E_Sprite;	
	else if constexpr (std::is_same_v<T, ALevel>)
		return EAsset::E_Level;
	else if constexpr (std::is_same_v<T, ASkeleton>)
		return EAsset::E_Skeleton;
	else if constexpr (std::is_same_v<T, AAnimationClip>)
		return EAsset::E_AnimationClip;
	else if constexpr (std::is_same_v<T, ASound>)
		return EAsset::E_Sound;
	else
		static_assert(false, "Unexpected Asset Type");
}

template <typename T>
Ptr<T> AssetManager::Find(const std::wstring& key) const {
	const auto type = GetAssetType<T>();
	const int idx = static_cast<int>(type);
	assert(idx >= 0 && idx < EAsset::Count);

	auto iter = mAssets[idx].find(key);
	if (iter == mAssets[idx].end())
		return nullptr;

	return static_cast<T*>(iter->second.Get());
}

template <typename T>
Ptr<T> AssetManager::Load(const std::wstring& key, const std::wstring& filePath) {
	if (key.empty())
		return nullptr;
	if (auto asset = Find<T>(key); asset != nullptr)
		return asset;

	Ptr<T> asset = NEW T;
	asset->SetKey(key);
	asset->SetRelativePath(filePath);
	if (!asset->Load(CONTENT_PATH + filePath) || !RegisterAsset(asset.Get()))
		return nullptr;

	return asset;
}

template <typename T>
Ptr<T> AssetManager::ForceLoad(const std::wstring& key, const std::wstring& filePath) {
	if (Find<T>(key) != nullptr)
		return nullptr;

	return Load<T>(key, filePath);
}

template<typename T>
Ptr<T> LoadAssetRef(FILE* pFile) {
	// 에셋이 null인지 기록합니다.
	bool IsNull = false;
	fread(&IsNull, sizeof(bool), 1, pFile);

	// 에셋의 키와 상대 경로를 저장합니다.
	if (IsNull) {
		std::wstring key = LoadWString(pFile);
		std::wstring filePath = LoadWString(pFile);
		return AssetManager::GetInstance()->Load<T>(key, filePath);
	}

	return nullptr;
}

#endif // __ASSET_MANAGER_INL__