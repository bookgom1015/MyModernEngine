#include "Renderer/VK/pch.h"
#include "Renderer/VK/VKLowRenderer.hpp"
#include "Renderer/VK/VKContext.hpp"
#include "Renderer/VK/VKDevice.hpp"
#include "Renderer/VK/VKSwapChain.hpp"
#include "Renderer/VK/VKQueueManager.hpp"

VKLowRenderer::VKLowRenderer() {}

VKLowRenderer::~VKLowRenderer() {}

bool VKLowRenderer::Initialize(
	HWND hMainWnd
	, unsigned width
	, unsigned height) {
	CheckReturn(CreateContext(hMainWnd));
	CheckReturn(CreateDevice());
	CheckReturn(CreateSwapChain(width, height));
	CheckReturn(CreateQueueManager());

	return true;
}

void VKLowRenderer::CleanUp() {
	if (mQueueManager) {
		mQueueManager->CleanUp();
		mQueueManager.reset();
	}
	if (mSwapChain) {
		mSwapChain->CleanUp();
		mSwapChain.reset();
	}
	if (mDevice) {
		mDevice->CleanUp();
		mDevice.reset();
	}
	if (mContext) {
		mContext->CleanUp();
		mContext.reset();
	}
}

bool VKLowRenderer::OnResize(unsigned width, unsigned height) {
	mbNeedToResize = true;
	mWidth = width;
	mHeight = height;

	return true;
}

bool VKLowRenderer::Resize(unsigned width, unsigned height) {
	CheckReturn(VKLowRenderer::Resize(width, height));

	return true;
}

bool VKLowRenderer::CreateContext(HWND hMainWnd) {
	mContext = std::make_unique<VKContext>();

	CheckReturn(mContext->Initialize(hMainWnd));

	return true;
}

bool VKLowRenderer::CreateDevice() {
	mDevice = std::make_unique<VKDevice>();

	CheckReturn(mDevice->Initialize(mContext.get()));

	return true;
}

bool VKLowRenderer::CreateSwapChain(unsigned width, unsigned height) {
	mSwapChain = std::make_unique<VKSwapChain>();

	CheckReturn(mSwapChain->Initialize(mContext.get(), mDevice.get(), width, height));

	return true;
}

bool VKLowRenderer::CreateQueueManager() {
	mQueueManager = std::make_unique<VKQueueManager>();

	CheckReturn(mQueueManager->Initialize(mContext.get(), mDevice.get(), VKSwapChain::SwapChainImageCount));

	return true;
}