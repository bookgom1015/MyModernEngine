#include "Renderer/VK/pch.h"
#include "Renderer/VK/VKUtil.hpp"

const std::vector<const CHAR*> VKUtil::ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const CHAR*> VKUtil::DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
const std::vector<const CHAR*> VKUtil::RequiredExtensions = {
	VK_KHR_SURFACE_EXTENSION_NAME
	, VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#ifdef _DEBUG
	, VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
};

std::vector<std::uint32_t> VKUtil::QueueFamilyIndices::GetSwapChainQueueFamilyIndices() const {
	if (GraphicsFamily == PresentFamily)
		return { *GraphicsFamily };

	return {
		*GraphicsFamily ,
		*PresentFamily
	};
}

std::vector<VkQueueFamilyProperties> VKUtil::QueueFamilyIndices::GetQueueFamilyProperties(const VkPhysicalDevice& physicalDevice) {
	std::uint32_t familyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(
		physicalDevice,
		&familyCount,
		nullptr);

	if (familyCount == 0)
		throw std::runtime_error("No Vulkan queue families found");

	std::vector<VkQueueFamilyProperties> properties(familyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(
		physicalDevice,
		&familyCount,
		properties.data());

	return properties;
}

VKUtil::QueueFamilyIndices VKUtil::QueueFamilyIndices::FindQueueFamilies(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface) {
	const auto properties = GetQueueFamilyProperties(physicalDevice);
	const auto familyCount = static_cast<std::uint32_t>(properties.size());

	std::vector<VkBool32> presentSupport(familyCount, VK_FALSE);

	for (std::uint32_t i = 0; i < familyCount; ++i) {
		const auto result = vkGetPhysicalDeviceSurfaceSupportKHR(
			physicalDevice,
			i,
			surface,
			&presentSupport[i]);

		if (VKFailed(result))
			throw std::runtime_error("Failed to query queue family present support");
	}

	QueueFamilyIndices indices{};
	// 1. 하나의 Family가 모든 역할을 담당할 수 있는지 먼저 검사한다.
	for (std::uint32_t i = 0; i < familyCount; ++i) {
		const auto& family = properties[i];

		if (SupportsGraphics(family) && SupportsCompute(family) && SupportsTransfer(family) && presentSupport[i] == VK_TRUE) {
			indices.GraphicsFamily = i;
			indices.ComputeFamily = i;
			indices.TransferFamily = i;
			indices.PresentFamily = i;

			return indices;
		}
	}
	// 2. Graphics Family를 먼저 선택한다.
	for (std::uint32_t i = 0; i < familyCount; ++i) {
		if (SupportsGraphics(properties[i])) {
			indices.GraphicsFamily = i;
			break;
		}
	}

	if (!indices.GraphicsFamily)
		throw std::runtime_error("No graphics-capable queue family found");

	const auto graphicsFamily = *indices.GraphicsFamily;
	const auto& graphicsProperties = properties[graphicsFamily];

	// 3. Graphics Family가 Compute를 지원하면 같은 Queue를 사용한다.
	if (SupportsCompute(graphicsProperties))
		indices.ComputeFamily = graphicsFamily;

	// 4. Graphics Family에서 Transfer 명령이 가능하면 재사용한다.
	if (SupportsTransfer(graphicsProperties))
		indices.TransferFamily = graphicsFamily;

	// 5. Graphics Family가 Present를 지원하면 재사용한다.
	if (presentSupport[graphicsFamily] == VK_TRUE)
		indices.PresentFamily = graphicsFamily;

	// 6. Compute를 처리할 Family가 없다면 별도로 찾는다.
	if (!indices.ComputeFamily) {
		// 가능한 경우 Graphics 없는 Compute Family를 우선 선택한다.
		for (std::uint32_t i = 0; i < familyCount; ++i) {
			if (IsDedicatedCompute(properties[i])) {
				indices.ComputeFamily = i;
				break;
			}
		}

		// 전용 Compute Family가 없다면 Compute 가능한 아무 Family나 선택한다.
		if (!indices.ComputeFamily) {
			for (std::uint32_t i = 0; i < familyCount; ++i) {
				if (SupportsCompute(properties[i])) {
					indices.ComputeFamily = i;
					break;
				}
			}
		}
	}

	// 7. Transfer를 처리할 Family가 없다면 별도로 찾는다.
	if (!indices.TransferFamily) {
		// 가능한 경우 Transfer-only Family를 우선 선택한다.
		for (std::uint32_t i = 0; i < familyCount; ++i) {
			if (IsDedicatedTransfer(properties[i])) {
				indices.TransferFamily = i;
				break;
			}
		}

		// 전용 Family가 없다면 Transfer 가능한 아무 Family나 선택한다.
		if (!indices.TransferFamily) {
			for (std::uint32_t i = 0; i < familyCount; ++i) {
				if (SupportsTransfer(properties[i])) {
					indices.TransferFamily = i;
					break;
				}
			}
		}
	}

	// 8. Present Family가 없다면 별도로 찾는다.
	if (!indices.PresentFamily) {
		for (std::uint32_t i = 0; i < familyCount; ++i) {
			if (properties[i].queueCount > 0 && presentSupport[i] == VK_TRUE) {
				indices.PresentFamily = i;
				break;
			}
		}
	}

	if (!indices.IsComplete())
		throw std::runtime_error("Required Vulkan queue families are not available");

	return indices;
}

std::vector<VkDeviceQueueCreateInfo> VKUtil::QueueFamilyIndices::BuildQueueCreateInfos(
	const QueueFamilyIndices& indices
	, const float* const queuePriority) {
	if (!indices.IsComplete())
		throw std::runtime_error("Queue family indices are incomplete");

	const std::set<std::uint32_t> uniqueFamilyIndices = {
		*indices.GraphicsFamily,
		*indices.ComputeFamily,
		*indices.TransferFamily,
		*indices.PresentFamily 
	};
	
	std::vector<VkDeviceQueueCreateInfo> createInfos{};
	createInfos.reserve(uniqueFamilyIndices.size());

	for (const std::uint32_t familyIndex : uniqueFamilyIndices) {
		VkDeviceQueueCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		createInfo.queueFamilyIndex = familyIndex;
		createInfo.queueCount = 1;
		createInfo.pQueuePriorities = queuePriority;

		createInfos.push_back(createInfo);
	}

	return createInfos;
}

bool VKUtil::QueueFamilyIndices::SupportsGraphics(const VkQueueFamilyProperties& props) {
	return props.queueCount > 0 && (props.queueFlags & VK_QUEUE_GRAPHICS_BIT);
}

bool VKUtil::QueueFamilyIndices::SupportsCompute(const VkQueueFamilyProperties& props) {
	return props.queueCount > 0 && (props.queueFlags & VK_QUEUE_COMPUTE_BIT);
}

bool VKUtil::QueueFamilyIndices::SupportsTransfer(const VkQueueFamilyProperties& props) {
	if (props.queueCount == 0)
		return false;

	constexpr VkQueueFlags flags = VK_QUEUE_TRANSFER_BIT 
		| VK_QUEUE_GRAPHICS_BIT 
		| VK_QUEUE_COMPUTE_BIT;

	return (props.queueFlags & flags) != 0;
}

bool VKUtil::QueueFamilyIndices::IsDedicatedCompute(const VkQueueFamilyProperties& props) {
	return SupportsCompute(props) && !SupportsGraphics(props);
}

bool VKUtil::QueueFamilyIndices::IsDedicatedTransfer(const VkQueueFamilyProperties& props) {
	return (props.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0 
		&& (props.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 
		&& (props.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0;
}

bool VKUtil::QueueKey::operator==(const QueueKey& other) const {
	return FamilyIndex == other.FamilyIndex && QueueIndex == other.QueueIndex;
}

bool VKUtil::CheckValidationLayersSupport() {
	UINT layerCount = 0;
	if (VKFailed(vkEnumerateInstanceLayerProperties(&layerCount, nullptr)))
		ReturnFalse("Failed to get numbuer of instance layer properties");

	std::vector<VkLayerProperties> availableLayers(layerCount);
	if (VKFailed(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data())))
		ReturnFalse("Failed to get instance layer properties");

#ifdef _DEBUG
	Logln("Available Layers:");
	for (const auto& layer : availableLayers)
		Logln("    ", layer.layerName);
#endif

	std::vector<const char*> unsupportedLayers;
	bool status = true;

	for (const auto layerName : ValidationLayers) {
		bool layerFound = false;

		for (const auto& layer : availableLayers) {
			if (std::strcmp(layerName, layer.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			unsupportedLayers.push_back(layerName);
			status = false;
		}
	}

	if (!status) {
		std::stringstream sstream{};

		for (const auto& layerName : unsupportedLayers)
			sstream << layerName << ' ';
		sstream << "is/are not supported";

		ReturnFalse(sstream.str());
	}

	return true;
}

VkResult VKUtil::CreateDebugUtilsMessengerEXT(
	const VkInstance& instance
	, VkDebugUtilsMessengerCreateInfoEXT* const pCreateInfo
	, VkAllocationCallbacks* const pAllocator
	, VkDebugUtilsMessengerEXT* const pDebugMessenger) {
	const auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		instance, 
		"vkCreateDebugUtilsMessengerEXT");

	if (func != nullptr) 
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	else 
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void VKUtil::DestroyDebugUtilsMessengerEXT(
	const VkInstance& instance
	, VkDebugUtilsMessengerEXT debugMessenger
	, VkAllocationCallbacks* const pAllocator) {
	const auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
		instance, 
		"vkDestroyDebugUtilsMessengerEXT");

	if (func != nullptr) 
		func(instance, debugMessenger, pAllocator);
}

namespace {
	VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity
		, VkDebugUtilsMessageTypeFlagsEXT messageType
		, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData
		, void* pUserData) {
		Logln(pCallbackData->pMessage);

		return VK_FALSE;
	}
}

void VKUtil::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo, void* const pUserData) {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
	createInfo.pUserData = pUserData;
}

bool VKUtil::SetUpDebugMessenger(
	const VkInstance& instance
	, VkDebugUtilsMessengerEXT& debugMessenger
	, void* pUserData) {
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	PopulateDebugMessengerCreateInfo(createInfo, pUserData);

	if (VKFailed(CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger)))
		ReturnFalse("Failed to create debug messenger");

	return true;
}

bool VKUtil::CheckDeviceExtensionsSupport(const VkPhysicalDevice& physicalDevice) {
	UINT extensionCount = 0;
	if (VKFailed(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr)))
		return FALSE;

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	if (VKFailed(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data())))
		return FALSE;

	std::set<std::string> requiredExtensions(DeviceExtensions.begin(), DeviceExtensions.end());
	for (const auto& extension : availableExtensions)
		requiredExtensions.erase(extension.extensionName);

	return requiredExtensions.empty();
}

