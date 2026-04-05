#pragma once

#include "D3D12RenderPassManager.hpp"

namespace EnvironmentMap {
	namespace Shader {
		enum Type {
			VS_IntegrateBrdf = 0,
			MS_IntegrateBrdf,
			PS_IntegrateBrdf,
			VS_DrawSkySphere,
			MS_DrawSkySphere,
			PS_DrawSkySphere,
			Count
		};
	}

	namespace RootSignature {
		enum Type {
			GR_IntegrateBrdf = 0,
			GR_DrawSkySphere,
			Count
		};

		namespace IntegrateBrdf {
			enum {
				CB_Pass = 0,
				Count
			};
		}

		namespace DrawSkySphere {
			enum {
				CB_Pass = 0,
				CB_Object,
				RC_Consts,
				SB_VertexBuffer,
				SB_IndexBuffer,
				Count
			};
		}
	}

	namespace PipelineState {
		enum Type {
			GP_IntegrateBrdf = 0,
			MP_IntegrateBrdf,
			GP_DrawSkySphere,
			MP_DrawSkySphere,
			Count
		};
	}
}

class D3D12EnvironmentMap : public D3D12RenderPass {
public:
	struct InitData {
		D3D12Device* Device;
		D3D12CommandObject* CommandObject;
		D3D12ShaderManager* ShaderManager;
		UINT Width;
		UINT Height;
	};

public:
	D3D12EnvironmentMap();
	virtual ~D3D12EnvironmentMap();

public:
	virtual bool Initialize(
		D3D12DescriptorHeap* const pDescHeap,
		void* const pData) override;

	virtual bool CompileShaders() override;
	virtual bool BuildRootSignatures() override;
	virtual bool BuildPipelineStates() override;
	virtual bool AllocateDescriptors() override;

public:
	__forceinline GpuResource* GetBrdfLutMap() const noexcept;
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetBrdfLutMapSrv() const noexcept;

public:
	bool DrawBrdfLutMap(D3D12FrameResource* const pFrameResource);

	bool DrawSkySphere(
		D3D12FrameResource* const pFrameResource,
		D3D12_VIEWPORT viewport,
		D3D12_RECT scissorRect,
		GpuResource* const backBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		GpuResource* const depthBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE dio_depthStencil,
		const std::vector<struct D3D12RenderItem*>& ritems);

private:
	bool DrawRenderItems(
		D3D12FrameResource* const pFrameResource,
		ID3D12GraphicsCommandList6* const pCmdList,
		const std::vector<D3D12RenderItem*>& ritems);

	bool BuildResources();
	bool BuildDescriptors();

private:
	InitData mInitData;

	std::array<Hash, EnvironmentMap::Shader::Count> mShaderHashes;

	std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, EnvironmentMap::RootSignature::Count> mRootSignatures;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, EnvironmentMap::PipelineState::Count> mPipelineStates;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	std::unique_ptr<GpuResource> mBrdfLutMap;
	D3D12DescriptorHeap::DescriptorAllocation mhBrdfLutMapSrv;
	D3D12DescriptorHeap::DescriptorAllocation mhBrdfLutMapRtv;
};

GpuResource* D3D12EnvironmentMap::GetBrdfLutMap() const noexcept { return mBrdfLutMap.get(); }

D3D12_GPU_DESCRIPTOR_HANDLE D3D12EnvironmentMap::GetBrdfLutMapSrv() const noexcept {
	return mpDescHeap->GetGpuHandle(mhBrdfLutMapSrv);
}

REGISTER_RENDER_PASS(D3D12EnvironmentMap);