#include "Renderer/pch_d3d12.h"
#include "Renderer/D3D12LowRenderer.hpp"

#include "Renderer/D3D12Device.hpp"
#include "Renderer/D3D12CommandObject.hpp"
#include "Renderer/D3D12DescriptorHeap.hpp"

D3D12LowRenderer::D3D12LowRenderer()
	: mDevice{}
	, mCommandObject{}
	, mDescriptorHeap{} {}

D3D12LowRenderer::~D3D12LowRenderer() {
	
}

bool D3D12LowRenderer::Initialize(
	LogFile* const pLogFile
	, unsigned width
	, unsigned height) {
	mpLogFile = pLogFile;

	CheckReturn(mpLogFile, CreateDevice());
	CheckReturn(mpLogFile, CreateCommandObject());
	CheckReturn(mpLogFile, CreateDescriptorHeap());

	return true;
}

bool D3D12LowRenderer::OnResize(unsigned width, unsigned height) {
	return true;
}

bool D3D12LowRenderer::CreateDevice() {
	mDevice = std::make_unique<D3D12Device>();

	CheckReturn(mpLogFile, mDevice->Initialize(mpLogFile));

	return true;
}

bool D3D12LowRenderer::CreateCommandObject() {
	mCommandObject = std::make_unique<D3D12CommandObject>();

	CheckReturn(mpLogFile, mCommandObject->Initialize(mpLogFile, mDevice.get()));

	return true;
}

bool D3D12LowRenderer::CreateDescriptorHeap() {
	mDescriptorHeap = std::make_unique<D3D12DescriptorHeap>();

	CheckReturn(mpLogFile, mDescriptorHeap->Initialize(mpLogFile, mDevice.get()));

	return true;
}