#include "pch.h"
#include "Renderer/D3D12/D3D12DescriptorHeap.hpp"

#include "Renderer/D3D12/D3D12Device.hpp"

D3D12DescriptorHeap::D3D12DescriptorHeap()
	: mpDevice{}
	, mRtvHeap{}
	, mDsvHeap{}
	, mCbvSrvUavHeap{}
	, mRtvDescriptorSize{}
	, mDsvDescriptorSize{}
	, mCbvSrvUavDescriptorSize{}
	, mRtvSize{}
	, mDsvSize{}
	, mCbvSrvUavSize{}
	, mRtvUsedCount{}
	, mDsvUsedCount{}
	, mCbvSrvUavUsedCount{}
	, mRtvFreeRanges{}
	, mDsvFreeRanges{}
	, mCbvSrvUavFreeRanges{} {}

D3D12DescriptorHeap::~D3D12DescriptorHeap() {}

bool D3D12DescriptorHeap::Initialize(D3D12Device* const pDevice) {
	mpDevice = pDevice;

	CheckReturn(BuildDescriptorSizes());

	// 초기 용량은 넉넉하게 잡는 걸 추천
	CheckReturn(CreateHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 128));
	CheckReturn(CreateHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 128));
	CheckReturn(CreateHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024));

	mRtvFreeRanges.clear();
	mDsvFreeRanges.clear();
	mCbvSrvUavFreeRanges.clear();

	mRtvFreeRanges.push_back({ 0, mRtvSize });
	mDsvFreeRanges.push_back({ 0, mDsvSize });
	mCbvSrvUavFreeRanges.push_back({ 0, mCbvSrvUavSize });

	mRtvUsedCount = 0;
	mDsvUsedCount = 0;
	mCbvSrvUavUsedCount = 0;

	return true;
}

