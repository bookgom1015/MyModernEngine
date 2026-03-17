#pragma once

struct LogFile;

class D3D12CommandObject;
class D3D12DescriptorHeap;
class D3D12SwapChain;

class GpuResource;

class D3D12Util;

class D3D12Device {
	friend class D3D12SwapChain;
	friend class D3D12DescriptorHeap;
	friend class D3D12CommandObject;

	friend class GpuResource;

	friend class D3D12Util;

	using Adapters = std::vector<std::pair<UINT, Microsoft::WRL::ComPtr<IDXGIAdapter1>>>;

public:
	D3D12Device();
	virtual ~D3D12Device();

public:
	bool Initialize(LogFile* const pLogFile);

public:
	void CreateShaderResourceView(
		ID3D12Resource* const pResource,
		const D3D12_SHADER_RESOURCE_VIEW_DESC* const pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor);
	void CreateUnorderedAccessView(
		ID3D12Resource* const pResource,
		ID3D12Resource* const pCounterResource,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC* const pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor);
	void CreateRenderTargetView(
		ID3D12Resource* const pResource,
		const D3D12_RENDER_TARGET_VIEW_DESC* const pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor);
	void CreateDepthStencilView(
		ID3D12Resource* const pResource,
		const D3D12_DEPTH_STENCIL_VIEW_DESC* const pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor);

	bool CreateCommandAllocator(
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& cmdAllocator);
	bool CreateCommandList(
		ID3D12CommandAllocator* const pCmdAllocator,
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6>& commandList);
	bool CreateFence(Microsoft::WRL::ComPtr<ID3D12Fence>& fence);

public:
	bool GetAdapters(std::vector<std::wstring>& adapters);

	UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType) const;

	__forceinline constexpr bool IsAllowingTearing() const noexcept;

	__forceinline ID3D12Device5* GetD3DDevice() const noexcept;

private:
	bool CreateFactory();

	bool SortAdapters();
	bool SelectAdapter(UINT adapterIndex, bool& bRaytracingSupported);

	bool CheckMeshShaderSupported();

private:
	LogFile* mpLogFile;

	bool mbRaytracingSupported;
	bool mbMeshShaderSupported;

	// Dxgi Factory and debug controller
	Microsoft::WRL::ComPtr<ID3D12Debug> mDebugController;

	Microsoft::WRL::ComPtr<IDXGIFactory4> mDxgiFactory;
	UINT mdxgiFactoryFlags;

	BOOL mbAllowTearing;

	Adapters mAdapters;

	// D3D12 Device
	Microsoft::WRL::ComPtr<ID3D12Device5> md3dDevice;
};

#include "D3D12Device.inl"