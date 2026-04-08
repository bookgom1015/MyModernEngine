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
			VS_CaptureSkySphere,
			GS_CaptureSkySphere,
			PS_CaptureSkySphere,
			VS_ConvoluteDiffuseIrradiance,
			GS_ConvoluteDiffuseIrradiance,
			PS_ConvoluteDiffuseIrradiance,
			VS_ConvoluteSpecularIrradiance,
			GS_ConvoluteSpecularIrradiance,
			PS_ConvoluteSpecularIrradiance,
			Count
		};
	}

	namespace RootSignature {
		enum Type {
			GR_IntegrateBrdf = 0,
			GR_DrawSkySphere,
			GR_CaptureEnvironment,
			GR_CaptureSkySphere,
			GR_ConvoluteDiffuseIrradiance,
			GR_ConvoluteSpecularIrradiance,
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

		namespace CaptureSkySphere {
			enum {
				CB_Pass = 0,
				CB_ProjectToCube,
				CB_Object,
				SI_EnvCubeMap,
				Count
			};
		}

		namespace ConvoluteDiffuseIrradiance {
			enum {
				CB_ProjectToCube = 0,
				SI_EnvCubeMap,
				Count
			};
		}

		namespace ConvoluteSpecularIrradiance {
			enum {
				CB_ProjectToCube = 0,
				RC_Consts,
				SI_EnvCubeMap,
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
			GP_CaptureSkySphere,
			GP_ConvoluteDiffuseIrradiance,
			GP_ConvoluteSpecularIrradiance,
			Count
		};
	}
}

struct D3D12ReflectionProbeSlot : public ReflectionProbeSlot {
	// CapturedCube
	std::unique_ptr<GpuResource> CapturedCube;
	D3D12DescriptorHeap::DescriptorAllocation CapturedCubeSrv;
	D3D12DescriptorHeap::DescriptorAllocation CapturedCubeRtv;
	D3D12DescriptorHeap::DescriptorAllocation CapturedCubeSrvFace[6]; // for Debugging

	// DiffuseIrradiance
	std::unique_ptr<GpuResource> DiffuseIrradiance;
	D3D12DescriptorHeap::DescriptorAllocation DiffuseIrradianceSrv;
	D3D12DescriptorHeap::DescriptorAllocation DiffuseIrradianceRtv;
	D3D12DescriptorHeap::DescriptorAllocation DiffuseIrradianceSrvFace[6]; // for Debugging

	// SpecularIrradiance
	std::unique_ptr<GpuResource> SpecularIrradiance;
	D3D12DescriptorHeap::DescriptorAllocation SpecularIrradianceSrv;
	D3D12DescriptorHeap::DescriptorAllocation SpecularIrradianceRtvs[5];

	// Texture
	int TextureIndex = -1;
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

	__forceinline UINT GetReflectionProbeCount() const noexcept;
	D3D12_GPU_DESCRIPTOR_HANDLE GetReflectionProbeCapturedCubeSrv(ReflectionProbeID id) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetReflectionProbeCapturedCubeSrv(size_t index) const;

	D3D12_GPU_DESCRIPTOR_HANDLE GetReflectionProbeCapturedCubeSrv(size_t index, UINT face) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetReflectionProbeDiffuseIrradianceSrv(size_t index, UINT face) const;

	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetReflectionProbeCapturedCubeSrvs() const;
	__forceinline GpuResource* GetReflectionProbeCapturedCube(size_t index) const;

	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetReflectionProbeDiffuseIrradianceSrvs() const;
	__forceinline GpuResource* GetReflectionProbeDiffuseIrradiance(size_t index) const;

	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetReflectionProbeSpecularIrradianceSrvs() const;
	__forceinline GpuResource* GetReflectionProbeSpecularIrradiance(size_t index) const;

public:
	ReflectionProbeID AddReflectionProbe(const ReflectionProbeDesc& desc);
	void UpdateReflectionProbe(ReflectionProbeID id, const ReflectionProbeDesc& desc);
	void RemoveReflectionProbe(ReflectionProbeID id);

	ReflectionProbeDesc* GetReflectionProbe(ReflectionProbeID id);
	const ReflectionProbeDesc* GetReflectionProbe(ReflectionProbeID id) const;

	ReflectionProbeDesc* GetReflectionProbe(size_t index);
	const ReflectionProbeDesc* GetReflectionProbe(size_t index) const;

	D3D12ReflectionProbeSlot* GetReflectionProbeSlot(size_t index);
	const D3D12ReflectionProbeSlot* GetReflectionProbeSlot(size_t index) const;

	ProbeSampleResult FindBestProbe(const Mat4& world) const;

	bool BakeReflectionProbes(
		D3D12FrameResource* const pFrameResource,
		const std::vector<struct D3D12RenderItem*>& staticRitems,
		const std::vector<struct D3D12RenderItem*>& skySphereRitems);

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

	bool BakeReflectionProbesWithStatics(
		D3D12FrameResource* const pFrameResource,
		const std::vector<struct D3D12RenderItem*>& ritems);
	bool BakeReflectionProbesWithSkySphere(
		D3D12FrameResource* const pFrameResource,
		const std::vector<struct D3D12RenderItem*>& ritems);

	bool DrawStaticRenderItems(
		D3D12FrameResource* const pFrameResource,
		ID3D12GraphicsCommandList6* const pCmdList,
		const std::vector<D3D12RenderItem*>& ritems);
	bool DrawSkySphereRenderItems(
		D3D12FrameResource* const pFrameResource,
		ID3D12GraphicsCommandList6* const pCmdList,
		const std::vector<D3D12RenderItem*>& ritems);

	bool ConvoluteDiffuseIrradiance(
		D3D12FrameResource* const pFrameResource);
	bool ConvoluteSpecularIrradiance(
		D3D12FrameResource* const pFrameResource);

	bool BuildResources();
	bool BuildDescriptors();

	bool BuildReflectionProbeResources(
		D3D12ReflectionProbeSlot* pSlot, 
		const ReflectionProbeDesc& desc,
		std::uint32_t slot);

	bool Contains(const ReflectionProbeDesc& probe, const Vec3& pos) const;
	float CalcScore(const ReflectionProbeDesc& probe, const Vec3& pos) const;

	bool ContainsBox(const ReflectionProbeDesc& probe, const Vec3& pos) const;
	bool ContainsSphere(const ReflectionProbeDesc& probe, const Vec3& pos) const;

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

	std::vector<D3D12DescriptorHeap::DescriptorAllocation> mhReflectionProbeCapturedCubeSrvs;
	std::vector<D3D12DescriptorHeap::DescriptorAllocation> mhReflectionProbeCapturedCubeRtvs;

	std::vector<D3D12DescriptorHeap::DescriptorAllocation> mhReflectionProbeDiffuseIrradianceSrvs;
	std::vector<D3D12DescriptorHeap::DescriptorAllocation> mhReflectionProbeDiffuseIrradianceRtvs;

	std::vector<D3D12DescriptorHeap::DescriptorAllocation> mhReflectionProbeSpecularIrradianceSrvs;
	std::vector<D3D12DescriptorHeap::DescriptorAllocation> mhReflectionProbeSpecularIrradianceRtvs[5];
};

#include "D3D12EnvironmentManager.inl"

REGISTER_RENDER_PASS(D3D12EnvironmentManager);