#pragma once

class VKContext;
class VKDevice;

class VKSwapChain {
public:
	static const unsigned SwapChainImageCount = 2;

public:
	VKSwapChain();
	virtual ~VKSwapChain();

	DISALLOW_COPY_AND_MOVE(VKSwapChain)

public:
	__forceinline VkSwapchainKHR GetHandle() const noexcept;

public:
	bool Initialize(VKContext* const pContext, VKDevice* const pDevice, unsigned width, unsigned height);
	void CleanUp();

	bool OnResize(unsigned width, unsigned height);

private:
	bool ConfigureSceneViewportAndScissorRect(unsigned width, unsigned height);

	bool CreateSwapChain(unsigned width, unsigned height);
	bool CreateImageViews();

private:
	bool bIsCleanedUp;

	VKContext* mpContext;
	VKDevice* mpDevice;

	VkSwapchainKHR mSwapChain;
	VkFormat mSwapChainImageFormat;
	VkExtent2D mSwapChainExtent;

	std::array<VkImage, SwapChainImageCount> mSwapChainImages;
	std::array<VkImageView, SwapChainImageCount> mSwapChainImageViews;

	VkViewport mScreenViewport;
	VkRect2D mScissorRect;
};

VkSwapchainKHR VKSwapChain::GetHandle() const noexcept { return mSwapChain; }