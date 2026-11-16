#include "Renderer\D3D12\pch.h"
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

		Logln("{}", buf);
	}
}

D3D12CommandObject::D3D12CommandObject()
	: mpDevice{}
	, mCurrentFrameFence{}
	, mCurrentUploadFence{}
	, mCurrentImmediateFence{}
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
	// 세 종류의 커맨드 리스트가 같은 큐에 제출되므로 한 번만 기다립니다.
	return FlushDirectCommand();
}

bool D3D12CommandObject::FlushDirectCommand() {
	UINT64 fence{};
	CheckReturn(SignalFrame(fence));

	return WaitFrameCompletion(fence);
}

bool D3D12CommandObject::FlushUploadCommand() {
	UINT64 fence{};
	CheckReturn(SignalUpload(fence));

	return WaitUploadCompletion(fence);
}

bool D3D12CommandObject::FlushImmediateCommand() {
	UINT64 fence{};
	CheckReturn(SignalImmediate(fence));

	return WaitImmediateCompletion(fence);
}

bool D3D12CommandObject::ResetDirectCommandListAllocator() {
	CheckHResult(mDirectCmdListAlloc->Reset());

	return true;
}

bool D3D12CommandObject::ExecuteDirectCommandList() {
    const auto cmdList = mDirectCommandList.Get();

	// 다이렉트 커맨드 리스트를 닫고 실행합니다.
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

namespace {
	bool WaitForFence(ID3D12Fence* fence, UINT64 value) {
		const UINT64 completed = fence->GetCompletedValue();

		if (completed == UINT64_MAX)
			ReturnFalse("The D3D12 device was removed while checking a fence");

		if (completed < value) {
			// 이벤트 핸들이 null이면 지정한 펜스 값에 도달할 때까지 호출이 대기합니다.
			CheckHResult(fence->SetEventOnCompletion(value, nullptr));

			if (fence->GetCompletedValue() == UINT64_MAX)
				ReturnFalse("The D3D12 device was removed while waiting for a fence");
		}

		return true;
	}
}

bool D3D12CommandObject::WaitFrameCompletion(UINT64 fence) {
	return WaitForFence(mFrameFence.Get(), fence);
}

bool D3D12CommandObject::WaitUploadCompletion(UINT64 fence) {
	return WaitForFence(mUploadFence.Get(), fence);
}

bool D3D12CommandObject::WaitImmediateCompletion(UINT64 fence) {
	return WaitForFence(mImmediateFence.Get(), fence);
}

UINT64 D3D12CommandObject::GetCompletedFrameFenceValue() const {
	return mFrameFence ? mFrameFence->GetCompletedValue() : 0;
}

bool D3D12CommandObject::SignalFrame(UINT64& fence) {
	const UINT64 fenceValue = mCurrentFrameFence + 1;
	CheckHResult(mCommandQueue->Signal(mFrameFence.Get(), fenceValue));

	mCurrentFrameFence = fenceValue;
	fence = fenceValue;

	return true;
}

UINT64 D3D12CommandObject::GetCompletedUploadFenceValue() const {
	return mUploadFence ? mUploadFence->GetCompletedValue() : 0;
}

bool D3D12CommandObject::SignalUpload(UINT64& fence) {
	const UINT64 fenceValue = mCurrentUploadFence + 1;
	CheckHResult(mCommandQueue->Signal(mUploadFence.Get(), fenceValue));

	mCurrentUploadFence = fenceValue;
	fence = fenceValue;

	return true;
}

UINT64 D3D12CommandObject::GetCompletedImmediateFenceValue() const {
	return mImmediateFence ? mImmediateFence->GetCompletedValue() : 0;
}

bool D3D12CommandObject::SignalImmediate(UINT64& fence) {
	const UINT64 fenceValue = mCurrentImmediateFence + 1;
	CheckHResult(mCommandQueue->Signal(mImmediateFence.Get(), fenceValue));

	mCurrentImmediateFence = fenceValue;
	fence = fenceValue;

	return true;
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
		mDirectCmdListAlloc.Get(),				// 연결된 커맨드 할당자입니다.
		nullptr,								// 초기 파이프라인 상태 객체입니다.
		IID_PPV_ARGS(&mDirectCommandList)));

	// 커맨드 리스트는 닫힌 상태로 시작합니다.
	// 처음 사용할 때 Reset을 호출하며,
	// Reset을 호출하려면 커맨드 리스트가 닫혀 있어야 합니다.
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
