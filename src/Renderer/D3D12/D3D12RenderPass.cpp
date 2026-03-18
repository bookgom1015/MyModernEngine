#include "pch.h"
#include "Renderer/D3D12/D3D12RenderPass.hpp"

#include "Renderer/D3D12/D3D12Device.hpp"
#include "Renderer/D3D12/D3D12DescriptorHeap.hpp"
#include "Renderer/D3D12/D3D12CommandObject.hpp"

#include "Renderer/D3D12/D3D12GpuResource.hpp"

D3D12RenderPass::D3D12RenderPass()
	: mpDescHeap{} {}

D3D12RenderPass::~D3D12RenderPass() {}

bool D3D12RenderPass::Initialize(
	D3D12DescriptorHeap* const pDescHeap
	, void* const pData) {
	mpDescHeap = pDescHeap;

	return true;
}

bool D3D12RenderPass::CompileShaders() { return true; }

bool D3D12RenderPass::BuildRootSignatures() { return true; }

bool D3D12RenderPass::BuildPipelineStates() { return true; }

bool D3D12RenderPass::AllocateDescriptors() { return true; }

bool D3D12RenderPass::BuildShaderTables(unsigned numRitems) { return true; }

bool D3D12RenderPass::OnResize(unsigned width, unsigned height) { return true; }

bool D3D12RenderPass::Update() { return true; }
