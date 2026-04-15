#pragma once

#include "Renderer/D3D12/D3D12RenderPassManager.hpp"

namespace Ssao {
	namespace Shader {
		enum Type {
			CS_SSAO = 0,
			Count
		};
	}

	namespace RootSignature {
		namespace Default {
			enum {
				CB_AO = 0,
				RC_Consts,
				SI_NormalDepthMap,
				SI_PositionMap,
				SI_RandomVectorMap,
				UO_AOCoefficientMap,
				UO_RayHitDistMap,
				UO_DebugMap,
				Count
			};
		}
	}

	namespace Resource {
		namespace AO {
			enum Type {
				E_AOCoefficient = 0,
				E_RayHitDistance,
				Count
			};
		}

		namespace TemporalCache {
			enum Type {
				E_TSPP = 0,
				E_RayHitDistance,
				E_AOCoefficientSquaredMean,
				Count
			};
		}
	}

	namespace Descriptor {
		namespace AO {
			enum Type {
				ES_AOCoefficient = 0,
				EU_AOCoefficient,
				ES_RayHitDistance,
				EU_RayHitDistance,
				Count
			};
		}

		namespace TemporalCache {
			enum Type {
				ES_TSPP = 0,
				EU_TSPP,
				ES_RayHitDistance,
				EU_RayHitDistance,
				ES_AOCoefficientSquaredMean,
				EU_AOCoefficientSquaredMean,
				Count
			};
		}

		namespace TemporalAO {
			enum Type {
				E_Srv = 0,
				E_Uav,
				Count
			};
		}
	}
}

class D3D12Ssao : public D3D12RenderPass {
public:
	struct InitData {
		D3D12Device* Device;
		D3D12CommandObject* CommandObject;
		D3D12ShaderManager* ShaderManager;
		UINT Width;
		UINT Height;
	};

public:
	D3D12Ssao();
	virtual ~D3D12Ssao();

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
	InitData mInitData;

	std::array<Hash, Ssao::Shader::Count> mShaderHashes;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPipelineState;

	std::array<std::unique_ptr<GpuResource>, Ssao::Resource::AO::Count> mAOResources;
	std::array<D3D12DescriptorHeap::DescriptorAllocation, Ssao::Descriptor::AO::Count> mhAOResourceDescs;

	std::array<std::array<std::unique_ptr<GpuResource>, Ssao::Resource::TemporalCache::Count>, 2> mTemporalCaches;
	std::array<std::array<D3D12DescriptorHeap::DescriptorAllocation, Ssao::Descriptor::TemporalCache::Count>, 2> mhTemporalCacheDescs;

	std::array<std::unique_ptr<GpuResource>, 2> mTemporalAOResources;
	std::array<std::array<D3D12DescriptorHeap::DescriptorAllocation, Ssao::Descriptor::TemporalAO::Count>, 2> mhTemporalAOResourceDescs;

	UINT mCurrentTemporalCacheFrameIndex;
	UINT mCurrentTemporalAOFrameIndex;
};

REGISTER_RENDER_PASS(D3D12Ssao);