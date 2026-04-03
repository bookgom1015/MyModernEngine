#pragma once

#include "D3D12RenderPassManager.hpp"

namespace Bloom {
	namespace Shader {
		enum Type {
			CS_BlendBloomWithDownSampled = 0,
			VS_ApplyBloom,
			MS_ApplyBloom,
			PS_ApplyBloom,
			Count
		};
	}

	namespace RootSignature {
		enum Type {
			GR_BlendBloomWithDownSampled = 0,
			GR_ApplyBloom,
			Count
		};

		namespace BlendBloomWithDownSampled {
			enum {
				RC_Consts = 0,
				SI_LowerScaleMap,
				UIO_HigherScaleMap,
				Count
			};
		}

		namespace ApplyBloom {
			enum {
				RC_Consts = 0,
				SI_BackBuffer,
				SI_BloomMap,
				Count
			};
		}
	}

	namespace PipelineState {
		enum Type {
			CP_ExtractHighlights = 0,
			CP_BlendBloomWithDownSampled,
			GP_ApplyBloom,
			MP_ApplyBloom,
			Count
		};
	}

	namespace Resource {
		enum Type {
			E_4thRes = 0,
			E_16thRes,
			E_64thRes,
			E_256thRes,
			Count
		};
	}
}

class D3D12Bloom : public D3D12RenderPass {
public:
	struct InitData {
		D3D12Device* Device;
		D3D12CommandObject* CommandObject;
		D3D12ShaderManager* ShaderManager;
		UINT Width;
		UINT Height;
	};

public:
	D3D12Bloom();
	virtual ~D3D12Bloom();

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
	__forceinline GpuResource* GetHighlightMap(Bloom::Resource::Type type) const noexcept;
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetHighlightMapSrv(Bloom::Resource::Type type) const;

	__forceinline GpuResource* GetBloomMap(Bloom::Resource::Type type) const noexcept;
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetBloomMapSrv(Bloom::Resource::Type type) const;

public:
	bool ApplyBloom(
		D3D12FrameResource* const pFrameResource,
		D3D12_VIEWPORT viewport,
		D3D12_RECT scissorRect,
		GpuResource* const pBackBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		GpuResource* const pBackBufferCopy,
		D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy);

private:
	bool BuildResources();
	bool BuildDescriptors();

	bool DownSampleHighlights(
		D3D12FrameResource* const pFrameResource,
		GpuResource* const pBackBuffer,
		D3D12_GPU_DESCRIPTOR_HANDLE si_backBuffer);
	bool UpSampleWithBlur(D3D12FrameResource* const pFrameResource);

private:
	InitData mInitData;

	std::array<Hash, Bloom::Shader::Count> mShaderHashes;

	std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, Bloom::RootSignature::Count> mRootSignatures;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, Bloom::PipelineState::Count> mPipelineStates;

	std::array<std::unique_ptr<GpuResource>, Bloom::Resource::Count> mHighlightMaps;
	std::array<D3D12DescriptorHeap::DescriptorAllocation, Bloom::Resource::Count> mhHighlightMapSrvs;
	std::array<D3D12DescriptorHeap::DescriptorAllocation, Bloom::Resource::Count> mhHighlightMapUavs;

	std::array<std::unique_ptr<GpuResource>, Bloom::Resource::Count> mBloomMaps;
	std::array<D3D12DescriptorHeap::DescriptorAllocation, Bloom::Resource::Count> mhBloomMapSrvs;
	std::array<D3D12DescriptorHeap::DescriptorAllocation, Bloom::Resource::Count> mhBloomMapUavs;
};

GpuResource* D3D12Bloom::GetHighlightMap(Bloom::Resource::Type type) const noexcept {
	return mHighlightMaps[type].get();
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Bloom::GetHighlightMapSrv(Bloom::Resource::Type type) const{
	return mpDescHeap->GetGpuHandle(mhHighlightMapSrvs[type]);
}

GpuResource* D3D12Bloom::GetBloomMap(Bloom::Resource::Type type) const noexcept {
	return mBloomMaps[type].get();
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Bloom::GetBloomMapSrv(Bloom::Resource::Type type) const{
	return mpDescHeap->GetGpuHandle(mhBloomMapSrvs[type]);
}

REGISTER_RENDER_PASS(D3D12Bloom);