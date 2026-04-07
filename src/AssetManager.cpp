#include "pch.h"
#include "AssetManager.hpp"

#include "EditorManager.hpp"
#include "GltfLoader.hpp"

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

bool AssetManager::Initialize() {
	CheckReturn(CreateBasicGeometries());
	CheckReturn(CreateDefaultMaterial());

	CheckReturn(LoadTextures());
	CheckReturn(LoadMeshes());
	CheckReturn(LoadGltfAssetBundles());
	CheckReturn(LoadLevels());

	mWatcherThread = std::thread(
		&AssetManager::WatchDirectory, this
		, std::format(L"{}", CONTENT_PATH));

	return true;
}

bool AssetManager::Update() {
	std::lock_guard<std::mutex> lock(mWatcherMutex);

	const auto now = std::chrono::steady_clock::now();

	for (auto iter = mFileStamps.begin(); iter != mFileStamps.end();) {
		auto timePoint = iter->first;

		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
			now - timePoint).count();
		if (elapsed < mDelay) {
			++iter;
			continue;
		}

		auto filePath = iter->second;
		auto ext = filePath.find(L".");
		if (ext == std::wstring::npos) {
			++iter;
			continue;
		}

		auto delim = filePath.find_first_of(L"\\");
		auto folder = filePath.substr(0, delim);

		switch (HashWString(folder)) {
		case HashWString(L"Texture"): {
			FORCE_LOAD(ATexture, filePath);
		}
			break;
		case HashWString(L"Sprite"): {
			//FORCE_LOAD(ASprite, filePath);
		}
			break;
		case HashWString(L"TileMap"): {
			//FORCE_LOAD(ATileMap, filePath);
		}
			break;
		case HashWString(L"Flipbook"): {
			//FORCE_LOAD(AFlipbook, filePath);
		}
			break;
		case HashWString(L"Level"): {
			FORCE_LOAD(ALevel, filePath);
		}
			break;
		}

		auto idx = iter - mFileStamps.begin();

		std::iter_swap(iter, mFileStamps.end() - 1);
		mFileStamps.pop_back();

		iter = mFileStamps.begin() + idx;
	}

	for (size_t i = 0, end = static_cast<size_t>(mLogs.size()); i < end; ++i)
		LOG_INFO(WStrToStr(mLogs[i]));
	mLogs.clear();

	return true;
}

bool AssetManager::IsChanged() {
	bool changed = mbChanged;
	mbChanged = false;

	return changed;
}

void AssetManager::WatchDirectory(const std::wstring& folderPath) {
	mhDirectory = CreateFileW(
		folderPath.c_str(),
		FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		nullptr,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		nullptr);

	if (mhDirectory == INVALID_HANDLE_VALUE) {
		std::lock_guard<std::mutex> lock(mWatcherMutex);

		mLogs.push_back(std::format(L"CreateFileW failed: {}", GetLastError()));

		return;
	}

	std::vector<BYTE> buffer(16 * 1024);
	DWORD bytesReturned = 0;

	while (!mbQuit) {		
		BOOL ok = ReadDirectoryChangesW(
			mhDirectory,
			buffer.data(),
			static_cast<DWORD>(buffer.size()),
			TRUE, // 하위 폴더까지 감시
			FILE_NOTIFY_CHANGE_FILE_NAME |
			FILE_NOTIFY_CHANGE_DIR_NAME |
			FILE_NOTIFY_CHANGE_LAST_WRITE |
			FILE_NOTIFY_CHANGE_CREATION,
			&bytesReturned,
			nullptr,
			nullptr
		);
		
		if (!ok) {
			std::lock_guard<std::mutex> lock(mWatcherMutex);

			auto err = GetLastError();

			if (err == ERROR_OPERATION_ABORTED)
				break; // CancelIoEx로 종료됨

			mLogs.push_back(std::format(L"ReadDirectoryChangesW failed: {}", err));
			
			// 다른 에러 처리
			continue;
		}

		BYTE* ptr = buffer.data();
		while (!mbQuit) {
			auto* notify = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(ptr);

			std::wstring fileName(notify->FileName,
				notify->FileNameLength / sizeof(WCHAR));
			
			switch (notify->Action) {
			case FILE_ACTION_ADDED: {				
				std::lock_guard<std::mutex> lock(mWatcherMutex);
				
				mLogs.push_back(std::format(
					L"[Created] {}", fileName));

				CreateStamp(fileName);
			}
								  break;
			case FILE_ACTION_REMOVED: {
				std::lock_guard<std::mutex> lock(mWatcherMutex);

				mLogs.push_back(std::format(
					L"[Deleted] {}", fileName));
			}
									break;
			case FILE_ACTION_MODIFIED: {
				std::lock_guard<std::mutex> lock(mWatcherMutex);

				mLogs.push_back(std::format(
					L"[Modified] {}", fileName));

				CreateStamp(fileName);
			}
									 break;
			case FILE_ACTION_RENAMED_OLD_NAME: {
				std::lock_guard<std::mutex> lock(mWatcherMutex);

				mLogs.push_back(std::format(
					L"[Renamed Old] {}", fileName));
			}
											 break;
			case FILE_ACTION_RENAMED_NEW_NAME: {
				std::lock_guard<std::mutex> lock(mWatcherMutex);

				mLogs.push_back(std::format(
					L"[Renamed New] {}", fileName));
			}
											 break;
			default: {
				std::lock_guard<std::mutex> lock(mWatcherMutex);

				mLogs.push_back(std::format(
					L"[UnknownNew] {}", fileName));
			}
				   break;
			}

			if (notify->NextEntryOffset == 0)
				break;

			ptr += notify->NextEntryOffset;
		}
	}

	CloseHandle(mhDirectory);
}

