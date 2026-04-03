#pragma once

#include "Renderer/D3D12/D3D12RenderPassManager.hpp"

namespace TextureScaler {
	namespace Shader {
		enum Type {
			CS_DownSample2x2 = 0,
			CS_DownSample4x4,
			CS_DownSample6x6,
			Count
		};
	}

	namespace RootSignature {
		enum Type {
			GR_DownSample2Nx2N = 0,
			Count
		};

		namespace DownSample2Nx2N {
			enum {
				RC_Consts = 0,
				SI_InputMap,
				UO_OutputMap,
				Count
			};
		}
	}

	namespace PipelineState {
		enum Type {
			CP_DownSample2x2 = 0,
			CP_DownSample4x4,
			CP_DownSample6x6,
			Count
		};
	}

	namespace KernelRadius {
		enum Type {
			E_DownSample2x2 = 1,
			E_DownSample4x4 = 2,
			E_DownSample6x6 = 3
		};
	}
}

class D3D12TextureScaler : public D3D12RenderPass {
public:
	struct InitData {
		D3D12Device* Device;
		D3D12CommandObject* CommandObject;
		D3D12ShaderManager* ShaderManager;
		UINT Width;
		UINT Height;
	};

public:
	D3D12TextureScaler();
	virtual ~D3D12TextureScaler();

public:
	virtual bool Initialize(
		D3D12DescriptorHeap* const pDescHeap,
		void* const pData) override;

	virtual bool CompileShaders() override;
	virtual bool BuildRootSignatures() override;
	virtual bool BuildPipelineStates() override;

public:
	bool DownSample2Nx2N(
		D3D12FrameResource* const pFrameResource,
		GpuResource* const pInputMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_inputMap,
		GpuResource* const pOutputMap,
		D3D12_GPU_DESCRIPTOR_HANDLE uo_outputMap,
		UINT srcTexDimX, UINT srcTexDimY, UINT dstTexDimX, UINT dstTexDimY,
		TextureScaler::KernelRadius::Type kernelRadius);

private:
	InitData mInitData;

	std::array<Hash, TextureScaler::Shader::Count> mShaderHashes;

	std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, TextureScaler::RootSignature::Count> mRootSignatures;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, TextureScaler::PipelineState::Count> mPipelineStates;
};

REGISTER_RENDER_PASS(D3D12TextureScaler);