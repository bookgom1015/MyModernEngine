#include "Renderer/VK/pch.h"
#include "Renderer/VK/VKDevice.hpp"
#include "Renderer/VK/VKContext.hpp"
#include "Renderer/VK/VKUtil.hpp"

VKDevice::VKDevice() 
	: bIsCleanedUp(false) {}

VKDevice::~VKDevice() {
	if (!bIsCleanedUp)
		CleanUp();
}

bool VKDevice::Initialize(VKContext* const pContext) {
	CheckReturn(SelectPhysicalDevices(pContext));
	CheckReturn(CreateLogicalDevice(pContext));

	return true;
}

void VKDevice::CleanUp() {
	if (VKIsValid(mDevice)) {
		vkDestroyDevice(mDevice, nullptr);
		mDevice = VK_NULL_HANDLE;
	}

	mCandidates.clear();

	mPhysicalDevice = VK_NULL_HANDLE;

	bIsCleanedUp = true;
}

bool VKDevice::SelectPhysicalDevices(VKContext* const pContext) {
	mCandidates.clear();

	UINT deviceCount = 0;
	if (vkEnumeratePhysicalDevices(pContext->GetInstance(), &deviceCount, nullptr) != VK_SUCCESS)
		ReturnFalse("Failed to count Vulkan physical devices");
	if (deviceCount == 0) 
		ReturnFalse("Failed to find GPU(s) with Vulkan support");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	if (vkEnumeratePhysicalDevices(pContext->GetInstance(), &deviceCount, devices.data()) != VK_SUCCESS)
		ReturnFalse("Failed to enumerate Vulkan physical devices");
	devices.resize(deviceCount);

	for (const auto& device : devices) {
		INT score = VKUtil::RateDeviceSuitability(device, pContext->GetSurface());
		mCandidates.emplace_back(score, device);
	}

	std::sort(mCandidates.begin(), mCandidates.end(), [](const auto& a, const auto& b) {
		return a.first > b.first;
	});

	for (const auto& pair : mCandidates) {
		const auto device = pair.second;
		CheckReturn(VKUtil::ShowDeviceInfo(device));
	}

	if (mCandidates.empty() || mCandidates.front().first <= 0)
		ReturnFalse("No suitable Vulkan physical device found");

	mPhysicalDevice = mCandidates.front().second;

	VkPhysicalDeviceProperties deviceProperties{};
	vkGetPhysicalDeviceProperties(mPhysicalDevice, &deviceProperties);
	Logln("Selected physical device: {}", deviceProperties.deviceName);

	if (VKUtil::IsDescriptorIndexingSupported(mPhysicalDevice)) {
		mbBindlessSupported = true;
		Logln("{} supports descriptor indexing", deviceProperties.deviceName);
	}
	else {
		mbBindlessSupported = false;
		Logln("{} does not support descriptor indexing", deviceProperties.deviceName);
	}

	if (VKUtil::IsRaytracingSupported(mPhysicalDevice)) {
		mbRaytracingSupported = true;
		Logln("{} supports ray-tracing", deviceProperties.deviceName);
	}
	else {
		mbRaytracingSupported = false;
		Logln("{} does not support ray-tracing", deviceProperties.deviceName);
	}

	return true;
}

bool VKDevice::CreateLogicalDevice(VKContext* const pContext) {
	float queuePriority = 1.f;

	VKUtil::QueueFamilyIndices indices = VKUtil::QueueFamilyIndices::FindQueueFamilies(mPhysicalDevice, pContext->GetSurface());

	const auto queueCreateInfos = VKUtil::QueueFamilyIndices::BuildQueueCreateInfos(indices, &queuePriority);

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.sampleRateShading = VK_TRUE;

	VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES };
	indexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
	indexingFeatures.runtimeDescriptorArray = VK_TRUE;

	VkPhysicalDeviceFeatures2 enabledFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
	enabledFeatures.features = deviceFeatures;
	if (mbBindlessSupported)
		enabledFeatures.pNext = &indexingFeatures;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pNext = &enabledFeatures;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<UINT>(queueCreateInfos.size());
	createInfo.enabledExtensionCount = static_cast<UINT>(VKUtil::DeviceExtensions.size());
	createInfo.ppEnabledExtensionNames = VKUtil::DeviceExtensions.data();
#ifdef _DEBUG
	createInfo.enabledLayerCount = static_cast<UINT>(VKUtil::ValidationLayers.size());
	createInfo.ppEnabledLayerNames = VKUtil::ValidationLayers.data();
#else
	createInfo.enabledLayerCount = 0;
#endif

	if (VKFailed(vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice)))
		ReturnFalse("Failed to create logical device");

	return true;
}