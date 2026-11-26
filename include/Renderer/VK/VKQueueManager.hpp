#pragma once

class VKContext;
class VKDevice;
class VKQueue;

class VKQueueManager {
public:
	enum QueueRole {
		E_Graphics = 0,
		E_Compute,
		E_Transfer,
		E_Present,
		Count
	};

public:
	VKQueueManager();
	virtual ~VKQueueManager();

	DISALLOW_COPY_AND_MOVE(VKQueueManager)

public:
	__forceinline VKQueue* GetQueue(QueueRole role);
	__forceinline const VKQueue* GetQueue(QueueRole role) const;

public:
	bool Initialize(VKContext* const pContext, VKDevice* const pDevice, unsigned swapChainImageCount);
	void CleanUp();

private:
	bool CreateQueues();
	VKQueue* GetOrCreateQueue(std::uint32_t familyIndex, std::uint32_t queueIndex, VkQueueFlags flags);

private:
	bool bIsCleanedUp;

	VKContext* mpContext;
	VKDevice* mpDevice;

	std::vector<std::unique_ptr<VKQueue>> mQueues;
	std::array<VKQueue*, static_cast<size_t>(QueueRole::Count)> mRoleQueues;
};

VKQueue* VKQueueManager::GetQueue(QueueRole role) { return mRoleQueues[role]; }

const VKQueue* VKQueueManager::GetQueue(QueueRole role) const { return mRoleQueues[role]; }