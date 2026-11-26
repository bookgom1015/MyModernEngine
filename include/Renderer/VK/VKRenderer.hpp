#pragma once

#include "Renderer/VK/VKLowRenderer.hpp"
#include "Renderer/VK/VKFrameContext.hpp"

class VKRenderer : public VKLowRenderer, public Singleton<VKRenderer> {
	SINGLETON(VKRenderer);

public:
	virtual bool Initialize(
		HWND hMainWnd,
		unsigned width, unsigned height) override;
	virtual void CleanUp() override;

public:
	virtual bool Update(float deltaTime) override;
	virtual bool Draw() override;

protected:
	virtual bool Resize(unsigned width, unsigned height);

private:
	bool CreateFrameContexts();
	void DestroyFrameContexts();

	bool CreateFrameSynchronizationObjects(VKFrameContext& frameContext);
	void DestroyFrameSynchronizationObjects(VKFrameContext& frameContext);

	bool BeginFrame();
	bool EndFrame();
	bool SubmitFrame(VKFrameContext& frameContext);
	bool PresentFrame(VKFrameContext& frameContext);

private:
	std::array<VKFrameContext, VKFrameContext::MaxFramesInFlight> mFrameContexts;
	std::uint32_t mCurrentFrameIndex;

	std::uint32_t mCurrentSwapChainImageIndex;

	bool mbFramebufferResized;
	bool mbShouldRecreateSwapChain;
};

#ifndef RENDERER
#define RENDERER VKRenderer::GetInstance()
#endif