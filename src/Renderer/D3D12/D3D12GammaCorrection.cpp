#include "pch.h"
#include "Renderer/D3D12/D3D12GammaCorrection.hpp"

D3D12GammaCorrection::D3D12GammaCorrection() {}

D3D12GammaCorrection::~D3D12GammaCorrection() {}

bool D3D12GammaCorrection::Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	mInitData = *reinterpret_cast<InitData*>(pData);

	return true;
}

bool D3D12GammaCorrection::CompileShaders() {
	return true;
}

bool D3D12GammaCorrection::BuildRootSignatures() {
	return true;
}

bool D3D12GammaCorrection::BuildPipelineStates() {
	return true;
}
