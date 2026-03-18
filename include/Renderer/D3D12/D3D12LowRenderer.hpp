#pragma once

#include "Renderer/Renderer.hpp"

class D3D12Device;
class D3D12CommandObject;
class D3D12DescriptorHeap;
class D3D12SwapChain;
class D3D12DepthStencilBuffer;

class D3D12LowRenderer : public Renderer {
public:
	D3D12LowRenderer();
	virtual ~D3D12LowRenderer();

public:
	virtual bool Initialize(
		HWND hMainWnd,
		unsigned width, 
		unsigned height) override;

	virtual bool OnResize(unsigned width, unsigned height) override;

private:
	bool CreateDevice();
	bool CreateCommandObject();
	bool CreateDescriptorHeap();
	bool CreateSwapChain(unsigned width, unsigned height);
	bool CreateDepthStencilBuffer(unsigned width, unsigned height);

	bool AllocateDescriptors();

protected:
	HWND mhMainWnd;

	std::unique_ptr<D3D12Device> mDevice;
	std::unique_ptr<D3D12CommandObject> mCommandObject;
	std::unique_ptr<D3D12DescriptorHeap> mDescriptorHeap;
	std::unique_ptr<D3D12SwapChain> mSwapChain;
	std::unique_ptr<D3D12DepthStencilBuffer> mDepthStencilBuffer;
};