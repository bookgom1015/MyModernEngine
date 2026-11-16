#include "pch.h"
#include "AssetManager.hpp"
#include "EditorManager.hpp"

#include <cwctype>
#include <utility>
#include "GltfLoader.hpp"

namespace {
	const std::unordered_set<std::wstring> TextureExtensions{
		L".png", L".jpg", L".jpeg", L".bmp", L".dds"
	};
	const std::unordered_set<std::wstring> LevelExtensions{ L".lv" };

	std::wstring GetAssetExtension(const std::filesystem::path& path) {
		auto extension = path.extension().wstring();
		std::transform(extension.begin(), extension.end(), extension.begin(), [](wchar_t c) {
			return static_cast<wchar_t>(std::towlower(c));
		});
		return extension;
	}
}

AssetManager::AssetManager()
	: mAssets{}
	, mbChanged{}
	, mDelay{ 250 } {}

AssetManager::~AssetManager() {
	if (mhWatcherStop)
		SetEvent(mhWatcherStop);

	if (mWatcherThread.joinable())
		mWatcherThread.join();

	if (mhWatcherStop)
		CloseHandle(mhWatcherStop);
}

bool AssetManager::Initialize() {
	CheckReturn(CreateBasicGeometries());
	CheckReturn(CreateDefaultMaterial());

	CheckReturn(LoadTextures());
	CheckReturn(LoadMeshes());
	CheckReturn(LoadGltfAssetBundles());
	CheckReturn(LoadSounds());
	CheckReturn(LoadLevels());

	mhWatcherStop = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	if (!mhWatcherStop)
		ReturnFalse("Failed to create asset watcher stop event: {}", GetLastError());

	mWatcherThread = std::thread(
		&AssetManager::WatchDirectory, this
		, std::format(L"{}", CONTENT_PATH));

	return true;
}

bool AssetManager::Update() {
	std::vector<std::wstring> paths;
	std::vector<std::wstring> logs;
	bool rescan = false;
	{
		std::lock_guard lock(mWatcherMutex);
		const auto now = std::chrono::steady_clock::now();

		for (auto iter = mFileStamps.begin(); iter != mFileStamps.end();) {
			if (now - iter->second >= std::chrono::milliseconds(mDelay)) {
				paths.push_back(iter->first);
				iter = mFileStamps.erase(iter);
			}
			else {
				++iter;
			}
		}

		logs.swap(mLogs);
		rescan = std::exchange(mbRescanPending, false);
	}

	for (const auto& log : logs)
		LOG_INFO(WStrToStr(log));

	const auto import = [this]<typename T>(const std::wstring& key) {
		if (Find<T>(key) == nullptr && Load<T>(key, key) == nullptr)
			LOG_INFO(std::format("Failed to import asset: {}", WStrToStr(key)));
	};

	for (const auto& key : paths) {
		const std::filesystem::path relativePath(key);
		const auto folder = *relativePath.begin();
		if (folder != L"Texture" && folder != L"Level")
			continue;

		std::error_code error;
		const auto status = std::filesystem::status(CONTENT_PATH + key, error);
		if (std::filesystem::is_directory(status)) {
			rescan = true;
			continue;
		}
		else if (!std::filesystem::is_regular_file(status)) {
			continue;
		}

		const auto extension = GetAssetExtension(relativePath);
		if (folder == L"Texture" && TextureExtensions.contains(extension))
			import.operator()<ATexture>(key);
		else if (folder == L"Level" && LevelExtensions.contains(extension))
			import.operator()<ALevel>(key);
	}

	if (rescan) {
		// 누락된 변경 알림이나 이동된 디렉터리의 파일을 찾습니다.
		LoadAssets(L"Texture", TextureExtensions, [&](const std::wstring& key) {
			import.operator()<ATexture>(key);
			return true;
		});
		LoadAssets(L"Level", LevelExtensions, [&](const std::wstring& key) {
			import.operator()<ALevel>(key);
			return true;
		});
	}

	return true;
}

bool AssetManager::IsChanged() {
	bool changed = mbChanged;
	mbChanged = false;

	return changed;
}

