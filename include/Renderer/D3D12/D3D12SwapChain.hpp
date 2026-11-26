#pragma once

#include "Renderer/D3D12/D3D12RenderPass.hpp"

struct LogFile;

class D3D12FrameResource;
class StreamlineRuntime;

class D3D12SwapChain : public D3D12RenderPass {
public:
	struct InitData {
		HWND MainWndHandle;
		D3D12Device* Device;
		D3D12CommandObject* CmdObject;

		unsigned Width;
		unsigned Height;
		unsigned RenderWidth;
		unsigned RenderHeight;
		StreamlineRuntime* Streamline;
		bool DlssEnabled;
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
	bool ResizeOutput(unsigned width, unsigned height, unsigned renderWidth, unsigned renderHeight, bool dlssEnabled);
	void ReleaseSwapChain();
	bool IsDlssEnabled() const { return mbDlssEnabled; }
	bool HasStreamlineSwapChain() const { return mInitData.Streamline != nullptr; }
	const D3D12_VIEWPORT& GetOutputViewport() const { return mOutputViewport; }
	const D3D12_RECT& GetOutputScissorRect() const { return mOutputScissor; }
	GpuResource* GetDlssOutput() const { return mDlssOutput.get(); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetDlssOutputSrv() const { return mpDescHeap->GetGpuHandle(mhDlssOutputSrv); }

public:
	bool ReadyToPresent(D3D12FrameResource* const pFrameResource);
	bool Present(bool bAllowTearing);
	void NextBackBuffer();

public:
	__forceinline const D3D12_VIEWPORT& GetScreenViewport() const noexcept;
	__forceinline const D3D12_RECT& GetScissorRect() const noexcept;

	__forceinline GpuResource* GetCurrentBackBuffer() const;
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetCurrentBackBufferSrv() const;
	__forceinline D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRtv() const;

	__forceinline GpuResource* GetHdrMap() const;
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetHdrMapSrv() const;
	__forceinline D3D12_CPU_DESCRIPTOR_HANDLE GetHdrMapRtv() const;

	__forceinline GpuResource* GetHdrMapCopy() const;
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetHdrMapCopySrv() const;

	__forceinline GpuResource* GetSceneMap() const;
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetSceneMapSrv() const;
	__forceinline D3D12_CPU_DESCRIPTOR_HANDLE GetSceneMapRtv() const;

	__forceinline GpuResource* GetSceneMapCopy() const;
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetSceneMapCopySrv() const;

private:
	bool CreateSwapChain();
	bool BuildSwapChainBuffers();
	bool BuildResources();
	bool BuildDescriptors();
	void UpdateViewports();

private:
	InitData mInitData;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;
	D3D12_VIEWPORT mOutputViewport{};
	D3D12_RECT mOutputScissor{};
	bool mbDlssEnabled{};
	std::unique_ptr<GpuResource> mDlssOutput;
	D3D12DescriptorHeap::DescriptorAllocation mhDlssOutputSrv;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> mSwapChain;
	std::array<std::unique_ptr<GpuResource>, SwapChainBufferCount> mSwapChainBuffers;
	std::array<D3D12DescriptorHeap::DescriptorAllocation, SwapChainBufferCount> mhBackBufferSrvs;
	std::array<D3D12DescriptorHeap::DescriptorAllocation, SwapChainBufferCount> mhBackBufferRtvs;

	std::unique_ptr<GpuResource> mHdrMap;
	D3D12DescriptorHeap::DescriptorAllocation mhHdrMapRtv;
	D3D12DescriptorHeap::DescriptorAllocation mhHdrMapSrv;

	std::unique_ptr<GpuResource> mHdrMapCopy;
	D3D12DescriptorHeap::DescriptorAllocation mhHdrMapCopySrv;

	std::unique_ptr<GpuResource> mSceneMap;
	D3D12DescriptorHeap::DescriptorAllocation mhSceneMapRtv;
	D3D12DescriptorHeap::DescriptorAllocation mhSceneMapSrv;

	std::unique_ptr<GpuResource> mSceneMapCopy;
	D3D12DescriptorHeap::DescriptorAllocation mhSceneMapCopySrv;

	UINT mCurrBackBuffer{};
};

#include "D3D12SwapChain.inl"