bool D3D12DescriptorHeap::Allocate(
	D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count, DescriptorAllocation& allocation) {
	if (type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
		return AllocateRtv(count, allocation);
	else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
		return AllocateDsv(count, allocation);
	else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
		return AllocateCbvSrvUav(count, allocation);
	else
		ReturnFalse("Invalid descriptor heap type for allocation");
}

bool D3D12DescriptorHeap::AllocateRtv(UINT count, DescriptorAllocation& allocation) {
	UINT index{};
	if (!AllocateFromFreeList(mRtvFreeRanges, count, index))
		ReturnFalse("RTV descriptor heap is full");

	allocation.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	allocation.Index = index;
	allocation.Count = count;

	mRtvUsedCount += count;
	return true;
}

bool D3D12DescriptorHeap::AllocateDsv(UINT count, DescriptorAllocation& allocation) {
	UINT index{};
	if (!AllocateFromFreeList(mDsvFreeRanges, count, index))
		ReturnFalse("DSV descriptor heap is full");

	allocation.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	allocation.Index = index;
	allocation.Count = count;

	mDsvUsedCount += count;
	return true;
}

bool D3D12DescriptorHeap::AllocateCbvSrvUav(UINT count, DescriptorAllocation& allocation) {
	UINT index{};
	if (!AllocateFromFreeList(mCbvSrvUavFreeRanges, count, index))
		ReturnFalse("CBV/SRV/UAV descriptor heap is full");

	allocation.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	allocation.Index = index;
	allocation.Count = count;

	mCbvSrvUavUsedCount += count;
	return true;
}

bool D3D12DescriptorHeap::Free(const DescriptorAllocation& allocation) {
	if (!allocation.IsValid())
		ReturnFalse("Invalid descriptor allocation for free");

	if (allocation.Type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
		return FreeRtv(allocation.Index, allocation.Count);
	else if (allocation.Type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
		return FreeDsv(allocation.Index, allocation.Count);
	else if (allocation.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
		return FreeCbvSrvUav(allocation.Index, allocation.Count);
	else
		ReturnFalse("Invalid descriptor heap type for free");
}

bool D3D12DescriptorHeap::FreeRtv(UINT index, UINT count) {
	CheckReturn(FreeToFreeList(mRtvFreeRanges, index, count, mRtvSize));

	assert(mRtvUsedCount >= count);
	mRtvUsedCount -= count;

	return true;
}

bool D3D12DescriptorHeap::FreeDsv(UINT index, UINT count) {
	CheckReturn(FreeToFreeList(mDsvFreeRanges, index, count, mDsvSize));

	assert(mDsvUsedCount >= count);
	mDsvUsedCount -= count;

	return true;
}

bool D3D12DescriptorHeap::FreeCbvSrvUav(UINT index, UINT count) {
	CheckReturn(FreeToFreeList(mCbvSrvUavFreeRanges, index, count, mCbvSrvUavSize));

	assert(mCbvSrvUavUsedCount >= count);
	mCbvSrvUavUsedCount -= count;

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

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetCpuHandle(
	const DescriptorAllocation& allocation) const {
	assert(allocation.IsValid());
	return GetCpuHandle(allocation.Type, allocation.Index);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetRtvCpuHandle(UINT index) const {
	assert(index < mRtvSize);

	auto handle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(index) * mRtvDescriptorSize;

	return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetDsvCpuHandle(UINT index) const {
	assert(index < mDsvSize);

	auto handle = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(index) * mDsvDescriptorSize;

	return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetCbvSrvUavCpuHandle(UINT index) const {
	assert(index < mCbvSrvUavSize);

	auto handle = mCbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(index) * mCbvSrvUavDescriptorSize;

	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetGpuHandle(
	D3D12_DESCRIPTOR_HEAP_TYPE type, UINT index) const {
	if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
		return GetCbvSrvUavGpuHandle(index);
	else {
		assert(false && "GPU descriptor handle is only valid for shader-visible heaps");
		return {};
	}
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetGpuHandle(
	const DescriptorAllocation& allocation) const {
	assert(allocation.IsValid());
	return GetGpuHandle(allocation.Type, allocation.Index);
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetCbvSrvUavGpuHandle(UINT index) const {
	assert(index < mCbvSrvUavSize);

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

bool D3D12DescriptorHeap::CreateHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count) {
	if (type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV) {
		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.Type = type;
		desc.NumDescriptors = count;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NodeMask = 0;

		CheckHResult(mpDevice->md3dDevice->CreateDescriptorHeap(
			&desc, IID_PPV_ARGS(&mRtvHeap)));

		mRtvSize = count;
	}
	else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV) {
		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.Type = type;
		desc.NumDescriptors = count;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		desc.NodeMask = 0;

		CheckHResult(mpDevice->md3dDevice->CreateDescriptorHeap(
			&desc, IID_PPV_ARGS(&mDsvHeap)));

		mDsvSize = count;
	}
	else if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.Type = type;
		desc.NumDescriptors = count;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		desc.NodeMask = 0;

		CheckHResult(mpDevice->md3dDevice->CreateDescriptorHeap(
			&desc, IID_PPV_ARGS(&mCbvSrvUavHeap)));

		mCbvSrvUavSize = count;
	}
	else {
		ReturnFalse("Invalid descriptor heap type for creation");
	}

	return true;
}

bool D3D12DescriptorHeap::AllocateFromFreeList(
	std::vector<FreeRange>& freeRanges,
	UINT requestCount,
	UINT& outIndex) {
	if (requestCount == 0)
		ReturnFalse("Descriptor allocation count must be greater than zero");

	for (size_t i = 0; i < freeRanges.size(); ++i) {
		auto& range = freeRanges[i];

		if (range.Count < requestCount)
			continue;

		outIndex = range.Start;

		range.Start += requestCount;
		range.Count -= requestCount;

		if (range.Count == 0)
			freeRanges.erase(freeRanges.begin() + static_cast<ptrdiff_t>(i));

		return true;
	}

	return false;
}

bool D3D12DescriptorHeap::FreeToFreeList(
	std::vector<FreeRange>& freeRanges,
	UINT index,
	UINT count,
	UINT capacity) {
	if (!ValidateRange(index, count, capacity))
		ReturnFalse("Descriptor free range is invalid");

	// 이미 free된 영역과 겹치는지 검사
	for (const auto& range : freeRanges) {
		const UINT freeBegin = range.Start;
		const UINT freeEnd = range.Start + range.Count;

		const UINT releaseBegin = index;
		const UINT releaseEnd = index + count;

		const bool overlaps = !(releaseEnd <= freeBegin || releaseBegin >= freeEnd);
		if (overlaps)
			ReturnFalse("Descriptor free range overlaps an existing free range (double free?)");
	}

	freeRanges.push_back({ index, count });

	std::sort(freeRanges.begin(), freeRanges.end(),
		[](const FreeRange& a, const FreeRange& b) {
			return a.Start < b.Start;
		});

	std::vector<FreeRange> merged;
	merged.reserve(freeRanges.size());

	for (const auto& range : freeRanges) {
		if (merged.empty()) {
			merged.push_back(range);
			continue;
		}

		auto& back = merged.back();
		if (back.Start + back.Count == range.Start) {
			back.Count += range.Count;
		}
		else {
			merged.push_back(range);
		}
	}

	freeRanges = std::move(merged);
	return true;
}

bool D3D12DescriptorHeap::ValidateRange(UINT index, UINT count, UINT capacity) const {
	if (count == 0)
		return false;

	if (index >= capacity)
		return false;

	if (index + count > capacity)
		return false;

	return true;
}