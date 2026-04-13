#pragma once

#include "D3D12RenderPassManager.hpp"

struct D3D12RenderItem;

namespace Rtao {
	namespace Shader {
		enum Type {
			Lib_RTAO = 0,
			Count
		};
	}

	namespace RootSignature {
		enum {
			SB_AccelerationStructure = 0,
			CB_AO,
			SI_PositionMap,
			SI_NormalDepthMap,
			SI_RayDirectionOriginDepthMap,
			SI_RayIndexOffsetMap,
			UO_AOCoefficientMap,
			UO_RayHitDistanceMap,
			UO_DebugMap,
			Count
		};
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

	namespace ShaderTable {
		enum Type {
			E_RayGenShader = 0,
			E_SortedRayGenShader,
			E_MissShader,
			E_HitGroupShader,
			Count
		};
	}
}

class D3D12Rtao : public D3D12RenderPass {
public:
	struct InitData {
		D3D12Device* Device;
		D3D12CommandObject* CommandObject;
		D3D12ShaderManager* ShaderManager;
		UINT Width;
		UINT Height;
	};

public:
	D3D12Rtao();
	virtual ~D3D12Rtao();

public:
	virtual bool Initialize(
		D3D12DescriptorHeap* const pDescHeap,
		void* const pData) override;

	virtual bool CompileShaders() override;
	virtual bool BuildRootSignatures() override;
	virtual bool BuildPipelineStates() override;
	virtual bool AllocateDescriptors() override;

	virtual bool OnResize(unsigned width, unsigned height) override;
	virtual bool BuildShaderTables(UINT numRitems) override;	

public:
	bool DrawAO(
		D3D12FrameResource* const pFrameResource,
		D3D12_GPU_VIRTUAL_ADDRESS accelStruct,
		GpuResource* const pPositionMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap,
		GpuResource* const pNormalDepthMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_normalDepthMap,
		GpuResource* const pRayDirectionOriginDepthMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_rayDirectionOriginDepthMap,
		GpuResource* const pRayInexOffsetMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_rayIndexOffsetMap,
		BOOL bRaySortingEnabled,
		BOOL bCheckboardRayGeneration);

	UINT MoveToNextTemporalCacheFrame();
	UINT MoveToNextTemporalAOFrame();

	bool BuildResources();
	bool BuildDescriptors();

private:
	InitData mInitData;

	std::array<Hash, Rtao::Shader::Count> mShaderHashes;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
	Microsoft::WRL::ComPtr<ID3D12StateObject> mStateObject;
	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> mStateObjectProp;

	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, Rtao::ShaderTable::Count> mShaderTables;

	std::array<std::unique_ptr<GpuResource>, Rtao::Resource::AO::Count> mAOResources;
	std::array<D3D12DescriptorHeap::DescriptorAllocation, Rtao::Descriptor::AO::Count> mhAOResourceDescs;

	std::array<std::array<std::unique_ptr<GpuResource>, Rtao::Resource::TemporalCache::Count>, 2> mTemporalCaches;
	std::array<std::array<D3D12DescriptorHeap::DescriptorAllocation, Rtao::Descriptor::TemporalCache::Count>, 2> mhTemporalCacheDescs;

	std::array<std::unique_ptr<GpuResource>, 2> mTemporalAOResources;
	std::array<std::array<D3D12DescriptorHeap::DescriptorAllocation, Rtao::Descriptor::TemporalAO::Count>, 2> mhTemporalAOResourceDescs;

	UINT mHitGroupShaderTableStrideInBytes;

	UINT mCurrentTemporalCacheFrameIndex;
	UINT mCurrentTemporalAOFrameIndex;
};

REGISTER_RENDER_PASS(D3D12Rtao);