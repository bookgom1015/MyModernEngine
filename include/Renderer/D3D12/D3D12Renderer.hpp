#pragma once

#include "Renderer/D3D12/D3D12LowRenderer.hpp"

extern "C" RendererAPI Renderer* CreateRenderer();
extern "C" RendererAPI void DestroyRenderer(Renderer* const pRenderer);

class D3D12FrameResource;

class D3D12Renderer : public D3D12LowRenderer {
public:
	D3D12Renderer();
	virtual ~D3D12Renderer();

public:
	RendererAPI virtual bool Initialize(
		LogFile* const pLogFile,
		HWND hMainWnd,
		unsigned width, unsigned height,
		DrawEditorCallback callback) override;

	RendererAPI virtual bool Update(float deltaTime) override;
	RendererAPI virtual bool Draw() override;

	RendererAPI virtual bool OnResize(unsigned width, unsigned height) override;

private:
	bool BuildFrameResources();

	bool PresentAndSignal();

	bool InitializeImGui();
	void CleanUpImGui();
	bool DrawImGui();

private:
	// Frame resources
	std::vector<std::unique_ptr<D3D12FrameResource>> mFrameResources;
	D3D12FrameResource* mpCurrentFrameResource;
	UINT mCurrentFrameResourceIndex;
};