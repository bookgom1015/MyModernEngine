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
	// 동일키로 먼저 등록된 에셋이 있는지 확인
	Ptr<T> pAsset = Find<T>(key);
	// 동일키로 먼저 등록된 에셋이 있으면, 그걸 반환
	if (pAsset != nullptr) return pAsset;

	// 에셋 객체 생성
	pAsset = NEW T;

	// 에셋이 자신이 매니저에 등롣될때 상요된 Key 와, 
	// 자신이 어떤 경로에 있는 파일로부터 로딩된 에셋인지 스스로 알 수 있도록 해줌
	pAsset->SetKey(key);
	pAsset->SetRelativePath(filePath);

	// 입력된 경로로부터 에셋 로딩작업 진행	
	pAsset->Load(CONTENT_PATH + filePath);

	// T 타입에 해당하는 실제 AssetType 확인
	EAsset::Type type = GetAssetType<T>();

	// 맵에 에셋등록
	mAssets[type].insert(std::make_pair(key, pAsset.Get()));

	pAsset->OnAdded();
	mbChanged = true;

	return pAsset;
}

template <typename T>
Ptr<T> AssetManager::ForceLoad(const std::wstring& key, const std::wstring& filePath) {
	Ptr<T> pAsset = NEW T;
	pAsset->Load(CONTENT_PATH + filePath);

	EAsset::Type type = GetAssetType<T>();
	mAssets[type].insert(std::make_pair(key, pAsset.Get()));

	pAsset->SetKey(key);
	pAsset->SetRelativePath(filePath);

	pAsset->OnAdded();
	mbChanged = true;

	return pAsset;
}

template<typename T>
Ptr<T> LoadAssetRef(FILE* pFile) {
	// Asset 이 Null 인지 아닌지 저장
	bool IsNull = false;
	fread(&IsNull, sizeof(bool), 1, pFile);

	// Asset 의 Key, RelativePath 저장
	if (IsNull) {
		std::wstring key = LoadWString(pFile);
		std::wstring filePath = LoadWString(pFile);
		return AssetManager::GetInstance()->Load<T>(key, filePath);
	}

	return nullptr;
}

#endif // __ASSET_MANAGER_INL__