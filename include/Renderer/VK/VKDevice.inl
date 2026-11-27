#ifndef __VK_DEVICE_INL__
#define __VK_DEVICE_INL__

VkPhysicalDevice VKDevice::GetPhysicalDevice() const { return mPhysicalDevice; }

VkDevice VKDevice::GetDevice() const { return mDevice; }

bool VKDevice::IsRaytracingSupported() const { return mbRaytracingSupported; }

#endif // __VK_DEVICE_INL__