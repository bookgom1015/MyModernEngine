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

	bool ResetDirectCommandListAllocator();
	bool ExecuteDirectCommandList();
	bool ResetDirectCommandList(
		ID3D12PipelineState* const pPipelineState = nullptr);
	bool ResetDirectCommandList(
		ID3D12CommandAllocator* const pAlloc, ID3D12PipelineState* const pPipelineState = nullptr);

	bool ResetUploadCommandListAllocator();
	bool ExecuteUploadCommandList();
	bool ResetUploadCommandList();
	bool ResetUploadCommandList(
		ID3D12CommandAllocator* const pAlloc);

	bool WaitFrameCompletion(UINT64 fence);
	bool WaitUploadCompletion(UINT64 fence);

	UINT64 GetCompletedFrameFenceValue() const;
	UINT64 SignalFrame();

	UINT64 GetCompletedUploadFenceValue() const;
	UINT64 SignalUpload();

public:
	__forceinline ID3D12CommandQueue* GetCommandQueue() const;
	__forceinline ID3D12GraphicsCommandList6* GetDirectCommandList() const;
	__forceinline ID3D12GraphicsCommandList6* GetUploadCommandList() const;

private:
#ifdef _DEBUG
	bool CreateDebugObjects();
#endif
	bool CreateCommandQueue();
	bool CreateDirectCommandObject();
	bool CreateUploadCommandObject();
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
	Microsoft::WRL::ComPtr<ID3D12Fence> mFrameFence;
	UINT64 mCurrentFrameFence;

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mUploadCmdListAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> mUploadCommandList;
	Microsoft::WRL::ComPtr<ID3D12Fence> mUploadFence;
	UINT64 mCurrentUploadFence;
};

#include "D3D12CommandObject.inl"