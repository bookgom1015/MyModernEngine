#include "pch.h"
#include "Renderer/VK/VKRenderer.hpp"
#include "Renderer/VK/VKUtil.hpp"
#include "Renderer/VK/VKDevice.hpp"
#include "Renderer/VK/VKSwapChain.hpp"
#include "Renderer/VK/VKQueueManager.hpp"
#include "Renderer/VK/VKQueue.hpp"

VKRenderer::VKRenderer() 
	: mCurrentFrameIndex(0)
	, mCurrentSwapChainImageIndex(0)
	, mbFramebufferResized(false)
	, mbShouldRecreateSwapChain(false) {}

VKRenderer::~VKRenderer() {
	if (!mbCleanedUp)
		CleanUp();
}

bool VKRenderer::Initialize(
	HWND hMainWnd
	, unsigned width
	, unsigned height) {
	CheckReturn(VKLowRenderer::Initialize(hMainWnd, width, height));

	CheckReturn(CreateFrameContexts());

	return true;
}

void VKRenderer::CleanUp() {
	DestroyFrameContexts();

	VKLowRenderer::CleanUp();

	mbCleanedUp = true;
}

bool VKRenderer::Update(float deltaTime) {
	if (mbNeedToResize) {
		CheckReturn(Resize(mWidth, mHeight));
		mbNeedToResize = false;
	}

	return true;
}

bool VKRenderer::Draw() {


	return true;
}

bool VKRenderer::Resize(unsigned width, unsigned height) {
	CheckReturn(VKLowRenderer::Resize(width, height));

	return true;
}

bool VKRenderer::CreateFrameContexts() {
	assert(mDevice != nullptr);
	assert(mQueueManager != nullptr);

	const auto GraphicsFamilyIndex = mQueueManager->GetQueue(VKQueueManager::QueueRole::E_Graphics)->GetFamilyIndex();
	const auto ComputeFamilyIndex = mQueueManager->GetQueue(VKQueueManager::QueueRole::E_Compute)->GetFamilyIndex();

	for (auto& frameContext : mFrameContexts) {
		frameContext.GraphicsCommandContext.Initialize(
			mDevice.get(), 
			GraphicsFamilyIndex, 
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		frameContext.ComputeCommandContext.Initialize(
			mDevice.get(),
			ComputeFamilyIndex,
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		CreateFrameSynchronizationObjects(frameContext);
	}

	return true;
}

void VKRenderer::DestroyFrameContexts() {
	if (!mDevice) return;

	vkDeviceWaitIdle(mDevice->GetDevice());

	for (auto& frameContext : mFrameContexts) {
		DestroyFrameSynchronizationObjects(frameContext);

		frameContext.GraphicsCommandContext.CleanUp();
		frameContext.ComputeCommandContext.CleanUp();
	}
}

bool VKRenderer::CreateFrameSynchronizationObjects(VKFrameContext& frameContext) {
	assert(mDevice != nullptr);
	const auto Device = mDevice->GetDevice();
	assert(VKIsValid(Device));
	assert(VKIsInvalid(frameContext.ImageAvailableSemaphore));
	assert(VKIsInvalid(frameContext.RenderFinishedSemaphore));
	assert(VKIsInvalid(frameContext.FrameFence));

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.pNext = nullptr;
	semaphoreInfo.flags = 0;

	if (VKFailed(vkCreateSemaphore(Device, &semaphoreInfo, nullptr, &frameContext.ImageAvailableSemaphore)))
		ReturnFalse("Failed to create image available semaphore");

	if (VKFailed(vkCreateSemaphore(Device, &semaphoreInfo, nullptr, &frameContext.RenderFinishedSemaphore)))
		ReturnFalse("Failed to create render finished semaphore");

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.pNext = nullptr;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (VKFailed(vkCreateFence(Device, &fenceInfo, nullptr, &frameContext.FrameFence)))
		ReturnFalse("Failed to create fence");

	return true;
}

void VKRenderer::DestroyFrameSynchronizationObjects(VKFrameContext& frameContext) {
	if (!mDevice) return;

	const auto Device = mDevice->GetDevice();

	if (VKIsValid(frameContext.ImageAvailableSemaphore)) {
		vkDestroySemaphore(Device, frameContext.ImageAvailableSemaphore, nullptr);
		frameContext.ImageAvailableSemaphore = VK_NULL_HANDLE;
	}

	if (VKIsValid(frameContext.RenderFinishedSemaphore)) {
		vkDestroySemaphore(Device, frameContext.RenderFinishedSemaphore, nullptr);
		frameContext.RenderFinishedSemaphore = VK_NULL_HANDLE;
	}

	if (VKIsValid(frameContext.FrameFence)) {
		vkDestroyFence(Device, frameContext.FrameFence, nullptr);
		frameContext.FrameFence = VK_NULL_HANDLE;
	}
}

bool VKRenderer::BeginFrame() {
	assert(mDevice != nullptr);
	const auto Device = mDevice->GetDevice();
	assert(VKIsValid(Device));

	auto& frameContext = mFrameContexts[mCurrentFrameIndex];
	assert(frameContext.IsValid());

	// 이 FrameContext를 사용했던 이전 GPU 작업 대기
	if (VKFailed(vkWaitForFences(Device, 1, &frameContext.FrameFence, VK_TRUE, UINT64_MAX))) 
		ReturnFalse("Failed to wait for fence");

	const auto acquireResult = vkAcquireNextImageKHR(
			Device,
			mSwapChain->GetHandle(),
			UINT64_MAX,
			frameContext.ImageAvailableSemaphore,
			VK_NULL_HANDLE,
			&mCurrentSwapChainImageIndex);

	if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
		//RecreateSwapChain();
		return true;
	}

	if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) 
		ReturnFalse("Failed to acquire swap chain image.");
	
	// 실제 Submit이 실행될 프레임에서만 Fence를 Reset한다.
	if (VKFailed(vkResetFences(Device, 1, &frameContext.FrameFence)))
		ReturnFalse("Failed to reset fence");

	// Fence가 완료되었으므로 이전 커맨드 버퍼를 안전하게 초기화할 수 있다.
	CheckReturn(frameContext.GraphicsCommandContext.Reset(VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT));

	CheckReturn(frameContext.GraphicsCommandContext.Begin());
	
	return true;
}

