#pragma once

struct LogFile;

class D3D12CommandObject;
class D3D12DescriptorHeap;
class D3D12SwapChain;

class D3D12Device {
	friend class D3D12SwapChain;
	friend class D3D12DescriptorHeap;
	friend class D3D12CommandObject;

	using Adapters = std::vector<std::pair<UINT, Microsoft::WRL::ComPtr<IDXGIAdapter1>>>;

public:
	D3D12Device();
	virtual ~D3D12Device();

public:
	bool Initialize(LogFile* const pLogFile);

public:
	bool GetAdapters(std::vector<std::wstring>& adapters);

	UINT DescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType) const;

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

	bool mbAllowTearing;

	Adapters mAdapters;

	// D3D12 Device
	Microsoft::WRL::ComPtr<ID3D12Device5> md3dDevice;
};