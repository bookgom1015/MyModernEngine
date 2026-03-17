#pragma once

#include "D3D12RenderPass.hpp"

class D3D12DepthStencilBuffer : public D3D12RenderPass {
public:
	struct InitData {
		D3D12Device* Device;
		unsigned Width;
		unsigned Height;
	};

public:
	D3D12DepthStencilBuffer();
	virtual ~D3D12DepthStencilBuffer();

public:
	virtual bool Initialize(
		LogFile* const pLogFile,
		D3D12DescriptorHeap* const pDescHeap,
		void* const pData) override;

	virtual bool AllocateDescriptors() override;
	virtual bool OnResize(unsigned width, unsigned height) override;

private:
	bool BuildResources();
	bool BuildDescriptors();

private:
	InitData mInitData;

	std::unique_ptr<GpuResource> mDepthStencilBuffer;
	UINT mhDepthStencilBufferSrv;
	UINT mhDepthStencilBufferDsv;
};