#include "Renderer/pch_d3d12.h"
#include "Renderer/D3D12CommandObject.hpp"

#include "Renderer/D3D12Device.hpp"

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

	return true;
}

bool D3D12CommandObject::CreateFence() {
	CheckHResult(mpLogFile, mpDevice->md3dDevice->CreateFence(
		0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));

	return true;
}
