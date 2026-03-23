#include "pch.h"
#include "Renderer/D3D12/D3D12Renderer.hpp"

#include "Renderer/D3D12/D3D12Util.hpp"
#include "Renderer/D3D12/D3D12Device.hpp"
#include "Renderer/D3D12/D3D12CommandObject.hpp"
#include "Renderer/D3D12/D3D12SwapChain.hpp"
#include "Renderer/D3D12/D3D12GpuResource.hpp"
#include "Renderer/D3D12/D3D12FrameResource.hpp"

#include "EditorManager.hpp"
#include "Renderer/D3D12/D3D12ShaderManager.hpp"
#include "Renderer/D3D12/D3D12RenderPassManager.hpp"

#include "Renderer/D3D12/D3D12RenderPasses.hpp"

#include "AMesh.hpp"

using namespace DirectX;

D3D12Renderer::D3D12Renderer() 
	: mFrameResources{}
	, mpCurrentFrameResource{}
	, mCurrentFrameResourceIndex{}
	, mhImGuiSrv{} {
	mDefaultMaterialData = {
		.Albedo = Vec3(1.f),
		.Roughness = 0.5f,
		.Specular = Vec3(0.08f),
		.Metalness = 0.0f
	};
}

D3D12Renderer::~D3D12Renderer() {}

bool D3D12Renderer::Initialize(
	HWND hMainWnd
	, unsigned width
	, unsigned height) {
	CheckReturn(D3D12LowRenderer::Initialize(hMainWnd, width, height));

	CheckReturn(BuildFrameResources());

	mShaderManager = std::make_unique<D3D12ShaderManager>();
	CheckReturn(mShaderManager->Initialize());

	CheckReturn(InitializeRenderPasses());
	
	CheckReturn(mCommandObject->FlushCommandQueue());
	
	return true;
}

bool D3D12Renderer::Update(float deltaTime) {
	mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) 
		% D3D12FrameResource::NumFrameResources;
	mpCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();
	
	CheckReturn(mCommandObject->WaitCompletion(mpCurrentFrameResource->mFence));
	CheckReturn(mpCurrentFrameResource->ResetCommandListAllocator());

	// Clean up completed texture upload buffers
	if (!mPendingUploads.empty()) {
		const UINT64 completed = mCommandObject->GetCompletedFenceValue();
		for (auto it = mPendingUploads.begin(); it != mPendingUploads.end();) {
			if (it->FenceValue <= completed) {
				CheckReturn(it->Callback());

				it = mPendingUploads.erase(it);
			}
			else {
				++it;
			}
		}
	}

	CheckReturn(UpdateConstantBuffers());

	return true;
}

bool D3D12Renderer::Draw() {
	CheckReturn(DrawScene());
	CheckReturn(DrawEditor());

	CheckReturn(PresentAndSignal());

	return true;
}

bool D3D12Renderer::OnResize(unsigned width, unsigned height) {
	CheckReturn(D3D12LowRenderer::OnResize(width, height));

	return true;
}

bool D3D12Renderer::LoadTexture(const std::wstring& filePath, const std::wstring& key) {
	CheckReturn(mCommandObject->ResetDirectCommandList(
		mpCurrentFrameResource->CommandAllocator(),
		nullptr));

	const auto CmdList = mCommandObject->GetDirectCommandList();
	mDescriptorHeap->SetDescriptorHeap(CmdList);

	D3D12Texture texture{};
	CheckReturn(D3D12Texture::LoadTextureFromFile(
		mDevice->GetD3DDevice(),
		CmdList,
		filePath,
		texture));

	D3D12DescriptorHeap::DescriptorAllocation alloc{};
	CheckReturn(mDescriptorHeap->AllocateCbvSrvUav(1, alloc));

	CheckReturn(D3D12Texture::BuildTextureShaderResourceView(
		mDevice->GetD3DDevice(),
		texture,
		mDescriptorHeap->GetCpuHandle(alloc)));

	// Store the texture so its UploadBuffer remains alive until the GPU finishes
	// the copy. We will track the fence value and remove the UploadBuffer later.
	mTextures[key] = { alloc, std::move(texture) };

	CheckReturn(mCommandObject->ExecuteDirectCommandList());

	// Record the fence value at which the GPU will have finished this upload.
	const UINT64 fenceValue = mCommandObject->IncreaseFence();
	CheckReturn(mCommandObject->Signal());

	mPendingUploads.push_back({ fenceValue, [&, key]() -> bool { 
		auto mapIt = mTextures.find(key);
		if (mapIt != mTextures.end()) {
			// Release upload buffer to free memory
			mapIt->second.second.ReleaseUploadBuffer();
		}
		return true;
	} });

	return true;
}

