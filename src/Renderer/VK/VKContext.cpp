#include "Renderer/VK/pch.h"
#include "Renderer/VK/VKContext.hpp"
#include "Renderer/VK/VKUtil.hpp"

VKContext::VKContext() 
	: bIsCleanedUp(false) {}

VKContext::~VKContext() {
	if (!bIsCleanedUp) 
		CleanUp();
}

bool VKContext::Initialize(HWND hWnd) {
	CheckReturn(CreateInstance());
	CheckReturn(CreateSurface(hWnd));

	return true;
}

void VKContext::CleanUp() {
	if (VKIsValid(mSurface)) {
		vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
		mSurface = VK_NULL_HANDLE;
	}
	
	if (VKIsValid(mInstance)) {
#ifdef _DEBUG
		VKUtil::DestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
#endif
		vkDestroyInstance(mInstance, nullptr);
		mInstance = VK_NULL_HANDLE;
	}

	bIsCleanedUp = true;
}

bool VKContext::CreateInstance() {
#ifdef _DEBUG
	CheckReturn(VKUtil::CheckValidationLayersSupport());
#endif

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "ModernGameEngine";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
	appInfo.pEngineName = "Modern Game Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	UINT availableExtensionCount = 0;
	if (VKFailed(vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr)))
		ReturnFalse("Failed to get number of instance extension properties");

	std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
	if (VKFailed(vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions.data())))
		ReturnFalse("Failed to get instance extension properties");

	std::vector<const CHAR*> missingExtensions;
	bool status = true;

	for (const auto& requiredExt : VKUtil::RequiredExtensions) {
		bool supported = false;

		for (const auto& availableExt : availableExtensions) {
			if (std::strcmp(requiredExt, availableExt.extensionName) == 0) {
				supported = true;
				break;
			}
		}

		if (!supported) {
			missingExtensions.push_back(requiredExt);
			status = false;
		}
	}

	if (!status) {
		Logln("Upsupported extensions:");
		for (const auto& missingExt : missingExtensions)
			Logln("    ", missingExt);

		return FALSE;
	}

	createInfo.enabledExtensionCount = static_cast<UINT>(VKUtil::RequiredExtensions.size());
	createInfo.ppEnabledExtensionNames = VKUtil::RequiredExtensions.data();

#ifdef _DEBUG
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

	createInfo.enabledLayerCount = static_cast<UINT>(VKUtil::ValidationLayers.size());
	createInfo.ppEnabledLayerNames = VKUtil::ValidationLayers.data();

	VKUtil::PopulateDebugMessengerCreateInfo(debugCreateInfo, nullptr);
	createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#else
	createInfo.enabledLayerCount = 0;
	createInfo.pNext = nullptr;
#endif

	if (VKFailed(vkCreateInstance(&createInfo, nullptr, &mInstance)))
		ReturnFalse("Failed to create instance");

#ifdef _DEBUG
	CheckReturn(VKUtil::SetUpDebugMessenger(mInstance, mDebugMessenger, nullptr));
#endif

	return true;
}

bool VKContext::CreateSurface(HWND hWnd) {
	VkWin32SurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = hWnd;
	createInfo.hinstance = GetModuleHandle(nullptr);

	if (VKFailed(vkCreateWin32SurfaceKHR(mInstance, &createInfo, nullptr, &mSurface)))
		ReturnFalse("Failed to create surface");

	return true;
}