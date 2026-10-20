#include "Renderer/VK/pch.h"
#include "Renderer/VK/VKCommandPool.hpp"
#include "Renderer/VK/VKUtil.hpp"
#include "Renderer/VK/VKDevice.hpp"

VKCommandPool::VKCommandPool() 
	: bIsCleanedUp(false) 
	, mpDevice(nullptr)
	, mCommandPool(VK_NULL_HANDLE) {}

VKCommandPool::~VKCommandPool() {
	if (!bIsCleanedUp)
		CleanUp();
}

bool VKCommandPool::Initialize(VKDevice* const pDevice, std::uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags) {
	assert(pDevice != nullptr);
	assert(queueFamilyIndex != VK_QUEUE_FAMILY_IGNORED);
	assert(mCommandPool == VK_NULL_HANDLE);

	mpDevice = pDevice;

	CheckReturn(CreateCommandPool(queueFamilyIndex, flags));

	return true;
}

void VKCommandPool::CleanUp() {
	if (VKIsValid(mCommandPool)) {
		if (mpDevice) {
			vkDestroyCommandPool(mpDevice->GetDevice(), mCommandPool, nullptr);

			mCommandPool = VK_NULL_HANDLE;
		}
	}

	mpDevice = nullptr;

	bIsCleanedUp = true;
}

bool VKCommandPool::Reset(VkCommandPoolResetFlags flags) {
	assert(mpDevice != nullptr);
	assert(VKIsValid(mCommandPool));

	if (VKFailed(vkResetCommandPool(mpDevice->GetDevice(), mCommandPool, flags)))
		ReturnFalse("Failed to reset command pool");

	return true;
}

bool VKCommandPool::CreateCommandPool(std::uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags) {
	VkCommandPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.queueFamilyIndex = queueFamilyIndex;
	createInfo.flags = flags;

	if (VKFailed(vkCreateCommandPool(mpDevice->GetDevice(), &createInfo, nullptr, &mCommandPool)))
		ReturnFalse("Failed to create command pool");

	return true;
}