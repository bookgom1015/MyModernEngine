#include "Renderer/pch_d3d12.h"
#include "Renderer/D3D12LowRenderer.hpp"

#include "Renderer/D3D12Factory.hpp"
#include "Renderer/D3D12Device.hpp"

D3D12LowRenderer::D3D12LowRenderer() {}

D3D12LowRenderer::~D3D12LowRenderer() {}

bool D3D12LowRenderer::Initialize(
	LogFile* const pLogFile
	, unsigned width, unsigned height) {
	mpLogFile = pLogFile;

	CheckReturn(mpLogFile, CreateFactory());
	CheckReturn(mpLogFile, CreateDevice());

	return true;
}

bool D3D12LowRenderer::OnResize(UINT width, UINT height) {
	return true;
}

bool D3D12LowRenderer::CreateFactory() {
	mFactory = std::make_unique<D3D12Factory>();
	CheckReturn(mpLogFile, mFactory->Initialize(mpLogFile));

	return true;
}

bool D3D12LowRenderer::CreateDevice() {
	mDevice = std::make_unique<D3D12Device>();

	CheckReturn(mpLogFile, mFactory->SortAdapters());
	CheckReturn(mpLogFile, mFactory->SelectAdapter(
		mDevice.get(), 0, mbRaytracingSupported));
	//
	//CheckReturn(mpLogFile, mDevice->Initialize(mpLogFile));
	//CheckReturn(mpLogFile, mDevice->CheckMeshShaderSupported(mbMeshShaderSupported));

	return true;
}