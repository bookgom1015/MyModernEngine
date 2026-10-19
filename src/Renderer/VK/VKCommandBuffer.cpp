#include "pch.h"
#include "Renderer/VK/VKCommandBuffer.hpp"
#include "Renderer/VK/VKUtil.hpp"
#include "Renderer/VK/VKDevice.hpp"

VKCommandBuffer::VKCommandBuffer() 
	: mCommandPool(VK_NULL_HANDLE)
    , mCommandBuffer(VK_NULL_HANDLE)
    , mLevel(VK_COMMAND_BUFFER_LEVEL_PRIMARY)
	, mFence(VK_NULL_HANDLE)
    , mbIsRecording(false) {}

VKCommandBuffer::~VKCommandBuffer() {}

bool VKCommandBuffer::Allocate(VKDevice* const pDevice
    , VkCommandPool commandPool
    , VkCommandBufferLevel level) {
    assert(pDevice != nullptr);
    assert(VKIsInvalid(mCommandBuffer));

    mpDevice = pDevice;
    mCommandPool = commandPool;
    mLevel = level;

    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.pNext = nullptr;
    allocateInfo.commandPool = mCommandPool;
    allocateInfo.level = mLevel;
    allocateInfo.commandBufferCount = 1;

    if (VKFailed(vkAllocateCommandBuffers(mpDevice->GetDevice(), &allocateInfo, &mCommandBuffer)))
        ReturnFalse("Failed to allocate command buffer");

	VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = nullptr;
    fenceInfo.flags = 0;

    if (VKFailed(vkCreateFence(mpDevice->GetDevice(), &fenceInfo, nullptr, &mFence)))
        ReturnFalse("Failed to create fence for command buffer");

    return true;
}

void VKCommandBuffer::Free() {
	assert(mpDevice != nullptr);
    assert(VKIsValid(mCommandPool));
    assert(!mbIsRecording);

    if (VKIsValid(mFence)) {
        vkDestroyFence(mpDevice->GetDevice(), mFence, nullptr);
        mFence = VK_NULL_HANDLE;
    }

    if (VKIsValid(mCommandBuffer)) {
        vkFreeCommandBuffers(
            mpDevice->GetDevice(),
            mCommandPool,
            1,
            &mCommandBuffer);
        mCommandBuffer = VK_NULL_HANDLE;
    }

    mpDevice = nullptr;
    mbIsRecording = false;
    mCommandPool = VK_NULL_HANDLE;
	mLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
}

bool VKCommandBuffer::Begin(VkCommandBufferUsageFlags usageFlags, const VkCommandBufferInheritanceInfo* const inheritanceInfo) {
	assert(VKIsValid(mCommandBuffer));
    assert(!mbIsRecording);

    // Secondary 커맨드 버퍼에서만 inheritanceInfo가 필요하다.
    if (mLevel == VK_COMMAND_BUFFER_LEVEL_PRIMARY) 
        assert(inheritanceInfo == nullptr);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = nullptr;
    beginInfo.pInheritanceInfo = inheritanceInfo;
    beginInfo.flags = usageFlags;

    if (VKFailed(vkBeginCommandBuffer(mCommandBuffer, &beginInfo)))
        ReturnFalse("Failed to begin command buffer");

    mbIsRecording = true;

    return true;
}

bool VKCommandBuffer::End() {
    assert(VKIsValid(mCommandBuffer));
    assert(mbIsRecording);

    if (VKFailed(vkEndCommandBuffer(mCommandBuffer)))
        ReturnFalse("Failed to end command buffer");

    mbIsRecording = false;

    return true;
}

bool VKCommandBuffer::Reset(VkCommandBufferResetFlags flags) {
    assert(VKIsValid(mCommandBuffer));
    assert(!mbIsRecording);

    if (VKFailed(vkResetCommandBuffer(mCommandBuffer, flags)))
        ReturnFalse("Failed to reset command buffer")

    return true;
}