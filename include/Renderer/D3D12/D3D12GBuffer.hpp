#pragma once

#include "D3D12RenderPassManager.hpp"

struct D3D12RenderItem;

namespace GBuffer {
	namespace Shader {
		enum Type {
			VS_GBuffer_Static = 0,
			VS_GBuffer_Skinned,
			MS_GBuffer_Static,
			MS_GBuffer_Skinned,
			PS_GBuffer,
			Count
		};
	}

	namespace RootSignature {
		namespace Default {
			enum {
				CB_Pass = 0,
				CB_Object,
				CB_Material,
				RC_Consts,
				SB_StaticVertexBuffer,
				SB_SkinnedVertexBuffer,
				SB_IndexBuffer,
				SB_BonePalette,
				SI_Textures_AlbedoMap,
				SI_Textures_NormalMap,
				Count
			};
		}
	}

	namespace PipelineState {
		enum Type {
			GP_GBuffer_Static = 0,
			GP_GBuffer_Skinned,
			MP_GBuffer_Static,
			MP_GBuffer_Skinned,
			Count
		};
	}

	namespace Resource {
		enum Type {
			E_Albedo = 0,
			E_Normal,
			E_NormalDepth,
			E_ReprojNormalDepth,
			E_CachedNormalDepth,
			E_RMS,
			E_Velocity,
			E_Position,
			Count
		};
	}

	namespace Descriptor {
		namespace Srv {
			enum Type {
				E_Albedo = 0,
				E_Normal,
				E_NormalDepth,
				E_ReprojNormalDepth,
				E_CachedNormalDepth,
				E_RMS,
				E_Velocity,
				E_Position,
				Count
			};
		}

		namespace Rtv {
			enum Type {
				E_Albedo = 0,
				E_Normal,
				E_NormalDepth,
				E_ReprojNormalDepth,
				E_RMS,
				E_Velocity,
				E_Position,
				Count
			};
		}
	}
}

class D3D12GBuffer : public D3D12RenderPass {
public:
	struct InitData {
		D3D12Device* Device;
		D3D12CommandObject* CommandObject;
		D3D12ShaderManager* ShaderManager;
		UINT Width;
		UINT Height;
	};

public:
	D3D12GBuffer();
	virtual ~D3D12GBuffer();

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
	bool DrawGBuffer(
		D3D12FrameResource* const pFrameResource,
		D3D12_VIEWPORT viewport,
		D3D12_RECT scissorRect,
		GpuResource* const backBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		GpuResource* const depthBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE do_depthBuffer,
		const std::vector<D3D12RenderItem*>& staticRitems,
		const std::vector<D3D12RenderItem*>& skinnedRitems,
		FLOAT ditheringMaxDist, FLOAT ditheringMinDist);

public:
	__forceinline GpuResource* GetAlbedoMap() const noexcept;
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetAlbedoMapSrv() const noexcept;

	__forceinline GpuResource* GetNormalMap() const noexcept;
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetNormalMapSrv() const noexcept;

	__forceinline GpuResource* GetNormalDepthMap() const noexcept;
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetNormalDepthMapSrv() const noexcept;

	__forceinline GpuResource* GetReprojNormalDepthMap() const noexcept;
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetReprojNormalDepthMapSrv() const noexcept;

	__forceinline GpuResource* GetRMSMap() const noexcept;
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetRMSMapSrv() const noexcept;

	__forceinline GpuResource* GetVelocityMap() const noexcept;
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetVelocityMapSrv() const noexcept;

	__forceinline GpuResource* GetPositionMap() const noexcept;
	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetPositionMapSrv() const noexcept;

private:
	bool DrawGBufferForStaticRitems(
		D3D12FrameResource* const pFrameResource,
		D3D12_VIEWPORT viewport,
		D3D12_RECT scissorRect,
		GpuResource* const backBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		GpuResource* const depthBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE do_depthBuffer,
		const std::vector<D3D12RenderItem*>& ritems,
		FLOAT ditheringMaxDist, FLOAT ditheringMinDist);
	bool DrawGBufferForSkinnedRitems(
		D3D12FrameResource* const pFrameResource,
		D3D12_VIEWPORT viewport,
		D3D12_RECT scissorRect,
		GpuResource* const backBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		GpuResource* const depthBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE do_depthBuffer,
		const std::vector<D3D12RenderItem*>& ritems,
		FLOAT ditheringMaxDist, FLOAT ditheringMinDist);
	bool DrawRenderItems(
		D3D12FrameResource* const pFrameResource,
		ID3D12GraphicsCommandList6* const pCmdList,
		const std::vector<D3D12RenderItem*>& ritems,
		FLOAT ditheringMaxDist, FLOAT ditheringMinDist,
		bool isSkinned);

	bool BuildResources();
	bool BuildDescriptors();

private:
	InitData mInitData;

	std::array<Hash, GBuffer::Shader::Count> mShaderHashes;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, GBuffer::PipelineState::Count> mPipelineStates;

	std::array<std::unique_ptr<GpuResource>, GBuffer::Resource::Count> mResources;
	std::array<D3D12DescriptorHeap::DescriptorAllocation, GBuffer::Descriptor::Srv::Count> mhSrvs;
	std::array<D3D12DescriptorHeap::DescriptorAllocation, GBuffer::Descriptor::Rtv::Count> mhRtvs;
};

#include "D3D12GBuffer.inl"

REGISTER_RENDER_PASS(D3D12GBuffer);