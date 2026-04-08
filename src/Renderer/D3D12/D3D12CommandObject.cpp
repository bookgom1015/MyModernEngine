#include "pch.h"
#include "Renderer/D3D12/D3D12CommandObject.hpp"

#include "Renderer/D3D12/D3D12Device.hpp"

namespace {
	void D3D12MessageCallback(
		D3D12_MESSAGE_CATEGORY category
		, D3D12_MESSAGE_SEVERITY severity
		, D3D12_MESSAGE_ID id
		, LPCSTR pDescription
		, void* pContext) {
		if (!pDescription) return;

		const char* sevStr{};
		switch (severity) {
		case D3D12_MESSAGE_SEVERITY_CORRUPTION:
			sevStr = "Corruption";
			break;
		case D3D12_MESSAGE_SEVERITY_ERROR:
			sevStr = "Error";
			break;
		case D3D12_MESSAGE_SEVERITY_WARNING:
			sevStr = "Warning";
			break;
		case D3D12_MESSAGE_SEVERITY_INFO:
			return;
			sevStr = "Info";
			break;
		case D3D12_MESSAGE_SEVERITY_MESSAGE:
			sevStr = "Message";
			break;
		}

		char buf[2048];
		_snprintf_s(buf, _TRUNCATE, "[%s] %s", sevStr, pDescription);

		Logln(buf);
	}
}

D3D12CommandObject::D3D12CommandObject()
	: mpDevice{}
	, mCurrentFrameFence{}
	, mCurrentUploadFence{}
#ifdef _DEBUG
	, mCallbakCookie{ 0x01010101 }
#endif
	{}

D3D12CommandObject::~D3D12CommandObject() {}

bool D3D12CommandObject::Initialize(D3D12Device* const pDevice) {
	mpDevice = pDevice;

#ifdef _DEBUG
	CheckReturn(CreateDebugObjects());
#endif
	CheckReturn(CreateCommandQueue());
	CheckReturn(CreateDirectCommandObject());
	CheckReturn(CreateUploadCommandObject());
	CheckReturn(CreateImmediateCommandObject());
	CheckReturn(CreateFence());

	return true;
}

bool D3D12CommandObject::FlushCommandQueue() {
	CheckReturn(FlushDirectCommand());
	CheckReturn(FlushUploadCommand());
	CheckReturn(FlushImmediateCommand());

	return true;
}

