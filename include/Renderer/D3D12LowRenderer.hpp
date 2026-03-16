#pragma once

#include "Renderer/Renderer.hpp"

class D3D12Device;
class D3D12CommandObject;
class D3D12DescriptorHeap;

class D3D12LowRenderer : public Renderer {
public:
	D3D12LowRenderer();
	virtual ~D3D12LowRenderer();

public:
	RendererAPI virtual bool Initialize(
		LogFile* const pLogFile,
		unsigned width, unsigned height) override;

	RendererAPI virtual bool OnResize(unsigned width, unsigned height) override;

private:
	bool CreateDevice();
	bool CreateCommandObject();
	bool CreateDescriptorHeap();

private:
	std::unique_ptr<D3D12Device> mDevice;
	std::unique_ptr<D3D12CommandObject> mCommandObject;
	std::unique_ptr<D3D12DescriptorHeap> mDescriptorHeap;
};