#include "pch.h"
#include "Renderer/D3D12/D3D12RenderPassManager.hpp"

#include "PathManager.hpp"
#include "Renderer/D3D12/D3D12ShaderManager.hpp"

D3D12RenderPassManager::D3D12RenderPassManager() {}

D3D12RenderPassManager::~D3D12RenderPassManager() {}

bool D3D12RenderPassManager::CompileShaders(D3D12ShaderManager* pShaderManager) {
	for (const auto& pair : mRenderPasses)
		CheckReturn(pair.second->CompileShaders());

	CheckReturn(pShaderManager->CompileShaders(
		std::format(L"{}Shader\\D3D12\\", CONTENT_PATH).c_str()));

	return true;
}

bool D3D12RenderPassManager::BuildRootSignatures() {
	for (const auto& pair : mRenderPasses)
		CheckReturn(pair.second->BuildRootSignatures());

	return true;
}

bool D3D12RenderPassManager::BuildPipelineStates() {
	for (const auto& pair : mRenderPasses)
		CheckReturn(pair.second->BuildPipelineStates());

	return true;
}

bool D3D12RenderPassManager::AllocateDescriptors() {
	for (const auto& pair : mRenderPasses)
		CheckReturn(pair.second->AllocateDescriptors());

	return true;
}

bool D3D12RenderPassManager::BuildShaderTables(unsigned numRitems) {
	for (const auto& pair : mRenderPasses)
		CheckReturn(pair.second->BuildShaderTables(numRitems));

	return true;
}

bool D3D12RenderPassManager::Update() {
	for (const auto& pair : mRenderPasses)
		CheckReturn(pair.second->Update());

	return true;
}

bool D3D12RenderPassManager::OnResize(unsigned width, unsigned height) {
	for (const auto& pair : mRenderPasses)
		CheckReturn(pair.second->OnResize(width, height));

	return true;
}
