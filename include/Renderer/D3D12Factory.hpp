#pragma once

struct LogFile;

class D3D12Device;

class D3D12Factory {
	using Adapters = std::vector<std::pair<UINT, Microsoft::WRL::ComPtr<IDXGIAdapter1>>>;

public:
	D3D12Factory();
	virtual ~D3D12Factory();

public:
	bool Initialize(LogFile* const pLogFile);

public:
	bool SortAdapters();
	bool GetAdapters(std::vector<std::wstring>& adapters);
	bool SelectAdapter(
		D3D12Device* const pDevice, 
		UINT adapterIndex, 
		bool& bRaytracingSupported);

private:
	bool CreateFactory();

private:
	LogFile* mpLogFile;

	Microsoft::WRL::ComPtr<ID3D12Debug> mDebugController{};

	Microsoft::WRL::ComPtr<IDXGIFactory4> mDxgiFactory{};
	UINT mdxgiFactoryFlags{};

	BOOL mbAllowTearing{};

	Adapters mAdapters{};
};