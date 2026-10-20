#include "Renderer/VK/pch.h"
#include "Renderer/VK/VKQueueManager.hpp"
#include "Renderer/VK/VKUtil.hpp"
#include "Renderer/VK/VKContext.hpp"
#include "Renderer/VK/VKDevice.hpp"
#include "Renderer/VK/VKQueue.hpp"

VKQueueManager::VKQueueManager() 
	: bIsCleanedUp(false) {}

VKQueueManager::~VKQueueManager() {
	if (!bIsCleanedUp) 
		CleanUp();
}

bool VKQueueManager::Initialize(VKContext* const pContext, VKDevice* const pDevice, unsigned swapChainImageCount) {
	mpContext = pContext;
	mpDevice = pDevice;

	CheckReturn(CreateQueues());

	return true;
}

void VKQueueManager::CleanUp() { 
	mpContext = nullptr;
	mpDevice = nullptr;

	mQueues.clear();

	bIsCleanedUp = true; 
}

bool VKQueueManager::CreateQueues() {
	const auto indices = VKUtil::QueueFamilyIndices::FindQueueFamilies(
		mpDevice->GetPhysicalDevice(),
		mpContext->GetSurface());

	if (!indices.IsComplete())
		ReturnFalse("Queue family selection is incomplete");

	const auto properties = VKUtil::QueueFamilyIndices::GetQueueFamilyProperties(mpDevice->GetPhysicalDevice());

	constexpr std::uint32_t QueueIndex = 0;

	auto const graphics = GetOrCreateQueue(*indices.GraphicsFamily, QueueIndex, properties[*indices.GraphicsFamily].queueFlags);
	auto const compute = GetOrCreateQueue(*indices.ComputeFamily, QueueIndex, properties[*indices.ComputeFamily].queueFlags);
	auto const transfer = GetOrCreateQueue(*indices.TransferFamily, QueueIndex, properties[*indices.TransferFamily].queueFlags);
	auto const present = GetOrCreateQueue(*indices.PresentFamily, QueueIndex, properties[*indices.PresentFamily].queueFlags);

	mRoleQueues[static_cast<size_t>(QueueRole::E_Graphics)] = graphics;
	mRoleQueues[static_cast<size_t>(QueueRole::E_Compute)] = compute;
	mRoleQueues[static_cast<size_t>(QueueRole::E_Transfer)] = transfer;
	mRoleQueues[static_cast<size_t>(QueueRole::E_Present)] = present;

	return true;
}

VKQueue* VKQueueManager::GetOrCreateQueue(std::uint32_t familyIndex, std::uint32_t queueIndex, VkQueueFlags flags) {
	for (const auto& queue : mQueues) {
		if (queue->GetFamilyIndex() == familyIndex && queue->GetQueueIndex() == queueIndex) 
			return queue.get();
	}

	VkQueue handle = VK_NULL_HANDLE;

	vkGetDeviceQueue(
		mpDevice->GetDevice(),
		familyIndex,
		queueIndex,
		&handle);

	if (!VKIsValid(handle))
		throw std::runtime_error("Failed to get Vulkan queue");

	auto queue = std::make_unique<VKQueue>(handle, familyIndex, queueIndex, flags);

	VKQueue* queuePtr = queue.get();
	mQueues.push_back(std::move(queue));

	return queuePtr;
}