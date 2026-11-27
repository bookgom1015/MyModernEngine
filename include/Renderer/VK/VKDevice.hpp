#pragma once

class VKContext;

class VKDevice {
public:
	VKDevice();
	virtual ~VKDevice();

	DISALLOW_COPY_AND_MOVE(VKDevice)

public:
	__forceinline VkPhysicalDevice GetPhysicalDevice() const;
	__forceinline VkDevice GetDevice() const;

	__forceinline bool IsRaytracingSupported() const;

public:
	bool Initialize(VKContext* const pContext);
	void CleanUp();

private:
	bool SelectPhysicalDevices(VKContext* const pContext);

	bool CreateLogicalDevice(VKContext* const pContext);

private:
	bool bIsCleanedUp;

	std::vector<std::pair<int, VkPhysicalDevice>> mCandidates;

	VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
	VkDevice mDevice = VK_NULL_HANDLE;

	bool mbBindlessSupported = false;
	bool mbRaytracingSupported = false;
};

#include "VKDevice.inl"