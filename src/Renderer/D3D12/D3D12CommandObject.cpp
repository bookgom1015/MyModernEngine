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

		auto* pLogFile = reinterpret_cast<LogFile*>(pContext);
		Logln(pLogFile, buf);
	}
}

D3D12CommandObject::D3D12CommandObject()
	: mpLogFile{}
#ifdef _DEBUG
	, mCallbakCookie{ 0x01010101 }
#endif
	, mpDevice{}
	, mCurrentFence{} {}

D3D12CommandObject::~D3D12CommandObject() {}

bool D3D12CommandObject::Initialize(LogFile* const pLogFile, D3D12Device* const pDevice) {
	mpLogFile = pLogFile;
	mpDevice = pDevice;

#ifdef _DEBUG
	CheckReturn(mpLogFile, CreateDebugObjects());
#endif
	CheckReturn(mpLogFile, CreateCommandQueue());
	CheckReturn(mpLogFile, CreateDirectCommandObject());
	CheckReturn(mpLogFile, CreateFence());

	return true;
}

bool D3D12CommandObject::FlushCommandQueue() {
	// Advance the fence value to mark commands up to this fence point.
	++mCurrentFence;

	// Add an instruction to the command queue to set a new fence point.
	// Because we are on the GPU timeline, the new fence point won't be set until the GPU finishes
	// processing all the commands prior to this Signal().
	CheckHResult(mpLogFile, mCommandQueue->Signal(mFence.Get(), mCurrentFence));

	// Wait until the GPU has compledted commands up to this fence point.
	if (mFence->GetCompletedValue() < mCurrentFence) {
		HANDLE eventHandle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (eventHandle == NULL) return FALSE;

		// Fire event when GPU hits current fence.
		CheckHResult(mpLogFile, mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

		// Wait until the GPU hits current fence.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	return true;
}

bool D3D12CommandObject::ResetCommandListAllocator() {
	CheckHResult(mpLogFile, mDirectCmdListAlloc->Reset());

	return true;
}

bool D3D12CommandObject::ExecuteDirectCommandList() {
    const auto cmdList = mDirectCommandList.Get();

	// Close and execute the direct command list
	CheckHResult(mpLogFile, cmdList->Close());
	ID3D12CommandList* ppCommandLists[] = { cmdList };
	mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	return true;
}

bool D3D12CommandObject::ResetDirectCommandList(ID3D12PipelineState* const pPipelineState) {
	CheckHResult(mpLogFile, mDirectCommandList->Reset(mDirectCmdListAlloc.Get(), pPipelineState));

	return true;
}

bool D3D12CommandObject::ResetDirectCommandList(
	ID3D12CommandAllocator* const pAlloc
	, ID3D12PipelineState* const pPipelineState) {
	CheckHResult(mpLogFile, mDirectCommandList->Reset(pAlloc, pPipelineState));

	return true;
}

bool D3D12CommandObject::WaitCompletion(UINT64 fence) {
	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (fence != 0 && mFence->GetCompletedValue() < fence) {
		const HANDLE eventHandle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (eventHandle == NULL) return FALSE;

		CheckHResult(mpLogFile, mFence->SetEventOnCompletion(fence, eventHandle));

		const auto status = WaitForSingleObject(eventHandle, INFINITE);
		if (status == WAIT_FAILED) ReturnFalse(mpLogFile, "Calling \'WaitForSingleObject\' failed");

		if (!CloseHandle(eventHandle)) ReturnFalse(mpLogFile, "Failed to close handle");
	}

	return true;
}

UINT64 D3D12CommandObject::IncreaseFence() {
	return ++mCurrentFence;
}

bool D3D12CommandObject::Signal() {
	CheckHResult(mpLogFile, mCommandQueue->Signal(mFence.Get(), mCurrentFence));

	return true;
}

#ifdef _DEBUG
bool D3D12CommandObject::CreateDebugObjects() {
	CheckHResult(mpLogFile, mpDevice->md3dDevice->QueryInterface(
		IID_PPV_ARGS(&mInfoQueue)));
	CheckHResult(mpLogFile, mInfoQueue->RegisterMessageCallback(
		D3D12MessageCallback, D3D12_MESSAGE_CALLBACK_IGNORE_FILTERS, mpLogFile, &mCallbakCookie));

	return true;
}
#endif

bool D3D12CommandObject::CreateCommandQueue() {
	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	CheckHResult(mpLogFile, mpDevice->md3dDevice->CreateCommandQueue(
		&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	return true;
}

bool D3D12CommandObject::CreateDirectCommandObject() {
	CheckHResult(mpLogFile, mpDevice->md3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mDirectCmdListAlloc)));
	CheckHResult(mpLogFile, mpDevice->md3dDevice->CreateCommandList(
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

bool D3D12CommandObject::CreateFence() {
	CheckHResult(mpLogFile, mpDevice->md3dDevice->CreateFence(
		0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));

	return true;
}
