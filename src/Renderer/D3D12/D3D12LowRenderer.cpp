#include "pch.h"
#include "Renderer/D3D12/D3D12LowRenderer.hpp"

#include "Renderer/D3D12/D3D12Device.hpp"
#include "Renderer/D3D12/D3D12CommandObject.hpp"
#include "Renderer/D3D12/D3D12DescriptorHeap.hpp"
#include "Renderer/D3D12/D3D12SwapChain.hpp"
#include "Renderer/D3D12/D3D12DepthStencilBuffer.hpp"

D3D12LowRenderer::D3D12LowRenderer()
	: mhMainWnd{}
	, mDevice{}
	, mCommandObject{}
	, mDescriptorHeap{}
	, mDepthStencilBuffer{} {}

D3D12LowRenderer::~D3D12LowRenderer() {}

bool D3D12LowRenderer::Initialize(
	HWND hMainWnd
	, unsigned width
	, unsigned height) {
	mhMainWnd = hMainWnd;

	CheckReturn(CreateDevice());
	CheckReturn(CreateCommandObject());
	CheckReturn(CreateDescriptorHeap());
	CheckReturn(CreateSwapChain(width, height));
	CheckReturn(CreateDepthStencilBuffer(width, height));

	CheckReturn(AllocateDescriptors());

	CheckReturn(mCommandObject->FlushCommandQueue());

	return true;
}

bool D3D12LowRenderer::OnResize(unsigned width, unsigned height) {
	CheckReturn(mSwapChain->OnResize(width, height));
	CheckReturn(mDepthStencilBuffer->OnResize(width, height));

#ifdef _DEBUG
	ConsoleLog(std::format("DxRenderer resized (Width: {}, Height: {})", width, height));
#endif

	return true;
}

bool D3D12LowRenderer::CreateDevice() {
	mDevice = std::make_unique<D3D12Device>();

	CheckReturn(mDevice->Initialize());

	return true;
}

bool D3D12LowRenderer::CreateCommandObject() {
	mCommandObject = std::make_unique<D3D12CommandObject>();

	CheckReturn(mCommandObject->Initialize(mDevice.get()));

	return true;
}

bool D3D12LowRenderer::CreateDescriptorHeap() {
	mDescriptorHeap = std::make_unique<D3D12DescriptorHeap>();

	CheckReturn(mDescriptorHeap->Initialize(mDevice.get()));

	return true;
}

bool D3D12LowRenderer::CreateSwapChain(unsigned width, unsigned height) {
	mSwapChain = std::make_unique<D3D12SwapChain>();

	D3D12SwapChain::InitData data{
		.MainWndHandle = mhMainWnd,
		.Device = mDevice.get(),
		.CmdObject = mCommandObject.get(),
		.Width = width,
		.Height = height
	};
	CheckReturn(mSwapChain->Initialize(mDescriptorHeap.get(), &data));

	return true;
}

bool D3D12LowRenderer::CreateDepthStencilBuffer(unsigned width, unsigned height) {
	mDepthStencilBuffer = std::make_unique<D3D12DepthStencilBuffer>();

	D3D12DepthStencilBuffer::InitData data{
		.Device = mDevice.get(),
		.Width = width,
		.Height = height
	};
	CheckReturn(mDepthStencilBuffer->Initialize(mDescriptorHeap.get(), &data));

	return true;
}

bool D3D12LowRenderer::AllocateDescriptors() {
	CheckReturn(mSwapChain->AllocateDescriptors());
	CheckReturn(mDepthStencilBuffer->AllocateDescriptors());	

	return true;
}