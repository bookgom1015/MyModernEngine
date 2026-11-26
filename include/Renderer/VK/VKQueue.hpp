#pragma once

class VKQueue {
public:
	VKQueue(VkQueue queue, std::uint32_t familyIndex, std::uint32_t queueIndex, VkQueueFlags flags);
	virtual ~VKQueue();

	DISALLOW_COPY_AND_MOVE(VKQueue)

public:
	__forceinline VkQueue GetHandle() const noexcept;

	__forceinline std::uint32_t GetFamilyIndex() const noexcept;
	__forceinline std::uint32_t GetQueueIndex() const noexcept;

	__forceinline VkQueueFlags GetFlags() const noexcept;

	__forceinline bool Supports(VkQueueFlags flags) const noexcept;

public:
	VkResult Submit(std::uint32_t submitCount, const VkSubmitInfo* const pSubmitInfos, VkFence fence);
	VkResult Submit2(std::uint32_t submitCount, const VkSubmitInfo2* const pSubmitInfos, VkFence fence);

	VkResult Present(const VkPresentInfoKHR& presentInfo);

	VkResult WaitIdle();

private:
	VkQueue mQueue;

	std::uint32_t mFamilyIndex;
	std::uint32_t mQueueIndex;

	VkQueueFlags mFlags;

	std::mutex mQueueMutex;
};

VkQueue VKQueue::GetHandle() const noexcept { return mQueue; }

std::uint32_t VKQueue::GetFamilyIndex() const noexcept { return mFamilyIndex; }

std::uint32_t VKQueue::GetQueueIndex() const noexcept { return mQueueIndex; }

VkQueueFlags VKQueue::GetFlags() const noexcept { return mFlags; }

bool VKQueue::Supports(VkQueueFlags flags) const noexcept { return (mFlags & flags) == flags; }