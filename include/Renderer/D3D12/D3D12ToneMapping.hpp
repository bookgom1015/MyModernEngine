#pragma once

#include "D3D12RenderPassManager.hpp"

namespace ToneMapping {
	namespace Shader {
		enum Type {
			VS_ToneMapping = 0,
			MS_ToneMapping,
			PS_ToneMapping,
			Count
		};
	}

	namespace RootSignature {
		namespace Default {
			enum {
				RC_Cosnts = 0,
				SI_Intermediate,
				UI_AvgLogLuminance,
				Count
			};
		}
	}

	namespace PipelineState {
		enum Type {
			GP_ToneMapping = 0,
			MP_ToneMapping,
			Count
		};
	}
}

class D3D12ToneMapping : public D3D12RenderPass{
public:
	struct InitData {
		D3D12Device* Device;
		D3D12CommandObject* CommandObject;
		D3D12ShaderManager* ShaderManager;
		UINT Width;
		UINT Height;
	};

public:
	D3D12ToneMapping();
	virtual ~D3D12ToneMapping();

public:
	virtual bool Initialize(
		D3D12DescriptorHeap* const pDescHeap,
		void* const pData) override;

	virtual bool CompileShaders() override;
	virtual bool BuildRootSignatures() override;
	virtual bool BuildPipelineStates() override;

public:
	bool Apply(
		D3D12FrameResource* const pFrameResource,
		const D3D12_VIEWPORT& viewport,
		const D3D12_RECT& scissorRect,
		GpuResource* const pBackBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		GpuResource* const pHdrMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_HdrMapSrv);

private:
	InitData mInitData;

	std::array<Hash, ToneMapping::Shader::Count> mShaderHashes;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, ToneMapping::PipelineState::Count> mPipelineStates;
};

REGISTER_RENDER_PASS(D3D12ToneMapping);