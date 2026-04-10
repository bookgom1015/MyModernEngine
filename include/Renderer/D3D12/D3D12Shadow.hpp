#pragma once

#include "Renderer/D3D12/D3D12RenderPassManager.hpp"

struct D3D12RenderItem;

namespace Shadow {
	namespace Shader {
		enum Type {
			VS_DrawDepth_Static = 0,
			VS_DrawDepth_Skinned,
			GS_DrawDepth,
			PS_DrawDepth,
			Count
		};
	}

	namespace RootSignature {
		namespace DrawDepth {
			enum {
				CB_Light = 0,
				CB_Object,
				CB_Material,
				SB_BonePalette,
				RC_Consts,
				Count
			};
		}
	}

	namespace PipelineState {
		enum Type {
			GP_DrawDepth_Static = 0,
			GP_DrawDepth_Skinned,
			Count
		};
	}
}

class D3D12Shadow : public D3D12RenderPass {
public:
	struct InitData {
		D3D12Device* Device;
		D3D12CommandObject* CommandObject;
		D3D12ShaderManager* ShaderManager;
		UINT Width;
		UINT Height;
	};

public:
	D3D12Shadow();
	virtual ~D3D12Shadow();

public:
	virtual bool Initialize(
		D3D12DescriptorHeap* const pDescHeap,
		void* const pData) override;

	virtual bool CompileShaders() override;
	virtual bool BuildRootSignatures() override;
	virtual bool BuildPipelineStates() override;
	virtual bool AllocateDescriptors() override;

public:
	__forceinline GpuResource* GetDepthMap() const;
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetDepthMapSrv() const;
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetDepthMapSrv(UINT index) const;

public:
	bool Run(
		D3D12FrameResource* const pFrameResource,
		const std::vector<D3D12RenderItem*>& staticRitems,
		const std::vector<D3D12RenderItem*>& skinnedRitems,
		const std::vector<const LightData*>& lights);

private:
	bool DrawDepthStatic(
		D3D12FrameResource* const pFrameResource,
		const std::vector<D3D12RenderItem*>& ritems,
		const LightData* light,
		UINT lightIndex);
	bool DrawDepthSkinned(
		D3D12FrameResource* const pFrameResource,
		const std::vector<D3D12RenderItem*>& ritems,
		const LightData* light,
		UINT lightIndex);

	bool DrawRenderItems(
		D3D12FrameResource* const pFrameResource,
		ID3D12GraphicsCommandList6* const pCmdList,
		const std::vector<D3D12RenderItem*>& ritems);

	bool BuildResources();
	bool BuildDescriptors();

private:
	InitData mInitData;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	std::array<Hash, Shadow::Shader::Count> mShaderHashes;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, 
		Shadow::PipelineState::Count> mPipelineStates;

	std::unique_ptr<GpuResource> mDepthArrayMap{};
	std::array<D3D12DescriptorHeap::DescriptorAllocation, MAX_LIGHT_TEX_COUNT> mhSrvs;
	D3D12DescriptorHeap::DescriptorAllocation mhSrv;
	std::array<D3D12DescriptorHeap::DescriptorAllocation, MAX_LIGHT_TEX_COUNT> mhDsvs;
	D3D12DescriptorHeap::DescriptorAllocation mhDsv;

};

GpuResource* D3D12Shadow::GetDepthMap() const { return mDepthArrayMap.get(); }

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Shadow::GetDepthMapSrv() const {
	return mpDescHeap->GetGpuHandle(mhSrv);
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Shadow::GetDepthMapSrv(UINT index) const {
	return mpDescHeap->GetGpuHandle(mhSrvs[index]);
}

REGISTER_RENDER_PASS(D3D12Shadow);