#include "pch.h"
#include "Renderer/D3D12/D3D12SwapChain.hpp"

#include "Renderer/D3D12/D3D12Device.hpp"
#include "Renderer/D3D12/D3D12CommandObject.hpp"

#include "Renderer/D3D12/D3D12FrameResource.hpp"

D3D12SwapChain::D3D12SwapChain()
	: mInitData{}
	, mScreenViewport{}
	, mScissorRect{} {}

D3D12SwapChain::~D3D12SwapChain() {}

bool D3D12SwapChain::Initialize(
	D3D12DescriptorHeap* const pDescHeap
	, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	mInitData = *reinterpret_cast<InitData*>(pData);

	mScreenViewport = { 
		0, 0, static_cast<FLOAT>(mInitData.Width), static_cast<FLOAT>(mInitData.Height), 0.f, 1.f };
	mScissorRect = { 
		0, 0, static_cast<LONG>(mInitData.Width), static_cast<LONG>(mInitData.Height) };

	for (UINT i = 0; i < SwapChainBufferCount; ++i)
		mSwapChainBuffers[i] = std::make_unique<GpuResource>();

	mHdrMap = std::make_unique<GpuResource>();
	mHdrMapCopy = std::make_unique<GpuResource>();
	mSceneMap = std::make_unique<GpuResource>();
	mSceneMapCopy = std::make_unique<GpuResource>();

	CheckReturn(CreateSwapChain());
	CheckReturn(BuildSwapChainBuffers());
	CheckReturn(BuildResources());

	return true;
}

bool D3D12SwapChain::AllocateDescriptors() {
	// SwapChainBuffer
	for (UINT i = 0; i < SwapChainBufferCount; ++i) {
		UINT offset = i == 0 ? 0 : 1;
		CheckReturn(mpDescHeap->AllocateCbvSrvUav(1, mhBackBufferSrvs[i]));
		CheckReturn(mpDescHeap->AllocateRtv(1, mhBackBufferRtvs[i]));
	}

	// HdrMap
	CheckReturn(mpDescHeap->AllocateCbvSrvUav(1, mhHdrMapSrv));
	CheckReturn(mpDescHeap->AllocateRtv(1, mhHdrMapRtv));

	// HdrMapCopy
	CheckReturn(mpDescHeap->AllocateCbvSrvUav(1, mhHdrMapCopySrv));

	// SceneMap
	CheckReturn(mpDescHeap->AllocateCbvSrvUav(1, mhSceneMapSrv));
	CheckReturn(mpDescHeap->AllocateRtv(1, mhSceneMapRtv));

	// SceneMapCopy
	CheckReturn(mpDescHeap->AllocateCbvSrvUav(1, mhSceneMapCopySrv));

	CheckReturn(BuildDescriptors());

	return true;
}

bool D3D12SwapChain::OnResize(unsigned width, unsigned height) {
	mInitData.Width = width;
	mInitData.Height = height;

	mScreenViewport = { 
		0, 0, static_cast<FLOAT>(mInitData.Width), static_cast<FLOAT>(mInitData.Height), 0.f, 1.f };
	mScissorRect = { 
		0, 0, static_cast<LONG>(mInitData.Width), static_cast<LONG>(mInitData.Height) };

	CheckReturn(BuildSwapChainBuffers());
	CheckReturn(BuildResources());
	CheckReturn(BuildDescriptors());

	return true;
}

