#pragma once

#include "D3D12RenderPassManager.hpp"

struct D3D12Texture;

namespace EnvironmentManager {
	namespace Shader {
		enum Type {
			VS_IntegrateBrdf = 0,
			MS_IntegrateBrdf,
			PS_IntegrateBrdf,
			VS_DrawSkySphere,
			MS_DrawSkySphere,
			PS_DrawSkySphere,
			VS_CaptureEnvironment,
			GS_CaptureEnvironment,
			PS_CaptureEnvironment,
			Count
		};
	}

	namespace RootSignature {
		enum Type {
			GR_IntegrateBrdf = 0,
			GR_DrawSkySphere,
			GR_CaptureEnvironment,
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
				SI_EnvCubeMap,
				Count
			};
		}

		namespace CaptureEnvironment {
			enum {
				CB_Pass = 0,
				CB_ProjectToCube,
				CB_Light,
				CB_Object,
				CB_Material,
				RC_Consts,
				SI_ShadowMap,
				SI_Textures_AlbedoMap,
				SI_Textures_NormalMap,
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
			GP_CaptureEnvironment,
			Count
		};
	}
}

struct D3D12ReflectionProbeSlot : public ReflectionProbeSlot {
	std::unique_ptr<GpuResource> CapturedCube;
	D3D12DescriptorHeap::DescriptorAllocation CapturedCubeSrv;
	D3D12DescriptorHeap::DescriptorAllocation CapturedCubeRtv;

	std::unique_ptr<GpuResource> DiffuseIrradiance;
	D3D12DescriptorHeap::DescriptorAllocation DiffuseIrradianceSrv;
	D3D12DescriptorHeap::DescriptorAllocation DiffuseIrradianceRtv;

	std::unique_ptr<GpuResource> SpecularIrradiance;
	D3D12DescriptorHeap::DescriptorAllocation SpecularIrradianceSrv;
	D3D12DescriptorHeap::DescriptorAllocation SpecularIrradianceRtvs[6];
};

class D3D12EnvironmentManager : public D3D12RenderPass {
public:
	struct InitData {
		D3D12Device* Device;
		D3D12CommandObject* CommandObject;
		D3D12ShaderManager* ShaderManager;
		UINT Width;
		UINT Height;
	};

public:
	D3D12EnvironmentManager();
	virtual ~D3D12EnvironmentManager();

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

	__forceinline D3D12Texture* GetGlobalDiffuseIrradianceMap() const noexcept;
	__forceinline const std::wstring& GetGlobalDiffuseIrradianceMapPath() const noexcept;
	__forceinline void SetGlobalDiffuseIrradianceMap(const std::wstring& key, D3D12Texture* const pTexture) noexcept;

	__forceinline D3D12Texture* GetGlobalSpecularIrradianceMap() const noexcept;
	__forceinline const std::wstring& GetGlobalSpecularIrradianceMapPath() const noexcept;
	__forceinline void SetGlobalSpecularIrradianceMap(const std::wstring& key, D3D12Texture* const pTexture) noexcept;

	__forceinline size_t GetReflectionProbeCount() const noexcept;
	D3D12_GPU_DESCRIPTOR_HANDLE GetReflectionProbeCapturedCubeSrv(ReflectionProbeID id) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetReflectionProbeCapturedCubeSrv(size_t index) const;

public:
	ReflectionProbeID AddReflectionProbe(const ReflectionProbeDesc& desc);
	void RemoveReflectionProbe(ReflectionProbeID id);

	ReflectionProbeDesc* GetReflectionProbe(ReflectionProbeID id);
	const ReflectionProbeDesc* GetReflectionProbe(ReflectionProbeID id) const;

	bool BakeReflectionProbes(
		D3D12FrameResource* const pFrameResource,
		const std::vector<struct D3D12RenderItem*>& ritems);

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

	bool DrawRenderItems(
		D3D12FrameResource* const pFrameResource,
		ID3D12GraphicsCommandList6* const pCmdList,
		const std::vector<D3D12RenderItem*>& ritems,
		UINT);

	bool BuildResources();
	bool BuildDescriptors();

	bool BuildReflectionProbeResources(D3D12ReflectionProbeSlot* pSlot, const ReflectionProbeDesc& desc);

private:
	InitData mInitData;

	std::array<Hash, EnvironmentManager::Shader::Count> mShaderHashes;

	std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, EnvironmentManager::RootSignature::Count> mRootSignatures;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, EnvironmentManager::PipelineState::Count> mPipelineStates;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	std::unique_ptr<GpuResource> mBrdfLutMap;
	D3D12DescriptorHeap::DescriptorAllocation mhBrdfLutMapSrv;
	D3D12DescriptorHeap::DescriptorAllocation mhBrdfLutMapRtv;

	D3D12Texture* mpGlobalDiffuseIrradianceTex;
	std::wstring mGlobalDiffuseIrradianceTexPath;

	D3D12Texture* mpGlobalSpecularIrradianceTex;
	std::wstring mGlobalSpecularIrradianceTexPath;

	std::unique_ptr<GpuResource> mDepthBufferArray;
	D3D12DescriptorHeap::DescriptorAllocation mhDepthBufferArrayDsv;

	std::vector<std::unique_ptr<D3D12ReflectionProbeSlot>> mReflectionProbes;
	std::vector<std::uint32_t> mFreeProbeSlots;
};

#include "D3D12EnvironmentManager.inl"

REGISTER_RENDER_PASS(D3D12EnvironmentManager);