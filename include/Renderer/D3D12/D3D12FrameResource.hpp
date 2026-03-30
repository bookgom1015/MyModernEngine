#pragma once

#include "Renderer/D3D12/D3D12UploadBuffer.hpp"
#include "Renderer/D3D12/D3D12ConstantBuffers.h"

struct LogFile;

class D3D12Device;

class D3D12FrameResource {
public:
	static const UINT NumFrameResources = 3;

public:
	D3D12FrameResource();
	virtual ~D3D12FrameResource();

public:
	bool Initialize(
		D3D12Device* const pDevice,
		UINT numPasses,
		UINT numObjects,
		UINT numMaterials);

public:
	bool ResetFrameCommandListAllocator();
	bool ResetUploadCommandListAllocator();

public:
	__forceinline ID3D12CommandAllocator* FrameCommandAllocator() const;
	__forceinline ID3D12CommandAllocator* UploadCommandAllocator() const;

private:
	bool CreateCommandListAllocators();
	bool BuildConstantBuffres(
		UINT numPasses,
		UINT numObjects,
		UINT numMaterials);

public:
	UINT64 mFrameFence;
	UINT64 mUploadFence;

	UploadBuffer<PassCB> PassCB;
	UploadBuffer<ObjectCB> ObjectCB;
	UploadBuffer<MaterialCB> MaterialCB;
	UploadBuffer<LightCB> LightCB;
	UploadBuffer<GizmoCB> GizmoCB;

private:
	D3D12Device* mpDevice;

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mFrameCmdAllocator;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mUploadCmdAllocator;
};

#include "D3D12FrameResource.inl"