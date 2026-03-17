#include "pch.h"
#include "Renderer/D3D12/D3D12DescriptorHeap.hpp"

#include "Renderer/D3D12/D3D12Device.hpp"

D3D12DescriptorHeap::D3D12DescriptorHeap()
	: mpLogFile{}
	, mpDevice{} 
	, mRtvHeap{}
	, mDsvHeap{}
	, mCbvSrvUavHeap{}
	, mRtvDescriptorSize{}
	, mDsvDescriptorSize{}
	, mCbvSrvUavDescriptorSize{}
	, mRtvCount{}
	, mRtvSize{}
	, mDsvCount{}
	, mDsvSize{}
	, mCbvSrvUavCount{}
	, mCbvSrvUavSize{} {}

D3D12DescriptorHeap::~D3D12DescriptorHeap() {}

bool D3D12DescriptorHeap::Initialize(LogFile* const pLogFile, D3D12Device* const pDevice) {
	mpLogFile = pLogFile;
	mpDevice = pDevice;

	CheckReturn(mpLogFile, BuildDescriptorSizes());

	CheckReturn(mpLogFile, Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 32));
	CheckReturn(mpLogFile, Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 32));
	CheckReturn(mpLogFile, Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 32));

	return true;
}

bool D3D12DescriptorHeap::Allocate(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count, UINT& index) {
	if (type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV) 
		AllocateRtv(count, index);
	else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV) 
		AllocateDsv(count, index);
	else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) 
		AllocateCbvSrvUav(count, index);
	else 
		ReturnFalse(mpLogFile, "Invalid descriptor heap type for allocation");

	return true;
}

bool D3D12DescriptorHeap::AllocateRtv(UINT count, UINT& index) {
	auto rtvCount = mRtvCount + count;
	if (rtvCount > mRtvSize) {
		rtvCount = std::max(rtvCount, mRtvSize * 2);
		Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, rtvCount);
	}

	index = mRtvCount;
	mRtvCount = rtvCount;

	return true;
}

bool D3D12DescriptorHeap::AllocateDsv(UINT count, UINT& index) {
	auto dsvCount = mDsvCount + count;
	if (dsvCount > mDsvSize) {
		dsvCount = std::max(dsvCount, mDsvSize * 2);
		Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, dsvCount);
	}

	index = mDsvCount;
	mDsvCount = dsvCount;

	return true;
}

bool D3D12DescriptorHeap::AllocateCbvSrvUav(UINT count, UINT& index) {
	auto cbvSrvUavCount = mCbvSrvUavCount + count;
	if (cbvSrvUavCount > mCbvSrvUavSize) {
		cbvSrvUavCount = std::max(cbvSrvUavCount, mCbvSrvUavSize * 2);
		Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, cbvSrvUavCount);
	}

	index = mCbvSrvUavCount;
	mCbvSrvUavCount = cbvSrvUavCount;

	return true;
}


D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetCpuHandle(
		D3D12_DESCRIPTOR_HEAP_TYPE type, UINT index) const {
	if (type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV) 
		return GetRtvCpuHandle(index);
	else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV) 
		return GetDsvCpuHandle(index);
	else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) 
		return GetCbvSrvUavCpuHandle(index);
	else {
		assert(false && "Invalid descriptor heap type for CPU handle retrieval");
		return {};
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetRtvCpuHandle(UINT index) const {
	assert(index < mRtvCount);

	auto handle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(index) * mRtvDescriptorSize;

	return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetDsvCpuHandle(UINT index) const {
	assert(index < mDsvCount);

	auto handle = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(index) * mDsvDescriptorSize;

	return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetCbvSrvUavCpuHandle(UINT index) const {
	assert(index < mCbvSrvUavCount);

	auto handle = mCbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();

	handle.ptr += static_cast<SIZE_T>(index) * mCbvSrvUavDescriptorSize;
	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetGpuHandle(
		D3D12_DESCRIPTOR_HEAP_TYPE type, UINT index) const {
	if (type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV) 
		return GetRtvGpuHandle(index);
	else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV) 
		return GetDsvGpuHandle(index);
	else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) 
		return GetCbvSrvUavGpuHandle(index);
	else {
		assert(false && "Invalid descriptor heap type for GPU handle retrieval");
		return {};
	}
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetRtvGpuHandle(UINT index) const {
	assert(index < mRtvCount);

	auto handle = mRtvHeap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<UINT64>(index) * mRtvDescriptorSize;

	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetDsvGpuHandle(UINT index) const {
	assert(index < mDsvCount);

	auto handle = mDsvHeap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<UINT64>(index) * mDsvDescriptorSize;

	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetCbvSrvUavGpuHandle(UINT index) const {
	assert(index < mCbvSrvUavCount);

	auto handle = mCbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<UINT64>(index) * mCbvSrvUavDescriptorSize;

	return handle;
}

bool D3D12DescriptorHeap::SetDescriptorHeap(ID3D12GraphicsCommandList4* const pCmdList) {
	ID3D12DescriptorHeap* heaps[] = { mCbvSrvUavHeap.Get() };
	pCmdList->SetDescriptorHeaps(_countof(heaps), heaps);

	return true;
}

bool D3D12DescriptorHeap::BuildDescriptorSizes() {
	mRtvDescriptorSize = mpDevice->md3dDevice->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = mpDevice->md3dDevice->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = mpDevice->md3dDevice->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	return true;
}

bool D3D12DescriptorHeap::Allocate(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count) {
	if (type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV) {
		auto desc = D3D12_DESCRIPTOR_HEAP_DESC{ type, count, D3D12_DESCRIPTOR_HEAP_FLAG_NONE };
		CheckHResult(mpLogFile, mpDevice->md3dDevice->CreateDescriptorHeap(
			&desc, IID_PPV_ARGS(&mRtvHeap)));
		mRtvSize = count;
	}
	else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV) {
		auto desc = D3D12_DESCRIPTOR_HEAP_DESC{ type, count, D3D12_DESCRIPTOR_HEAP_FLAG_NONE };
		CheckHResult(mpLogFile, mpDevice->md3dDevice->CreateDescriptorHeap(
			&desc, IID_PPV_ARGS(&mDsvHeap)));
		mDsvSize = count;
	}
	else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
		auto desc = D3D12_DESCRIPTOR_HEAP_DESC{ type, count, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE };
		CheckHResult(mpLogFile, mpDevice->md3dDevice->CreateDescriptorHeap(
			&desc, IID_PPV_ARGS(&mCbvSrvUavHeap)));
		mCbvSrvUavSize = count;
	}
	else {
		ReturnFalse(mpLogFile, "Invalid descriptor heap type for allocation");
	}

	return true;
}