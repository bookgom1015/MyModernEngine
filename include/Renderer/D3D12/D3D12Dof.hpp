#pragma once

#include "D3D12RenderPassManager.hpp"

namespace Dof {
	namespace Shader {
		enum Type {
			CS_CaclFocalDistance = 0,
			CS_CircleOfConfusion,

			VS_Bokeh,
			MS_Bokeh,
			PS_Bokeh,

			VS_BokehBlurNxN,
			MS_BokehBlurNxN,
			PS_BokehBlurNxN,

			Count
		};
	}

	namespace RootSignature {
		enum Type {
			GR_CalcFocalDistance = 0,
			GR_CircleOfConfusion,
			GR_Bokeh,
			GR_BokehBlurNxN,

			Count
		};

		namespace CalcFocalDistance {
			enum {
				CB_Pass = 0,
				SI_PositionMap,
				UO_FocalDistanceBuffer,

				Count
			};
		}

		namespace CircleOfConfusion {
			enum {
				CB_Pass = 0,
				RC_Consts,
				SI_DepthMap,
				UI_FocalDistanceBuffer,
				UO_CircleOfConfusionMap,

				Count
			};
		}

		namespace Bokeh {
			enum {
				RC_Consts = 0,
				CB_Pass,
				SI_BackBuffer,
				SI_CoCMap,
				SI_DepthMap,

				Count
			};
		}

		namespace BokehBlurNxN {
			enum {
				RC_Consts = 0,
				CB_Pass,
				SI_InputMap,
				SI_CoCMap,
				SI_DepthMap,

				Count
			};
		}
	}

	namespace PipelineState {
		enum Type {
			CP_CalcFocalDistance = 0,
			CP_CircleOfConfusion,

			GP_Bokeh,
			MP_Bokeh,

			GP_BokehBlurNxN,
			MP_BokehBlurNxN,

			Count
		};
	}

	namespace Resource {
		enum Type {
			E_FocalDistanceBuffer = 0,
			E_CircleOfConfusionMap,

			Count
		};
	}
}

class D3D12Dof : public D3D12RenderPass {
public:
	struct InitData {
		D3D12Device* Device;
		D3D12CommandObject* CommandObject;
		D3D12ShaderManager* ShaderManager;
		UINT Width;
		UINT Height;
	};

public:
	D3D12Dof();
	virtual ~D3D12Dof();

	DISALLOW_COPY_AND_MOVE(D3D12Dof)

public:
	virtual bool Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) override;

	virtual bool CompileShaders() override;
	virtual bool BuildRootSignatures() override;
	virtual bool BuildPipelineStates() override;
	virtual bool AllocateDescriptors() override;

	virtual bool OnResize(unsigned width, unsigned height) override;

public:
	bool CalcFocalDistance(
		D3D12FrameResource* const pFrameResource,
		GpuResource* const pPositionMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap);
	bool CircleOfConfusion(
		D3D12FrameResource* const pFrameResource,
		GpuResource* const pDepthMap, D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap,
		float focusRange);
	bool Bokeh(
		D3D12FrameResource* const pFrameResource,
		D3D12_VIEWPORT viewport, D3D12_RECT scissorRect,
		GpuResource* const pBackBuffer, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		GpuResource* const pBackBufferCopy, D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy,
		D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap,
		UINT numSamples, float bokehRadius, float threshold, float highlightPower, UINT bokehShape,
		bool protectDepthEdges);
	bool BokehBlur(
		D3D12FrameResource* const pFrameResource,
		D3D12_VIEWPORT viewport, D3D12_RECT scissorRect,
		GpuResource* const pBackBuffer, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		GpuResource* const pBackBufferCopy, D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy,
		D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap, float maxBlurRadius, bool protectDepthEdges);

private:
	bool BuildSizeDependentResources();
	bool BuildSizeIndependentResources();
	bool BuildDescriptors();

private:
	InitData mInitData;

	std::array<Hash, Dof::Shader::Count> mShaderHashes;

	std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, Dof::RootSignature::Count> mRootSignatures;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, Dof::PipelineState::Count> mPipelineStates;

	std::array<std::unique_ptr<GpuResource>, Dof::Resource::Count> mResources;
	D3D12DescriptorHeap::DescriptorAllocation mhCircleOfConfusionMapSrv;
	D3D12DescriptorHeap::DescriptorAllocation mhCircleOfConfusionMapUav;
};

REGISTER_RENDER_PASS(D3D12Dof);
