#include "pch.h"
#include "Renderer/D3D12/D3D12Brdf.hpp"

D3D12Brdf::D3D12Brdf() {}

D3D12Brdf::~D3D12Brdf() {}

bool D3D12Brdf::Initialize(
	D3D12DescriptorHeap* const pDescHeap
	, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	mInitData = *reinterpret_cast<InitData*>(pData);

	return true;
}

bool D3D12Brdf::CompileShaders() {
	

	return true;
}

bool D3D12Brdf::BuildRootSignatures() {
	return true;
}

bool D3D12Brdf::BuildPipelineStates() {
	return true;
}

bool D3D12Brdf::AllocateDescriptors() {
	return true;
}

bool D3D12Brdf::OnResize(unsigned width, unsigned height) {
	return true;
}
