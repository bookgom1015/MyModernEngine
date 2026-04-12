#include "pch.h"
#include "Renderer/D3D12/D3D12Svgf.hpp"

D3D12Svgf:: D3D12Svgf() {}

D3D12Svgf::~D3D12Svgf() {}

bool D3D12Svgf::Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	mInitData = *reinterpret_cast<InitData*>(pData);

	CheckReturn(BuildResources());

	return true;
}

bool D3D12Svgf::CompileShaders() {
	return true;
}

bool D3D12Svgf::BuildRootSignatures() {
	return true;
}

bool D3D12Svgf::BuildPipelineStates() {
	return true;
}

bool D3D12Svgf::AllocateDescriptors() {
	return true;
}

bool D3D12Svgf::BuildResources() {
	return true;
}

bool D3D12Svgf::BuildDescriptors() {
	return true;
}
