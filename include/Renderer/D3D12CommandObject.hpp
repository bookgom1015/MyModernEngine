#pragma once

struct LogFile;

class D3D12Device;

class D3D12CommandObject {
public:
	D3D12CommandObject();
	virtual ~D3D12CommandObject();

public:
	bool Initialize(LogFile* const pLogFile, D3D12Device* const pDevice);

private:
#ifdef _DEBUG
	bool CreateDebugObjects();
#endif
	bool CreateCommandQueue();
	bool CreateDirectCommandObject();
	bool CreateFence();

private:
	LogFile* mpLogFile;

	D3D12Device* mpDevice;

#ifdef _DEBUG
	// Debugging
	Microsoft::WRL::ComPtr<ID3D12InfoQueue1> mInfoQueue;
	DWORD mCallbakCookie;
#endif

	// Command objects
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> mDirectCommandList;

	Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
	UINT64 mCurrentFence;
};