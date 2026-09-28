#pragma once

class VKContext {
public:
	VKContext();
	virtual ~VKContext();

	DISALLOW_COPY_AND_MOVE(VKContext)

public:
	__forceinline VkInstance GetInstance() const;
	__forceinline VkSurfaceKHR GetSurface() const;

public:
	bool Initialize(HWND hWnd);
	void CleanUp();

private:
	bool CreateInstance();
	bool CreateSurface(HWND hWnd);

private:
	bool bIsCleanedUp;

	VkInstance mInstance;
	VkDebugUtilsMessengerEXT mDebugMessenger;

	VkSurfaceKHR mSurface;
};

VkInstance VKContext::GetInstance() const { return mInstance; }

VkSurfaceKHR VKContext::GetSurface() const { return mSurface; }