#include "pch.h"
#include "Renderer/D3D12/D3D12Renderer.hpp"

#include "Renderer/D3D12/D3D12Device.hpp"
#include "Renderer/D3D12/D3D12CommandObject.hpp"
#include "Renderer/D3D12/D3D12SwapChain.hpp"
#include "Renderer/D3D12/D3D12GpuResource.hpp"
#include "Renderer/D3D12/D3D12FrameResource.hpp"

#include "EditorManager.hpp"

D3D12Renderer::D3D12Renderer() 
	: mFrameResources{}
	, mpCurrentFrameResource{}
	, mCurrentFrameResourceIndex{}
	, mhImGuiSrv{} {}

D3D12Renderer::~D3D12Renderer() {}

bool D3D12Renderer::Initialize(
	HWND hMainWnd
	, unsigned width
	, unsigned height) {
	CheckReturn(D3D12LowRenderer::Initialize(hMainWnd, width, height));

	CheckReturn(BuildFrameResources());

	CheckReturn(mCommandObject->FlushCommandQueue());

	return true;
}

bool D3D12Renderer::Update(float deltaTime) {
	mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) 
		% D3D12FrameResource::NumFrameResources;
	mpCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();
	
	CheckReturn(mCommandObject->WaitCompletion(mpCurrentFrameResource->mFence));
	CheckReturn(mpCurrentFrameResource->ResetCommandListAllocator());

	return true;
}

bool D3D12Renderer::Draw() {
	CheckReturn(DrawEditor());

	CheckReturn(PresentAndSignal());

	return true;
}

bool D3D12Renderer::OnResize(unsigned width, unsigned height) {
	CheckReturn(D3D12LowRenderer::OnResize(width, height));

	return true;
}

bool D3D12Renderer::AllocateImGuiSrv(
	D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle
	, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle) {
	if (!mDescriptorHeap->AllocateCbvSrvUav(1, mhImGuiSrv))
		return false;

	*outCpuHandle = mDescriptorHeap->GetCpuHandle(mhImGuiSrv);
	*outGpuHandle = mDescriptorHeap->GetGpuHandle(mhImGuiSrv);

	return true;
}

void D3D12Renderer::FreeImGuiSrv(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle) {
	mDescriptorHeap->Free(mhImGuiSrv);
}

void D3D12Renderer::ImGuiSrvAlloc(
	ImGui_ImplDX12_InitInfo* info
	, D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle
	, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle) {
	if (!RENDERER->AllocateImGuiSrv(outCpuHandle, outGpuHandle)) {
		outCpuHandle->ptr = 0;
		outGpuHandle->ptr = 0;
	}
}

void D3D12Renderer::ImGuiSrvFree(
	ImGui_ImplDX12_InitInfo* info
	, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle
	, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle) {
	RENDERER->FreeImGuiSrv(cpuHandle);
}

void D3D12Renderer::BuildDX12InitInfo(ImGui_ImplDX12_InitInfo& outInitInfo) const {
	outInitInfo = {};
	outInitInfo.Device = mDevice->GetD3DDevice();
	outInitInfo.CommandQueue = mCommandObject->GetCommandQueue();
	outInitInfo.NumFramesInFlight = D3D12SwapChain::SwapChainBufferCount;
	outInitInfo.RTVFormat = SwapChain::BackBufferFormat;
	outInitInfo.DSVFormat = DepthStencilBuffer::DepthStencilBufferFormat;
	outInitInfo.SrvDescriptorHeap = mDescriptorHeap->GetCbvSrvUavHeap();
	outInitInfo.SrvDescriptorAllocFn = &D3D12Renderer::ImGuiSrvAlloc;
	outInitInfo.SrvDescriptorFreeFn = &D3D12Renderer::ImGuiSrvFree;
}

ID3D12GraphicsCommandList* D3D12Renderer::GetCommandList() const noexcept {
	return mCommandObject->GetDirectCommandList();
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Renderer::GetSceneMapSrv() const {
	return mSwapChain->GetSceneMapSrv();
}

bool D3D12Renderer::BuildFrameResources() {
	for (UINT i = 0; i < D3D12FrameResource::NumFrameResources; i++) {
		mFrameResources.push_back(std::make_unique<D3D12FrameResource>());

		CheckReturn(mFrameResources.back()->Initialize(mDevice.get()));
	}

	mCurrentFrameResourceIndex = 0;
	mpCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();

	return true;
}

bool D3D12Renderer::DrawEditor() {
	CheckReturn(mCommandObject->ResetDirectCommandList(
		mpCurrentFrameResource->CommandAllocator(),
		nullptr));

	const auto CmdList = mCommandObject->GetDirectCommandList();
	mDescriptorHeap->SetDescriptorHeap(CmdList);

	CmdList->RSSetViewports(1, &mSwapChain->GetScreenViewport());
	CmdList->RSSetScissorRects(1, &mSwapChain->GetScissorRect());

	mSwapChain->GetCurrentBackBuffer()->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

	const auto rtv = mSwapChain->GetCurrentBackBufferRtv();

	FLOAT clearValues[4] = { 0.1f, 0.1f, 0.1f, 1.f };
	CmdList->ClearRenderTargetView(rtv, clearValues, 0, nullptr);
	CmdList->OMSetRenderTargets(1, &rtv, TRUE, nullptr);

	EditorManager::GetInstance()->Draw();

	CheckReturn(mCommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12Renderer::PresentAndSignal() {
	CheckReturn(mSwapChain->ReadyToPresent(mpCurrentFrameResource));
	CheckReturn(mSwapChain->Present(mDevice->IsAllowingTearing()));
	mSwapChain->NextBackBuffer();

	mpCurrentFrameResource->mFence = mCommandObject->IncreaseFence();

	CheckReturn(mCommandObject->Signal());

	return true;
}