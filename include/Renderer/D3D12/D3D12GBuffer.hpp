#pragma once

#include "D3D12RenderPass.hpp"
#include "D3D12RenderPassManager.hpp"

class D3D12GBuffer : public D3D12RenderPass {
public:
	struct InitData {
		D3D12Device* Device;
		D3D12CommandObject* CommandObject;
		UINT Width;
		UINT Height;
	};

public:
	D3D12GBuffer();
	virtual ~D3D12GBuffer();

public:
	virtual bool Initialize(
		D3D12DescriptorHeap* const pDescHeap,
		void* const pData) override;

	virtual bool CompileShaders() override;
	virtual bool BuildRootSignatures() override;
	virtual bool BuildPipelineStates() override;
	virtual bool AllocateDescriptors() override;

	virtual bool OnResize(unsigned width, unsigned height) override;

private:
	bool BuildResources();
	bool BuildDescriptors();

private:

private:
	InitData mInitData;
};

REGISTER_RENDER_PASS(D3D12GBuffer);