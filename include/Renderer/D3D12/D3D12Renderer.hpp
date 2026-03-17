#pragma once

#include "Renderer/D3D12/D3D12LowRenderer.hpp"

struct ImGui_ImplDX12_InitInfo;

class D3D12FrameResource;

class D3D12Renderer : public D3D12LowRenderer, public Singleton<D3D12Renderer> {
	SINGLETON(D3D12Renderer);

public:
	virtual bool Initialize(
		LogFile* const pLogFile,
		HWND hMainWnd,
		unsigned width, unsigned height) override;

	virtual bool Update(float deltaTime) override;
	virtual bool Draw() override;
	virtual bool DrawEditor(DrawEditorFunc func) override;

	virtual bool OnResize(unsigned width, unsigned height) override;

private:
	bool BuildFrameResources();

	bool PresentAndSignal();

	bool InitializeImGui();
	void CleanUpImGui();

	static void ImGuiSrvAlloc(
		ImGui_ImplDX12_InitInfo* info,
		D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle,
		D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle);
	static void ImGuiSrvFree(
		ImGui_ImplDX12_InitInfo* info, 
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);

	bool AllocateImGuiSrv(
		D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle,
		D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle);
	void FreeImGuiSrv(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);
	
private:
	// Frame resources
	std::vector<std::unique_ptr<D3D12FrameResource>> mFrameResources;
	D3D12FrameResource* mpCurrentFrameResource;
	UINT mCurrentFrameResourceIndex;
};

#ifndef RENDERER
#define RENDERER D3D12Renderer::GetInstance()
#endif