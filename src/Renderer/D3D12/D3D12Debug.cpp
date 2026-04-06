#include "pch.h"
#include "Renderer/D3D12/D3D12Debug.hpp"

D3D12Debug::D3D12Debug() {}

D3D12Debug::~D3D12Debug() {}

bool D3D12Debug::Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	mInitData = *reinterpret_cast<InitData*>(pData);

	return true;
}

bool D3D12Debug::CompileShaders() {


	return true;
}

bool D3D12Debug::BuildRootSignatures() {


	return true;
}

bool D3D12Debug::BuildPipelineStates() {


	return true;
}
