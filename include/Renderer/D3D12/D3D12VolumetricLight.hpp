#pragma once

#include "Renderer/D3D12/D3D12RenderPassManager.hpp"

namespace VolumetricLight {
	namespace Shader {
		enum Type {
			CS_CalculateScatteringAndDensity = 0,
			CS_AccumulateScattering,
			CS_BlendScattering,
			VS_ApplyFog,
			PS_ApplyFog,
			PS_ApplyFog_Tricubic,
			Count
		};
	}

	namespace RootSignature {
		enum Type {
			GR_CalculateScatteringAndDensity = 0,
			GR_AccumulateScattering,
			GR_BlendScattering,
			GR_ApplyFog,
			Count
		};

		namespace CalculateScatteringAndDensity {
			enum {
				CB_Pass = 0,
				CB_Light,
				RC_Consts,
				SI_DepthMaps,
				UO_FrustumVolumeMap,
				Count
			};
		}

		namespace AccumulateScattering {
			enum {
				RC_Consts = 0,
				UIO_FrustumVolumeMap,
				Count
			};
		}

		namespace BlendScattering {
			enum {
				SI_PreviousScattering = 0,
				UIO_CurrentScattering,
				Count
			};
		}

		namespace ApplyFog {
			enum {
				CB_Pass = 0,
				RC_Consts,
				SI_PositionMap,
				SI_FrustumVolumeMap,
				Count
			};
		}
	}

	namespace PipelineState {
		enum Type {
			CP_CalculateScatteringAndDensity = 0,
			CP_AccumulateScattering,
			CP_BlendScattering,
			GP_ApplyFog,
			GP_ApplyFog_Tricubic,
			Count
		};
	}

	namespace Descriptor {
		enum FrustumVolumeMap {
			E_Srv = 0,
			E_Uav,
			Count
		};
	}
}

class D3D12VolumetricLight : public D3D12RenderPass {
public:
	struct InitData {
		D3D12Device* Device;
		D3D12CommandObject* CommandObject;
		D3D12ShaderManager* ShaderManager;
		UINT Width;
		UINT Height;
		UINT Depth;
	};

public:
	D3D12VolumetricLight();
	virtual ~D3D12VolumetricLight();

public:
	virtual bool Initialize(
		D3D12DescriptorHeap* const pDescHeap,
		void* const pData) override;

	virtual bool CompileShaders() override;
	virtual bool BuildRootSignatures() override;
	virtual bool BuildPipelineStates() override;
	virtual bool AllocateDescriptors() override;

public:
	bool BuildFog(
		D3D12FrameResource* const pFrameResource,
		GpuResource* pDepthMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_depthMaps,
		FLOAT nearZ, FLOAT farZ);

	bool ApplyFog(
		D3D12FrameResource* const pFrameResource,
		GpuResource* pBackBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		GpuResource* pPositionMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap,
		D3D12_VIEWPORT viewport, D3D12_RECT scissorRect,
		FLOAT nearZ, FLOAT farZ);

private:
	bool CalculateScatteringAndDensity(
		D3D12FrameResource* const pFrameResource,
		GpuResource* pDepthMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_depthMaps,
		FLOAT nearZ, FLOAT farZ);
	bool AccumulateScattering(
		D3D12FrameResource* const pFrameResource,
		FLOAT nearZ, FLOAT farZ);
	bool BlendScattering(D3D12FrameResource* const pFrameResource);

	bool BuildResources();
	bool BuildDescriptors();

private:
	InitData mInitData;

	std::array<Hash, VolumetricLight::Shader::Count> mShaderHashes;

	std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, 
		VolumetricLight::RootSignature::Count> mRootSignatures;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, 
		VolumetricLight::PipelineState::Count> mPipelineStates;

	std::array<std::unique_ptr<GpuResource>, 2> mFrustumVolumeMaps;
	std::array<std::array<D3D12DescriptorHeap::DescriptorAllocation, 2>, 
		VolumetricLight::Descriptor::FrustumVolumeMap::Count> mhFrustumVolumeMapDescs;

	UINT mFrameCount{};
	UINT mCurrentFrame{};
	UINT mPreviousFrame{};

};

REGISTER_RENDER_PASS(D3D12VolumetricLight);