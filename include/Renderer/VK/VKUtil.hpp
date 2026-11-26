#pragma once

#ifndef VKSucceeded 
#define VKSucceeded(__Exp) ((__Exp) == VK_SUCCESS)
#endif

#ifndef VKFailed 
#define VKFailed(__Exp) ((__Exp) != VK_SUCCESS)
#endif

#ifndef VKIsValid
#define VKIsValid(__Exp) ((__Exp) != VK_NULL_HANDLE)
#endif

#ifndef VKIsInvalid
#define VKIsInvalid(__Exp) ((__Exp) == VK_NULL_HANDLE)
#endif

class VKUtil {
public:
	struct QueueFamilyIndices {
	public:
		__forceinline bool IsComplete() const;
		__forceinline bool IsUniversal() const;

	public:
		std::vector<std::uint32_t> GetSwapChainQueueFamilyIndices() const;

	public:
		static std::vector<VkQueueFamilyProperties> GetQueueFamilyProperties(const VkPhysicalDevice& physicalDevice);

		static QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface);

		static std::vector<VkDeviceQueueCreateInfo> BuildQueueCreateInfos(
			const QueueFamilyIndices& indices, 
			const float* const queuePriority);

		static bool SupportsGraphics(const VkQueueFamilyProperties& props);
		static bool SupportsCompute(const VkQueueFamilyProperties& props);
		static bool SupportsTransfer(const VkQueueFamilyProperties& props);

		static bool IsDedicatedCompute(const VkQueueFamilyProperties& props);
		static bool IsDedicatedTransfer(const VkQueueFamilyProperties& props);

	public:
		std::optional<std::uint32_t> GraphicsFamily;
		std::optional<std::uint32_t> ComputeFamily;
		std::optional<std::uint32_t> TransferFamily;
		std::optional<std::uint32_t> PresentFamily;
	};

	struct QueueKey {
	public:
		bool operator==(const QueueKey& other) const;

	public:
		std::uint32_t FamilyIndex;
		std::uint32_t QueueIndex;
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR Capabilities{};
		std::vector<VkSurfaceFormatKHR> Formats{};
		std::vector<VkPresentModeKHR> PresentModes{};
	};

public:
	static bool CheckValidationLayersSupport();

	static VkResult CreateDebugUtilsMessengerEXT(
		const VkInstance& instance,
		VkDebugUtilsMessengerCreateInfoEXT* const pCreateInfo,
		VkAllocationCallbacks* const pAllocator,
		VkDebugUtilsMessengerEXT* const pDebugMessenger);
	static void DestroyDebugUtilsMessengerEXT(
		const VkInstance& instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		VkAllocationCallbacks* const pAllocator);
	static void PopulateDebugMessengerCreateInfo(
		VkDebugUtilsMessengerCreateInfoEXT& createInfo, 
		void* const pUserData);
	static bool SetUpDebugMessenger(
		const VkInstance& instance, 
		VkDebugUtilsMessengerEXT& debugMessenger, 
		void* const pUserData);

	static bool CheckDeviceExtensionsSupport(const VkPhysicalDevice& physicalDevice);

	static SwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface);

	static bool IsDeviceSuitable(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface);
	static int RateDeviceSuitability(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface);

	static bool ShowDeviceInfo(const VkPhysicalDevice& physicalDevice);

	static bool IsRaytracingSupported(VkPhysicalDevice physicalDevice);

	static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& inAvailableFormats);
	static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& inAvailablePresentMods);
	static VkExtent2D ChooseSwapExtent(UINT width, UINT height, const VkSurfaceCapabilitiesKHR& inCapabilities);

	static bool CreateImageView(
		const VkDevice& device,
		const VkImage& image,
		const VkFormat& format,
		UINT mipLevles,
		const VkImageAspectFlags& aspectFlags,
		VkImageView& imageView);

public:
	static const std::vector<const CHAR*> ValidationLayers;
	static const std::vector<const CHAR*> DeviceExtensions;
	static const std::vector<const CHAR*> RequiredExtensions;
};

bool VKUtil::QueueFamilyIndices::IsComplete() const {
	return GraphicsFamily.has_value()
		&& ComputeFamily.has_value()
		&& TransferFamily.has_value()
		&& PresentFamily.has_value(); 
}

bool VKUtil::QueueFamilyIndices::IsUniversal() const {
	return IsComplete() 
		&& GraphicsFamily == ComputeFamily
		&& GraphicsFamily == TransferFamily
		&& GraphicsFamily == PresentFamily;
}