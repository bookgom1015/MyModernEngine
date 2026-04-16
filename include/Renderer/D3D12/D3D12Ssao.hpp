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

public:
	__forceinline GpuResource* GetAOMap() const;
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetAOMapSrv() const;

	__forceinline GpuResource* GetTemporalAOMap() const;
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetTemporalAOMapSrv() const;

	__forceinline GpuResource* GetAOCoefficientResource(
		Ssao::Resource::AO::Type type) const;
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetAOCoefficientDescriptor(
		Ssao::Descriptor::AO::Type type) const;

	__forceinline GpuResource* GetTemporalAOCoefficientResource(UINT frame) const;
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetTemporalAOCoefficientSrv(UINT frame) const;
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetTemporalAOCoefficientUav(UINT frame) const;

	__forceinline GpuResource* GetTemporalCacheResource(
		Ssao::Resource::TemporalCache::Type type, UINT frame) const;
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetTemporalCacheDescriptor(
		Ssao::Descriptor::TemporalCache::Type type, UINT frame) const;

	__forceinline constexpr UINT CurrentTemporalCacheFrameIndex() const noexcept;
	__forceinline constexpr UINT CurrentTemporalAOFrameIndex() const noexcept;

public:
	bool DrawAO(
		D3D12FrameResource* const pFrameResource,
		GpuResource* const pCurrNormalDepthMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_currNormalDepthMap,
		GpuResource* const pPositionMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap);

	UINT MoveToNextTemporalCacheFrame();
	UINT MoveToNextTemporalAOFrame();

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

GpuResource* D3D12Ssao::GetAOMap() const { 
	return mAOResources[Ssao::Resource::AO::E_AOCoefficient].get();
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Ssao::GetAOMapSrv() const {
	return mpDescHeap->GetGpuHandle(mhAOResourceDescs[Ssao::Descriptor::AO::ES_AOCoefficient]);
}

GpuResource* D3D12Ssao::GetTemporalAOMap() const {
	return mTemporalAOResources[mCurrentTemporalAOFrameIndex].get();
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Ssao::GetTemporalAOMapSrv() const {
	return mpDescHeap->GetGpuHandle(
		mhTemporalAOResourceDescs[mCurrentTemporalAOFrameIndex][Ssao::Descriptor::TemporalAO::E_Srv]);
}

GpuResource* D3D12Ssao::GetAOCoefficientResource(Ssao::Resource::AO::Type type) const {
	return mAOResources[type].get();
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Ssao::GetAOCoefficientDescriptor(Ssao::Descriptor::AO::Type type) const {
	return mpDescHeap->GetGpuHandle(mhAOResourceDescs[type]);
}

GpuResource* D3D12Ssao::GetTemporalAOCoefficientResource(UINT frame) const {
	return mTemporalAOResources[frame].get();
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Ssao::GetTemporalAOCoefficientSrv(UINT frame) const {
	return mpDescHeap->GetGpuHandle(mhTemporalAOResourceDescs[frame][Ssao::Descriptor::TemporalAO::E_Srv]);
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Ssao::GetTemporalAOCoefficientUav(UINT frame) const {
	return mpDescHeap->GetGpuHandle(mhTemporalAOResourceDescs[frame][Ssao::Descriptor::TemporalAO::E_Uav]);
}

GpuResource* D3D12Ssao::GetTemporalCacheResource(
	Ssao::Resource::TemporalCache::Type type, UINT frame) const {
	return mTemporalCaches[frame][type].get();
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Ssao::GetTemporalCacheDescriptor(
	Ssao::Descriptor::TemporalCache::Type type, UINT frame) const {
	return mpDescHeap->GetGpuHandle(mhTemporalCacheDescs[frame][type]);
}

constexpr UINT D3D12Ssao::CurrentTemporalCacheFrameIndex() const noexcept {
	return mCurrentTemporalCacheFrameIndex;
}

constexpr UINT D3D12Ssao::CurrentTemporalAOFrameIndex() const noexcept {
	return mCurrentTemporalAOFrameIndex;
}

REGISTER_RENDER_PASS(D3D12Ssao);