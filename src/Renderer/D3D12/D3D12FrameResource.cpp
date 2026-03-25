#include "pch.h"
#include "Renderer/D3D12/D3D12FrameResource.hpp"

#include "Renderer/D3D12/D3D12Device.hpp"

D3D12FrameResource::D3D12FrameResource() 
	: mpDevice{}
	, mCmdAllocator{}
	, mFence{} {}

D3D12FrameResource::~D3D12FrameResource() {}

bool D3D12FrameResource::Initialize(
	D3D12Device* const pDevice
	, UINT numPasses
	, UINT numObjects
	, UINT numMaterials) {
	mpDevice = pDevice;

	CheckReturn(CreateCommandListAllocator());
	CheckReturn(BuildConstantBuffres(numPasses, numObjects, numMaterials));

	return true;
}

bool D3D12FrameResource::ResetCommandListAllocator() {
	CheckHResult(mCmdAllocator->Reset());

	return true;
}

bool D3D12FrameResource::CreateCommandListAllocator() {
	CheckReturn(mpDevice->CreateCommandAllocator(mCmdAllocator));

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

	return true;
}