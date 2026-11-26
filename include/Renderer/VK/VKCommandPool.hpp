#pragma once

class VKDevice;

class VKCommandPool {
public:
	VKCommandPool();
	virtual ~VKCommandPool();

	DISALLOW_COPY_AND_MOVE(VKCommandPool)

public:
	__forceinline VkCommandPool GetHandle() const noexcept;

	__forceinline bool IsValid() const noexcept;

public:
	bool Initialize(VKDevice* const pDevice, std::uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags);
	void CleanUp();

public:
	bool Reset(VkCommandPoolResetFlags flags = 0);

private:
	bool CreateCommandPool(std::uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags);

private:
	bool bIsCleanedUp;

	VKDevice* mpDevice;

	VkCommandPool mCommandPool;
};

VkCommandPool VKCommandPool::GetHandle() const noexcept { return mCommandPool; }

bool VKCommandPool::IsValid() const noexcept { return mCommandPool != VK_NULL_HANDLE; }