#include "pch.h"
#include "Renderer/D3D12/D3D12FrameResource.hpp"

#include "Renderer/D3D12/D3D12Device.hpp"

D3D12FrameResource::D3D12FrameResource() 
	: mpLogFile{}
	, mpDevice{}
	, mCmdAllocator{}
	, mFence{} {}

D3D12FrameResource::~D3D12FrameResource() {}

bool D3D12FrameResource::Initialize(
	LogFile* const pLogFile
	, D3D12Device* const pDevice) {
	mpLogFile = pLogFile;
	mpDevice = pDevice;

	CheckReturn(mpLogFile, CreateCommandListAllocator());
	//CheckReturn(mpLogFile, BuildConstantBuffres(numPasses, numObjects, numMaterials));

	return true;
}

bool D3D12FrameResource::ResetCommandListAllocator() {
	CheckHResult(mpLogFile, mCmdAllocator->Reset());

	return true;
}

bool D3D12FrameResource::CreateCommandListAllocator() {
	CheckReturn(mpLogFile, mpDevice->CreateCommandAllocator(mCmdAllocator));

	return true;
}

bool D3D12FrameResource::BuildConstantBuffres(
	UINT numPasses
	, UINT numObjects
	, UINT numMaterials) {


	return true;
}