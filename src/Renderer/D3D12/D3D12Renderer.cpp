#include "Renderer/D3D12/pch_d3d12.h"
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

extern "C" RendererAPI Renderer* CreateRenderer() {
	return NEW D3D12Renderer();
}

extern "C" RendererAPI void DestroyRenderer(Renderer* const pRenderer) {
    delete pRenderer;
}

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
	, unsigned height
	, DrawEditorCallback callback) {
	CheckReturn(mpLogFile, D3D12LowRenderer::Initialize(pLogFile, hMainWnd, width, height, callback));

	CheckReturn(mpLogFile, BuildFrameResources());

	CheckReturn(mpLogFile, InitializeImGui());

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
	CheckReturn(mpLogFile, DrawImGui());

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
    // Make process DPI aware and obtain main monitor scale
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(
        ::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;
    //io.ConfigDockingAlwaysTabBar = true;
    //io.ConfigDockingTransparentPayload = true;

    io.Fonts->AddFontFromFileTTF(
        "C:\\Windows\\Fonts\\malgun.ttf",
        18.f,
        NULL,
        io.Fonts->GetGlyphRangesKorean());

	io.Fonts->AddFontDefault();
	io.Fonts->Build();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)
    io.ConfigDpiScaleFonts = true;          // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
    io.ConfigDpiScaleViewports = true;      // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

	UINT mhImGuiIdx{};
	mDescriptorHeap->AllocateCbvSrvUav(1, mhImGuiIdx);

	auto mhImGuiCpuSrv = mDescriptorHeap->GetCbvSrvUavCpuHandle(mhImGuiIdx);
	auto mhImGuiGpuSrv = mDescriptorHeap->GetCbvSrvUavGpuHandle(mhImGuiIdx);

    // Setup Platform/Renderer backends
    CheckReturn(mpLogFile, ImGui_ImplWin32_Init(mhMainWnd));
	CheckReturn(mpLogFile, ImGui_ImplDX12_Init(
		mDevice->GetD3DDevice(),
		D3D12SwapChain::SwapChainBufferCount,
		SwapChain::BackBufferFormat,
		mDescriptorHeap->GetCbvSrvUavHeap(),
		mhImGuiCpuSrv,
		mhImGuiGpuSrv));

	return true;
}

void D3D12Renderer::CleanUpImGui() {
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

bool D3D12Renderer::DrawImGui() {
	CheckReturn(mpLogFile, mCommandObject->ResetDirectCommandList(
		mpCurrentFrameResource->CommandAllocator(),
		nullptr));

	const auto CmdList = mCommandObject->GetDirectCommandList();
	mDescriptorHeap->SetDescriptorHeap(CmdList);

	CmdList->RSSetViewports(1, &mSwapChain->GetScreenViewport());
	CmdList->RSSetScissorRects(1, &mSwapChain->GetScissorRect());

	mSwapChain->GetCurrentBackBuffer()->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

	const auto rtv = mSwapChain->GetCurrentBackBufferRtv();

	FLOAT clearValues[4] = { 0.1f, 0.1f, 0.1, 1.f };
	CmdList->ClearRenderTargetView(rtv, clearValues, 0, nullptr);
	CmdList->OMSetRenderTargets(1, &rtv, TRUE, nullptr);

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	mDrawEditorCallback();

	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), CmdList);

	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	CheckReturn(mpLogFile, mCommandObject->ExecuteDirectCommandList());

	return true;
}