VKUtil::SwapChainSupportDetails VKUtil::QuerySwapChainSupport(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface) {
	SwapChainSupportDetails details{};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.Capabilities);

	UINT formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
	if (formatCount != 0) {
		details.Formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.Formats.data());
	}

	UINT presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
	if (presentModeCount != 0) {
		details.PresentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.PresentModes.data());
	}

	return details;
}

bool VKUtil::IsDeviceSuitable(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface) {
	QueueFamilyIndices indices = QueueFamilyIndices::FindQueueFamilies(physicalDevice, surface);

	bool extensionSupported = CheckDeviceExtensionsSupport(physicalDevice);
	bool swapChainAdequate = false;
	if (extensionSupported) {
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice, surface);
		swapChainAdequate = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures{};
	vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

	return indices.IsComplete() && extensionSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

int VKUtil::RateDeviceSuitability(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface) {
	int score = 0;

	VkPhysicalDeviceProperties deviceProperties{};
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

	switch (deviceProperties.deviceType) {
	case VK_PHYSICAL_DEVICE_TYPE_OTHER:
		score += 100;
		break;
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
		score += 300;
		break;
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		score += 1000;
		break;
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
		score += 150;
		break;
	case VK_PHYSICAL_DEVICE_TYPE_CPU:
		score += 10;
		break;
	}
	score += deviceProperties.limits.maxImageDimension2D;

	VkPhysicalDeviceFeatures deviceFeatures{};
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

	if (!deviceFeatures.geometryShader || !IsDeviceSuitable(physicalDevice, surface)) {
		Logln("    This device is not suitable");
		return 0;
	}

	return score;
}

bool VKUtil::ShowDeviceInfo(const VkPhysicalDevice& physicalDevice) {
	VkPhysicalDeviceProperties deviceProperties{};
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

	Logln("Physical device properties:");
	Logln("    Device name: ", deviceProperties.deviceName);

	std::stringstream sstream{};
	sstream << "    Device type: ";
	switch (deviceProperties.deviceType) {
	case VK_PHYSICAL_DEVICE_TYPE_OTHER:
		sstream << "Other type";
		break;
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
		sstream << "Integrated GPU";
		break;
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		sstream << "Discrete GPU";
		break;
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
		sstream << "Virtual GPU";
		break;
	case VK_PHYSICAL_DEVICE_TYPE_CPU:
		sstream << "CPU type";
		break;
	}
	Logln(sstream.str().c_str());

	{
		UINT variant = VK_API_VERSION_VARIANT(deviceProperties.driverVersion);
		UINT major = VK_API_VERSION_MAJOR(deviceProperties.driverVersion);
		UINT minor = VK_API_VERSION_MINOR(deviceProperties.driverVersion);
		UINT patch = VK_API_VERSION_PATCH(deviceProperties.driverVersion);
		Logln(std::format("    Device version: {}.{}.{}.{}", variant, major, minor, patch));
	}
	{
		UINT variant = VK_API_VERSION_VARIANT(deviceProperties.apiVersion);
		UINT major = VK_API_VERSION_MAJOR(deviceProperties.apiVersion);
		UINT minor = VK_API_VERSION_MINOR(deviceProperties.apiVersion);
		UINT patch = VK_API_VERSION_PATCH(deviceProperties.apiVersion);
		Logln(std::format("    API version: {}.{}.{}.{}", variant, major, minor, patch));
	}

	return true;
}

bool VKUtil::IsRaytracingSupported(VkPhysicalDevice physicalDevice) {
	std::uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());

	for (const auto& ext : extensions) {
		if (strcmp(ext.extensionName, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) == 0)
			return true;
	}

	return false;
}

