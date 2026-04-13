#pragma once

#include "Renderer/D3D12/D3D12RenderPassManager.hpp"

struct D3D12RenderItem;

namespace Svgf {
	namespace Shader {
		enum Type {
			CS_TemporalSupersamplingReverseReproject = 0,
			CS_TemporalSupersamplingBlendWithCurrentFrame,
			CS_CalcParticalDepthDerivative,
			CS_CalcLocalMeanVariance,
			CS_FillinCheckerboard,
			CS_EdgeStoppingFilterGaussian3x3,
			CS_DisocclusionBlur3x3,
			Count
		};
	}

	namespace RootSignature {
		enum Type {
			GR_TemporalSupersamplingReverseReproject = 0,
			GR_TemporalSupersamplingBlendWithCurrentFrame,
			GR_CalcDepthPartialDerivative,
			GR_CalcLocalMeanVariance,
			GR_FillInCheckerboard,
			GR_AtrousWaveletTransformFilter,
			GR_DisocclusionBlur,
			Count
		};

		namespace TemporalSupersamplingReverseReproject {
			enum {
				CB_CrossBilateralFilter = 0,
				RC_Consts,
				SI_NormalDepth,
				SI_ReprojectedNormalDepth,
				SI_Velocity,
				SI_DepthPartialDerivative,
				SI_CachedNormalDepth,
				SI_CachedValue,
				SI_CachedValueSquaredMean,
				SI_CachedTSPP,
				SI_CachedRayHitDistance,
				UO_CachedTSPP,
				UO_TSPPSquaredMeanRayHitDistacne,
				Count
			};
		}

		namespace TemporalSupersamplingBlendWithCurrentFrame {
			enum {
				CB_TSPPBlendWithCurrentFrame = 0,
				SI_AOCoefficient,
				SI_LocalMeanVaraince,
				SI_RayHitDistance,
				SI_TSPPSquaredMeanRayHitDistance,
				UO_TemporalAOCoefficient,
				UO_TSPP,
				UO_AOCoefficientSquaredMean,
				UO_RayHitDistance,
				UO_VarianceMap,
				UO_BlurStrength,
				Count
			};
		}

		namespace CalcDepthPartialDerivative {
			enum {
				RC_Consts = 0,
				SI_DepthMap,
				UO_DepthPartialDerivative,
				Count
			};
		}

		namespace CalcLocalMeanVariance {
			enum {
				CB_LocalMeanVariance = 0,
				SI_AOCoefficient,
				UO_LocalMeanVariance,
				Count
			};
		}

		namespace FillInCheckerboard {
			enum {
				CB_LocalMeanVariance = 0,
				UIO_LocalMeanVariance,
				Count
			};
		}

		namespace AtrousWaveletTransformFilter {
			enum {
				CB_AtrousFilter = 0,
				RC_Consts,
				SI_TemporalValue,
				SI_NormalDepth,
				SI_Variance,
				SI_HitDistance,
				SI_DepthPartialDerivative,
				SI_TSPP,
				UO_TemporalValue,
				Count
			};
		}

		namespace DisocclusionBlur {
			enum {
				RC_Consts = 0,
				SI_DepthMap,
				SI_BlurStrength,
				SI_RoughnessMetalnessMap,
				UIO_AOCoefficient,
				Count
			};
		}
	}

	namespace PipelineState {
		enum Type {
			CP_TemporalSupersamplingReverseReproject = 0,
			CP_TemporalSupersamplingBlendWithCurrentFrame,
			CP_CalcDepthPartialDerivative,
			CP_CalcLocalMeanVariance,
			CP_FillInCheckerboard,
			CP_EdgeStoppingFilterGaussian3x3,
			CP_DisocclusionBlur,
			Count
		};
	}

	namespace Resource {
		namespace LocalMeanVariance {
			enum {
				E_Raw = 0,
				E_Smoothed,
				Count
			};
		}

		namespace Variance {
			enum {
				E_Raw = LocalMeanVariance::Count,
				E_Smoothed,
				Count
			};
		}

		enum {
			E_CachedValue = Variance::Count,
			E_CachedSquaredMean,
			E_DepthPartialDerivative,
			E_DisocclusionBlurStrength,
			E_TSPPSquaredMeanRayHitDistance,
			Count
		};
	}

	namespace Descriptor {
		namespace LocalMeanVariance {
			enum {
				ES_Raw = 0,
				EU_Raw,
				ES_Smoothed,
				EU_Smoothed,
				Count
			};
		}

		namespace Variance {
			enum {
				ES_Raw = LocalMeanVariance::Count,
				EU_Raw,
				ES_Smoothed,
				EU_Smoothed,
				Count
			};
		}

		enum {
			ES_DepthPartialDerivative = Variance::Count,
			EU_DepthPartialDerivative,
			ES_DisocclusionBlurStrength,
			EU_DisocclusionBlurStrength,
			ES_TSPPSquaredMeanRayHitDistance,
			EU_TSPPSquaredMeanRayHitDistance,
			Count
		};
	}

	namespace Value {
		enum Type {
			E_Contrast,
			E_Color
		};
	}
}

class D3D12Svgf : public D3D12RenderPass {
public:
	struct InitData {
		D3D12Device* Device;
		D3D12CommandObject* CommandObject;
		D3D12ShaderManager* ShaderManager;
		UINT Width;
		UINT Height;
	};

public:
	D3D12Svgf();
	virtual ~D3D12Svgf();

public:
	virtual bool Initialize(
		D3D12DescriptorHeap* const pDescHeap,
		void* const pData) override;

	virtual bool CompileShaders() override;
	virtual bool BuildRootSignatures() override;
	virtual bool BuildPipelineStates() override;
	virtual bool AllocateDescriptors() override;

	virtual bool OnResize(unsigned width, unsigned height) override;

public:

public:

private:
	bool BuildResources();
	bool BuildDescriptors();

private:
	InitData mInitData;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	std::array<Hash, Svgf::Shader::Count> mShaderHashes;

	std::array < Microsoft::WRL::ComPtr<ID3D12RootSignature>, Svgf::RootSignature::Count> mRootSignatures;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, Svgf::PipelineState::Count> mPipelineStates;

	std::array<std::unique_ptr<GpuResource>, Svgf::Resource::Count> mResources;
	D3D12DescriptorHeap::DescriptorAllocation mhDescs[Svgf::Descriptor::Count];

};

REGISTER_RENDER_PASS(D3D12Svgf);