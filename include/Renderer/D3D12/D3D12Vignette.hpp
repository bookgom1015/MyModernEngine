#pragma once

#include "D3D12RenderPassManager.hpp"

namespace Vignette {
	namespace Shader {
		enum Type {
			VS_Vignette = 0,
			MS_Vignette,
			PS_Vignette,
			Count
		};
	}

	namespace RootSignature {
		namespace Default {
			enum {
				RC_Consts = 0,
				SI_BackBuffer,
				Count
			};
		}
	}

	namespace PipelineState {
		enum Type {
			GP_Vignette = 0,
			MP_Vignette,
			Count
		};
	}
}

class D3D12Vignette : public D3D12RenderPass {
public:
	struct InitData {
		D3D12Device* Device;
		D3D12CommandObject* CommandObject;
		D3D12ShaderManager* ShaderManager;
		UINT Width;
		UINT Height;
	};

public:
	D3D12Vignette();
	virtual ~D3D12Vignette();

public:
	virtual bool Initialize(
		D3D12DescriptorHeap* const pDescHeap,
		void* const pData) override;

	virtual bool CompileShaders() override;
	virtual bool BuildRootSignatures() override;
	virtual bool BuildPipelineStates() override;

public:
	bool ApplyVignette(
		D3D12FrameResource* const pFrameResource,
		const D3D12_VIEWPORT& viewport,
		const D3D12_RECT& scissorRect,
		GpuResource* const pBackBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		GpuResource* const pBackBufferCopy,
		D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy);

private:
	InitData mInitData;

	std::array<Hash, Vignette::Shader::Count> mShaderHashes;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, Vignette::PipelineState::Count> mPipelineStates;
};

REGISTER_RENDER_PASS(D3D12Vignette);