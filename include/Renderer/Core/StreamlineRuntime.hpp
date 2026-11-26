#pragma once

namespace sl { struct Constants; }
struct ID3D12Device;
struct ID3D12GraphicsCommandList;
struct ID3D12Resource;
struct IDXGISwapChain1;

class StreamlineRuntime {
public:
	struct Resource {
		ID3D12Resource* Texture;
		unsigned State;
	};

public:
	StreamlineRuntime();
	virtual ~StreamlineRuntime();

	DISALLOW_COPY_AND_MOVE(StreamlineRuntime)

public:
	bool Initialize();
	void Shutdown();

public:
	bool CheckFeatureSupport(LUID adapterLuid) const;

	bool RegisterDevice(ID3D12Device* device) const;

	bool UpgradeSwapChain(IDXGISwapChain1*& swapChain) const;

	bool ConfigureDlss(unsigned width, unsigned height, unsigned& renderWidth, unsigned& renderHeight) const;

	void ReleaseDlssResources() const;

	bool EvaluateDlss(
		ID3D12GraphicsCommandList* commands, 
		const sl::Constants& constants,
		unsigned frameIndex, 
		const Resource (&resources)[5]) const;

private:
	HMODULE mStreamlineModule;
	FARPROC mStreamlineShutdown;
};
