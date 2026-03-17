#include "pch.h"
#include "Renderer/D3D12/D3D12Renderer.hpp"

#include "Renderer/D3D12/D3D12Device.hpp"
#include "Renderer/D3D12/D3D12CommandObject.hpp"
#include "Renderer/D3D12/D3D12DescriptorHeap.hpp"
#include "Renderer/D3D12/D3D12SwapChain.hpp"
#include "Renderer/D3D12/D3D12GpuResource.hpp"
#include "Renderer/D3D12/D3D12FrameResource.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx12.h>

D3D12Renderer::D3D12Renderer() 
	: mFrameResources{}
	, mpCurrentFrameResource{}
	, mCurrentFrameResourceIndex{} {}

D3D12Renderer::~D3D12Renderer() {
	CleanUpImGui();
}

bool D3D12Renderer::Initialize(
	LogFile* const pLogFile
	, HWND hMainWnd
	, unsigned width
	, unsigned height) {
	CheckReturn(mpLogFile, D3D12LowRenderer::Initialize(pLogFile, hMainWnd, width, height));

	CheckReturn(mpLogFile, BuildFrameResources());

	CheckReturn(mpLogFile, InitializeImGui());

	CheckReturn(mpLogFile, mCommandObject->FlushCommandQueue());

	return true;
}

bool D3D12Renderer::Update(float deltaTime) {
	mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) 
		% D3D12FrameResource::NumFrameResources;
	mpCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();
	
	CheckReturn(mpLogFile, mCommandObject->WaitCompletion(mpCurrentFrameResource->mFence));
	CheckReturn(mpLogFile, mpCurrentFrameResource->ResetCommandListAllocator());

	return true;
}

bool D3D12Renderer::Draw() {

	return true;
}

bool D3D12Renderer::DrawEditor(DrawEditorFunc func) {
	CheckReturn(mpLogFile, mCommandObject->ResetDirectCommandList(
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
	
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	
	ImGui::DockSpaceOverViewport(
		0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
	
	func();
	
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), CmdList);
	
	// Update and Render additional Platform Windows
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}
	
	CheckReturn(mpLogFile, mCommandObject->ExecuteDirectCommandList());

	CheckReturn(mpLogFile, PresentAndSignal());

	return true;
}

bool D3D12Renderer::OnResize(unsigned width, unsigned height) {
	CheckReturn(mpLogFile, D3D12LowRenderer::OnResize(width, height));

	return true;
}

bool D3D12Renderer::BuildFrameResources() {
	for (UINT i = 0; i < D3D12FrameResource::NumFrameResources; i++) {
		mFrameResources.push_back(std::make_unique<D3D12FrameResource>());

		CheckReturn(mpLogFile, mFrameResources.back()->Initialize(mpLogFile, mDevice.get()));
	}

	mCurrentFrameResourceIndex = 0;
	mpCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();

	return true;
}

bool D3D12Renderer::PresentAndSignal() {
	CheckReturn(mpLogFile, mSwapChain->ReadyToPresent(mpCurrentFrameResource));
	CheckReturn(mpLogFile, mSwapChain->Present(mDevice->IsAllowingTearing()));
	mSwapChain->NextBackBuffer();

	mpCurrentFrameResource->mFence = mCommandObject->IncreaseFence();

	CheckReturn(mpLogFile, mCommandObject->Signal());

	return true;
}


bool D3D12Renderer::InitializeImGui() {
	ImGui_ImplWin32_EnableDpiAwareness();

	float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(
		::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigWindowsMoveFromTitleBarOnly = true;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	io.ConfigDpiScaleFonts = true;
	io.ConfigDpiScaleViewports = true;

	io.Fonts->AddFontFromFileTTF(
		"C:\\Windows\\Fonts\\malgun.ttf",
		18.0f,
		nullptr,
		io.Fonts->GetGlyphRangesKorean());

	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowMenuButtonPosition = ImGuiDir_None;
	style.WindowRounding = 0.0f;
	style.ScaleAllSizes(main_scale);
	style.FontScaleDpi = main_scale;

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	CheckReturn(mpLogFile, ImGui_ImplWin32_Init(mhMainWnd));

	ImGui_ImplDX12_InitInfo initInfo = {};
	initInfo.Device = mDevice->GetD3DDevice();
	initInfo.CommandQueue = mCommandObject->GetCommandQueue();
	initInfo.NumFramesInFlight = D3D12SwapChain::SwapChainBufferCount;
	initInfo.RTVFormat = SwapChain::BackBufferFormat;
	initInfo.DSVFormat = DepthStencilBuffer::DepthStencilBufferFormat;
	initInfo.SrvDescriptorHeap = mDescriptorHeap->GetCbvSrvUavHeap();
	initInfo.SrvDescriptorAllocFn = &D3D12Renderer::ImGuiSrvAlloc;
	initInfo.SrvDescriptorFreeFn = &D3D12Renderer::ImGuiSrvFree;
	initInfo.UserData = this;

	CheckReturn(mpLogFile, ImGui_ImplDX12_Init(&initInfo));

	return true;
}

void D3D12Renderer::CleanUpImGui() {
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void D3D12Renderer::ImGuiSrvAlloc(
	ImGui_ImplDX12_InitInfo* info
	, D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle
	, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle) {
	auto renderer = static_cast<D3D12Renderer*>(info->UserData);
	if (!renderer) {
		outCpuHandle->ptr = 0;
		outGpuHandle->ptr = 0;
		return;
	}
	if (!renderer->AllocateImGuiSrv(outCpuHandle, outGpuHandle)) {
		outCpuHandle->ptr = 0;
		outGpuHandle->ptr = 0;
	}
}

void D3D12Renderer::ImGuiSrvFree(
	ImGui_ImplDX12_InitInfo* info
	, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle
	, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle) {
	auto* renderer = static_cast<D3D12Renderer*>(info->UserData);
	if (!renderer) return;

	renderer->FreeImGuiSrv(cpuHandle);
}

bool D3D12Renderer::AllocateImGuiSrv(
	D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle
	, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle) {
	UINT index{};
	if (!mDescriptorHeap->AllocateCbvSrvUav(1, index))
		return false;

	*outCpuHandle = mDescriptorHeap->GetCbvSrvUavCpuHandle(index);
	*outGpuHandle = mDescriptorHeap->GetCbvSrvUavGpuHandle(index);

	return true;
}

void D3D12Renderer::FreeImGuiSrv(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle) {
	// mDescriptorHeap->FreeCbvSrvUav(index, 1);
}