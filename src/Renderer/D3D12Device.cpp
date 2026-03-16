#include "Renderer/pch_d3d12.h"
#include "Renderer/D3D12Device.hpp"

#include "Renderer/D3D12CommandObject.hpp"
#include "Renderer/D3D12DescriptorHeap.hpp"
#include "Renderer/D3D12SwapChain.hpp"

using namespace Microsoft::WRL;

D3D12Device::D3D12Device()
	: mpLogFile{}
	, mbRaytracingSupported{}
	, mbMeshShaderSupported{}
	, mDebugController{}
	, mDxgiFactory{}
	, mdxgiFactoryFlags{}
	, mbAllowTearing{}
	, mAdapters{}
	, md3dDevice{} {}

D3D12Device::~D3D12Device() {}

bool D3D12Device::Initialize(LogFile* const pLogFile) {
	mpLogFile = pLogFile;

	CheckReturn(mpLogFile, CreateFactory());
	CheckReturn(mpLogFile, SortAdapters());
	CheckReturn(mpLogFile, SelectAdapter(0, mbRaytracingSupported));
	CheckReturn(mpLogFile, CheckMeshShaderSupported());

	return true;
}

bool D3D12Device::GetAdapters(std::vector<std::wstring>& adapters) {
	for (const auto& pair : mAdapters) {
		auto adapter = pair.second;

		DXGI_ADAPTER_DESC1 desc{};
		if (FAILED(adapter->GetDesc1(&desc))) continue;

		adapters.push_back(desc.Description);
	}

	return true;
}

UINT D3D12Device::DescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType) const {
	return md3dDevice->GetDescriptorHandleIncrementSize(descriptorHeapType);
}

bool D3D12Device::CreateFactory() {
#ifdef _DEBUG
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&mDebugController)))) {
		mDebugController->EnableDebugLayer();
		mdxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	CheckHResult(mpLogFile, CreateDXGIFactory2(
		mdxgiFactoryFlags, IID_PPV_ARGS(&mDxgiFactory)));

	ComPtr<IDXGIFactory5> factory5{};
	CheckHResult(mpLogFile, mDxgiFactory.As(&factory5));

	const auto supported = factory5->CheckFeatureSupport(
		DXGI_FEATURE_PRESENT_ALLOW_TEARING
		, &mbAllowTearing
		, sizeof(mbAllowTearing));
	if (SUCCEEDED(supported)) mbAllowTearing = true;

	return true;
}

bool D3D12Device::SortAdapters() {
	mAdapters.clear();

#if _DEBUG
	WLogln(mpLogFile, L"Adapters:");
#endif

	for (UINT i = 0;; ++i) {
		ComPtr<IDXGIAdapter1> adapter{};
		if (mDxgiFactory->EnumAdapters1(i, &adapter) == DXGI_ERROR_NOT_FOUND)
			break;

		DXGI_ADAPTER_DESC1 desc{};
		if (FAILED(adapter->GetDesc1(&desc))) continue;

		const UINT sram = static_cast<UINT>(desc.SharedSystemMemory / (1024 * 1024));
		const UINT vram = static_cast<UINT>(desc.DedicatedVideoMemory / (1024 * 1024));
		const UINT score = static_cast<UINT>(desc.DedicatedSystemMemory + desc.DedicatedVideoMemory);
		mAdapters.emplace_back(score, adapter);

#if _DEBUG
		auto msg = std::format(L"    {} ( Shared system memory: {}MB , Dedicated video memory: {}MB )",
			desc.Description, sram, vram);
		WLogln(mpLogFile, msg);
#endif
	}

	// Sort descending by score
	std::sort(mAdapters.begin(), mAdapters.end(), [](const auto& a, const auto& b) {
		return a.first > b.first;
	});

	return true;
}

bool D3D12Device::SelectAdapter(UINT adapterIndex, bool& bRaytracingSupported) {
	const auto& adapter = mAdapters[adapterIndex].second;

	// Try to create hardware device.
	const auto hr = D3D12CreateDevice(
		adapter.Get(),
		D3D_FEATURE_LEVEL_12_1,
		IID_PPV_ARGS(md3dDevice.GetAddressOf()));
	if (FAILED(hr)) ReturnFalse(mpLogFile, "Failed to create device");

	DXGI_ADAPTER_DESC desc{};
	adapter->GetDesc(&desc);

#ifdef _DEBUG
	WLogln(mpLogFile, desc.Description, L" is selected");
#endif

	D3D12_FEATURE_DATA_D3D12_OPTIONS5 ops{};
	const auto featureSupport = md3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_D3D12_OPTIONS5, &ops, sizeof(ops));
	if (FAILED(featureSupport)) {
		md3dDevice = nullptr;
		auto msg = std::format("CheckFeatureSupport failed: 0x{:08X}", featureSupport);
		ReturnFalse(mpLogFile, msg);
	}

	if (ops.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0) {
		bRaytracingSupported = true;
		WLogln(mpLogFile, L"Selected device supports ray-tracing");
	}
	else {
		bRaytracingSupported = false;
		WLogln(mpLogFile, L"Selected device does not support ray-tracing");
	}

	return true;
}

bool D3D12Device::CheckMeshShaderSupported() {
	D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7{};
	const auto featureSupport = md3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7));

	if (FAILED(featureSupport)) {
		std::stringstream ssstream;
		ssstream << "CheckFeatureSupport failed: " << std::hex << featureSupport;
		ReturnFalse(mpLogFile, ssstream.str().c_str());
	}

	if (options7.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED) {
		WLogln(mpLogFile, L"Selected device supports mesh shader");
		mbMeshShaderSupported = true;
	}
	else {
		WLogln(mpLogFile, L"Selected device does not support mesh shader");
		mbMeshShaderSupported = false;
	}

	return true;
}