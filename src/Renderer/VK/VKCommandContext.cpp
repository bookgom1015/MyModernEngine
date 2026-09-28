#include "pch.h"
#include "Renderer/VK/VKCommandContext.hpp"
#include "Renderer/VK/VKUtil.hpp"
#include "Renderer/VK/VKDevice.hpp"
#include "Renderer/VK/VKCommandPool.hpp"

VKCommandContext::VKCommandContext() 
	: bIsCleanedUp(false) {}

VKCommandContext::~VKCommandContext() {
	if (!bIsCleanedUp)
		CleanUp();
}

bool VKCommandContext::Initialize(VKDevice* const pDevice, std::uint32_t queueFamilyIndex, VkCommandPoolCreateFlags poolFlags) {
	assert(pDevice != nullptr);
	assert(!IsValid());

	CheckReturn(mCommandPool.Initialize(pDevice, queueFamilyIndex, poolFlags));
	CheckReturn(mCommandBuffer.Allocate(pDevice, mCommandPool.GetHandle(), VK_COMMAND_BUFFER_LEVEL_PRIMARY));

	return true;
}

void VKCommandContext::CleanUp() {
	// 커맨드 버퍼가 풀을 참조하므로 반드시 버퍼부터 해제한다.
	mCommandBuffer.Free();
	mCommandPool.CleanUp();

	bIsCleanedUp = true;
}

bool VKCommandContext::Reset(VkCommandPoolResetFlags flags) {
	assert(IsValid());
	assert(!mCommandBuffer.IsRecording());

	// 풀을 리셋하면 이 풀에서 할당된 모든 커맨드 버퍼가 Initial 상태가 된다.
	CheckReturn(mCommandPool.Reset(flags));

	return true;
}

bool VKCommandContext::Begin(VkCommandBufferUsageFlags usageFlags) {
	assert(IsValid());

	CheckReturn(mCommandBuffer.Begin(usageFlags));

	return true;
}

bool VKCommandContext::End() {
	assert(IsValid());

	CheckReturn(mCommandBuffer.End());

	return true;
}