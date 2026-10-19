#pragma

#include "Renderer/VK/VKCommandPool.hpp"
#include "Renderer/VK/VKCommandBuffer.hpp"

class VKDevice;

class VKCommandContext {
public:
	VKCommandContext();
	virtual ~VKCommandContext();

	DISALLOW_COPY_AND_MOVE(VKCommandContext);

public:
    __forceinline VkCommandPool GetCommandPool() const noexcept;
    __forceinline VkCommandBuffer GetCommandBuffer() const noexcept;

    __forceinline bool IsValid() const noexcept;
    __forceinline bool IsValid(size_t index) const noexcept;
    __forceinline bool IsRecording() const;
    __forceinline bool IsRecording(size_t index) const;

public:
    bool Initialize(VKDevice* const pDevice, std::uint32_t queueFamilyIndex, VkCommandPoolCreateFlags poolFlags);
    void CleanUp();

public:
	bool AcquireCommandBuffer();

    bool Reset(VkCommandPoolResetFlags flags = 0);

    bool Begin(VkCommandBufferUsageFlags usageFlags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    bool End();

private:
	static constexpr unsigned CommandBufferCount = 8;

private:
    bool bIsCleanedUp;
    /*
     * 선언 순서가 중요하다.
     *
     * 멤버는 선언 역순으로 파괴되므로:
     * 1. mCommandBuffer 파괴
     * 2. mCommandPool 파괴
     *
     * 순서로 처리된다.
     */
	VKCommandPool mCommandPool;
	std::vector<std::unique_ptr<VKCommandBuffer>> mCommandBuffers;

	size_t mCurrentCommandBufferIndex;
};

VkCommandPool VKCommandContext::GetCommandPool() const noexcept { return mCommandPool.GetHandle(); }

VkCommandBuffer VKCommandContext::GetCommandBuffer() const noexcept { return mCommandBuffers.front()->GetHandle(); }

bool VKCommandContext::IsValid() const noexcept { 
	if (!mCommandPool.IsValid() || mCommandBuffers.empty())
		return false;

	for (const auto& CommandBuffer : mCommandBuffers) {
		if (!CommandBuffer->IsValid())
			return false;
	}

    return true;
}

bool VKCommandContext::IsValid(size_t index) const noexcept { 
	return mCommandPool.IsValid() && index < mCommandBuffers.size() && mCommandBuffers[index]->IsValid();
}

bool VKCommandContext::IsRecording() const {
    assert(mCurrentCommandBufferIndex < mCommandBuffers.size());
    return mCommandBuffers[mCurrentCommandBufferIndex]->IsRecording();
}

bool VKCommandContext::IsRecording(size_t index) const { 
    assert(index < mCommandBuffers.size());
    return mCommandBuffers[index]->IsRecording(); 
}