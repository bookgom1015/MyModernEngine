#include "pch.h"
#include "Renderer/D3D12/D3D12FrameResource.hpp"

#include "Renderer/D3D12/D3D12Device.hpp"

D3D12FrameResource::D3D12FrameResource() 
	: mpDevice{}
	, mFrameCmdAllocator{}
	, mFrameFence{}
	, mUploadFence{} {}

D3D12FrameResource::~D3D12FrameResource() {}

bool D3D12FrameResource::Initialize(
	D3D12Device* const pDevice
	, UINT numPasses
	, UINT numObjects
	, UINT numMaterials) {
	mpDevice = pDevice;

	CheckReturn(CreateCommandListAllocators());
	CheckReturn(BuildConstantBuffres(numPasses, numObjects, numMaterials));

	return true;
}

bool D3D12FrameResource::ResetFrameCommandListAllocator() {
	CheckHResult(mFrameCmdAllocator->Reset());

	return true;
}

bool D3D12FrameResource::ResetUploadCommandListAllocator() {
	CheckHResult(mUploadCmdAllocator->Reset());

	return true;
}

bool D3D12FrameResource::ResetImmediateCommandListAllocator() {
	CheckHResult(mImmediateCmdAllocator->Reset());

	return true;
}

bool D3D12FrameResource::CreateCommandListAllocators() {
	CheckReturn(mpDevice->CreateCommandAllocator(mFrameCmdAllocator));
	CheckReturn(mpDevice->CreateCommandAllocator(mUploadCmdAllocator));
	CheckReturn(mpDevice->CreateCommandAllocator(mImmediateCmdAllocator));

	return true;
}

bool D3D12FrameResource::BuildConstantBuffres(
	UINT numPasses
	, UINT numObjects
	, UINT numMaterials) {
	CheckReturn(PassCB.Initialize(mpDevice, numPasses, 1, TRUE));
	CheckReturn(ObjectCB.Initialize(mpDevice, numObjects, 1, TRUE));
	CheckReturn(MaterialCB.Initialize(mpDevice, numMaterials, 1, TRUE));
	CheckReturn(LightCB.Initialize(mpDevice, 1, 1, TRUE));
	CheckReturn(GizmoCB.Initialize(mpDevice, 1, 1, TRUE));

	CheckReturn(ProjectToCubeCB.Initialize(mpDevice, 32, 1, TRUE));
	CheckReturn(ProbeSB.Initialize(mpDevice, 32, 1, FALSE));

	for (UINT i = 0; i < 2; ++i) 
		CheckReturn(BoneSB[i].Initialize(mpDevice, 1024, 1, FALSE));

	CheckReturn(DebugLineVB.Initialize(mpDevice, 1024, 1, FALSE));

	return true;
}