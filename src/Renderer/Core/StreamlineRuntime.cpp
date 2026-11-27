#include "Renderer/Core/pch.h"
#include "Renderer/Core/StreamlineRuntime.hpp"

#include <sl.h>
#include <sl_security.h>
#include <sl_dlss.h>
#include <d3d12.h>
#include <dxgi1_2.h>

namespace {
	const sl::ViewportHandle DlssViewport{0};

	void StreamlineLogCallback(sl::LogType type, const char* msg) {
		if (type == sl::LogType::eWarn) {
			ConsoleLog("[Streamline][Warning] {}", msg);
			Logln("[Streamline][Warning] {}", msg);
		}
		else if (type == sl::LogType::eError) {
			ConsoleLog("[Streamline][Error] {}", msg);
			Logln("[Streamline][Error] {}", msg);
		}
	}
}

StreamlineRuntime::StreamlineRuntime()
	: mStreamlineModule(nullptr)
	, mStreamlineShutdown(nullptr) {}

StreamlineRuntime::~StreamlineRuntime() { Shutdown(); }

bool StreamlineRuntime::Initialize() {
	if (mStreamlineModule) return true;

	wchar_t executablePath[32768];
	const auto pathLength = GetModuleFileNameW(nullptr, executablePath, ARRAYSIZE(executablePath));
	if (pathLength == 0 || pathLength == ARRAYSIZE(executablePath))
		ReturnFalse("[Streamline] Failed to determine the executable path");

	const auto modulePath = std::filesystem::path(executablePath).parent_path() / L"sl.interposer.dll";
	if (!sl::security::verifyEmbeddedSignature(modulePath.c_str()))
		ReturnFalse("[Streamline] Failed to verify sl.interposer.dll");

	const auto module = LoadLibraryExW(modulePath.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (!module)
		ReturnFalse("[Streamline] Failed to load sl.interposer.dll (Win32 error: {})", GetLastError());

	const auto init = reinterpret_cast<PFun_slInit*>(GetProcAddress(module, "slInit"));
	const auto shutdown = GetProcAddress(module, "slShutdown");
	if (!init || !shutdown) {
		FreeLibrary(module);
		ReturnFalse("[Streamline] sl.interposer.dll is missing slInit or slShutdown");
	}

	const sl::Feature features[] = { sl::kFeatureDLSS };
	sl::Preferences pref{};
	pref.featuresToLoad = features;
	pref.numFeaturesToLoad = ARRAYSIZE(features);
	pref.flags |= sl::PreferenceFlags::eUseManualHooking | sl::PreferenceFlags::eUseFrameBasedResourceTagging;
	pref.engineVersion = "1.0";
	pref.projectId = "0bae89f5-adb7-497c-ad1a-6e1803e92332";
#ifdef _DEBUG
	pref.showConsole = true; // for debugging, set to false in production
#else
	pref.showConsole = false;
#endif
	pref.logLevel = sl::LogLevel::eDefault;
	pref.pathsToPlugins = {}; // change this if Streamline plugins are not located next to the executable
	pref.numPathsToPlugins = 0; // change this if Streamline plugins are not located next to the executable
	pref.pathToLogsAndData = {}; // change this to enable logging to a file
	//pref.logMessageCallback = StreamlineLogCallback;

	if (const auto res = init(pref, sl::kSDKVersion); res != sl::Result::eOk) {
		FreeLibrary(module);
		ReturnFalse("[Streamline] slInit failed (result: {})", static_cast<int>(res));
	}

	mStreamlineModule = module;
	mStreamlineShutdown = shutdown;
	Logln("[Streamline] Initialized successfully");

	return true;
}

void StreamlineRuntime::Shutdown() {
	if (mStreamlineModule) {
		const auto shutdown = reinterpret_cast<PFun_slShutdown*>(mStreamlineShutdown);
		if (const auto res = shutdown(); res != sl::Result::eOk)
			ConsoleLog("[Streamline] slShutdown failed (result: {})", static_cast<int>(res));

		FreeLibrary(mStreamlineModule);
		mStreamlineModule = nullptr;
		mStreamlineShutdown = nullptr;
	}
}

bool StreamlineRuntime::CheckFeatureSupport(LUID adapterLuid) const {
	if (!mStreamlineModule) return false;

	const auto isFeatureSupported = reinterpret_cast<PFun_slIsFeatureSupported*>(
		GetProcAddress(mStreamlineModule, "slIsFeatureSupported"));
	if (!isFeatureSupported)
		ReturnFalse("[Streamline] sl.interposer.dll is missing slIsFeatureSupported");

	sl::AdapterInfo adapterInfo{};
	adapterInfo.deviceLUID = reinterpret_cast<uint8_t*>(&adapterLuid);
	adapterInfo.deviceLUIDSizeInBytes = sizeof(adapterLuid);

	const auto result = isFeatureSupported(sl::kFeatureDLSS, adapterInfo);
	if (result != sl::Result::eOk)
		ReturnFalse("[Streamline] DLSS support check failed (result: {})", static_cast<int>(result));

	return true;
}

bool StreamlineRuntime::RegisterDevice(ID3D12Device* device) const {
	const auto setDevice = reinterpret_cast<PFun_slSetD3DDevice*>(GetProcAddress(mStreamlineModule, "slSetD3DDevice"));

	if (!setDevice)
		ReturnFalse("[Streamline] sl.interposer.dll is missing slSetD3DDevice");
	if (const auto result = setDevice(device); result != sl::Result::eOk)
		ReturnFalse("[Streamline] D3D12 device registration failed (result: {})", static_cast<int>(result));

	return true;
}

bool StreamlineRuntime::UpgradeSwapChain(IDXGISwapChain1*& swapChain) const {
	const auto upgrade = reinterpret_cast<PFun_slUpgradeInterface*>(GetProcAddress(mStreamlineModule, "slUpgradeInterface"));

	void* instance = swapChain;
	if (!upgrade || upgrade(&instance) != sl::Result::eOk) return false;

	swapChain = static_cast<IDXGISwapChain1*>(instance);

	return true;
}

bool StreamlineRuntime::ConfigureDlss(unsigned width, unsigned height, unsigned& renderWidth, unsigned& renderHeight) const {
	if (width < 32 || height < 32) {
		Logln("[Streamline] Output {}x{} is below the DLSS minimum", width, height);
		return false;
	}

	const auto getFeatureFunction = reinterpret_cast<PFun_slGetFeatureFunction*>(
		GetProcAddress(mStreamlineModule, "slGetFeatureFunction"));

	void* getSettingsAddress{};
	void* setOptionsAddress{};
	if (!getFeatureFunction 
		|| getFeatureFunction(sl::kFeatureDLSS, "slDLSSGetOptimalSettings", getSettingsAddress) != sl::Result::eOk 
		|| getFeatureFunction(sl::kFeatureDLSS, "slDLSSSetOptions", setOptionsAddress) != sl::Result::eOk 
		|| !getSettingsAddress 
		|| !setOptionsAddress)
		ReturnFalse("[Streamline] Failed to resolve DLSS settings functions");

	const auto getSettings = reinterpret_cast<PFun_slDLSSGetOptimalSettings*>(getSettingsAddress);
	const auto setOptions = reinterpret_cast<PFun_slDLSSSetOptions*>(setOptionsAddress);

	sl::DLSSOptions options{};
	options.mode = sl::DLSSMode::eBalanced;
	options.outputWidth = width;
	options.outputHeight = height;
	// 톤매핑과 같은 Eye Adaption 노출을 사용하도록 DLSS 자동 노출을 끕니다.
	options.useAutoExposure = sl::Boolean::eFalse;

	sl::DLSSOptimalSettings settings{};
	if (const auto result = getSettings(options, settings); result != sl::Result::eOk)
		ReturnFalse("[Streamline] DLSS optimal settings query failed (result: {})", static_cast<int>(result));

	if (!settings.optimalRenderWidth || !settings.optimalRenderHeight ||
		settings.optimalRenderWidth > width || settings.optimalRenderHeight > height)
		ReturnFalse(
			"[Streamline] Invalid DLSS input size {}x{} for output {}x{}",
			settings.optimalRenderWidth, settings.optimalRenderHeight, width, height);

	if (const auto result = setOptions(DlssViewport, options); result != sl::Result::eOk)
		ReturnFalse("[Streamline] DLSS options failed (result: {})", static_cast<int>(result));

	renderWidth = settings.optimalRenderWidth;
	renderHeight = settings.optimalRenderHeight;

	Logln("[Streamline] DLSS Balanced configured for {}x{} -> {}x{}", renderWidth, renderHeight, width, height);

	return true;
}

void StreamlineRuntime::ReleaseDlssResources() const {
	const auto freeResources = reinterpret_cast<PFun_slFreeResources*>(GetProcAddress(mStreamlineModule, "slFreeResources"));
	if (freeResources) freeResources(sl::kFeatureDLSS, DlssViewport);
}

bool StreamlineRuntime::EvaluateDlss(ID3D12GraphicsCommandList* commands, const sl::Constants& constants,
	unsigned frameIndex, const Resource (&resources)[5]) const {
	const auto getToken = reinterpret_cast<PFun_slGetNewFrameToken*>(GetProcAddress(mStreamlineModule, "slGetNewFrameToken"));
	const auto setConstants = reinterpret_cast<PFun_slSetConstants*>(GetProcAddress(mStreamlineModule, "slSetConstants"));
	const auto evaluate = reinterpret_cast<PFun_slEvaluateFeature*>(GetProcAddress(mStreamlineModule, "slEvaluateFeature"));
	if (!getToken || !setConstants || !evaluate) return false;

	sl::FrameToken* token{};
	if (getToken(token, &frameIndex) != sl::Result::eOk || !token) return false;
	if (setConstants(constants, *token, DlssViewport) != sl::Result::eOk) return false;

	const sl::BufferType types[] = {
		sl::kBufferTypeScalingInputColor,
		sl::kBufferTypeScalingOutputColor,
		sl::kBufferTypeDepth, sl::kBufferTypeMotionVectors, sl::kBufferTypeExposure };

	sl::Resource textures[ARRAYSIZE(types)];
	sl::ResourceTag tags[ARRAYSIZE(types)] = {
		{ &textures[0], types[0], sl::ResourceLifecycle::eValidUntilEvaluate },
		{ &textures[1], types[1], sl::ResourceLifecycle::eValidUntilEvaluate },
		{ &textures[2], types[2], sl::ResourceLifecycle::eValidUntilEvaluate },
		{ &textures[3], types[3], sl::ResourceLifecycle::eValidUntilEvaluate },
		{ &textures[4], types[4], sl::ResourceLifecycle::eValidUntilEvaluate }
	};

	for (unsigned i = 0; i < ARRAYSIZE(textures); ++i)
		textures[i] = { sl::ResourceType::eTex2d, resources[i].Texture, resources[i].State };

	// 이 리소스들은 DLSS 평가에만 사용합니다. 호출에 직접 전달해 불필요한 복제를 피합니다.
	const sl::BaseStructure* inputs[] = { &DlssViewport, &tags[0], &tags[1], &tags[2], &tags[3], &tags[4] };
	const auto result = evaluate(sl::kFeatureDLSS, *token, inputs, ARRAYSIZE(inputs), commands);
	if (result != sl::Result::eOk)
		ReturnFalse("[Streamline] DLSS evaluation failed (result: {})", static_cast<int>(result));

	return true;
}
