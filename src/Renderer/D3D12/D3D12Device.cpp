#include "Renderer/D3D12/pch.h"
#include "Renderer/D3D12/D3D12Device.hpp"

#include "Renderer/D3D12/D3D12CommandObject.hpp"
#include "Renderer/D3D12/D3D12DescriptorHeap.hpp"
#include "Renderer/D3D12/D3D12SwapChain.hpp"

#include "Renderer/D3D12/D3D12GpuResource.hpp"

#include "Renderer/D3D12/D3D12Util.hpp"

using namespace Microsoft::WRL;

D3D12Device::D3D12Device()
	: mbRaytracingSupported{}
	, mbMeshShaderSupported{}
	, mDebugController{}
	, mDxgiFactory{}
	, mdxgiFactoryFlags{}
	, mbAllowTearing{}
	, mAdapters{}
	, md3dDevice{} {}

D3D12Device::~D3D12Device() {
#ifdef _DEBUG
	if (mDebugController && md3dDevice) {
		ComPtr<ID3D12DebugDevice> debugDevice;
		if (SUCCEEDED(md3dDevice.As(&debugDevice))) 
			debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
	}
#endif
}

bool D3D12Device::Initialize() {
	CheckReturn(CreateFactory());
	CheckReturn(SortAdapters());
	CheckReturn(SelectAdapter(0, mbRaytracingSupported));
	CheckReturn(CheckMeshShaderSupported());

	return true;
}

void D3D12Device::CreateShaderResourceView(
	ID3D12Resource* const pResource
	, const D3D12_SHADER_RESOURCE_VIEW_DESC* const pDesc
	, D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor) {
	md3dDevice->CreateShaderResourceView(pResource, pDesc, destDescriptor);
}

void D3D12Device::CreateUnorderedAccessView(
	ID3D12Resource* const pResource
	, ID3D12Resource* const pCounterResource
	, const D3D12_UNORDERED_ACCESS_VIEW_DESC* const pDesc
	, D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor) {
	md3dDevice->CreateUnorderedAccessView(pResource, pCounterResource, pDesc, destDescriptor);
}

void D3D12Device::CreateRenderTargetView(
	ID3D12Resource* const pResource
	, const D3D12_RENDER_TARGET_VIEW_DESC* const pDesc
	, D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor) {
	md3dDevice->CreateRenderTargetView(pResource, pDesc, destDescriptor);
}

void D3D12Device::CreateDepthStencilView(
	ID3D12Resource* const pResource
	, const D3D12_DEPTH_STENCIL_VIEW_DESC* const pDesc
	, D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor) {
	md3dDevice->CreateDepthStencilView(pResource, pDesc, destDescriptor);
}

bool D3D12Device::CreateCommandAllocator(ComPtr<ID3D12CommandAllocator>& cmdAllocator) {
	CheckHResult(md3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)));

	return true;
}

bool D3D12Device::CreateCommandList(
	ID3D12CommandAllocator* const pCmdAllocator
	, ComPtr<ID3D12GraphicsCommandList6>& commandList) {
	CheckHResult(md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		pCmdAllocator,	// 연결된 커맨드 할당자입니다.
		nullptr,		// 초기 파이프라인 상태 객체입니다.
		IID_PPV_ARGS(&commandList)));

	// 커맨드 리스트는 닫힌 상태로 시작합니다.
	// 처음 사용할 때 Reset을 호출하며,
	// Reset을 호출하려면 커맨드 리스트가 닫혀 있어야 합니다.
	commandList->Close();

	return true;
}

bool D3D12Device::CreateFence(Microsoft::WRL::ComPtr<ID3D12Fence>& fence) {
	CheckHResult(md3dDevice->CreateFence(
		0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

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

UINT D3D12Device::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType) const {
	return md3dDevice->GetDescriptorHandleIncrementSize(descriptorHeapType);
}

bool D3D12Device::CreateFactory() {
#ifdef _DEBUG
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&mDebugController)))) {
		mDebugController->EnableDebugLayer();
		mdxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	CheckHResult(CreateDXGIFactory2(mdxgiFactoryFlags, IID_PPV_ARGS(&mDxgiFactory)));

	ComPtr<IDXGIFactory5> factory5{};
	CheckHResult(mDxgiFactory.As(&factory5));

	const auto supported = factory5->CheckFeatureSupport(
		DXGI_FEATURE_PRESENT_ALLOW_TEARING
		, &mbAllowTearing
		, sizeof(mbAllowTearing));
	if (SUCCEEDED(supported)) mbAllowTearing = TRUE;

	return true;
}

bool D3D12Device::SortAdapters() {
	mAdapters.clear();

#if _DEBUG
	WLogln(L"Adapters:");
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
		WLogln(L"    {} ( Shared system memory: {}MB , Dedicated video memory: {}MB )", desc.Description, sram, vram);
#endif
	}

	// 점수가 높은 순서로 정렬합니다.
	std::sort(mAdapters.begin(), mAdapters.end(), [](const auto& a, const auto& b) {
		return a.first > b.first;
	});

	return true;
}

bool D3D12Device::SelectAdapter(UINT adapterIndex, bool& bRaytracingSupported) {
	const auto& adapter = mAdapters[adapterIndex].second;

	// 하드웨어 디바이스 생성을 시도합니다.
	const auto hr = D3D12CreateDevice(
		adapter.Get(),
		D3D_FEATURE_LEVEL_12_1,
		IID_PPV_ARGS(md3dDevice.GetAddressOf()));
	if (FAILED(hr)) ReturnFalse("Failed to create device");

	DXGI_ADAPTER_DESC desc{};
	adapter->GetDesc(&desc);

#ifdef _DEBUG
	WLogln(L"{} is selected", desc.Description);
#endif

	D3D12_FEATURE_DATA_D3D12_OPTIONS5 ops{};
	const auto featureSupport = md3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_D3D12_OPTIONS5, &ops, sizeof(ops));
	if (FAILED(featureSupport)) {
		md3dDevice = nullptr;
		ReturnFalse("CheckFeatureSupport failed: 0x{:08X}", featureSupport);
	}

	if (ops.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0) {
		bRaytracingSupported = true;
		WLogln(L"Selected device supports ray-tracing");
	}
	else {
		bRaytracingSupported = false;
		WLogln(L"Selected device does not support ray-tracing");
	}

	return true;
}

bool D3D12Device::CheckMeshShaderSupported() {
	D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7{};
	const auto featureSupport = md3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7));

	if (FAILED(featureSupport)) 
		ReturnFalse("CheckFeatureSupport failed: 0x{:08X}", featureSupport);

	if (options7.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED) {
		WLogln(L"Selected device supports mesh shader");
		mbMeshShaderSupported = true;
	}
	else {
		WLogln(L"Selected device does not support mesh shader");
		mbMeshShaderSupported = false;
	}

	return true;
}