bool AssetManager::AddAsset(const std::wstring& key, Ptr<Asset> asset) {
	auto iter = mAssets[asset->GetType()].find(key);
	auto end = mAssets[asset->GetType()].end();
	if (iter != end) ReturnFalse(std::format("Asset with key '{}' already exists.", WStrToStr(key)));

	asset->SetKey(key);
	asset->SetRelativePath(key);
	mAssets[asset->GetType()].insert(make_pair(key, asset));

	CheckReturn(asset->OnAdded());

	mbChanged = true;

	return true;
}

bool AssetManager::GetAssetNames(EAsset::Type type, std::vector<std::wstring>& names) const {
	for (const auto& pair : mAssets[type])
		names.push_back(pair.first);

	return true;
}

Ptr<Asset> AssetManager::FindAsset(EAsset::Type type, const std::wstring& key) const {
	auto iter = mAssets[type].find(key);
	if (iter == mAssets[type].end()) return nullptr;

	return iter->second;
}

void AssetManager::CreateStamp(const std::wstring& fileName) {
	FileStamp stamp = { std::chrono::steady_clock::now(), fileName };

	auto iter = std::find_if(
		mFileStamps.begin(), mFileStamps.end(), [&](const FileStamp& stamp) {
			return stamp.second == fileName;
		}
	);
	if (iter != mFileStamps.end()) {
		if (stamp.first > iter->first) {
			iter_swap(iter, mFileStamps.end() - 1);
			mFileStamps.pop_back();

			mFileStamps.push_back(stamp);
		}
	}
	else {
		mFileStamps.push_back(stamp);
	}
}

bool AssetManager::LoadAssets(
	const std::wstring& folder
	, const std::unordered_set<std::string>& extensions
	, const LoadFunc& func) {
	auto contentPath = std::wstring(CONTENT_PATH);
	auto folderPath = std::format(L"{}{}", contentPath, folder);
	auto root = std::filesystem::path(folderPath);
	if (!std::filesystem::exists(root)) ReturnFalseFormat("Could not find root folder {}", root.string());

	for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
		if (!entry.is_regular_file()) continue;

		std::string ext = entry.path().extension().string();
		if (extensions.contains(ext)) {
			auto filePath = StrToWStr(entry.path().string());
			auto filePathAfterConent = filePath.substr(contentPath.size());

			CheckReturn(func(filePathAfterConent));
		}
	}

	return true;
}

