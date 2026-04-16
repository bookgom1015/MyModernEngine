#pragma once

#pragma once

#include "Renderer/D3D12/D3D12RenderPassManager.hpp"

namespace EyeAdaption {
	namespace Shader {
		enum Type {
			CS_ClearHistogram = 0,
			CS_LuminanceHistogram,
			CS_PercentileExtract,
			CS_TemporalSmoothing,
			Count
		};
	}

	namespace RootSignature {
		enum Type {
			GR_LuminanceHistogram = 0,
			GR_PercentileExtract,
			GR_TemporalSmoothing,
			Count
		};

		namespace LuminanceHistogram {
			enum {
				RC_Consts = 0,
				SI_BackBuffer,
				UO_HistogramBuffer,
				Count
			};
		}

		namespace PercentileExtract {
			enum {
				RC_Consts = 0,
				UI_HistogramBuffer,
				UO_AvgLogLuminance,
				Count
			};
		}

		namespace TemporalSmoothing {
			enum {
				RC_Consts = 0,
				UI_AvgLogLuminance,
				UO_SmoothedLum,
				UIO_PrevLum,
				Count
			};
		}
	}

	namespace PipelineState {
		enum Type {
			CP_ClearHistogram = 0,
			CP_LuminanceHistogram,
			CP_PercentileExtract,
			CP_TemporalSmoothing,
			Count
		};
	}
}

class D3D12EyeAdaption : public D3D12RenderPass {
public:
	struct InitData {
		D3D12Device* Device;
		D3D12CommandObject* CommandObject;
		D3D12ShaderManager* ShaderManager;
		UINT Width;
		UINT Height;
	};

public:
	D3D12EyeAdaption();
	virtual ~D3D12EyeAdaption();

public:
	virtual bool Initialize(
		D3D12DescriptorHeap* const pDescHeap,
		void* const pData) override;

	virtual bool CompileShaders() override;
	virtual bool BuildRootSignatures() override;
	virtual bool BuildPipelineStates() override;

	virtual bool OnResize(unsigned width, unsigned height) override;

public:
	__forceinline GpuResource* GetLuminanceMap() const;

public:
	bool ClearHistogram(D3D12FrameResource* const pFrameResource);
	bool BuildLuminanceHistogram(
		D3D12FrameResource* const pFrameResource,
		GpuResource* const pBackBuffer, 
		D3D12_GPU_DESCRIPTOR_HANDLE si_backBuffer);
	bool PercentileExtract(D3D12FrameResource* const pFrameResource);
	bool TemporalSmoothing(D3D12FrameResource* const pFrameResource, FLOAT dt);

private:
	bool BuildResources();

private:
	InitData mInitData;

	std::array<Hash, EyeAdaption::Shader::Count> mShaderHashes;

	std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, 
		EyeAdaption::RootSignature::Count> mRootSignatures;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>,
		EyeAdaption::PipelineState::Count> mPipelineStates;

	std::unique_ptr<GpuResource> mHistogramBuffer;
	std::unique_ptr<GpuResource> mAvgLogLuminance;
	std::unique_ptr<GpuResource> mPrevLuminance;
	std::unique_ptr<GpuResource> mSmoothedLuminance;
};

#include "D3D12EyeAdaption.inl"

REGISTER_RENDER_PASS(D3D12EyeAdaption);