bool D3D12Renderer::AddMesh(const std::wstring& key, class AMesh* pMesh) {
	const UINT VerticesByteSize = pMesh->VerticesByteSize();
	const UINT IndicesByteSize = pMesh->IndicesByteSize();

	const auto Vertices = pMesh->Vertices();
	const auto Indices = pMesh->Indices();

	D3D12MeshData data{};

	CheckHResult(D3DCreateBlob(VerticesByteSize, &data.VertexBufferCPU));
	CopyMemory(data.VertexBufferCPU->GetBufferPointer(), Vertices, VerticesByteSize);

	CheckHResult(D3DCreateBlob(IndicesByteSize, &data.IndexBufferCPU));
	CopyMemory(data.IndexBufferCPU->GetBufferPointer(), Indices, IndicesByteSize);

	{
		CheckReturn(mCommandObject->ResetDirectCommandList(
			mpCurrentFrameResource->CommandAllocator(),
			0));

		const auto CmdList = mCommandObject->GetDirectCommandList();

		CheckReturn(D3D12Util::CreateDefaultBuffer(
			mDevice.get(),
			CmdList,
			Vertices,
			VerticesByteSize,
			data.VertexBufferUploader,
			data.VertexBufferGPU));

		CheckReturn(D3D12Util::CreateDefaultBuffer(
			mDevice.get(),
			CmdList,
			Indices,
			IndicesByteSize,
			data.IndexBufferUploader,
			data.IndexBufferGPU));

		CheckReturn(mCommandObject->ExecuteDirectCommandList());

		const UINT64 fenceValue = mCommandObject->IncreaseFence();
		CheckReturn(mCommandObject->Signal());

		mpCurrentFrameResource->mFence = fenceValue;
		data.Fence = fenceValue;

		mPendingUploads.push_back({ fenceValue, [&, cap_key = std::wstring(key)]() -> bool {
			auto mapIt = mMeshes.find(cap_key);
			if (mapIt != mMeshes.end()) {
				// Release upload buffer to free memory
				mapIt->second.ReleaseUploadBuffers();
			}
			return true;
		} });
	}

	data.VertexByteStride = static_cast<UINT>(sizeof(Vertex));
	data.VertexBufferByteSize = VerticesByteSize;
	data.IndexFormat = DXGI_FORMAT_R32_UINT;
	data.IndexBufferByteSize = IndicesByteSize;
	data.IndexByteStride = sizeof(std::uint32_t);

	mMeshes[key] = std::move(data);

	return true;
}

bool D3D12Renderer::AddRenderItem(
	const std::wstring& key
	, const std::wstring& meshKey
	, const std::wstring& matKey) {
	const auto meshIter = mMeshes.find(meshKey);
	if (meshIter == mMeshes.end()) ReturnFalse("Mesh not found");

	D3D12RenderItem ritem{};
	ritem.NumFramesDirty = D3D12FrameResource::NumFrameResources;
	ritem.ObjectCBIndex = mRenderItemIndexCounter++;

	ritem.MeshData = &meshIter->second;

	const auto matIter = mMaterials.find(matKey);
	if (matIter != mMaterials.end()) 
		ritem.MaterialData = &matIter->second;
	else 
		ritem.MaterialData = &mDefaultMaterialData;

	ritem.PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	ritem.IndexCount = meshIter->second.IndexBufferByteSize / meshIter->second.IndexByteStride;
	ritem.StartIndexLocation = 0;
	ritem.BaseVertexLocation = 0;

	mRenderItems[key] = std::move(ritem);

	return true;
}