bool D3D12SwapChain::ReadyToPresent(D3D12FrameResource* const pFrameResource) {
	CheckReturn(mInitData.CmdObject->ResetDirectCommandList(pFrameResource->FrameCommandAllocator()));
	const auto cmdList = mInitData.CmdObject->GetDirectCommandList();

	mSwapChainBuffers[mCurrBackBuffer]->Transite(cmdList, D3D12_RESOURCE_STATE_PRESENT);

	CheckReturn(mInitData.CmdObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12SwapChain::Present(bool bAllowTearing) {
	CheckHResult(mSwapChain->Present(
		0, bAllowTearing ? DXGI_PRESENT_ALLOW_TEARING : 0));

	return true;
}

void D3D12SwapChain::NextBackBuffer() {
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
}

bool D3D12SwapChain::CreateSwapChain() {
	// Release the previous swapchain we will be recreating.
	if (mSwapChain) mSwapChain.Reset();

	DXGI_SWAP_CHAIN_DESC1 desc{};
	desc.Width = mInitData.Width;
	desc.Height = mInitData.Height;
	desc.Format = SwapChain::BackBufferFormat;
	desc.Stereo = FALSE;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = SwapChainBufferCount;
	desc.Scaling = DXGI_SCALING_STRETCH;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH 
		| (mInitData.Device->IsAllowingTearing() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);

	// Note: Swap chain uses queue to perfrom flush.
	CheckHResult(mInitData.Device->mDxgiFactory->CreateSwapChainForHwnd(
		mInitData.CmdObject->GetCommandQueue(),
		mInitData.MainWndHandle,
		&desc,
		nullptr, nullptr,
		mSwapChain.GetAddressOf()));

	CheckHResult(mInitData.Device->mDxgiFactory->MakeWindowAssociation(
		mInitData.MainWndHandle, DXGI_MWA_NO_ALT_ENTER));

	return true;
}

bool D3D12SwapChain::BuildSwapChainBuffers() {
	// Resize the previous resources we will be creating.
	for (UINT i = 0; i < SwapChainBufferCount; ++i) {
		auto& buffer = mSwapChainBuffers[i];
		if (buffer) buffer->Reset();
	}

	// Resize the swap chain.
	CheckHResult(mSwapChain->ResizeBuffers(
		SwapChainBufferCount,
		mInitData.Width,
		mInitData.Height,
		SwapChain::BackBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH 
		| (mInitData.Device->IsAllowingTearing() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0)));

	for (UINT i = 0; i < SwapChainBufferCount; ++i) {
		const auto buffer = mSwapChainBuffers[i].get();
		buffer->OnResize(mSwapChain.Get(), i);
	}

	mCurrBackBuffer = 0;

	return true;
}

bool D3D12SwapChain::BuildResources() {
	D3D12_RESOURCE_DESC rscDesc{};
	rscDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rscDesc.Alignment = 0;
	rscDesc.Width = mInitData.Width;
	rscDesc.Height = mInitData.Height;
	rscDesc.DepthOrArraySize = 1;
	rscDesc.MipLevels = 1;
	rscDesc.SampleDesc.Count = 1;
	rscDesc.SampleDesc.Quality = 0;
	rscDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	// HdrMap
	{
		rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		rscDesc.Format = HDR_FORMAT;

		auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		CheckReturn(mHdrMap->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"SwapChain_HdrMap"));
	}
	// HdrMapCopy
	{
		rscDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		rscDesc.Format = HDR_FORMAT;

		auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		CheckReturn(mHdrMapCopy->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"SwapChain_HdrMapCopy"));
	}
	// SceneMap
	{
		rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		rscDesc.Format = HDR_FORMAT;

		auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		CheckReturn(mSceneMap->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"SwapChain_SceneMap"));
	}
	// SceneMapCopy
	{
		rscDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		rscDesc.Format = HDR_FORMAT;

		auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		CheckReturn(mSceneMapCopy->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"SwapChain_SceneMapCopy"));
	}

	return true;
}

bool D3D12SwapChain::BuildDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.f;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;

	// SwapChainBuffer
	for (UINT i = 0; i < SwapChainBufferCount; ++i) {
		srvDesc.Format = SwapChain::BackBufferFormat;
		rtvDesc.Format = SwapChain::BackBufferFormat;

		const auto backBuffer = mSwapChainBuffers[i]->Resource();

		mInitData.Device->md3dDevice->CreateShaderResourceView(
			backBuffer, &srvDesc, mpDescHeap->GetCpuHandle(mhBackBufferSrvs[i]));
		mInitData.Device->md3dDevice->CreateRenderTargetView(
			backBuffer, nullptr, mpDescHeap->GetCpuHandle(mhBackBufferRtvs[i]));
	}
	// HdrMap
	{
		srvDesc.Format = HDR_FORMAT;
		rtvDesc.Format = HDR_FORMAT;

		const auto HdrMap = mHdrMap->Resource();

		mInitData.Device->md3dDevice->CreateShaderResourceView(
			HdrMap, &srvDesc, mpDescHeap->GetCpuHandle(mhHdrMapSrv));
		mInitData.Device->md3dDevice->CreateRenderTargetView(
			HdrMap, nullptr, mpDescHeap->GetCpuHandle(mhHdrMapRtv));
	}
	// HdrMapCopy
	{
		srvDesc.Format = HDR_FORMAT;
		rtvDesc.Format = HDR_FORMAT;

		mInitData.Device->md3dDevice->CreateShaderResourceView(
			mHdrMapCopy->Resource(), &srvDesc, mpDescHeap->GetCpuHandle(mhHdrMapCopySrv));
	}
	// SceneMap
	{
		srvDesc.Format = HDR_FORMAT;
		rtvDesc.Format = HDR_FORMAT;

		const auto sceneMap = mSceneMap->Resource();

		mInitData.Device->md3dDevice->CreateShaderResourceView(
			sceneMap, &srvDesc, mpDescHeap->GetCpuHandle(mhSceneMapSrv));
		mInitData.Device->md3dDevice->CreateRenderTargetView(
			sceneMap, nullptr, mpDescHeap->GetCpuHandle(mhSceneMapRtv));
	}
	// SceneMapCopy
	{
		srvDesc.Format = HDR_FORMAT;
		rtvDesc.Format = HDR_FORMAT;

		mInitData.Device->md3dDevice->CreateShaderResourceView(
			mSceneMapCopy->Resource(), &srvDesc, mpDescHeap->GetCpuHandle(mhSceneMapCopySrv));
	}

	return true;
}