void AssetManager::WatchDirectory(const std::wstring& folderPath) {
	const auto logError = [this](std::wstring_view operation, DWORD error) {
		std::lock_guard lock(mWatcherMutex);
		mLogs.push_back(std::format(L"{} failed: {}", operation, error));
	};

	const auto directory = CreateFileW(folderPath.c_str(), FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr);
	if (directory == INVALID_HANDLE_VALUE) {
		logError(L"CreateFileW", GetLastError());

		return;
	}

	const auto completion = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	if (!completion) {
		logError(L"CreateEventW", GetLastError());
		CloseHandle(directory);

		return;
	}

	alignas(DWORD) BYTE buffer[16 * 1024]{};
	const HANDLE events[]{ mhWatcherStop, completion };
	while (WaitForSingleObject(mhWatcherStop, 0) == WAIT_TIMEOUT) {
		OVERLAPPED request{};
		request.hEvent = completion;
		ResetEvent(completion);

		if (!ReadDirectoryChangesW(directory, buffer, sizeof(buffer), TRUE,
			FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
			nullptr, &request, nullptr)) {
			logError(L"ReadDirectoryChangesW", GetLastError());
			break;
		}

		DWORD bytesReturned{};
		const auto wait = WaitForMultipleObjects(_countof(events), events, FALSE, INFINITE);
		if (wait != WAIT_OBJECT_0 + 1) {
			if (wait == WAIT_FAILED)
				logError(L"WaitForMultipleObjects", GetLastError());

			// 요청이 완료될 때까지 버퍼와 OVERLAPPED를 유지합니다.
			CancelIoEx(directory, &request);
			GetOverlappedResult(directory, &request, &bytesReturned, TRUE);

			break;
		}

		if (!GetOverlappedResult(directory, &request, &bytesReturned, FALSE)) {
			const auto error = GetLastError();
			if (error != ERROR_NOTIFY_ENUM_DIR) {
				logError(L"GetOverlappedResult", error);
				break;
			}
			bytesReturned = 0;
		}

		{
			std::lock_guard lock(mWatcherMutex);
			if (bytesReturned == 0) {
				mbRescanPending = true;
				continue;
			}

			for (DWORD offset = 0; offset < bytesReturned;) {
				const auto* notify = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(buffer + offset);
				const std::wstring key(notify->FileName, notify->FileNameLength / sizeof(WCHAR));

				switch (notify->Action) {
				case FILE_ACTION_ADDED:
				case FILE_ACTION_MODIFIED:
				case FILE_ACTION_RENAMED_NEW_NAME:
					mFileStamps[key] = std::chrono::steady_clock::now();
					break;
				case FILE_ACTION_REMOVED:
				case FILE_ACTION_RENAMED_OLD_NAME:
					mFileStamps.erase(key);
					break;
				}

				if (notify->NextEntryOffset == 0)
					break;

				offset += notify->NextEntryOffset;
			}
		}
	}

	CloseHandle(completion);
	CloseHandle(directory);
}

bool AssetManager::AddAsset(const std::wstring& key, Ptr<Asset> asset) {
	if (asset == nullptr || key.empty())
		ReturnFalse("Asset and key must be valid");

	if (mAssets[asset->GetType()].contains(key))
		ReturnFalse("Asset with key '{}' already exists.", WStrToStr(key));

	asset->SetKey(key);
	asset->SetRelativePath(key);

	return RegisterAsset(asset);
}

bool AssetManager::RegisterAsset(Ptr<Asset> asset) {
	const auto& key = asset->GetKey();
	auto& assets = mAssets[asset->GetType()];

	const auto [iter, inserted] = assets.try_emplace(key, asset);
	if (!inserted)
		ReturnFalse("Asset with key '{}' already exists.", WStrToStr(key));

	if (!asset->OnAdded()) {
		assets.erase(iter);
		return false;
	}

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

bool AssetManager::LoadAssets(
	const std::wstring& folder
	, const std::unordered_set<std::wstring>& extensions
	, const LoadFunc& func) {

	const std::filesystem::path contentPath(CONTENT_PATH);
	const auto root = contentPath / folder;

	std::error_code error;
	std::filesystem::recursive_directory_iterator iter(root, error), end;

	for (; !error && iter != end; iter.increment(error)) {
		std::error_code statusError;
		if (!iter->is_regular_file(statusError))
			continue;

		if (extensions.contains(GetAssetExtension(iter->path())))
			CheckReturn(func(iter->path().lexically_relative(contentPath).wstring()));
	}

	if (error)
		ReturnFalse("Could not enumerate asset folder '{}': {}", WStrToStr(root.wstring()), error.message());

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
	CheckReturn(LoadAssets(L"Texture", TextureExtensions, [&](const std::wstring& path) {
		auto texture = LOAD(ATexture, path.c_str());
		if (texture == nullptr) 
			ReturnFalse("Failed to load texture: {}", WStrToStr(path));
		return 
			true;
	}));

	return true;
}

bool AssetManager::LoadMeshes() { return true; }

bool AssetManager::LoadGltfAssetBundles() {
	CheckReturn(LoadAssets(L"Gltf",
		{
			L".glb",
			L".gltf"
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

bool AssetManager::LoadSounds() {
	CheckReturn(LoadAssets(L"Sound",
		{
			L".wav"
		}
		, [&](const std::wstring& path) {
			auto sound = LOAD(ASound, path.c_str());
			if (sound == nullptr) ReturnFalse("Failed to load sound: {}", WStrToStr(path));
			return true;
		}));

	return true;
}

bool AssetManager::LoadLevels() {
	CheckReturn(LoadAssets(L"Level", LevelExtensions
		, [&](const std::wstring& path) {
			auto level = LOAD(ALevel, path.c_str());
			if (level == nullptr) ReturnFalse("Failed to load level: {}", WStrToStr(path));
			return true;
		}));

	return true;
}