bool D3D12Renderer::UpdateRenderItemMesh(const std::wstring& key, const std::wstring& meshKey) {
	const auto iter = mRenderItems.find(key);
	if (iter == mRenderItems.end()) ReturnFalse("Render item not found");

	auto& ritem = iter->second;

	ritem.NumFramesDirty = D3D12FrameResource::NumFrameResources;

	const auto meshIter = mMeshes.find(meshKey);
	if (meshIter == mMeshes.end()) ReturnFalse("Mesh not found");

	ritem.MeshData = &meshIter->second;

	ritem.PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	ritem.IndexCount = meshIter->second.IndexBufferByteSize / meshIter->second.IndexByteStride;
	ritem.StartIndexLocation = 0;
	ritem.BaseVertexLocation = 0;

	return true;
}

bool D3D12Renderer::UpdateRenderItemMaterial(const std::wstring& key, const std::wstring& matKey) {
	return true;
}

bool D3D12Renderer::AllocateImGuiSrv(
	D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle
	, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle) {
	if (!mDescriptorHeap->AllocateCbvSrvUav(1, mhImGuiSrv))
		return false;

	*outCpuHandle = mDescriptorHeap->GetCpuHandle(mhImGuiSrv);
	*outGpuHandle = mDescriptorHeap->GetGpuHandle(mhImGuiSrv);

	return true;
}

void D3D12Renderer::FreeImGuiSrv(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle) {
	mDescriptorHeap->Free(mhImGuiSrv);
}

void D3D12Renderer::ImGuiSrvAlloc(
	ImGui_ImplDX12_InitInfo* info
	, D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle
	, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle) {
	if (!RENDERER->AllocateImGuiSrv(outCpuHandle, outGpuHandle)) {
		outCpuHandle->ptr = 0;
		outGpuHandle->ptr = 0;
	}
}

void D3D12Renderer::ImGuiSrvFree(
	ImGui_ImplDX12_InitInfo* info
	, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle
	, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle) {
	RENDERER->FreeImGuiSrv(cpuHandle);
}

void D3D12Renderer::BuildDX12InitInfo(ImGui_ImplDX12_InitInfo& outInitInfo) const {
	outInitInfo = {};
	outInitInfo.Device = mDevice->GetD3DDevice();
	outInitInfo.CommandQueue = mCommandObject->GetCommandQueue();
	outInitInfo.NumFramesInFlight = D3D12SwapChain::SwapChainBufferCount;
	outInitInfo.RTVFormat = SwapChain::BackBufferFormat;
	outInitInfo.DSVFormat = DepthStencilBuffer::DepthStencilBufferFormat;
	outInitInfo.SrvDescriptorHeap = mDescriptorHeap->GetCbvSrvUavHeap();
	outInitInfo.SrvDescriptorAllocFn = &D3D12Renderer::ImGuiSrvAlloc;
	outInitInfo.SrvDescriptorFreeFn = &D3D12Renderer::ImGuiSrvFree;
}

