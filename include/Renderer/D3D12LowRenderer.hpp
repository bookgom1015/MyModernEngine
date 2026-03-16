#pragma once

#include "Renderer/Renderer.hpp"

class D3D12Factory;
class D3D12Device;

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
	bool CreateFactory();
	bool CreateDevice();

private:
	bool mbRaytracingSupported;
	bool mbMeshShaderSupported;

	std::unique_ptr<D3D12Factory> mFactory;
	std::unique_ptr<D3D12Device> mDevice;
};