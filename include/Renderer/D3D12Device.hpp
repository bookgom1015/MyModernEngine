#pragma once

struct LogFile;

class D3D12Factory;

class D3D12Device {
	friend class D3D12Factory;

public:
	D3D12Device();
	virtual ~D3D12Device();

public:
	bool Initialize(LogFile* const pLogFile);

public:
	bool QueryInterface(Microsoft::WRL::ComPtr<ID3D12InfoQueue1>& infoQueue);

	bool CreateCommandQueue(Microsoft::WRL::ComPtr<ID3D12CommandQueue>& cmdQueue);
	bool CreateCommandAllocator(Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& cmdAllocator);
	bool CreateCommandList(
		ID3D12CommandAllocator* const pCmdAllocator,
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6>& cmdList);
	bool CreateFence(Microsoft::WRL::ComPtr<ID3D12Fence>& fence);

	bool CreateRtvDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descHeap, UINT numDescs);
	bool CreateDsvDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descHeap, UINT numDescs);
	bool CreateCbvUavSrvDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descHeap, UINT numDescs);
	UINT DescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType) const;

	bool CheckMeshShaderSupported(bool& bMeshShaderSupported) const;

private:
	LogFile* mpLogFile;

	Microsoft::WRL::ComPtr<ID3D12Device5> md3dDevice;
};