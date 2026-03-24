#pragma once

#include "Renderer/D3D12/D3D12LowRenderer.hpp"

#include "Renderer/D3D12/D3D12DescriptorHeap.hpp"

#include "Renderer/D3D12/D3D12Texture.hpp"
#include "Renderer/D3D12/D3D12MeshData.hpp"
#include "Renderer/D3D12/D3D12MaterialData.h"
#include "Renderer/D3D12/D3D12RenderItem.hpp"

struct ImGui_ImplDX12_InitInfo;

class D3D12FrameResource;

class D3D12ShaderManager;

class D3D12Renderer : public D3D12LowRenderer, public Singleton<D3D12Renderer> {
	SINGLETON(D3D12Renderer);

private:
	struct PendingUpload {
		UINT64 FenceValue;
		std::function<bool()> Callback;
	};

public:
	virtual bool Initialize(
		HWND hMainWnd,
		unsigned width, unsigned height) override;

	virtual bool Update(float deltaTime) override;
	virtual bool Draw() override;

	virtual bool OnResize(unsigned width, unsigned height) override;

public:
	bool LoadTexture(const std::wstring& filePath, const std::wstring& key);

	bool AddMesh(const std::wstring& key, class AMesh* pMesh);

	bool AddRenderItem(
		const std::wstring& key, 
		const std::wstring& meshKey, 
		const std::wstring& matKey);
	bool UpdateRenderItemMesh(const std::wstring& key, const std::wstring& meshKey);
	bool UpdateRenderItemMaterial(const std::wstring& key, const std::wstring& matKey);

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

	bool UpdateConstantBuffers();
	bool UpdatePassCB();
	bool UpdateObjectCB();
	bool UpdateMaterialCB();

	bool DrawScene();
	bool DrawEditor();

	bool PresentAndSignal();
	
private:
	// Frame resources
	std::vector<std::unique_ptr<D3D12FrameResource>> mFrameResources;
	D3D12FrameResource* mpCurrentFrameResource;
	UINT mCurrentFrameResourceIndex;

	D3D12DescriptorHeap::DescriptorAllocation mhImGuiSrv;

	std::vector<PendingUpload> mPendingUploads;

	std::unique_ptr<D3D12ShaderManager> mShaderManager;

	std::unordered_map<std::wstring, 
		std::pair<D3D12DescriptorHeap::DescriptorAllocation, D3D12Texture>> mTextures;

	std::unordered_map<std::wstring, D3D12MaterialData> mMaterials;
	D3D12MaterialData mDefaultMaterialData;

	std::unordered_map<std::wstring, D3D12MeshData> mMeshes;

	std::unordered_map<std::wstring, std::unique_ptr<D3D12RenderItem>> mRenderItems;
	UINT mRenderItemIndexCounter;
};

#ifndef RENDERER
#define RENDERER D3D12Renderer::GetInstance()
#endif