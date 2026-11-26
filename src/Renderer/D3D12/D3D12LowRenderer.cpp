#include "Renderer/D3D12/pch.h"
#include "Renderer/D3D12/D3D12LowRenderer.hpp"

#include "Renderer/D3D12/D3D12Device.hpp"
#include "Renderer/D3D12/D3D12CommandObject.hpp"
#include "Renderer/D3D12/D3D12DescriptorHeap.hpp"
#include "Renderer/D3D12/D3D12SwapChain.hpp"
#include "Renderer/D3D12/D3D12DepthStencilBuffer.hpp"

D3D12LowRenderer::D3D12LowRenderer()
	: mhMainWnd{}
	, mbDlssAvailable{}
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

	const bool streamlineInitialized = mStreamline.Initialize();

	CheckReturn(CreateDevice());
	if (streamlineInitialized) {
		const auto luid = mDevice->GetAdapterLuid();
		mbDlssAvailable = luid 
			&& mStreamline.CheckFeatureSupport(*luid) 
			&& mStreamline.RegisterDevice(mDevice->GetD3DDevice());
	}
	CheckReturn(CreateCommandObject());
	CheckReturn(CreateDescriptorHeap());
	CheckReturn(CreateSwapChain(width, height));
	CheckReturn(CreateDepthStencilBuffer(
		static_cast<unsigned>(mSwapChain->GetScreenViewport().Width),
		static_cast<unsigned>(mSwapChain->GetScreenViewport().Height)));

	CheckReturn(AllocateDescriptors());
	if (mbDlssAvailable) {
		mOutputDepth = std::make_unique<D3D12DepthStencilBuffer>();
		D3D12DepthStencilBuffer::InitData outputDepthData{
			.Device = mDevice.get(), .Width = width, .Height = height
		};
		CheckReturn(mOutputDepth->Initialize(mDescriptorHeap.get(), &outputDepthData));
		CheckReturn(mOutputDepth->AllocateDescriptors());
	}

	CheckReturn(mCommandObject->FlushDirectCommand());

	return true;
}

void D3D12LowRenderer::CleanUp() {
	if (mSwapChain) {
		// 렌더러가 큐를 비운 뒤, Streamline이 로드된 상태에서 DLSS와 Present 프록시를 해제합니다.
		if (mSwapChain->IsDlssEnabled()) mStreamline.ReleaseDlssResources();
		mSwapChain->ReleaseSwapChain();
	}
	mStreamline.Shutdown();
}

bool D3D12LowRenderer::OnResize(unsigned width, unsigned height) {
	if (!width || !height) return true;
	mbNeedToResize = true;
	mWidth = width;
	mHeight = height;

	return true;
}

bool D3D12LowRenderer::Resize(unsigned width, unsigned height) {
	if (mSwapChain->IsDlssEnabled())
		mStreamline.ReleaseDlssResources();

	auto renderWidth = width;
	auto renderHeight = height;
	const bool dlssEnabled = mbDlssAvailable && mStreamline.ConfigureDlss(width, height, renderWidth, renderHeight);
	CheckReturn(mSwapChain->ResizeOutput(width, height, renderWidth, renderHeight, dlssEnabled));
	CheckReturn(mDepthStencilBuffer->OnResize(renderWidth, renderHeight));
	if (dlssEnabled) CheckReturn(mOutputDepth->OnResize(width, height));

#ifdef _DEBUG
	ConsoleLog("DxRenderer resized (Width: {}, Height: {})", width, height);
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

	auto renderWidth = width;
	auto renderHeight = height;
	const bool dlssEnabled = mbDlssAvailable && mStreamline.ConfigureDlss(width, height, renderWidth, renderHeight);
	D3D12SwapChain::InitData data{
		.MainWndHandle = mhMainWnd,
		.Device = mDevice.get(),
		.CmdObject = mCommandObject.get(),
		.Width = width,
		.Height = height,
		.RenderWidth = renderWidth,
		.RenderHeight = renderHeight,
		.Streamline = mbDlssAvailable ? &mStreamline : nullptr,
		.DlssEnabled = dlssEnabled
	};
	CheckReturn(mSwapChain->Initialize(mDescriptorHeap.get(), &data));
	mbDlssAvailable = mSwapChain->HasStreamlineSwapChain();

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
