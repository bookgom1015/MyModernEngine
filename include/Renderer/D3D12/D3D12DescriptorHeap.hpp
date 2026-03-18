#pragma once

struct LogFile;

class D3D12Device;

class D3D12DescriptorHeap {
public:
	struct DescriptorAllocation {
		D3D12_DESCRIPTOR_HEAP_TYPE Type{};
		UINT Index{ UINT_MAX };
		UINT Count{};

		bool IsValid() const noexcept { return Index != UINT_MAX && Count > 0; }
	};

private:
	struct FreeRange {
		UINT Start{};
		UINT Count{};
	};

public:
	D3D12DescriptorHeap();
	virtual ~D3D12DescriptorHeap();

public:
	bool Initialize(D3D12Device* const pDevice);

public:
	bool Allocate(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count, DescriptorAllocation& allocation);
	bool AllocateRtv(UINT count, DescriptorAllocation& allocation);
	bool AllocateDsv(UINT count, DescriptorAllocation& allocation);
	bool AllocateCbvSrvUav(UINT count, DescriptorAllocation& allocation);

	bool Free(const DescriptorAllocation& allocation);
	bool FreeRtv(UINT index, UINT count);
	bool FreeDsv(UINT index, UINT count);
	bool FreeCbvSrvUav(UINT index, UINT count);

	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(
		D3D12_DESCRIPTOR_HEAP_TYPE type, UINT index) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(const DescriptorAllocation& allocation) const;

	D3D12_CPU_DESCRIPTOR_HANDLE GetRtvCpuHandle(UINT index) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDsvCpuHandle(UINT index) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCbvSrvUavCpuHandle(UINT index) const;

	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(
		D3D12_DESCRIPTOR_HEAP_TYPE type, UINT index) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(const DescriptorAllocation& allocation) const;

	D3D12_GPU_DESCRIPTOR_HANDLE GetCbvSrvUavGpuHandle(UINT index) const;

	bool SetDescriptorHeap(ID3D12GraphicsCommandList4* const pCmdList);

public:
	__forceinline ID3D12DescriptorHeap* GetCbvSrvUavHeap() const noexcept;

private:
	bool BuildDescriptorSizes();

	bool CreateHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count);

	bool AllocateFromFreeList(
		std::vector<FreeRange>& freeRanges,
		UINT requestCount,
		UINT& outIndex);

	bool FreeToFreeList(
		std::vector<FreeRange>& freeRanges,
		UINT index,
		UINT count,
		UINT capacity);

	bool ValidateRange(UINT index, UINT count, UINT capacity) const;

private:
	D3D12Device* mpDevice;

	// Descriptor heaps
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvSrvUavHeap;

	// Descriptor handle sizes
	UINT mRtvDescriptorSize;
	UINT mDsvDescriptorSize;
	UINT mCbvSrvUavDescriptorSize;

	// Capacity
	UINT mRtvSize;
	UINT mDsvSize;
	UINT mCbvSrvUavSize;

	// Used count (optional stats)
	UINT mRtvUsedCount;
	UINT mDsvUsedCount;
	UINT mCbvSrvUavUsedCount;

	// Free lists
	std::vector<FreeRange> mRtvFreeRanges;
	std::vector<FreeRange> mDsvFreeRanges;
	std::vector<FreeRange> mCbvSrvUavFreeRanges;
};

ID3D12DescriptorHeap* D3D12DescriptorHeap::GetCbvSrvUavHeap() const noexcept {
	return mCbvSrvUavHeap.Get();
}