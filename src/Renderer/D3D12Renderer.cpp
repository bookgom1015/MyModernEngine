#include "Renderer/pch_d3d12.h"
#include "Renderer/D3D12Renderer.hpp"

extern "C" RendererAPI Renderer* CreateRenderer() {
	return NEW D3D12Renderer();
}

extern "C" RendererAPI void DestroyRenderer(Renderer* const pRenderer) {
    delete pRenderer;
}

D3D12Renderer::D3D12Renderer() {}

D3D12Renderer::~D3D12Renderer() {}

bool D3D12Renderer::Initialize(
	LogFile* const pLogFile
	, unsigned width, unsigned height) {
	CheckReturn(mpLogFile, D3D12LowRenderer::Initialize(pLogFile, width, height));

	return true;
}

bool D3D12Renderer::Update(float deltaTime) {
	return true;
}

bool D3D12Renderer::Draw() {
	return true;
}

bool D3D12Renderer::OnResize(unsigned width, unsigned height) {
	CheckReturn(mpLogFile, D3D12LowRenderer::OnResize(width, height));

	return true;
}