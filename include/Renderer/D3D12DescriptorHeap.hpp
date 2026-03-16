#pragma once

struct LogFile;

class D3D12Device;

class D3D12DescriptorHeap {
public:
	D3D12DescriptorHeap();
	virtual ~D3D12DescriptorHeap();

public:
	bool Initialize(LogFile* const pLogFile, D3D12Device* const pDevice);

public:
	bool Allocate(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count, UINT& index);

	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT index) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT index) const;

private:
	bool BuildDescriptorSizes();

	bool Allocate(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count);

private:
	LogFile* mpLogFile;

	D3D12Device* mpDevice;

	// Descriptor heaps
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvSrvUavHeap;

	// Descriptor handle sizes
	UINT mRtvDescriptorSize;
	UINT mDsvDescriptorSize;
	UINT mCbvSrvUavDescriptorSize;

	UINT mRtvCount;
	UINT mRtvSize;
	UINT mDsvCount;
	UINT mDsvSize;
	UINT mCbvSrvUavCount;
	UINT mCbvSrvUavSize;
};