#pragma once

#include "Renderer/D3D12/D3D12LowRenderer.hpp"

#include "Renderer/D3D12/D3D12DescriptorHeap.hpp"

#include "Renderer/D3D12/D3D12Texture.hpp"
#include "Renderer/D3D12/D3D12MeshData.hpp"
#include "Renderer/D3D12/D3D12MaterialData.h"
#include "Renderer/D3D12/D3D12RenderItem.hpp"

#include "Renderer/Interface/RenderVertex.hpp"

struct ImGui_ImplDX12_InitInfo;

class D3D12FrameResource;
class D3D12ShaderManager;

class D3D12Renderer : public D3D12LowRenderer, public Singleton<D3D12Renderer> {
	SINGLETON(D3D12Renderer);
	friend class D3D12RendererTests;

private:
	struct PendingUpload {
		UINT64 FenceValue;
		std::function<bool()> Callback;
	};

	struct CreateMeshRequest {
		// 정적 지오메트리
		std::vector<RenderVertex>			StaticVertices;
		std::vector<std::uint32_t>			StaticIndices;
		std::vector<RenderPrimitive>		StaticPrimitives;

		// 스키닝 지오메트리
		std::vector<RenderSkinnedVertex>	SkinnedVertices;
		std::vector<std::uint32_t>			SkinnedIndices;
		std::vector<RenderPrimitive>		SkinnedPrimitives;
	};

	enum RenderLayer {
		E_Static,
		E_Skinned,
		E_SkySphere,
		Count
	};

public:
	virtual bool Initialize(
		HWND hMainWnd,
		unsigned width, unsigned height) override;
	virtual void CleanUp() override;

	virtual bool Update(float deltaTime) override;
	virtual bool Draw() override;

protected:
	virtual bool Resize(unsigned width, unsigned height) override;

public:
	virtual bool AddTexture(const std::wstring& filePath, const std::wstring& key) override;
	virtual bool AddMesh(const std::wstring& key, const RenderMeshDesc& desc) override;

	virtual const std::wstring& GetGlobalDiffuseIrradianceMapPath() const override;
	virtual void SetGlobalDiffuseIrradianceMap(const std::wstring& key) override;
	
	virtual const std::wstring& GetGlobalSpecularIrradianceMapPath() const override;
	virtual void SetGlobalSpecularIrradianceMap(const std::wstring& key) override;

	virtual bool BakeReflectionProbes() override;
	virtual ReflectionProbeID AddReflectionProbe(const ReflectionProbeDesc& desc) override;
	virtual bool HasReflectionProbe(const ReflectionProbeID& id) const override;
	virtual void UpdateReflectionProbe(ReflectionProbeID id, const ReflectionProbeDesc& desc) override;
	virtual void RemoveReflectionProbe(const ReflectionProbeID& id) override;

	virtual void AddDebugColliderShape(const DebugColliderShape& shape) override;

public:
	bool AllocateImGuiSrv(
		D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle,
		D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle);
	void FreeImGuiSrv(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);

	static void ImGuiSrvAlloc(
		ImGui_ImplDX12_InitInfo* info,
		D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle,
		D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle);
	static void ImGuiSrvFree(
		ImGui_ImplDX12_InitInfo* info,
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);

	void BuildDX12InitInfo(ImGui_ImplDX12_InitInfo& outInitInfo) const;

public:
	ID3D12GraphicsCommandList* GetCommandList() const noexcept;

	D3D12_GPU_DESCRIPTOR_HANDLE GetSceneMapSrv() const;

private:
	bool BuildFrameResources();
																			
	bool InitializeRenderPasses();

	bool ProcessPendingUploads();
	bool CleanUpCompletedUploads();

	bool BuildRenderItems();

	bool UpdateConstantBuffers();
	bool UpdatePassCB();
	bool UpdateLightCB();
	bool UpdateGizmoCB();
	bool UpdateObjectCB();
	bool UpdateMaterialCB();
	bool UpdateProjectToCubeCB();
	bool UpdateAmbientOcclusionCB();
	bool UpdateCalcLocalMeanVarianceCB();
	bool UpdateBlendWithCurrentFrameCB();
	bool UpdateCrossBilateralFilterCB();
	bool UpdateAtrousWaveletTransformFilterCB();
	bool UpdateBoneSB();
	bool UpdateProbeSB();
	bool UpdateDebugLineVB();
	bool UpdateScreenSpaceReflectionCB();

	bool DrawScene();
	bool EvaluateDlss(GpuResource* velocityMap);
	bool DrawAO();
	bool ComputeBRDF();
	bool ApplyEyeAdaption();

	bool DrawEditor();

	bool PresentAndSignal();
	
private:
	Mat4 mDlssPreviousViewProj{};
	Vec2 mDlssJitter{};

	unsigned mFrameIndex{};
	bool mbResetDlssHistory{true};

	// 프레임 리소스
	std::vector<std::unique_ptr<D3D12FrameResource>> mFrameResources;
	D3D12FrameResource* mpCurrentFrameResource;
	UINT mCurrentFrameResourceIndex;

	D3D12DescriptorHeap::DescriptorAllocation mhImGuiSrv;

	std::vector<PendingUpload> mPendingUploads;

	std::unique_ptr<D3D12ShaderManager> mShaderManager;

	std::unordered_map<std::wstring, std::unique_ptr<D3D12Texture>> mTextures;
	std::unordered_map<std::wstring, std::unique_ptr<D3D12MeshData>> mStaticMeshes;
	std::unordered_map<std::wstring, std::unique_ptr<D3D12MeshData>> mSkinnedMeshes;

	std::unordered_map<std::wstring /* 키 */, DirectX::ScratchImage> mPendingTextureCreates;
	std::unordered_map<std::wstring /* 키 */, CreateMeshRequest> mPendingMeshCreates;

	std::vector<std::unique_ptr<D3D12RenderItem>> mRenderItems[RenderLayer::Count];
	UINT mObjectCBCount;

	std::vector<D3D12MaterialData> mMaterials;

	DirectX::BoundingSphere mSceneBounds;

	std::unordered_map<UINT, D3D12DescriptorHeap::DescriptorAllocation> mImGuiSrvAllocs;

	std::vector<Mat4> mCurrentFrameBonePalette;
	std::vector<Mat4> mPreviousFrameBonePalette;

	std::vector<DebugColliderShape> mDebugColliderShapes;
};

#ifndef RENDERER
#define RENDERER D3D12Renderer::GetInstance()
#endif