bool D3D12CommandObject::FlushDirectCommand() {
	// Advance the fence value to mark commands up to this fence point.
	++mCurrentFrameFence;

	// Add an instruction to the command queue to set a new fence point.
	// Because we are on the GPU timeline, the new fence point won't be set until the GPU finishes
	// processing all the commands prior to this Signal().
	CheckHResult(mCommandQueue->Signal(mFrameFence.Get(), mCurrentFrameFence));

	// Wait until the GPU has compledted commands up to this fence point.
	if (mFrameFence->GetCompletedValue() < mCurrentFrameFence) {
		HANDLE eventHandle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (eventHandle == NULL) return FALSE;

		// Fire event when GPU hits current fence.
		CheckHResult(mFrameFence->SetEventOnCompletion(mCurrentFrameFence, eventHandle));

		// Wait until the GPU hits current fence.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	return true;
}

bool D3D12CommandObject::FlushUploadCommand() {
	++mCurrentUploadFence;

	CheckHResult(mCommandQueue->Signal(mUploadFence.Get(), mCurrentUploadFence));

	if (mUploadFence->GetCompletedValue() < mCurrentUploadFence) {
		HANDLE eventHandle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (eventHandle == NULL) return FALSE;

		CheckHResult(mUploadFence->SetEventOnCompletion(mCurrentUploadFence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	return true;
}

bool D3D12CommandObject::FlushImmediateCommand() {
	++mCurrentImmediateFence;

	CheckHResult(mCommandQueue->Signal(mImmediateFence.Get(), mCurrentImmediateFence));

	if (mImmediateFence->GetCompletedValue() < mCurrentImmediateFence) {
		HANDLE eventHandle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (eventHandle == NULL) return FALSE;

		CheckHResult(mImmediateFence->SetEventOnCompletion(mCurrentImmediateFence, eventHandle));

		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	return true;
}

bool D3D12CommandObject::ResetDirectCommandListAllocator() {
	CheckHResult(mDirectCmdListAlloc->Reset());

	return true;
}

bool D3D12CommandObject::ExecuteDirectCommandList() {
    const auto cmdList = mDirectCommandList.Get();

	// Close and execute the direct command list
	CheckHResult(cmdList->Close());
	ID3D12CommandList* ppCommandLists[] = { cmdList };
	mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	return true;
}

bool D3D12CommandObject::ResetDirectCommandList(ID3D12PipelineState* const pPipelineState) {
	CheckHResult(mDirectCommandList->Reset(mDirectCmdListAlloc.Get(), pPipelineState));

	return true;
}

bool D3D12CommandObject::ResetDirectCommandList(
	ID3D12CommandAllocator* const pAlloc
	, ID3D12PipelineState* const pPipelineState) {
	CheckHResult(mDirectCommandList->Reset(pAlloc, pPipelineState));

	return true;
}

bool D3D12CommandObject::ResetUploadCommandListAllocator() {
	CheckHResult(mUploadCmdListAlloc->Reset());

	return true;
}

bool D3D12CommandObject::ExecuteUploadCommandList() {
	const auto cmdList = mUploadCommandList.Get();

	CheckHResult(cmdList->Close());
	ID3D12CommandList* ppCommandLists[] = { cmdList };
	mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	return true;
}

bool D3D12CommandObject::ResetUploadCommandList() {
	CheckHResult(mUploadCommandList->Reset(mUploadCmdListAlloc.Get(), nullptr));

	return true;
}

bool D3D12CommandObject::ResetUploadCommandList(ID3D12CommandAllocator* const pAlloc) {
	CheckHResult(mUploadCommandList->Reset(pAlloc, nullptr));

	return true;
}

bool D3D12CommandObject::ResetImmediateCommandListAllocator() {
	CheckHResult(mImmediateCmdListAlloc->Reset());

	return true;
}

bool D3D12CommandObject::ExecuteImmediateCommandList() {
	const auto cmdList = mImmediateCommandList.Get();

	CheckHResult(cmdList->Close());
	ID3D12CommandList* ppCommandLists[] = { cmdList };
	mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	return true;
}

bool D3D12CommandObject::ResetImmediateCommandList(ID3D12PipelineState* const pPipelineState) {
	CheckHResult(mImmediateCommandList->Reset(mImmediateCmdListAlloc.Get(), pPipelineState));

	return true;
}

bool D3D12CommandObject::ResetImmediateCommandList(
	ID3D12CommandAllocator* const pAlloc
	, ID3D12PipelineState* const pPipelineState) {
	CheckHResult(mImmediateCommandList->Reset(pAlloc, pPipelineState));

	return true;
}

bool D3D12CommandObject::WaitFrameCompletion(UINT64 fence) {
	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (fence != 0 && mFrameFence->GetCompletedValue() < fence) {
		const HANDLE eventHandle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (eventHandle == NULL) return FALSE;

		CheckHResult(mFrameFence->SetEventOnCompletion(fence, eventHandle));
		const auto status = WaitForSingleObject(eventHandle, INFINITE);
		if (status == WAIT_FAILED) ReturnFalse("Calling \'WaitForSingleObject\' failed");

		if (!CloseHandle(eventHandle)) ReturnFalse("Failed to close handle");
	}

	return true;
}

bool D3D12CommandObject::WaitUploadCompletion(UINT64 fence) {
	if (fence != 0 && mUploadFence->GetCompletedValue() < fence) {
		const HANDLE eventHandle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (eventHandle == NULL) return FALSE;

		CheckHResult(mUploadFence->SetEventOnCompletion(fence, eventHandle));
		const auto status = WaitForSingleObject(eventHandle, INFINITE);
		if (status == WAIT_FAILED) ReturnFalse("Calling \'WaitForSingleObject\' failed");

		if (!CloseHandle(eventHandle)) ReturnFalse("Failed to close handle");
	}

	return true;
}

bool D3D12CommandObject::WaitImmediateCompletion(UINT64 fence) {
	if (fence != 0 && mImmediateFence->GetCompletedValue() < fence) {
		const HANDLE eventHandle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (eventHandle == NULL) return FALSE;

		CheckHResult(mImmediateFence->SetEventOnCompletion(fence, eventHandle));
		const auto status = WaitForSingleObject(eventHandle, INFINITE);
		if (status == WAIT_FAILED) ReturnFalse("Calling \'WaitForSingleObject\' failed");

		if (!CloseHandle(eventHandle)) ReturnFalse("Failed to close handle");
	}

	return true;
}

UINT64 D3D12CommandObject::GetCompletedFrameFenceValue() const {
	return mFrameFence ? mFrameFence->GetCompletedValue() : 0;
}

UINT64 D3D12CommandObject::SignalFrame() {
	const UINT64 fenceValue = ++mCurrentFrameFence;
	CheckHResult(mCommandQueue->Signal(mFrameFence.Get(), fenceValue));

	return fenceValue;
}

UINT64 D3D12CommandObject::GetCompletedUploadFenceValue() const {
	return mUploadFence ? mUploadFence->GetCompletedValue() : 0;
}

UINT64 D3D12CommandObject::SignalUpload() {
	const UINT64 fenceValue = ++mCurrentUploadFence;
	CheckHResult(mCommandQueue->Signal(mUploadFence.Get(), fenceValue));

	return fenceValue;
}

UINT64 D3D12CommandObject::GetCompletedImmediateFenceValue() const {
	return mImmediateFence ? mImmediateFence->GetCompletedValue() : 0;
}

UINT64 D3D12CommandObject::SignalImmediate() {
	const UINT64 fenceValue = ++mCurrentImmediateFence;
	CheckHResult(mCommandQueue->Signal(mImmediateFence.Get(), fenceValue));

	return fenceValue;
}

#ifdef _DEBUG
bool D3D12CommandObject::CreateDebugObjects() {
	CheckHResult(mpDevice->md3dDevice->QueryInterface(
		IID_PPV_ARGS(&mInfoQueue)));
	CheckHResult(mInfoQueue->RegisterMessageCallback(
		D3D12MessageCallback, D3D12_MESSAGE_CALLBACK_IGNORE_FILTERS, nullptr, &mCallbakCookie));

	return true;
}
#endif

bool D3D12CommandObject::CreateCommandQueue() {
	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	CheckHResult(mpDevice->md3dDevice->CreateCommandQueue(
		&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	return true;
}

bool D3D12CommandObject::CreateDirectCommandObject() {
	CheckHResult(mpDevice->md3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mDirectCmdListAlloc)));
	CheckHResult(mpDevice->md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT, 
		mDirectCmdListAlloc.Get(),				// Associated command allocator
		nullptr,								// Initial PipelineStateObject
		IID_PPV_ARGS(&mDirectCommandList)));

	// Start off in a closed state.  This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	mDirectCommandList->Close();

	return true;
}

bool D3D12CommandObject::CreateUploadCommandObject() {
	CheckHResult(mpDevice->md3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mUploadCmdListAlloc)));
	CheckHResult(mpDevice->md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mUploadCmdListAlloc.Get(),
		nullptr,
		IID_PPV_ARGS(&mUploadCommandList)));

	mUploadCommandList->Close();

	return true;
}

bool D3D12CommandObject::CreateImmediateCommandObject() {
	CheckHResult(mpDevice->md3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mImmediateCmdListAlloc)));
	CheckHResult(mpDevice->md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mImmediateCmdListAlloc.Get(),
		nullptr,
		IID_PPV_ARGS(&mImmediateCommandList)));

	mImmediateCommandList->Close();

	return true;
}

bool D3D12CommandObject::CreateFence() {
	CheckHResult(mpDevice->md3dDevice->CreateFence(
		0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFrameFence)));

	CheckHResult(mpDevice->md3dDevice->CreateFence(
		0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mUploadFence)));

	CheckHResult(mpDevice->md3dDevice->CreateFence(
		0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mImmediateFence)));

	return true;
}