bool AssetManager::CreateBasicGeometries() {
	Ptr<AMesh> boxMesh = NEW AMesh;
	boxMesh->CreateBox();
	CheckReturn(AddAsset(L"Box", boxMesh.Get()));

	Ptr<AMesh> sphereMesh = NEW AMesh;
	sphereMesh->CreateSphere();
	CheckReturn(AddAsset(L"Sphere", sphereMesh.Get()));

	Ptr<AMesh> planeMesh = NEW AMesh;
	planeMesh->CreatePlane();
	CheckReturn(AddAsset(L"Plane", planeMesh.Get()));

	Ptr<AMesh> cylinderMesh = NEW AMesh;
	cylinderMesh->CreateCylinder();
	CheckReturn(AddAsset(L"Cylinder", cylinderMesh.Get()));

	Ptr<AMesh> pyramidMesh = NEW AMesh;
	pyramidMesh->CreatePyramid();
	CheckReturn(AddAsset(L"Pyramid", pyramidMesh.Get()));

	Ptr<AMesh> torusMesh = NEW AMesh;
	torusMesh->CreateTorus();
	CheckReturn(AddAsset(L"Torus", torusMesh.Get()));

	Ptr<AMesh> prismMesh = NEW AMesh;
	prismMesh->CreatePrism();
	CheckReturn(AddAsset(L"Prism", prismMesh.Get()));

	Ptr<AMesh> hemiSphereMesh = NEW AMesh;
	hemiSphereMesh->CreateHemisphere();
	CheckReturn(AddAsset(L"Hemisphere", hemiSphereMesh.Get()));

	Ptr<AMesh> capsuleMesh = NEW AMesh;
	capsuleMesh->CreateCapsule();
	CheckReturn(AddAsset(L"Capsule", capsuleMesh.Get()));

	Ptr<AMesh> tetrahedronMesh = NEW AMesh;
	tetrahedronMesh->CreateTetrahedron();
	CheckReturn(AddAsset(L"Tetrahedron", tetrahedronMesh.Get()));

	Ptr<AMesh> octahedronMesh = NEW AMesh;
	octahedronMesh->CreateOctahedron();
	CheckReturn(AddAsset(L"Octahedron", octahedronMesh.Get()));

	Ptr<AMesh> icosahedronMesh = NEW AMesh;
	icosahedronMesh->CreateIcosahedron();
	CheckReturn(AddAsset(L"Icosahedron", icosahedronMesh.Get()));

	return true;
}

bool AssetManager::CreateDefaultMaterial() {
	Ptr<AMaterial> defaultMat = NEW AMaterial;
	defaultMat->SetAlbedo(Vec3(1.f));
	defaultMat->SetSpecular(1.f);
	defaultMat->SetRoughness(0.5f);	
	defaultMat->SetMetalic(0.f);
	defaultMat->SetRenderDomain(ERenderDomain::E_Opaque);
	CheckReturn(AddAsset(L"Default Material", defaultMat.Get()));

	Ptr<AMaterial> skySphereMat = NEW AMaterial;
	skySphereMat->SetRenderDomain(ERenderDomain::E_SkySphere);
	CheckReturn(AddAsset(L"Sky Sphere Material", skySphereMat.Get()));

	return true;
}

bool AssetManager::LoadTextures() {
	CheckReturn(LoadAssets(L"Texture\\",
		{
			".png",
			".jpg",
			".jpeg",
			".bmp",
			".dds"
		}
		, [&](const std::wstring& path) {
			auto texture = LOAD(ATexture, path.c_str());
			if (texture == nullptr) ReturnFalseFormat("Failed to load texture: {}", WStrToStr(path));
			return true;
		}));

	return true;
}

bool AssetManager::LoadMeshes() { return true; }

bool AssetManager::LoadGltfAssetBundles() {
	CheckReturn(LoadAssets(L"Gltf\\",
		{
			".glb",
			".gltf"
		}
		, [&](const std::wstring& path) {
			std::wstring filePath = CONTENT_PATH + path;

			GltfLoadResultCPU gltf{};
			CheckReturn(GltfLoader::LoadGltfCpu(WStrToStr(filePath), gltf));

			const std::wstring baseKey = path;

			{
				auto mesh = NEW AMesh;
				CheckReturn(mesh->BuildFromGltf(filePath, gltf.Mesh));
				CheckReturn(AddAsset(baseKey + L":Mesh", mesh));
			}

			if (!gltf.Skeleton.Skins.empty()) {
				auto skeleton = NEW ASkeleton;
				CheckReturn(skeleton->BuildFromGltf(filePath, gltf.Skeleton));
				CheckReturn(AddAsset(baseKey + L":Skeleton", skeleton));
			}

			if (!gltf.AnimationSet.Animations.empty()) {
				auto animSet = NEW AAnimationClip;
				CheckReturn(animSet->BuildFromGltf(filePath, gltf.AnimationSet));
				CheckReturn(AddAsset(baseKey + L":Animation", animSet));
			}

			return true;
		}));

	return true;
}

bool AssetManager::LoadLevels() {
	CheckReturn(LoadAssets(L"Level\\",
		{
			".lv"
		}
		, [&](const std::wstring& path) {
			auto level = LOAD(ALevel, path.c_str());
			if (level == nullptr) ReturnFalseFormat("Failed to load level: {}", WStrToStr(path));
			return true;
		}));

	return true;
}