#include "Renderer\D3D12\pch.h"
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

	// 초기 용량을 설정
	// TODO: 추후 동적으로 확장하도록 수정
	CheckReturn(CreateHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 256));
	CheckReturn(CreateHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 256));
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

		// 요청한 크기에 맞는 충분한 공간이 있는지 확인합니다.
		if (range.Count < requestCount)
			continue;

		// 요청한 크기에 맞는 공간을 찾았으므로, 해당 범위를 할당하고 업데이트합니다.
		outIndex = range.Start;
		range.Start += requestCount;
		range.Count -= requestCount;

		// 만약 더 이상 사용 가능한 공간이 없다면, freeRanges에서 제거합니다.
		if (range.Count == 0)
			freeRanges.erase(freeRanges.begin() + static_cast<ptrdiff_t>(i));

		return true;
	}

	ReturnFalse("No suitable free range found for descriptor allocation");
}

bool D3D12DescriptorHeap::FreeToFreeList(
	std::vector<FreeRange>& freeRanges,
	UINT index,
	UINT count,
	UINT capacity) {
	if (!ValidateRange(index, count, capacity))
		ReturnFalse("Descriptor free range is invalid");

	const UINT releaseBegin = index;
	const UINT releaseEnd = index + count;

	for (const auto& range : freeRanges) {
		const UINT freeBegin = range.Start;
		const UINT freeEnd = range.Start + range.Count;

		// 해제하려는 범위가 이미 존재하는 free range와 겹치는지 확인합니다.
		const bool overlaps = releaseEnd > freeBegin && releaseBegin < freeEnd;
		if (overlaps)
			ReturnFalse("Descriptor free range overlaps an existing free range (double free?)");
	}

	// 해제하려는 범위를 freeRanges에 추가하고, 이후 병합을 위해 정렬합니다.
	freeRanges.push_back({ index, count });
	std::sort(freeRanges.begin(), freeRanges.end(),
		[](const FreeRange& a, const FreeRange& b) {
			return a.Start < b.Start;
		});

	std::vector<FreeRange> merged;
	merged.reserve(freeRanges.size());
	
	// 인접한 free range를 병합합니다.
	for (const auto& range : freeRanges) {
		if (merged.empty()) {
			merged.push_back(range);
			continue;
		}

		auto& back = merged.back();
		// 만약 현재 범위가 이전 범위와 인접해 있다면, 두 범위를 병합합니다.
		if (back.Start + back.Count == range.Start) {
			back.Count += range.Count;
		}
		// 그렇지 않다면, 현재 범위를 새로운 범위로 추가합니다.
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