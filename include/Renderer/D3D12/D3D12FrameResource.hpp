#pragma once

#include "Renderer/D3D12/D3D12UploadBuffer.hpp"
#include "Renderer/D3D12/D3D12ConstantBuffers.h"

struct LogFile;

class D3D12Device;

class D3D12FrameResource {
public:
	static const UINT NumFrameResources = 3;
	static const UINT CurrentBonePaletteIndex = 0;
	static const UINT PreviousBonePaletteIndex = 1;

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
	bool ResetImmediateCommandListAllocator();

public:
	__forceinline ID3D12CommandAllocator* FrameCommandAllocator() const;
	__forceinline ID3D12CommandAllocator* UploadCommandAllocator() const;
	__forceinline ID3D12CommandAllocator* ImmediateCommandAllocator() const;

private:
	bool CreateCommandListAllocators();
	bool BuildConstantBuffres(
		UINT numPasses,
		UINT numObjects,
		UINT numMaterials);

public:
	UINT64 mFrameFence;
	UINT64 mUploadFence;
	UINT64 mImmediateFence;

	UploadBuffer<PassCB> PassCB;
	UploadBuffer<ObjectCB> ObjectCB;
	UploadBuffer<MaterialCB> MaterialCB;
	UploadBuffer<LightCB> LightCB;
	UploadBuffer<GizmoCB> GizmoCB;
	UploadBuffer<ProjectToCubeCB> ProjectToCubeCB;
	UploadBuffer<AmbientOcclusionCB> AmbientOcclusionCB;

	UploadBuffer<CrossBilateralFilterCB> CrossBilateralFilterCB;
	UploadBuffer<CalcLocalMeanVarianceCB> CalcLocalMeanVarianceCB;
	UploadBuffer<BlendWithCurrentFrameCB> BlendWithCurrentFrameCB;
	UploadBuffer<AtrousWaveletTransformFilterCB> AtrousWaveletTransformFilterCB;

	UploadBuffer<ReflectionProbeMetaData> ProbeSB;
	UploadBuffer<Mat4> BoneSB[2];

	UploadBuffer<DebugLineVertex> DebugLineVB;

private:
	D3D12Device* mpDevice;

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mFrameCmdAllocator;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mUploadCmdAllocator;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mImmediateCmdAllocator;
};

#include "D3D12FrameResource.inl"