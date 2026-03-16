#include "Renderer/pch_d3d12.h"
#include "Renderer/D3D12Device.hpp"

#include "Renderer/D3D12Factory.hpp"

using namespace Microsoft::WRL;

D3D12Device::D3D12Device() {}

D3D12Device::~D3D12Device() {}

bool D3D12Device::Initialize(LogFile* const pLogFile) {
	mpLogFile = pLogFile;

	return true;
}

bool D3D12Device::QueryInterface(ComPtr<ID3D12InfoQueue1>& infoQueue) {
	CheckHResult(mpLogFile, md3dDevice->QueryInterface(IID_PPV_ARGS(&infoQueue)));

	return true;
}

bool D3D12Device::CreateCommandQueue(ComPtr<ID3D12CommandQueue>& cmdQueue) {
	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	CheckHResult(mpLogFile, md3dDevice->CreateCommandQueue(
		&queueDesc, IID_PPV_ARGS(&cmdQueue)));

	return true;
}

bool D3D12Device::CreateCommandAllocator(ComPtr<ID3D12CommandAllocator>& cmdAllocator) {
	CheckHResult(mpLogFile, md3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)));

	return true;
}

bool D3D12Device::CreateCommandList(
	ID3D12CommandAllocator* const pCmdAllocator
	, ComPtr<ID3D12GraphicsCommandList6>& cmdList) {
	CheckHResult(mpLogFile, md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		pCmdAllocator,	// Associated command allocator
		nullptr,		// Initial PipelineStateObject
		IID_PPV_ARGS(&cmdList)));

	// Start off in a closed state.  This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	cmdList->Close();

	return true;
}

bool D3D12Device::CreateFence(ComPtr<ID3D12Fence>& fence) {
	CheckHResult(mpLogFile, md3dDevice->CreateFence(
		0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

	return true;
}

bool D3D12Device::CreateRtvDescriptorHeap(
	ComPtr<ID3D12DescriptorHeap>& descHeap
	, UINT numDescs) {
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
	rtvHeapDesc.NumDescriptors = numDescs;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;

	CheckHResult(mpLogFile, md3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(&descHeap)));

	return true;
}

bool D3D12Device::CreateDsvDescriptorHeap(
	ComPtr<ID3D12DescriptorHeap>& descHeap
	, UINT numDescs) {
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
	dsvHeapDesc.NumDescriptors = numDescs;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;

	CheckHResult(mpLogFile, md3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(&descHeap)));

	return true;
}

bool D3D12Device::CreateCbvUavSrvDescriptorHeap(
	ComPtr<ID3D12DescriptorHeap>& descHeap
	, UINT numDescs) {
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.NumDescriptors = numDescs;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	CheckHResult(mpLogFile, md3dDevice->CreateDescriptorHeap(
		&heapDesc, IID_PPV_ARGS(&descHeap)));

	return true;
}

UINT D3D12Device::DescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType) const {
	return md3dDevice->GetDescriptorHandleIncrementSize(descriptorHeapType);
}

bool D3D12Device::CheckMeshShaderSupported(bool& bMeshShaderSupported) const {
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
		bMeshShaderSupported = true;
	}
	else {
		WLogln(mpLogFile, L"Selected device does not support mesh shader");
		bMeshShaderSupported = false;
	}
}