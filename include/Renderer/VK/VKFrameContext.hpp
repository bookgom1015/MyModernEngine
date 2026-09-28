#pragma once

#include "Renderer/VK/VKCommandContext.hpp"

struct VKFrameContext {
	static constexpr std::uint32_t MaxFramesInFlight = 2;

	VKCommandContext GraphicsCommandContext;

	VkSemaphore ImageAvailableSemaphore = VK_NULL_HANDLE;
	VkSemaphore RenderFinishedSemaphore = VK_NULL_HANDLE;

	VkFence FrameFence = VK_NULL_HANDLE;

	bool IsValid() const noexcept {		
		return GraphicsCommandContext.IsValid()
			&& ImageAvailableSemaphore != VK_NULL_HANDLE 
			&& RenderFinishedSemaphore != VK_NULL_HANDLE 
			&& FrameFence != VK_NULL_HANDLE;
	}

	bool IsRecording() const noexcept { return GraphicsCommandContext.IsRecording(); }
};