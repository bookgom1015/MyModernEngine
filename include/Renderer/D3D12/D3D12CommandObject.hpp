#pragma once

struct LogFile;

class D3D12Device;

class D3D12CommandObject {
public:
	D3D12CommandObject();
	virtual ~D3D12CommandObject();

public:
	bool Initialize(D3D12Device* const pDevice);

public:
	bool FlushCommandQueue();

	bool ResetCommandListAllocator();

	bool ExecuteDirectCommandList();
	bool ResetDirectCommandList(ID3D12PipelineState* const pPipelineState = nullptr);
	bool ResetDirectCommandList(
		ID3D12CommandAllocator* const pAlloc, ID3D12PipelineState* const pPipelineState = nullptr);

	bool WaitCompletion(UINT64 fence);
	UINT64 IncreaseFence();

	bool Signal();

public:
	__forceinline ID3D12CommandQueue* GetCommandQueue() const;
	__forceinline ID3D12GraphicsCommandList6* GetDirectCommandList() const;

private:
#ifdef _DEBUG
	bool CreateDebugObjects();
#endif
	bool CreateCommandQueue();
	bool CreateDirectCommandObject();
	bool CreateFence();

private:
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
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> mCommandList;

	Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
	UINT64 mCurrentFence;
};

#include "D3D12CommandObject.inl"