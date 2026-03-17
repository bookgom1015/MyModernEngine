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
	LogFile* const pLogFile
	, HWND hMainWnd
	, unsigned width
	, unsigned height) {
	mpLogFile = pLogFile;
	mhMainWnd = hMainWnd;

	CheckReturn(mpLogFile, CreateDevice());
	CheckReturn(mpLogFile, CreateCommandObject());
	CheckReturn(mpLogFile, CreateDescriptorHeap());
	CheckReturn(mpLogFile, CreateSwapChain(width, height));
	CheckReturn(mpLogFile, CreateDepthStencilBuffer(width, height));

	CheckReturn(mpLogFile, AllocateDescriptors());

	CheckReturn(mpLogFile, mCommandObject->FlushCommandQueue());

	return true;
}

bool D3D12LowRenderer::OnResize(unsigned width, unsigned height) {
	CheckReturn(mpLogFile, mSwapChain->OnResize(width, height));
	CheckReturn(mpLogFile, mDepthStencilBuffer->OnResize(width, height));

#ifdef _DEBUG
	ConsoleLog(std::format("DxRenderer resized (Width: {}, Height: {})", width, height));
#endif

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

bool D3D12LowRenderer::CreateSwapChain(unsigned width, unsigned height) {
	mSwapChain = std::make_unique<D3D12SwapChain>();

	D3D12SwapChain::InitData data{
		.MainWndHandle = mhMainWnd,
		.Device = mDevice.get(),
		.CmdObject = mCommandObject.get(),
		.Width = width,
		.Height = height
	};
	CheckReturn(mpLogFile, mSwapChain->Initialize(mpLogFile, mDescriptorHeap.get(), &data));

	return true;
}

bool D3D12LowRenderer::CreateDepthStencilBuffer(unsigned width, unsigned height) {
	mDepthStencilBuffer = std::make_unique<D3D12DepthStencilBuffer>();

	D3D12DepthStencilBuffer::InitData data{
		.Device = mDevice.get(),
		.Width = width,
		.Height = height
	};
	CheckReturn(mpLogFile, mDepthStencilBuffer->Initialize(mpLogFile, mDescriptorHeap.get(), &data));

	return true;
}

bool D3D12LowRenderer::AllocateDescriptors() {
	CheckReturn(mpLogFile, mSwapChain->AllocateDescriptors());
	CheckReturn(mpLogFile, mDepthStencilBuffer->AllocateDescriptors());	

	return true;
}