VkSurfaceFormatKHR VKUtil::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& inAvailableFormats) {
	for (const auto& availableFormat : inAvailableFormats) {
		if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB 
			&& availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
			return availableFormat;
	}

	return inAvailableFormats[0];
}

VkPresentModeKHR VKUtil::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& inAvailablePresentMods) {
	for (const auto& availablePresentMode : inAvailablePresentMods) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) 
			return availablePresentMode;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VKUtil::ChooseSwapExtent(UINT width, UINT height, const VkSurfaceCapabilitiesKHR& inCapabilities) {
	if (inCapabilities.currentExtent.width != UINT32_MAX) 
		return inCapabilities.currentExtent;

	VkExtent2D actualExtent = { static_cast<UINT>(width), static_cast<UINT>(height) };

	actualExtent.width = std::max(
		inCapabilities.minImageExtent.width, 
		std::min(inCapabilities.maxImageExtent.width, actualExtent.width));

	actualExtent.height = std::max(
		inCapabilities.minImageExtent.height, 
		std::min(inCapabilities.maxImageExtent.height, actualExtent.height));

	return actualExtent;
}

bool VKUtil::CreateImageView(
	const VkDevice& device
	, const VkImage& image
	, const VkFormat& format
	, UINT mipLevles
	, const VkImageAspectFlags& aspectFlags
	, VkImageView& imageView) {
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevles;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (VKFailed(vkCreateImageView(device, &viewInfo, nullptr, &imageView)))
		ReturnFalse("Failed to create texture image view");

	return true;
}