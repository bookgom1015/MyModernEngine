#include "Renderer/VK/pch.h"
#include "Renderer/VK/VKQueue.hpp"

VKQueue::VKQueue(VkQueue queue, std::uint32_t familyIndex, std::uint32_t queueIndex, VkQueueFlags flags) 
	: mQueue(queue)
	, mFamilyIndex(familyIndex)
	, mQueueIndex(queueIndex)
	, mFlags(flags) {}

VKQueue::~VKQueue() {}

VkResult VKQueue::Submit(std::uint32_t submitCount, const VkSubmitInfo* const pSubmitInfos, VkFence fence) {
	const std::scoped_lock lock(mQueueMutex);

	return vkQueueSubmit(mQueue, submitCount, pSubmitInfos, fence);
}

VkResult VKQueue::Submit2(std::uint32_t submitCount, const VkSubmitInfo2* const pSubmitInfos, VkFence fence) {
	const std::scoped_lock lock(mQueueMutex);

	return vkQueueSubmit2(mQueue, submitCount, pSubmitInfos, fence);
}

VkResult VKQueue::Present(const VkPresentInfoKHR& presentInfo) {
	const std::scoped_lock lock(mQueueMutex);

	return vkQueuePresentKHR(mQueue, &presentInfo);
}

VkResult VKQueue::WaitIdle() {
	const std::scoped_lock lock(mQueueMutex);

	return vkQueueWaitIdle(mQueue);
}