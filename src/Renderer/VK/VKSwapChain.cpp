#include "Renderer/VK/pch.h"
#include "Renderer/VK/VKSwapChain.hpp"
#include "Renderer/VK/VKUtil.hpp"
#include "Renderer/VK/VKContext.hpp"
#include "Renderer/VK/VKDevice.hpp"

VKSwapChain::VKSwapChain() 
	: bIsCleanedUp(false)
	, mpContext(nullptr)
	, mpDevice(nullptr)
	, mSwapChain(VK_NULL_HANDLE) {}

VKSwapChain::~VKSwapChain() {
	if (!bIsCleanedUp) 
		CleanUp();
}

bool VKSwapChain::Initialize(VKContext* const pContext, VKDevice* const pDevice, unsigned width, unsigned height) {
	mpContext = pContext;
	mpDevice = pDevice;

	CheckReturn(ConfigureSceneViewportAndScissorRect(width, height));
	CheckReturn(CreateSwapChain(width, height));
	CheckReturn(CreateImageViews());

	return true;
}

void VKSwapChain::CleanUp() {
	if (mpDevice) {
		for (unsigned i = 0; i < SwapChainImageCount; ++i) {
			const auto swapChainImageView = mSwapChainImageViews[i];
			if (VKIsValid(swapChainImageView))
				vkDestroyImageView(mpDevice->GetDevice(), swapChainImageView, nullptr);
		}

		if (VKIsValid(mSwapChain)) {
			vkDestroySwapchainKHR(mpDevice->GetDevice(), mSwapChain, nullptr);

			mSwapChain = VK_NULL_HANDLE;
		}
	}

	mpContext = nullptr;
	mpDevice = nullptr;

	bIsCleanedUp = true;
}

bool VKSwapChain::OnResize(unsigned width, unsigned height) {
	CheckReturn(ConfigureSceneViewportAndScissorRect(width, height));
	CheckReturn(CreateSwapChain(width, height));
	CheckReturn(CreateImageViews());

	return true;
}

bool VKSwapChain::ConfigureSceneViewportAndScissorRect(unsigned width, unsigned height) {
	mScreenViewport = {
		0.f, 0.f,
		static_cast<float>(width),
		static_cast<float>(height),
		0.f, 1.f
	};

	mScissorRect = {
		{ 0, 0 },
		{ width, height }
	};

	return true;
}

bool VKSwapChain::CreateSwapChain(unsigned width, unsigned height) {
	if (!mpContext || !mpDevice)
		ReturnFalse("Invalid context or device");

	VKUtil::SwapChainSupportDetails swapChainSupport = VKUtil::QuerySwapChainSupport(
		mpDevice->GetPhysicalDevice(), 
		mpContext->GetSurface());

	VkSurfaceFormatKHR surfaceFormat = VKUtil::ChooseSwapSurfaceFormat(swapChainSupport.Formats);
	VkPresentModeKHR presentMode = VKUtil::ChooseSwapPresentMode(swapChainSupport.PresentModes);
	VkExtent2D extent = VKUtil::ChooseSwapExtent(width, height, swapChainSupport.Capabilities);

	unsigned imageCount = SwapChainImageCount;

	if (imageCount < swapChainSupport.Capabilities.minImageCount ||
		(swapChainSupport.Capabilities.maxImageCount > 0 && imageCount > swapChainSupport.Capabilities.maxImageCount)) {
		ReturnFalse("Swap-chain does not support the specified number of images");
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = mpContext->GetSurface();
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	VKUtil::QueueFamilyIndices indices = VKUtil::QueueFamilyIndices::FindQueueFamilies(
		mpDevice->GetPhysicalDevice(), 
		mpContext->GetSurface());

	const auto swapChainIndices = indices.GetSwapChainQueueFamilyIndices();

	if (swapChainIndices.size() == 1) {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 1;
		createInfo.pQueueFamilyIndices = nullptr;		
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = static_cast<std::uint32_t>(swapChainIndices.size());
		createInfo.pQueueFamilyIndices = swapChainIndices.data();
	}

	createInfo.preTransform = swapChainSupport.Capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (VKFailed(vkCreateSwapchainKHR(mpDevice->GetDevice(), &createInfo, nullptr, &mSwapChain)))
		ReturnFalse("Failed to create swap chain");

	if (VKFailed(vkGetSwapchainImagesKHR(mpDevice->GetDevice(), mSwapChain, &imageCount, mSwapChainImages.data())))
		ReturnFalse("Failed to get swap chain images");

	mSwapChainImageFormat = surfaceFormat.format;
	mSwapChainExtent = extent;

	return true;
}

bool VKSwapChain::CreateImageViews() {
	for (size_t i = 0, end = mSwapChainImages.size(); i < end; ++i) {
		CheckReturn(VKUtil::CreateImageView(
			mpDevice->GetDevice(),
			mSwapChainImages[i],
			mSwapChainImageFormat,
			1,
			VK_IMAGE_ASPECT_COLOR_BIT,
			mSwapChainImageViews[i]));
	}

	return true;
}