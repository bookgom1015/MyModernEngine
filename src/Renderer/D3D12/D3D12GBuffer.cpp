#include "pch.h"
#include "Renderer/D3D12/D3D12GBuffer.hpp"

D3D12GBuffer::D3D12GBuffer() {}

D3D12GBuffer::~D3D12GBuffer() {}

bool D3D12GBuffer::Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	mInitData = *reinterpret_cast<InitData*>(pData);

	CheckReturn(BuildResources());
	
	return true;
}

bool D3D12GBuffer::CompileShaders() {
	return true;
}

bool D3D12GBuffer::BuildRootSignatures() {
	return true;
}

bool D3D12GBuffer::BuildPipelineStates() {
	return true;
}

bool D3D12GBuffer::AllocateDescriptors() {
	return true;
}

bool D3D12GBuffer::OnResize(unsigned width, unsigned height) {
	return true;
}

bool D3D12GBuffer::BuildResources() {
	return true;
}

bool D3D12GBuffer::BuildDescriptors() {
	return true;
}