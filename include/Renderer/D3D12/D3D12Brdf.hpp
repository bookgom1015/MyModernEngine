#pragma once

#include "D3D12RenderPassManager.hpp"

class D3D12Brdf : public D3D12RenderPass {
public:
	struct InitData {
		D3D12Device* Device;
		D3D12CommandObject* CommandObject;
		D3D12ShaderManager* ShaderManager;
		UINT Width;
		UINT Height;
	};

public:
	D3D12Brdf();
	virtual ~D3D12Brdf();

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
	InitData mInitData;

	std::array<Hash, GBuffer::Shader::Count> mShaderHashes;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, GBuffer::PipelineState::Count> mPipelineStates;

	std::array<std::unique_ptr<GpuResource>, GBuffer::Resource::Count> mResources;
	std::array<D3D12DescriptorHeap::DescriptorAllocation, GBuffer::Descriptor::Srv::Count> mhSrvs;
	std::array<D3D12DescriptorHeap::DescriptorAllocation, GBuffer::Descriptor::Rtv::Count> mhRtvs;
};