ID3D12GraphicsCommandList* D3D12Renderer::GetCommandList() const noexcept {
	return mCommandObject->GetDirectCommandList();
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Renderer::GetSceneMapSrv() const {
	return mSwapChain->GetSceneMapSrv();
}

bool D3D12Renderer::BuildFrameResources() {
	for (UINT i = 0; i < D3D12FrameResource::NumFrameResources; i++) {
		mFrameResources.push_back(std::make_unique<D3D12FrameResource>());

		CheckReturn(mFrameResources.back()->Initialize(
			mDevice.get(), 1, 256, 128));
	}

	mCurrentFrameResourceIndex = 0;
	mpCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();

	return true;
}

bool D3D12Renderer::InitializeRenderPasses() {
	{
		auto gbuffer = RENDER_PASS_MANAGER->Get<D3D12GBuffer>();

		D3D12GBuffer::InitData initData{
			.Device = mDevice.get(),
			.CommandObject = mCommandObject.get(),
			.Width = static_cast<UINT>(mSwapChain->GetScreenViewport().Width),
			.Height = static_cast<UINT>(mSwapChain->GetScreenViewport().Height)
		};

		gbuffer->Initialize(mDescriptorHeap.get(), &initData);
	}

	CheckReturn(RENDER_PASS_MANAGER->CompileShaders(mShaderManager.get()));
	CheckReturn(RENDER_PASS_MANAGER->BuildRootSignatures());
	CheckReturn(RENDER_PASS_MANAGER->BuildPipelineStates());
	CheckReturn(RENDER_PASS_MANAGER->AllocateDescriptors());

	return true;
}

bool D3D12Renderer::UpdateConstantBuffers() {
	CheckReturn(UpdatePassCB());
	CheckReturn(UpdateObjectCB());
	CheckReturn(UpdateMaterialCB());

	return true;
}

bool D3D12Renderer::UpdatePassCB() {
	return true;
}

bool D3D12Renderer::UpdateObjectCB() {
	for (auto& pair : mRenderItems) {
		auto& ritem = pair.second;
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if (ritem.NumFramesDirty > 0) {
			const XMMATRIX PrevWorld = XMLoadFloat4x4(&ritem.PrevWorld);
			const XMMATRIX World = XMLoadFloat4x4(&ritem.World);
			const XMMATRIX TexTransform = XMLoadFloat4x4(&ritem.TexTransform);

			ObjectCB objCB;

			XMStoreFloat4x4(&objCB.PrevWorld, XMMatrixTranspose(PrevWorld));
			XMStoreFloat4x4(&objCB.World, XMMatrixTranspose(World));
			XMStoreFloat4x4(&objCB.TexTransform, XMMatrixTranspose(TexTransform));

			mpCurrentFrameResource->ObjectCB.CopyCB(objCB, ritem.ObjectCBIndex);

			// Next FrameResource need to be updated too.
			--ritem.NumFramesDirty;
		}
	}

	return true;
}

bool D3D12Renderer::UpdateMaterialCB() {
	return true;
}

bool D3D12Renderer::DrawScene() {
	CheckReturn(mCommandObject->ResetDirectCommandList(
		mpCurrentFrameResource->CommandAllocator(),
		nullptr));

	const auto CmdList = mCommandObject->GetDirectCommandList();
	mDescriptorHeap->SetDescriptorHeap(CmdList);

	auto rtv = mSwapChain->GetSceneMapRtv();
	CmdList->OMSetRenderTargets(1, &rtv, TRUE, nullptr);

	CheckReturn(mCommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12Renderer::DrawEditor() {
	CheckReturn(mCommandObject->ResetDirectCommandList(
		mpCurrentFrameResource->CommandAllocator(),
		nullptr));

	const auto CmdList = mCommandObject->GetDirectCommandList();
	mDescriptorHeap->SetDescriptorHeap(CmdList);

	CmdList->RSSetViewports(1, &mSwapChain->GetScreenViewport());
	CmdList->RSSetScissorRects(1, &mSwapChain->GetScissorRect());

	mSwapChain->GetCurrentBackBuffer()->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

	const auto rtv = mSwapChain->GetCurrentBackBufferRtv();

	FLOAT clearValues[4] = { 0.1f, 0.1f, 0.1f, 1.f };
	CmdList->ClearRenderTargetView(rtv, clearValues, 0, nullptr);
	CmdList->OMSetRenderTargets(1, &rtv, TRUE, nullptr);

	EditorManager::GetInstance()->Draw();

	CheckReturn(mCommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12Renderer::PresentAndSignal() {
	CheckReturn(mSwapChain->ReadyToPresent(mpCurrentFrameResource));
	CheckReturn(mSwapChain->Present(mDevice->IsAllowingTearing()));
	mSwapChain->NextBackBuffer();

	mpCurrentFrameResource->mFence = mCommandObject->IncreaseFence();

	CheckReturn(mCommandObject->Signal());

	return true;
}