bool VKRenderer::EndFrame() {
	auto& frameContext = mFrameContexts[mCurrentFrameIndex];
	assert(frameContext.GraphicsCommandContext.IsValid());

	CheckReturn(frameContext.GraphicsCommandContext.End());

	CheckReturn(SubmitFrame(frameContext));
	CheckReturn(PresentFrame(frameContext));

	mCurrentFrameIndex = (mCurrentFrameIndex + 1) % VKFrameContext::MaxFramesInFlight;

	return true;
}

bool VKRenderer::SubmitFrame(VKFrameContext& frameContext) {
	assert(frameContext.IsValid());
	assert(!frameContext.GraphicsCommandContext.IsRecording(0));

	const auto CommandBuffer = frameContext.GraphicsCommandContext.GetCommandBuffer();
	assert(VKIsValid(CommandBuffer));

	// 스왑체인 이미지가 준비될 때까지 대기한다.
	// 스왑체인 이미지를 처음 사용하는 지점이 Color Attachment Output 단계이므로 이 단계에서 Semaphore를 기다린다.
	VkSemaphoreSubmitInfo waitSemaphoreInfo{};
	waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	waitSemaphoreInfo.pNext = nullptr;
	waitSemaphoreInfo.semaphore = frameContext.ImageAvailableSemaphore;
	// Binary Semaphore에서는 value를 사용하지 않는다.
	waitSemaphoreInfo.value = 0;
	waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	waitSemaphoreInfo.deviceIndex = 0;

	// 제출할 커맨드 버퍼 정보
	VkCommandBufferSubmitInfo commandBufferSubmitInfo{};
	commandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	commandBufferSubmitInfo.pNext = nullptr;
	commandBufferSubmitInfo.commandBuffer = CommandBuffer;
	commandBufferSubmitInfo.deviceMask = 0;

	// 렌더링 완료 후 Present가 기다릴 Semaphore
	// 현재 제출에 포함된 모든 명령이 완료된 후, Signal되도록 ALL_COMMANDS를 사용한다.
	VkSemaphoreSubmitInfo signalSemaphoreInfo{};
	signalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	signalSemaphoreInfo.pNext = nullptr;
	signalSemaphoreInfo.semaphore = frameContext.RenderFinishedSemaphore;
	signalSemaphoreInfo.value = 0;
	signalSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	signalSemaphoreInfo.deviceIndex = 0;

	VkSubmitInfo2 submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreInfoCount = 1;
	submitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;
	submitInfo.commandBufferInfoCount = 1;
	submitInfo.pCommandBufferInfos = &commandBufferSubmitInfo;
	submitInfo.signalSemaphoreInfoCount = 1;
	submitInfo.pSignalSemaphoreInfos = &signalSemaphoreInfo;

	if (VKFailed(vkQueueSubmit2(
		mQueueManager->GetQueue(VKQueueManager::QueueRole::E_Graphics)->GetHandle()
		, 1
		, &submitInfo
		, frameContext.FrameFence)))
		ReturnFalse("Failed to submit command buffer");

	return true;
}

bool VKRenderer::PresentFrame(VKFrameContext& frameContext) {
	assert(VKIsValid(frameContext.RenderFinishedSemaphore));

	const auto SwapChain = mSwapChain->GetHandle();
	assert(VKIsValid(SwapChain));

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	// Graphics Queue에서 렌더링이 끝날 때 Signal되는 Semaphore를 Present Queue가 기다린다.
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &frameContext.RenderFinishedSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &SwapChain;
	presentInfo.pImageIndices = &mCurrentSwapChainImageIndex;
	// 스왑체인이 하나뿐이면 pResults 없이 vkQueuePresentKHR의 반환값만 확인하면 된다.
	presentInfo.pResults = nullptr;

	const auto PresentResult = vkQueuePresentKHR(
		mQueueManager->GetQueue(VKQueueManager::QueueRole::E_Present)->GetHandle(),
		&presentInfo);

	mbNeedToResize =
		PresentResult == VK_ERROR_OUT_OF_DATE_KHR 
		|| PresentResult == VK_SUBOPTIMAL_KHR 
		|| mbFramebufferResized;

	if (PresentResult != VK_SUCCESS
		&& PresentResult != VK_SUBOPTIMAL_KHR
		&& PresentResult != VK_ERROR_OUT_OF_DATE_KHR)
		ReturnFalse("Failed to present swap chain image.");

	return true;
}