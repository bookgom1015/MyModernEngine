#pragma once

#include "D3D12RenderPassManager.hpp"

namespace BRDF {
	namespace Shader {
		enum Type {
			VS_ComputeBRDF = 0,
			MS_ComputeBRDF,
			PS_ComputeBRDF,
			VS_IntegrateIrradiance,
			MS_IntegrateIrradiance,
			PS_IntegrateIrradiance,
			Count
		};
	}	

	namespace RootSignature {
		enum Type {
			GR_ComputeBRDF = 0,
			GR_IntegrateIrradiance,
			Count
		};

		namespace ComputeBRDF {
			enum {
				CB_Pass = 0,
				CB_Light,
				RC_Consts,
				SI_AlbedoMap,
				SI_NormalMap,
				SI_DepthMap,
				SI_RMSMap,
				SI_PositionMap,
				SI_ShadowMap,
				Count
			};
		}

		namespace IntegrateIrradiance {
			enum {
				CB_Pass = 0,
				RC_Consts,
				SI_BackBuffer,
				SI_AlbedoMap,
				SI_NormalMap,
				SI_DepthMap,
				SI_RMSMap,
				SI_PositionMap,
				SI_BrdfLutMap,
				SI_GlobalDiffuseIrradianceMap,
				SI_GlobalSpecularIrradianceMap,
				SI_LocalDiffuseIrradianceMap,
				SI_LocalSpecularIrradianceMap,
				Count
			};
		}
	}

	namespace PipelineState {
		enum Type {
			GP_ComputeBRDF = 0,
			MP_ComputeBRDF,
			GP_IntegrateIrradiance,
			MP_IntegrateIrradiance,
			Count
		};
	}
}

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

public:
	bool ComputeBRDF(
		D3D12FrameResource* const pFrameResource,
		D3D12_VIEWPORT viewport, D3D12_RECT scissorRect,
		GpuResource* const pBackBuffer, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		GpuResource* const pAlbedoMap, D3D12_GPU_DESCRIPTOR_HANDLE si_albedoMap,
		GpuResource* const pNormalMap, D3D12_GPU_DESCRIPTOR_HANDLE si_normalMap,
		GpuResource* const pDepthMap, D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap,
		GpuResource* const pRMSMap, D3D12_GPU_DESCRIPTOR_HANDLE si_rmsMap,
		GpuResource* const pPositionMap, D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap,
		GpuResource* const pShadowMap, D3D12_GPU_DESCRIPTOR_HANDLE si_shadowMap);
	bool IntegrateIrradiance(
		D3D12FrameResource* const pFrameResource,
		D3D12_VIEWPORT viewport, D3D12_RECT scissorRect,
		GpuResource* const pBackBuffer, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		GpuResource* const pBackBufferCopy, D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy,
		GpuResource* const pAlbedoMap, D3D12_GPU_DESCRIPTOR_HANDLE si_albedoMap,
		GpuResource* const pNormalMap, D3D12_GPU_DESCRIPTOR_HANDLE si_normalMap,
		GpuResource* const pDepthMap, D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap,
		GpuResource* const pRMSMap, D3D12_GPU_DESCRIPTOR_HANDLE si_rmsMap,
		GpuResource* const pPositionMap, D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap);

private:
	InitData mInitData;

	std::array<Hash, BRDF::Shader::Count> mShaderHashes;

	std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, BRDF::RootSignature::Count> mRootSignatures;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, BRDF::PipelineState::Count> mPipelineStates;
};

REGISTER_RENDER_PASS(D3D12Brdf);