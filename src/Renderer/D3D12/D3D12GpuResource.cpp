#include "pch.h"
#include "Renderer/D3D12/D3D12GpuResource.hpp"

#include "Renderer/D3D12/D3D12Device.hpp"

GpuResource::GpuResource() 
	: mResource{}
	, mCurrState{ D3D12_RESOURCE_STATE_COMMON } {}

GpuResource::~GpuResource() {}

bool GpuResource::Initialize(
	D3D12Device* const pDevice
	, const D3D12_HEAP_PROPERTIES* const pHeapProp
	, D3D12_HEAP_FLAGS heapFlag
	, const D3D12_RESOURCE_DESC* const pRscDesc
	, D3D12_RESOURCE_STATES initialState
	, const D3D12_CLEAR_VALUE* const pOptClear
	, LPCWSTR pName) {
	CheckHResult(pDevice->md3dDevice->CreateCommittedResource(
		pHeapProp,
		heapFlag,
		pRscDesc,
		initialState,
		pOptClear,
		IID_PPV_ARGS(&mResource)
	));
	if (pName != nullptr) mResource->SetName(pName);

	mCurrState = initialState;

	return true;
}

bool GpuResource::OnResize(IDXGISwapChain* const pSwapChain, UINT index) {
	CheckHResult(pSwapChain->GetBuffer(index, IID_PPV_ARGS(&mResource)));

	mCurrState = D3D12_RESOURCE_STATE_PRESENT;

	return true;
}

void GpuResource::Swap(Microsoft::WRL::ComPtr<ID3D12Resource>& srcResource) {
	srcResource.Swap(mResource);

	mCurrState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
}

void GpuResource::Swap(
	ID3D12GraphicsCommandList* const pCmdList
	, Microsoft::WRL::ComPtr<ID3D12Resource>& srcResource
	, D3D12_RESOURCE_STATES initialState) {
	srcResource.Swap(mResource);

	if (initialState == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) return;

	auto transit = CD3DX12_RESOURCE_BARRIER::Transition(
		mResource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, initialState);
	pCmdList->ResourceBarrier(1, &transit);

	mCurrState = initialState;
}

void GpuResource::Transite(ID3D12GraphicsCommandList* const pCmdList, D3D12_RESOURCE_STATES state) {
	if (mCurrState == state) return;

	auto transit = CD3DX12_RESOURCE_BARRIER::Transition(
		mResource.Get(), mCurrState, state);
	pCmdList->ResourceBarrier(1, &transit);

	mCurrState = state;
}
