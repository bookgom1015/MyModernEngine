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

	VkPhysicalDevice mPhysicalDevice;
	VkDevice mDevice;

	bool bRaytracingSupported;
};

VkPhysicalDevice VKDevice::GetPhysicalDevice() const { return mPhysicalDevice; }

VkDevice VKDevice::GetDevice() const { return mDevice; }

bool VKDevice::IsRaytracingSupported() const { return bRaytracingSupported; }