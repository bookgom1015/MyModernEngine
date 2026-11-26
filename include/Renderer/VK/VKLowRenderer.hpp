#pragma once

#include "Renderer/Renderer.hpp"

class VKContext;
class VKDevice;
class VKSwapChain;
class VKQueueManager;

class VKLowRenderer : public Renderer {
public:
	VKLowRenderer();
	virtual ~VKLowRenderer();

	DISALLOW_COPY_AND_MOVE(VKLowRenderer)

public:
	virtual bool Initialize(
		HWND hMainWnd,
		unsigned width,
		unsigned height) override;
	virtual void CleanUp() override;

	virtual bool OnResize(unsigned width, unsigned height) override final;

protected:
	virtual bool Resize(unsigned width, unsigned height);

private:
	bool CreateContext(HWND hMainWnd);
	bool CreateDevice();
	bool CreateSwapChain(unsigned width, unsigned height);
	bool CreateQueueManager();

protected:
	bool mbNeedToResize;
	unsigned mWidth;
	unsigned mHeight;

	std::unique_ptr<VKContext> mContext;
	std::unique_ptr<VKDevice> mDevice;
	std::unique_ptr<VKSwapChain> mSwapChain;
	std::unique_ptr<VKQueueManager> mQueueManager;
};