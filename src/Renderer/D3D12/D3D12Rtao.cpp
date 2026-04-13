#include "pch.h"
#include "Renderer/D3D12/D3D12Rtao.hpp"

D3D12Rtao::D3D12Rtao() {}

D3D12Rtao::~D3D12Rtao() {}

bool D3D12Rtao::Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) {
	return true;
}

bool D3D12Rtao::CompileShaders() {
	return true;
}

bool D3D12Rtao::BuildRootSignatures() {
	return true;
}

bool D3D12Rtao::BuildPipelineStates() {
	return true;
}

bool D3D12Rtao::AllocateDescriptors() {
	return true;
}


bool D3D12Rtao::OnResize(unsigned width, unsigned height) {
	return true;
}

bool D3D12Rtao::BuildShaderTables(UINT numRitems) {
	return true;
}

bool D3D12Rtao::DrawAO(
	D3D12FrameResource* const pFrameResource
	, D3D12_GPU_VIRTUAL_ADDRESS accelStruct
	, GpuResource* const pPositionMap
	, D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap
	, GpuResource* const pNormalDepthMap
	, D3D12_GPU_DESCRIPTOR_HANDLE si_normalDepthMap
	, GpuResource* const pRayDirectionOriginDepthMap
	, D3D12_GPU_DESCRIPTOR_HANDLE si_rayDirectionOriginDepthMap
	, GpuResource* const pRayInexOffsetMap
	, D3D12_GPU_DESCRIPTOR_HANDLE si_rayIndexOffsetMap
	, BOOL bRaySortingEnabled, BOOL bCheckboardRayGeneration) {
	return true;
}

UINT D3D12Rtao::MoveToNextTemporalCacheFrame() {
	return true;
}

UINT D3D12Rtao::MoveToNextTemporalAOFrame() {
	return true;
}

bool D3D12Rtao::BuildResources() {
	return true;
}

bool D3D12Rtao::BuildDescriptors() {
	return true;
}
