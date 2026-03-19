#pragma once

#include "Renderer/D3D12/D3D12RenderPass.hpp"

struct LogFile;

class D3D12FrameResource;

class D3D12SwapChain : public D3D12RenderPass {
public:
	struct InitData {
		HWND MainWndHandle;
		D3D12Device* Device;
		D3D12CommandObject* CmdObject;

		unsigned Width;
		unsigned Height;
	};

	static const UINT SwapChainBufferCount = 2;

public:
	D3D12SwapChain();
	virtual ~D3D12SwapChain();

public:
	virtual bool Initialize(
		D3D12DescriptorHeap* const pDescHeap,
		void* const pData) override;

	virtual bool AllocateDescriptors() override;
	virtual bool OnResize(unsigned width, unsigned height) override;

public:
	bool ReadyToPresent(D3D12FrameResource* const pFrameResource);
	bool Present(bool bAllowTearing);
	void NextBackBuffer();

public:
	__forceinline const D3D12_VIEWPORT& GetScreenViewport() const noexcept;
	__forceinline const D3D12_RECT& GetScissorRect() const noexcept;

	__forceinline GpuResource* GetCurrentBackBuffer() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetCurrentBackBufferSrv() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRtv() const;

	D3D12_GPU_DESCRIPTOR_HANDLE GetSceneMapSrv() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetSceneMapRtv() const;

private:
	bool CreateSwapChain();
	bool BuildSwapChainBuffers();
	bool BuildResources();
	bool BuildDescriptors();

private:
	InitData mInitData;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> mSwapChain;
	std::array<std::unique_ptr<GpuResource>, SwapChainBufferCount> mSwapChainBuffers;
	std::array<D3D12DescriptorHeap::DescriptorAllocation, SwapChainBufferCount> mhBackBufferSrvs;
	std::array<D3D12DescriptorHeap::DescriptorAllocation, SwapChainBufferCount> mhBackBufferRtvs;

	std::unique_ptr<GpuResource> mSceneMap;
	D3D12DescriptorHeap::DescriptorAllocation mhSceneMapRtv;
	D3D12DescriptorHeap::DescriptorAllocation mhSceneMapSrv;

	std::unique_ptr<GpuResource> mSceneMapCopy;
	D3D12DescriptorHeap::DescriptorAllocation mhSceneMapCopySrv;

	UINT mCurrBackBuffer{};
};

#include "D3D12SwapChain.inl"