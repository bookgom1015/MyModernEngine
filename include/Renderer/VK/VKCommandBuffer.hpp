#pragma once

class VKDevice;

class VKCommandBuffer {
public:
	VKCommandBuffer();
	virtual ~VKCommandBuffer();

	DISALLOW_COPY_AND_MOVE(VKCommandBuffer);

public:
	__forceinline VkCommandBuffer GetHandle() const noexcept;

	__forceinline bool IsValid() const noexcept;
	__forceinline bool IsRecording() const noexcept;

public:
	bool Allocate(
		VKDevice* const pDevice,
		VkCommandPool commandPool,
		VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	void Free();

	bool Begin(VkCommandBufferUsageFlags usageFlags = 0, const VkCommandBufferInheritanceInfo* const inheritanceInfo = nullptr);
	bool End();

	bool Reset(VkCommandBufferResetFlags flags = 0);

private:
	VKDevice* mpDevice;
	VkCommandPool mCommandPool;

	VkCommandBuffer mCommandBuffer;

	VkCommandBufferLevel mLevel;

	bool mbIsRecording;
};

VkCommandBuffer VKCommandBuffer::GetHandle() const noexcept { return mCommandBuffer; }

bool VKCommandBuffer::IsValid() const noexcept { return mCommandBuffer != VK_NULL_HANDLE; }

bool VKCommandBuffer::IsRecording() const noexcept { return mbIsRecording; }