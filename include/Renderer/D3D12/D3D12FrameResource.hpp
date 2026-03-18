#pragma once

struct LogFile;

class D3D12Device;

class D3D12FrameResource {
public:
	static const UINT NumFrameResources = 3;

public:
	D3D12FrameResource();
	virtual ~D3D12FrameResource();

public:
	bool Initialize(D3D12Device* const pDevice);

public:
	bool ResetCommandListAllocator();

public:
	__forceinline ID3D12CommandAllocator* CommandAllocator() const;

private:
	bool CreateCommandListAllocator();
	bool BuildConstantBuffres(
		UINT numPasses,
		UINT numObjects,
		UINT numMaterials);

public:
	UINT64 mFence;

private:
	D3D12Device* mpDevice;

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCmdAllocator;
};

#include "D3D12FrameResource.inl"