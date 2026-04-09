#include "pch.h"
#include "Renderer/D3D12/D3D12Renderer.hpp"

#include "Renderer/D3D12/D3D12Util.hpp"
#include "Renderer/D3D12/D3D12Device.hpp"
#include "Renderer/D3D12/D3D12CommandObject.hpp"
#include "Renderer/D3D12/D3D12SwapChain.hpp"
#include "Renderer/D3D12/D3D12DepthStencilBuffer.hpp"
#include "Renderer/D3D12/D3D12GpuResource.hpp"
#include "Renderer/D3D12/D3D12FrameResource.hpp"

#include "Renderer/D3D12/D3D12ShaderManager.hpp"
#include "Renderer/D3D12/D3D12RenderPassManager.hpp"
#include "Renderer/D3D12/D3D12RenderPasses.hpp"

#include "EditorManager.hpp"
#include "LevelManager.hpp"
#include "ShaderArgumentManager.hpp"

#include "CTransform.hpp"

#include "AMesh.hpp"

using namespace DirectX;

D3D12Renderer::D3D12Renderer() 
	: mFrameResources{}
	, mpCurrentFrameResource{}
	, mCurrentFrameResourceIndex{}
	, mhImGuiSrv{} {
	mSceneBounds.Center = XMFLOAT3(0.f, 0.f, 0.f);
	const FLOAT WidthSquared = 16.f * 16.f;
	mSceneBounds.Radius = sqrtf(WidthSquared + WidthSquared);
}

D3D12Renderer::~D3D12Renderer() {}

bool D3D12Renderer::Initialize(
	HWND hMainWnd
	, unsigned width
	, unsigned height) {
	CheckReturn(D3D12LowRenderer::Initialize(hMainWnd, width, height));
	CheckReturn(EDITOR_MANAGER->Initialize());

	CheckReturn(BuildFrameResources());

	mShaderManager = std::make_unique<D3D12ShaderManager>();
	CheckReturn(mShaderManager->Initialize());

	CheckReturn(InitializeRenderPasses());
	
	CheckReturn(mCommandObject->FlushDirectCommand());

	auto environmentManager = RENDER_PASS_MANAGER->Get<D3D12EnvironmentManager>();
	CheckReturn(environmentManager->DrawBrdfLutMap(mpCurrentFrameResource));
		
	return true;
}

void D3D12Renderer::CleanUp() {
	mCommandObject->FlushCommandQueue();
}

bool D3D12Renderer::Update(float deltaTime) {
	mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) 
		% D3D12FrameResource::NumFrameResources;
	mpCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();
	
	CheckReturn(mCommandObject->WaitFrameCompletion(mpCurrentFrameResource->mFrameFence));
	CheckReturn(mpCurrentFrameResource->ResetFrameCommandListAllocator());

	CheckReturn(mCommandObject->WaitUploadCompletion(mpCurrentFrameResource->mUploadFence));
	CheckReturn(mpCurrentFrameResource->ResetUploadCommandListAllocator());

	CheckReturn(mCommandObject->WaitImmediateCompletion(mpCurrentFrameResource->mImmediateFence));
	CheckReturn(mpCurrentFrameResource->ResetImmediateCommandListAllocator());

	CheckReturn(ProcessPendingUploads());
	CheckReturn(CleanUpCompletedUploads());

	CheckReturn(RENDER_PASS_MANAGER->Update());
	CheckReturn(EDITOR_MANAGER->Update());

	CheckReturn(BuildRenderItems());
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

	CheckReturn(RENDER_PASS_MANAGER->OnResize(width, height));

	return true;
}

bool D3D12Renderer::AddTexture(const std::wstring& filePath, const std::wstring& key) {
	if (mTextures.contains(key))
		 ReturnFalse(std::format("key already exists: {}", WStrToStr(key)));
	if (mPendingTextureCreates.contains(key))
		ReturnFalse(std::format("key already exists in pending creates: {}", WStrToStr(key)));
	
	mPendingTextureCreates[key] = filePath;

	return true;
}

bool D3D12Renderer::AddMesh(const std::wstring& key, AMesh* pMesh) {
	if (mStaticMeshes.contains(key) || mSkinnedMeshes.contains(key))
		ReturnFalse(std::format("key already exists: {}", WStrToStr(key)));

	if (mPendingMeshCreates.contains(key))
		ReturnFalse(std::format("key already exists in pending creates: {}", WStrToStr(key)));

	CreateMeshRequest req{};
	req.StaticVertices.assign(
		pMesh->GetStaticVertices(), pMesh->GetStaticVertices() + pMesh->GetStaticVertexCount());
	req.StaticIndices.assign(
		pMesh->GetStaticIndices(), pMesh->GetStaticIndices() + pMesh->GetStaticIndexCount());
	req.StaticPrimitives = pMesh->GetStaticPrimitives();

	req.SkinnedVertices.assign(
		pMesh->GetSkinnedVertices(), pMesh->GetSkinnedVertices() + pMesh->GetSkinnedVertexCount());
	req.SkinnedIndices.assign(
		pMesh->GetSkinnedIndices(), pMesh->GetSkinnedIndices() + pMesh->GetSkinnedIndexCount());
	req.SkinnedPrimitives = pMesh->GetSkinnedPrimitives();

	mPendingMeshCreates.emplace(key, std::move(req));

	return true;
}

const std::wstring& D3D12Renderer::GetGlobalDiffuseIrradianceMapPath() const {
	const auto environmentManager = RENDER_PASS_MANAGER->Get<D3D12EnvironmentManager>();
	return environmentManager->GetGlobalDiffuseIrradianceMapPath();
}

void D3D12Renderer::SetGlobalDiffuseIrradianceMap(const std::wstring& key) {
	std::wstring path = key;

	if (!mTextures.contains(key)) {
		LOG_WARNING_FORMAT(
			"Missing texture for global diffuse irradiance map: {}", WStrToStr(key));
		return;
	}

	const auto environmentManager = RENDER_PASS_MANAGER->Get<D3D12EnvironmentManager>();
	environmentManager->SetGlobalDiffuseIrradianceMap(key, mTextures[key].get());
}

const std::wstring& D3D12Renderer::GetGlobalSpecularIrradianceMapPath() const {
	const auto environmentManager = RENDER_PASS_MANAGER->Get<D3D12EnvironmentManager>();
	return environmentManager->GetGlobalSpecularIrradianceMapPath();
}

void D3D12Renderer::SetGlobalSpecularIrradianceMap(const std::wstring& key) {
	std::wstring path = key;

	if (!mTextures.contains(path)) {
		LOG_WARNING_FORMAT(
			"Missing texture for global specular irradiance map: {}", WStrToStr(path));
		return;
	}

	const auto environmentManager = RENDER_PASS_MANAGER->Get<D3D12EnvironmentManager>();
	environmentManager->SetGlobalSpecularIrradianceMap(path, mTextures[path].get());
}

bool D3D12Renderer::BakeReflectionProbes() {
	const auto environmentManager = RENDER_PASS_MANAGER->Get<D3D12EnvironmentManager>();
	if (environmentManager->GetReflectionProbeCount() == 0) {
		//LOG_INFO("No reflection probes to bake.");
		return true;
	}

	std::vector<D3D12RenderItem*> staticRitems{};
	for (const auto& ritem : mRenderItems[D3D12Renderer::E_Static])
		staticRitems.push_back(ritem.get());

	std::vector<D3D12RenderItem*> skySphereRitems{};
	for (const auto& ritem : mRenderItems[D3D12Renderer::E_SkySphere])
		skySphereRitems.push_back(ritem.get());

	CheckReturn(environmentManager->BakeReflectionProbes(
		mpCurrentFrameResource, staticRitems, skySphereRitems));

	return true;
}

ReflectionProbeID D3D12Renderer::AddReflectionProbe(const ReflectionProbeDesc& desc) {
	return RENDER_PASS_MANAGER->Get<D3D12EnvironmentManager>()->AddReflectionProbe(desc);
}

void D3D12Renderer::UpdateReflectionProbe(ReflectionProbeID id, const ReflectionProbeDesc& desc) {
	RENDER_PASS_MANAGER->Get<D3D12EnvironmentManager>()->UpdateReflectionProbe(id, desc);
}

void D3D12Renderer::RemoveReflectionProbe(const ReflectionProbeID& id) {
	RENDER_PASS_MANAGER->Get<D3D12EnvironmentManager>()->RemoveReflectionProbe(id);
}

void D3D12Renderer::AddDebugColliderShape(const DebugColliderShape& shape) {
	mDebugColliderShapes.push_back(shape);
}

bool D3D12Renderer::AllocateImGuiSrv(
	D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle
	, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle) {
	D3D12DescriptorHeap::DescriptorAllocation alloc;
	if (!mDescriptorHeap->AllocateCbvSrvUav(1, alloc))
		return false;

	*outCpuHandle = mDescriptorHeap->GetCpuHandle(alloc);
	*outGpuHandle = mDescriptorHeap->GetGpuHandle(alloc);

	mImGuiSrvAllocs[static_cast<UINT>(outCpuHandle->ptr)] = alloc;

	return true;
}

void D3D12Renderer::FreeImGuiSrv(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle) {
	auto it = mImGuiSrvAllocs.find(static_cast<UINT>(cpuHandle.ptr));
	if (it != mImGuiSrvAllocs.end()) {
		mDescriptorHeap->Free(it->second);
		mImGuiSrvAllocs.erase(it);
	}
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
	outInitInfo.NumFramesInFlight = D3D12FrameResource::NumFrameResources;
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
	// GBuffer
	{
		auto gbuffer = RENDER_PASS_MANAGER->Get<D3D12GBuffer>();

		D3D12GBuffer::InitData initData{
			.Device = mDevice.get(),
			.CommandObject = mCommandObject.get(),
			.ShaderManager = mShaderManager.get(),
			.Width = static_cast<UINT>(mSwapChain->GetScreenViewport().Width),
			.Height = static_cast<UINT>(mSwapChain->GetScreenViewport().Height)
		};
		CheckReturn(gbuffer->Initialize(mDescriptorHeap.get(), &initData));
	}
	// BRDF
	{
		auto brdf = RENDER_PASS_MANAGER->Get<D3D12Brdf>();

		D3D12Brdf::InitData initData{
			.Device = mDevice.get(),
			.CommandObject = mCommandObject.get(),
			.ShaderManager = mShaderManager.get(),
			.Width = static_cast<UINT>(mSwapChain->GetScreenViewport().Width),
			.Height = static_cast<UINT>(mSwapChain->GetScreenViewport().Height)
		};
		CheckReturn(brdf->Initialize(mDescriptorHeap.get(), &initData));
	}
	// ToneMapping
	{
		auto toneMapping = RENDER_PASS_MANAGER->Get<D3D12ToneMapping>();
		D3D12ToneMapping::InitData initData{
			.Device = mDevice.get(),
			.CommandObject = mCommandObject.get(),
			.ShaderManager = mShaderManager.get(),
			.Width = static_cast<UINT>(mSwapChain->GetScreenViewport().Width),
			.Height = static_cast<UINT>(mSwapChain->GetScreenViewport().Height)
		};
		CheckReturn(toneMapping->Initialize(mDescriptorHeap.get(), &initData));
	}
	// GammaCorrection
	{
		auto gammaCorrection = RENDER_PASS_MANAGER->Get<D3D12GammaCorrection>();
		D3D12GammaCorrection::InitData initData{
			.Device = mDevice.get(),
			.CommandObject = mCommandObject.get(),
			.ShaderManager = mShaderManager.get(),
			.Width = static_cast<UINT>(mSwapChain->GetScreenViewport().Width),
			.Height = static_cast<UINT>(mSwapChain->GetScreenViewport().Height)
		};
		CheckReturn(gammaCorrection->Initialize(mDescriptorHeap.get(), &initData));
	}
	// Gizmo
	{
		auto gizmo = RENDER_PASS_MANAGER->Get<D3D12Gizmo>();
		D3D12Gizmo::InitData initData{
			.Device = mDevice.get(),
			.CommandObject = mCommandObject.get(),
			.ShaderManager = mShaderManager.get(),
			.Width = static_cast<UINT>(mSwapChain->GetScreenViewport().Width),
			.Height = static_cast<UINT>(mSwapChain->GetScreenViewport().Height)
		};
		CheckReturn(gizmo->Initialize(mDescriptorHeap.get(), &initData));
	}
	// Shadow
	{
		auto shadow = RENDER_PASS_MANAGER->Get<D3D12Shadow>();
		D3D12Shadow::InitData initData{
			.Device = mDevice.get(),
			.CommandObject = mCommandObject.get(),
			.ShaderManager = mShaderManager.get(),
			.Width = 2048,
			.Height = 2048
		};
		CheckReturn(shadow->Initialize(mDescriptorHeap.get(), &initData));
	}
	// TAA
	{
		auto taa = RENDER_PASS_MANAGER->Get<D3D12Taa>();
		D3D12Taa::InitData initData{
			.Device = mDevice.get(),
			.CommandObject = mCommandObject.get(),
			.ShaderManager = mShaderManager.get(),
			.Width = static_cast<UINT>(mSwapChain->GetScreenViewport().Width),
			.Height = static_cast<UINT>(mSwapChain->GetScreenViewport().Height)
		};
		CheckReturn(taa->Initialize(mDescriptorHeap.get(), &initData));
	}
	// TextureScaler
	{
		auto textureScaler = RENDER_PASS_MANAGER->Get<D3D12TextureScaler>();
		D3D12TextureScaler::InitData initData{
			.Device = mDevice.get(),
			.CommandObject = mCommandObject.get(),
			.ShaderManager = mShaderManager.get(),
			.Width = static_cast<UINT>(mSwapChain->GetScreenViewport().Width),
			.Height = static_cast<UINT>(mSwapChain->GetScreenViewport().Height)
		};
		CheckReturn(textureScaler->Initialize(mDescriptorHeap.get(), &initData));
	}
	// Bloom
	{
		auto bloom = RENDER_PASS_MANAGER->Get<D3D12Bloom>();
		D3D12Bloom::InitData initData{
			.Device = mDevice.get(),
			.CommandObject = mCommandObject.get(),
			.ShaderManager = mShaderManager.get(),
			.Width = static_cast<UINT>(mSwapChain->GetScreenViewport().Width),
			.Height = static_cast<UINT>(mSwapChain->GetScreenViewport().Height)
		};
		CheckReturn(bloom->Initialize(mDescriptorHeap.get(), &initData));
	}
	// BlurFilter
	{
		auto blurFilter = RENDER_PASS_MANAGER->Get<D3D12BlurFilter>();
		D3D12BlurFilter::InitData initData{
			.Device = mDevice.get(),
			.CommandObject = mCommandObject.get(),
			.ShaderManager = mShaderManager.get(),
			.Width = static_cast<UINT>(mSwapChain->GetScreenViewport().Width),
			.Height = static_cast<UINT>(mSwapChain->GetScreenViewport().Height)
		};
		CheckReturn(blurFilter->Initialize(mDescriptorHeap.get(), &initData));
	}
	// Vignette
	{
		auto vignette = RENDER_PASS_MANAGER->Get<D3D12Vignette>();
		D3D12Vignette::InitData initData{
			.Device = mDevice.get(),
			.CommandObject = mCommandObject.get(),
			.ShaderManager = mShaderManager.get(),
			.Width = static_cast<UINT>(mSwapChain->GetScreenViewport().Width),
			.Height = static_cast<UINT>(mSwapChain->GetScreenViewport().Height)
		};
		CheckReturn(vignette->Initialize(mDescriptorHeap.get(), &initData));
	}
	// EnvironmentManager
	{
		auto environmentManager = RENDER_PASS_MANAGER->Get<D3D12EnvironmentManager>();
		D3D12EnvironmentManager::InitData initData{
			.Device = mDevice.get(),
			.CommandObject = mCommandObject.get(),
			.ShaderManager = mShaderManager.get(),
			.Width = static_cast<UINT>(mSwapChain->GetScreenViewport().Width),
			.Height = static_cast<UINT>(mSwapChain->GetScreenViewport().Height)
		};
		CheckReturn(environmentManager->Initialize(mDescriptorHeap.get(), &initData));
	}
	// Debug
	{
		auto debug = RENDER_PASS_MANAGER->Get<D3D12Debug>();
		D3D12Debug::InitData initData{
			.Device = mDevice.get(),
			.CommandObject = mCommandObject.get(),
			.ShaderManager = mShaderManager.get(),
			.Width = static_cast<UINT>(mSwapChain->GetScreenViewport().Width),
			.Height = static_cast<UINT>(mSwapChain->GetScreenViewport().Height)
		};
		CheckReturn(debug->Initialize(mDescriptorHeap.get(), &initData));
	}

	CheckReturn(RENDER_PASS_MANAGER->CompileShaders(mShaderManager.get()));
	CheckReturn(RENDER_PASS_MANAGER->BuildRootSignatures());
	CheckReturn(RENDER_PASS_MANAGER->BuildPipelineStates());
	CheckReturn(RENDER_PASS_MANAGER->AllocateDescriptors());

	return true;
}

bool D3D12Renderer::ProcessPendingUploads() {
	if (mPendingTextureCreates.empty() && mPendingMeshCreates.empty())
		return true;

	CheckReturn(mCommandObject->ResetUploadCommandList(
		mpCurrentFrameResource->UploadCommandAllocator()));

	const auto CmdList = mCommandObject->GetUploadCommandList();
	CheckReturn(mDescriptorHeap->SetDescriptorHeap(CmdList));

	for (const auto& req : mPendingTextureCreates) {
		auto texture = std::make_unique<D3D12Texture>();
		CheckReturn(D3D12Texture::LoadTextureFromFile(
			mDevice->GetD3DDevice(),
			CmdList,
			req.second,
			texture.get()));

		CheckReturn(mDescriptorHeap->AllocateCbvSrvUav(1, texture->Allocation));

		CheckReturn(D3D12Texture::BuildTextureShaderResourceView(
			mDevice->GetD3DDevice(),
			texture.get(),
			mDescriptorHeap->GetCpuHandle(texture->Allocation)));

		// Store the texture so its UploadBuffer remains alive until the GPU finishes
		// the copy. We will track the fence value and remove the UploadBuffer later.
		mTextures[req.first] = std::move(texture);
	}

	for (const auto& req : mPendingMeshCreates) {
		auto mesh = req.second;

		// 정적 버텍스가 있다면 정적 버텍스와 인덱스 업로드
		if (mesh.StaticVertices.size() > 0) {
			const UINT VerticesByteSize =
				static_cast<UINT>(sizeof(Vertex) * mesh.StaticVertices.size());
			const UINT IndicesByteSize =
				static_cast<UINT>(sizeof(std::uint32_t) * mesh.StaticIndices.size());

			const auto Vertices = mesh.StaticVertices.data();
			const auto Indices = mesh.StaticIndices.data();

			auto data = std::make_unique<D3D12MeshData>();

			CheckHResult(D3DCreateBlob(VerticesByteSize, &data->VertexBufferCPU));
			CopyMemory(data->VertexBufferCPU->GetBufferPointer(), Vertices, VerticesByteSize);

			CheckHResult(D3DCreateBlob(IndicesByteSize, &data->IndexBufferCPU));
			CopyMemory(data->IndexBufferCPU->GetBufferPointer(), Indices, IndicesByteSize);

			CheckReturn(D3D12Util::CreateDefaultBuffer(
				mDevice.get(),
				CmdList,
				Vertices,
				VerticesByteSize,
				data->VertexBufferUploader,
				data->VertexBufferGPU));

			CheckReturn(D3D12Util::CreateDefaultBuffer(
				mDevice.get(),
				CmdList,
				Indices,
				IndicesByteSize,
				data->IndexBufferUploader,
				data->IndexBufferGPU));

			data->TotalVertexCount = static_cast<UINT>(mesh.StaticVertices.size());
			data->VertexByteStride = static_cast<UINT>(sizeof(Vertex));
			data->VertexBufferByteSize = VerticesByteSize;

			data->TotalIndexCount = static_cast<UINT>(mesh.StaticIndices.size());
			data->IndexBufferByteSize = IndicesByteSize;
			data->IndexByteStride = sizeof(std::uint32_t);
			data->IndexFormat = DXGI_FORMAT_R32_UINT;

			data->Primitives = mesh.StaticPrimitives;

			mStaticMeshes[req.first] = std::move(data);
		}
		// 스키닝 버텍스가 있다면 스키닝 버텍스와 인덱스 업로드
		if (mesh.SkinnedVertices.size() > 0) {
			const UINT VerticesByteSize = 
				static_cast<UINT>(sizeof(SkinnedVertex) * mesh.SkinnedVertices.size());
			const UINT IndicesByteSize = 
				static_cast<UINT>(sizeof(std::uint32_t) * mesh.SkinnedIndices.size());

			const auto Vertices = mesh.SkinnedVertices.data();
			const auto Indices = mesh.SkinnedIndices.data();
			auto data = std::make_unique<D3D12MeshData>();

			CheckHResult(D3DCreateBlob(VerticesByteSize, &data->VertexBufferCPU));
			CopyMemory(data->VertexBufferCPU->GetBufferPointer(), Vertices, VerticesByteSize);

			CheckHResult(D3DCreateBlob(IndicesByteSize, &data->IndexBufferCPU));
			CopyMemory(data->IndexBufferCPU->GetBufferPointer(), Indices, IndicesByteSize);

			CheckReturn(D3D12Util::CreateDefaultBuffer(
				mDevice.get(),
				CmdList,
				Vertices,
				VerticesByteSize,
				data->VertexBufferUploader,
				data->VertexBufferGPU));

			CheckReturn(D3D12Util::CreateDefaultBuffer(
				mDevice.get(),
				CmdList,
				Indices,
				IndicesByteSize,
				data->IndexBufferUploader,
				data->IndexBufferGPU));

			data->TotalVertexCount = static_cast<UINT>(mesh.SkinnedVertices.size());
			data->VertexByteStride = static_cast<UINT>(sizeof(SkinnedVertex));
			data->VertexBufferByteSize = VerticesByteSize;

			data->TotalIndexCount = static_cast<UINT>(mesh.SkinnedIndices.size());
			data->IndexBufferByteSize = IndicesByteSize;
			data->IndexByteStride = sizeof(std::uint32_t);
			data->IndexFormat = DXGI_FORMAT_R32_UINT;

			data->Primitives = mesh.SkinnedPrimitives;

			mSkinnedMeshes[req.first] = std::move(data);
		}
	}

	CheckReturn(mCommandObject->ExecuteUploadCommandList());

	const UINT64 fenceValue = mCommandObject->SignalUpload();
	mpCurrentFrameResource->mUploadFence = fenceValue;

	// 방금 만든 리소스들의 upload buffer release 예약
	for (auto& req : mPendingTextureCreates) {
		const auto key = req.first;
		mPendingUploads.push_back({
			fenceValue,
			[this, key]() -> bool {
				auto it = mTextures.find(key);
				if (it != mTextures.end())
					it->second->ReleaseUploadBuffer();
				return true;
			}
		});
	}

	for (auto& req : mPendingMeshCreates) {
		const auto key = req.first;
		mPendingUploads.push_back({
			fenceValue,
			[this, key]() -> bool {
				{
					auto it = mStaticMeshes.find(key);
					if (it != mStaticMeshes.end())
						it->second->ReleaseUploadBuffers();
				}
				{
					auto it = mSkinnedMeshes.find(key);
					if (it != mSkinnedMeshes.end())
						it->second->ReleaseUploadBuffers();
				}
				return true;
			}
		});
	}

	mPendingTextureCreates.clear();
	mPendingMeshCreates.clear();

	return true;
}

bool D3D12Renderer::CleanUpCompletedUploads() {
	// Clean up completed texture upload buffers
	if (!mPendingUploads.empty()) {
		const UINT64 completed = mCommandObject->GetCompletedUploadFenceValue();
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

	return true;
}

bool D3D12Renderer::BuildRenderItems() {
	auto camera = GetActiveCamera();
	if (camera == nullptr) return true;

	camera->SortRenderObjects();

	mMaterials.clear();

	// 이전 프레임의 본 팔레트를 지우기 전에 미리 저장
	mPreviousFrameBonePalette = mCurrentFrameBonePalette;
	mCurrentFrameBonePalette.clear();

	for (auto& renderItemLayer : mRenderItems) 
		renderItemLayer.clear();
	mObjectCBCount = 0;

	decltype(auto) opaques = camera->GetRenderDomainObjects(ERenderDomain::E_Opaque);
	for (const auto& opaque : opaques) {
		auto transform = opaque.Object->Transform();
		auto renderComponent = opaque.Object->GetRenderComponent();

		const auto staticIter = mStaticMeshes.find(renderComponent->GetMesh()->GetKey());
		const auto skinnedIter = mSkinnedMeshes.find(renderComponent->GetMesh()->GetKey());

		const auto staticFound = staticIter != mStaticMeshes.end();
		const auto skinnedFound = skinnedIter != mSkinnedMeshes.end();
		if (!staticFound && !skinnedFound) ReturnFalse("Mesh not found");

		auto materialIndex = opaque.SourcePrimitiveIndex;
		auto staticIndex = opaque.StaticPrimitiveIndex;
		auto skinnedIndex = opaque.SkinnedPrimitiveIndex;

		if (staticFound && staticIndex >= 0) {
			const auto meshData = staticIter->second.get();

			auto& staticRitems = mRenderItems[RenderLayer::E_Static];

			auto objCBIndex = mObjectCBCount++;
			auto matCBIndex = static_cast<int>(mMaterials.size());

			D3D12MaterialData matData{
				.MaterialCBIndex = matCBIndex,
				.Albedo = renderComponent->GetAlbedo(materialIndex),
				.Roughness = renderComponent->GetRoughness(materialIndex),
				.Metalness = renderComponent->GetMetalic(materialIndex),
				.Specular = renderComponent->GetSpecular(materialIndex),
			};

			mMaterials.push_back(matData);

			auto mat = renderComponent->GetMaterial(materialIndex);
			auto albedoMap = (mat != nullptr) ? mat->GetAlbedoMap() : nullptr;
			auto normalMap = (mat != nullptr) ? mat->GetNormalMap() : nullptr;

			const auto& drawPrimitive = meshData->Primitives[staticIndex];
			const auto& srcPrimitive = renderComponent->GetMesh()->GetMeshPrimitives()[materialIndex];

			Mat4 finalWorld = transform->GetWorldMatrix();
			Mat4 finalPrevWorld = transform->GetPrevWorldMatrix();

			auto skeletalMeshRender = opaque.Object->SkeletalMeshRender();
			if (skeletalMeshRender != nullptr && srcPrimitive.NodeIndex >= 0) {
				const Mat4& nodeGlobal = skeletalMeshRender->GetNodeGlobalPose(srcPrimitive.NodeIndex);
				finalWorld = nodeGlobal * transform->GetWorldMatrix(); 

				const Mat4& nodePrevGlobal = skeletalMeshRender->GetPrevNodeGlobalPose(srcPrimitive.NodeIndex);
				finalPrevWorld = nodePrevGlobal * transform->GetPrevWorldMatrix();
			}

			auto ritem = std::make_unique<D3D12RenderItem>();
			ritem->ObjectCBIndex = objCBIndex;
			ritem->MaterialCBIndex = matCBIndex;
			ritem->AlbedoMap = (albedoMap != nullptr) ? mTextures[albedoMap->GetKey()].get() : nullptr;
			ritem->NormalMap = (normalMap != nullptr) ? mTextures[normalMap->GetKey()].get() : nullptr;
			ritem->MeshData = staticIter->second.get();
			ritem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			ritem->World = finalWorld;
			ritem->PrevWorld = finalPrevWorld;
			ritem->IndexCount = drawPrimitive.IndexCount;
			ritem->StartIndexLocation = drawPrimitive.StartIndexLocation;
			ritem->BaseVertexLocation = drawPrimitive.BaseVertexLocation;

			staticRitems.push_back(std::move(ritem));
		}

		if (skinnedFound && skinnedIndex >= 0) {
			const auto meshData = skinnedIter->second.get();
			auto& skinnedRitems = mRenderItems[RenderLayer::E_Skinned];

			auto objCBIndex = mObjectCBCount++;
			auto matCBIndex = static_cast<int>(mMaterials.size());

			D3D12MaterialData matData{
				.MaterialCBIndex = matCBIndex,
				.Albedo = renderComponent->GetAlbedo(materialIndex),
				.Roughness = renderComponent->GetRoughness(materialIndex),
				.Metalness = renderComponent->GetMetalic(materialIndex),
				.Specular = renderComponent->GetSpecular(materialIndex),
			};
			mMaterials.push_back(matData);

			auto mat = renderComponent->GetMaterial(materialIndex);
			auto albedoMap = (mat != nullptr) ? mat->GetAlbedoMap() : nullptr;
			auto normalMap = (mat != nullptr) ? mat->GetNormalMap() : nullptr;

			const auto& drawPrimitive = meshData->Primitives[skinnedIndex];
			const auto& srcPrimitive = renderComponent->GetMesh()->GetMeshPrimitives()[materialIndex];

			auto ritem = std::make_unique<D3D12RenderItem>();
			ritem->ObjectCBIndex = objCBIndex;
			ritem->MaterialCBIndex = matCBIndex;
			ritem->AlbedoMap = (albedoMap != nullptr) ? mTextures[albedoMap->GetKey()].get() : nullptr;
			ritem->NormalMap = (normalMap != nullptr) ? mTextures[normalMap->GetKey()].get() : nullptr;
			ritem->MeshData = skinnedIter->second.get();
			ritem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			ritem->World = transform->GetWorldMatrix();
			ritem->PrevWorld = transform->GetPrevWorldMatrix();
			ritem->IndexCount = drawPrimitive.IndexCount;
			ritem->StartIndexLocation = drawPrimitive.StartIndexLocation;
			ritem->BaseVertexLocation = drawPrimitive.BaseVertexLocation;
			ritem->SkinIndex = srcPrimitive.SkinIndex;

			auto skeletalMeshRender = opaque.Object->SkeletalMeshRender();
			if (skeletalMeshRender == nullptr)
				ReturnFalse("Skinned primitive found, but SkeletalMeshRender is null.");

			const auto& palette = skeletalMeshRender->GetPalette(srcPrimitive.SkinIndex);
			//if (palette.empty())
			//	ReturnFalse(std::format("Palette not found for skin {}", srcPrimitive.SkinIndex));

			ritem->BonePaletteOffset = static_cast<UINT>(mCurrentFrameBonePalette.size());

			mCurrentFrameBonePalette.insert(
				mCurrentFrameBonePalette.end(),
				palette.begin(),
				palette.end());

			// 첫 프레임에는 이전 프레임 본 팔레트가 없으므로 현재 프레임 본 팔레트를 이전 프레임 본 팔레트로 복사
			if (mPreviousFrameBonePalette.size() != mCurrentFrameBonePalette.size()) 
				mPreviousFrameBonePalette = mCurrentFrameBonePalette;

			skinnedRitems.push_back(std::move(ritem));
		}
	}

	decltype(auto) skySpheres = camera->GetRenderDomainObjects(ERenderDomain::E_SkySphere);
	for (const auto& skySphere : skySpheres) {
		auto transform = skySphere.Object->Transform();
		auto renderComponent = skySphere.Object->SkySphereRender();

		const auto staticIter = mStaticMeshes.find(renderComponent->GetMesh()->GetKey());
				
		const auto meshData = staticIter->second.get();
		const auto& drawPrimitive = meshData->Primitives[skySphere.StaticPrimitiveIndex];

		auto objCBIndex = mObjectCBCount++;

		auto ritem = std::make_unique<D3D12RenderItem>();
		ritem->ObjectCBIndex = objCBIndex;
		ritem->MaterialCBIndex = -1;
		ritem->EnvironmentMap = renderComponent->GetEnvironmentCubeMap() != nullptr
			? mTextures[renderComponent->GetEnvironmentCubeMap()->GetKey()].get()
			: nullptr;
		ritem->MeshData = staticIter->second.get();
		ritem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		ritem->World = transform->GetWorldMatrix();
		ritem->PrevWorld = transform->GetPrevWorldMatrix();
		ritem->IndexCount = drawPrimitive.IndexCount;
		ritem->StartIndexLocation = drawPrimitive.StartIndexLocation;
		ritem->BaseVertexLocation = drawPrimitive.BaseVertexLocation;

		auto& skySphereRitems = mRenderItems[RenderLayer::E_SkySphere];
		skySphereRitems.push_back(std::move(ritem));
	}

	return true;
}

bool D3D12Renderer::UpdateConstantBuffers() {
	CheckReturn(UpdatePassCB());
	CheckReturn(UpdateLightCB());
	CheckReturn(UpdateGizmoCB());
	CheckReturn(UpdateObjectCB());
	CheckReturn(UpdateMaterialCB());
	CheckReturn(UpdateBoneSB());
	CheckReturn(UpdateProjectToCubeCB());
	CheckReturn(UpdateProbeSB());
	CheckReturn(UpdateDebugLineVB());

	return true;
}

bool D3D12Renderer::UpdatePassCB() {
	auto camera = GetActiveCamera();
	if (camera == nullptr) return true;

	static PassCB passCB{
		.ViewProj = Identity4x4
	};

	// Transform NDC space [-1 , +1]^2 to texture space [0, 1]^2
	const XMMATRIX T(
		0.5f, 0.f, 0.f, 0.f,
		0.f, -0.5f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.5f, 0.5f, 0.f, 1.f
	);

	const XMMATRIX view = XMLoadFloat4x4(&camera->GetViewMatrix());
	const XMMATRIX proj = XMLoadFloat4x4(&camera->GetProjMatrix());
	const XMMATRIX viewProj = XMMatrixMultiply(view, proj);

	auto viewDet = XMMatrixDeterminant(view);
	const XMMATRIX invView = XMMatrixInverse(&viewDet, view);

	auto projDet = XMMatrixDeterminant(proj);
	const XMMATRIX invProj = XMMatrixInverse(&projDet, proj);

	auto viewProjDet = XMMatrixDeterminant(viewProj);
	const XMMATRIX invViewProj = XMMatrixInverse(&viewProjDet, viewProj);

	const XMMATRIX viewProjTex = XMMatrixMultiply(viewProj, T);

	passCB.PrevViewProj = passCB.ViewProj;
	XMStoreFloat4x4(&passCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&passCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&passCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&passCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&passCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&passCB.InvViewProj, XMMatrixTranspose(invViewProj));
	XMStoreFloat4x4(&passCB.ViewProjTex, XMMatrixTranspose(viewProjTex));
	XMStoreFloat3(&passCB.EyePosW, camera->GetCameraPosition());

	if (SHADER_ARGUMENT_MANAGER->TAA.Enabled) {
		const auto taa = RENDER_PASS_MANAGER->Get<D3D12Taa>();

		const auto OffsetIndex = static_cast<UINT>(
			mCommandObject->GetCompletedFrameFenceValue() % D3D12Taa::HaltonSequenceSize);
		passCB.JitteredOffset = taa->HaltonSequence(OffsetIndex);
	}
	else {
		passCB.JitteredOffset = { 0.f, 0.f };
	}

	mpCurrentFrameResource->PassCB.CopyCB(passCB);

	return true;
}

bool D3D12Renderer::UpdateLightCB() {
	static LightCB ligthCB{};

	const auto LightCount = LEVEL_MANAGER->GetLightCount();

	ligthCB.LightCount = LightCount;

	const XMMATRIX T(
		0.5f, 0.f, 0.f, 0.f,
		0.f, -0.5f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.5f, 0.5f, 0.f, 1.f
	);

	for (UINT i = 0, idx = 0; i < LightCount; ++i) {
		auto light = const_cast<LightData*>(LEVEL_MANAGER->GetLightData(i));

		if (light->Type == ELight::E_Directional) {
			const XMVECTOR lightDir = XMLoadFloat3(&light->Direction);
			const XMVECTOR lightPos = -2.f * mSceneBounds.Radius * lightDir;
			const XMVECTOR targetPos = XMLoadFloat3(&mSceneBounds.Center);
			const XMVECTOR lightUp = UnitVector::UpVector;
			const XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

			// Transform bounding sphere to light space.
			XMFLOAT3 sphereCenterLS;
			XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));

			// Ortho frustum in light space encloses scene.
			const FLOAT l = sphereCenterLS.x - mSceneBounds.Radius;
			const FLOAT b = sphereCenterLS.y - mSceneBounds.Radius;
			const FLOAT n = sphereCenterLS.z - mSceneBounds.Radius;
			const FLOAT r = sphereCenterLS.x + mSceneBounds.Radius;
			const FLOAT t = sphereCenterLS.y + mSceneBounds.Radius;
			const FLOAT f = sphereCenterLS.z + mSceneBounds.Radius;

			const XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

			const XMMATRIX viewProj = XMMatrixMultiply(lightView, lightProj);			
			light->Matrix0 = XMMatrixTranspose(viewProj);

			const XMMATRIX S = lightView * lightProj * T;
			light->Matrix1 = XMMatrixTranspose(S);

			XMStoreFloat3(&light->Position, lightPos);

			light->BaseIndex = idx;
			light->IndexStride = 1;
			idx += 1;
		}
		else if (light->Type == ELight::E_Point || light->Type == ELight::E_Tube) {
			const auto proj = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.f, 0.1f, 50.f);

			XMVECTOR pos;
			if (light->Type == ELight::E_Tube) {
				const auto Pos0 = XMLoadFloat3(&light->Position);
				const auto Pos1 = XMLoadFloat3(&light->Position1);

				pos = (Pos0 + Pos1) * 0.5f;
			}
			else {
				pos = XMLoadFloat3(&light->Position);
			}

			// Positive +X
			{
				const auto target = pos + XMVectorSet(1.f, 0.f, 0.f, 0.f);
				const auto view_px = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 1.f, 0.f, 0.f));
				const auto vp_px = view_px * proj;
				light->Matrix0 = XMMatrixTranspose(vp_px);
			}
			// Positive -X
			{
				const auto target = pos + XMVectorSet(-1.f, 0.f, 0.f, 0.f);
				const auto view_nx = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 1.f, 0.f, 0.f));
				const auto vp_nx = view_nx * proj;
				light->Matrix1 = XMMatrixTranspose(vp_nx);
			}
			// Positive +Y
			{
				const auto target = pos + XMVectorSet(0.f, 1.f, 0.f, 0.f);
				const auto view_py = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 0.f, -1.f, 0.f));
				const auto vp_py = view_py * proj;
				light->Matrix2 = XMMatrixTranspose(vp_py);
			}
			// Positive -Y
			{
				const auto target = pos + XMVectorSet(0.f, -1.f, 0.f, 0.f);
				const auto view_ny = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 0.f, 1.f, 0.f));
				const auto vp_ny = view_ny * proj;
				light->Matrix3 = XMMatrixTranspose(vp_ny);
			}
			// Positive +Z
			{
				const auto target = pos + XMVectorSet(0.f, 0.f, 1.f, 0.f);
				const auto view_pz = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 1.f, 0.f, 0.f));
				const auto vp_pz = view_pz * proj;
				light->Matrix4 = XMMatrixTranspose(vp_pz);
			}
			// Positive -Z
			{
				const auto target = pos + XMVectorSet(0.f, 0.f, -1.f, 0.f);
				const auto view_nz = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 1.f, 0.f, 0.f));
				const auto vp_nz = view_nz * proj;
				light->Matrix5 = XMMatrixTranspose(vp_nz);
			}

			light->BaseIndex = idx;
			light->IndexStride = 6;
			idx += 6;
		}
		else if (light->Type == ELight::E_Spot) {
			const auto Proj = XMMatrixPerspectiveFovLH(
				light->OuterConeAngle * DegToRad, 1.f, 0.1f, light->AttenuationRadius);
			const auto Pos = light->Position;
			const auto Direction = light->Direction;

			const auto UpVector = CalcUpVector(Direction);

			const auto Target = Pos + Direction;
			const auto View = XMMatrixLookAtLH(Pos, Target, UpVector);
			const auto ViewProj = View * Proj;
			light->Matrix0 = XMMatrixTranspose(ViewProj);

			const auto S = View * Proj * T;
			light->Matrix1 = XMMatrixTranspose(S);

			light->BaseIndex = idx;
			light->IndexStride = 1;
			idx += 1;
		}
		else if (light->Type == ELight::E_Rectangle) {
			const XMVECTOR lightDir = XMLoadFloat3(&light->Direction);
			const XMVECTOR lightUp = CalcUpVector(light->Direction);
			const XMVECTOR lightRight = XMVector3Cross(lightUp, lightDir);
			XMStoreFloat3(&light->Up, lightUp);
			XMStoreFloat3(&light->Right, lightRight);

			const XMVECTOR LightCenter = XMLoadFloat3(&light->Center);
			const FLOAT HalfSizeX = light->RectSize.x * 0.5f;
			const FLOAT HalfSizeY = light->RectSize.y * 0.5f;
			const XMVECTOR LightPos0 = LightCenter + lightUp * HalfSizeY + lightRight * HalfSizeX;
			const XMVECTOR LightPos1 = LightCenter + lightUp * HalfSizeY - lightRight * HalfSizeX;
			const XMVECTOR LightPos2 = LightCenter - lightUp * HalfSizeY - lightRight * HalfSizeX;
			const XMVECTOR LightPos3 = LightCenter - lightUp * HalfSizeY + lightRight * HalfSizeX;
			XMStoreFloat3(&light->Position, LightPos0);
			XMStoreFloat3(&light->Position1, LightPos1);
			XMStoreFloat3(&light->Position2, LightPos2);
			XMStoreFloat3(&light->Position3, LightPos3);

			light->BaseIndex = idx;
			light->IndexStride = 1;
			idx += 1;
		}

		ligthCB.Lights[i] = *light;
		mpCurrentFrameResource->LightCB.CopyCB(ligthCB);
	}

	return true;
}

bool D3D12Renderer::UpdateGizmoCB() {
	auto camera = GetActiveCamera();
	if (camera == nullptr) return true;

	auto view = camera->GetUnitViewMatrix();

	auto det = XMMatrixDeterminant(view);
	auto invView = XMMatrixInverse(&det, view);

	auto proj = camera->GetOrthoProjMatrix();
	auto viewProj = XMMatrixMultiply(view, proj);

	GizmoCB gizmoCB{
		.View = XMMatrixTranspose(view),
		.InvView = XMMatrixTranspose(invView),
		.Proj = XMMatrixTranspose(proj),
		.UnitViewProj = XMMatrixTranspose(viewProj),
		.ViewportSize = Vec2(mSwapChain->GetScreenViewport().Width, mSwapChain->GetScreenViewport().Height),
		.LineThickness = 48.f
	};

	mpCurrentFrameResource->GizmoCB.CopyCB(gizmoCB);

	return true;
}

bool D3D12Renderer::UpdateObjectCB() {
	for (auto& layer : mRenderItems) {
		for (auto& ritem : layer) {
			ObjectCB objCB{};

			XMStoreFloat4x4(&objCB.PrevWorld, XMMatrixTranspose(ritem->PrevWorld));
			XMStoreFloat4x4(&objCB.World, XMMatrixTranspose(ritem->World));
			XMStoreFloat4x4(&objCB.TexTransform, XMMatrixTranspose(ritem->TexTransform));

			objCB.BonePaletteOffset = ritem->BonePaletteOffset;

			mpCurrentFrameResource->ObjectCB.CopyCB(objCB, ritem->ObjectCBIndex);
		}
	}

	return true;
}

bool D3D12Renderer::UpdateMaterialCB() {
 	for (auto& matData : mMaterials) {
		MaterialCB matCB{};

		matCB.Albedo = matData.Albedo;
		matCB.Roughness = matData.Roughness;
		matCB.Metalness = matData.Metalness;
		matCB.Specular = matData.Specular;
		matCB.MatTransform = matData.MatTransform;

		mpCurrentFrameResource->MaterialCB.CopyCB(matCB, matData.MaterialCBIndex);
	}

	return true;
}

bool D3D12Renderer::UpdateBoneSB() {
	const UINT capacity = 1024; // 지금 생성한 크기와 동일
	const UINT count = static_cast<UINT>(mCurrentFrameBonePalette.size());

	if (count > capacity)
		ReturnFalse(std::format("Bone palette overflow: {} > {}", count, capacity));

	for (UINT i = 0; i < count; ++i) {
		mpCurrentFrameResource->BoneSB[D3D12FrameResource::CurrentBonePaletteIndex]
			.CopyData(i, XMMatrixTranspose(mCurrentFrameBonePalette[i]));

		mpCurrentFrameResource->BoneSB[D3D12FrameResource::PreviousBonePaletteIndex]
			.CopyData(i, XMMatrixTranspose(mPreviousFrameBonePalette[i]));
	}

	return true;
}

bool D3D12Renderer::UpdateProjectToCubeCB() {
	auto environmentManager = RENDER_PASS_MANAGER->Get<D3D12EnvironmentManager>();
	auto size = environmentManager->GetReflectionProbeCount();

	for (UINT i = 0; i < size; ++i) {
		ProjectToCubeCB  projToCubeCB{};

		auto reflectionProbe = environmentManager->GetReflectionProbe(i);
		if (reflectionProbe == nullptr) continue;

		auto length = reflectionProbe->BoxExtents.Length();

		auto world = reflectionProbe->World;

		// 프로브의 월드 위치
		const Vec4 eye = XMVector3TransformCoord(XMVectorSet(0.f, 0.f, 0.f, 1.f), world);

		// 프로브의 월드 축(회전 반영)
		const Vec4 axisX = XMVector3Normalize(
			XMVector3TransformNormal(UnitVector::RightVector, world));
		const Vec4 axisY = XMVector3Normalize(
			XMVector3TransformNormal(UnitVector::UpVector, world));
		const Vec4 axisZ = XMVector3Normalize(
			XMVector3TransformNormal(UnitVector::ForwardVector, world));

		XMStoreFloat4x4(
			&projToCubeCB.Proj,
			XMMatrixTranspose(XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.f, 0.1f, length)));

		// +X
		XMStoreFloat4x4(
			&projToCubeCB.Views[0],
			XMMatrixTranspose(XMMatrixLookAtLH(eye, eye + axisX, axisY)));

		// -X
		XMStoreFloat4x4(
			&projToCubeCB.Views[1],
			XMMatrixTranspose(XMMatrixLookAtLH(eye, eye - axisX, axisY)));

		// +Y
		XMStoreFloat4x4(
			&projToCubeCB.Views[2],
			XMMatrixTranspose(XMMatrixLookAtLH(eye, eye + axisY, -axisZ)));

		// -Y
		XMStoreFloat4x4(
			&projToCubeCB.Views[3],
			XMMatrixTranspose(XMMatrixLookAtLH(eye, eye - axisY, axisZ)));

		// +Z
		XMStoreFloat4x4(
			&projToCubeCB.Views[4],
			XMMatrixTranspose(XMMatrixLookAtLH(eye, eye + axisZ, axisY)));

		// -Z
		XMStoreFloat4x4(
			&projToCubeCB.Views[5],
			XMMatrixTranspose(XMMatrixLookAtLH(eye, eye - axisZ, axisY)));

		mpCurrentFrameResource->ProjectToCubeCB.CopyCB(projToCubeCB, i);
	}

	return true;
}

bool D3D12Renderer::UpdateProbeSB() {
	const auto envrionmentManager = RENDER_PASS_MANAGER->Get<D3D12EnvironmentManager>();
	const auto count = envrionmentManager->GetReflectionProbeCount();

	for (UINT i = 0; i < count; ++i) {
		const auto reflectionProbeSlot = envrionmentManager->GetReflectionProbeSlot(i);
		if (reflectionProbeSlot == nullptr) continue;

		const auto& reflectionProbe = reflectionProbeSlot->Desc;

		ReflectionProbeMetaData probeData{};
		probeData.InvWorld = XMMatrixTranspose(reflectionProbe.World.Invert());
		
		probeData.BoxExtents = reflectionProbe.BoxExtents;
		probeData.Radius = reflectionProbe.Radius;

		probeData.Shape = reflectionProbe.Shape;
		probeData.IBLIndex = reflectionProbeSlot->TextureIndex;
		probeData.Priority = reflectionProbe.Priority;
		probeData.Flags = reflectionProbe.Enabled ? 1 : 0;

		probeData.BlendDistance = reflectionProbe.BlendDistance;

		mpCurrentFrameResource->ProbeSB.CopyData(i, probeData);
	}

	return true;
}

bool D3D12Renderer::UpdateDebugLineVB() {
	auto envrionmentManager = RENDER_PASS_MANAGER->Get<D3D12EnvironmentManager>();
	auto debug = RENDER_PASS_MANAGER->Get<D3D12Debug>();

	auto count = envrionmentManager->GetReflectionProbeCount();
	for (UINT i = 0; i < count; ++i) {
		auto reflectionProbe = envrionmentManager->GetReflectionProbe(i);
		if (reflectionProbe == nullptr) continue;

		debug->BuildReflectionProbeDebugLines(*reflectionProbe);
	}

	for (const auto& shape : mDebugColliderShapes) {
		if (shape.Type == ECollider::E_Box) {
			debug->AddWireBox(shape.World, shape.HalfExtents, Vec4(0.f, 1.f, 0.f, 1.f));
		}
		else if (shape.Type == ECollider::E_Sphere) {
			debug->AddWireSphere(shape.World, shape.Radius, Vec4(0.f, 1.f, 0.f, 1.f), 8, 16);
		}
		else if (shape.Type == ECollider::E_Capsule) {
			debug->AddWireCapsule(
				shape.World, shape.Radius, shape.HalfSegment, Vec4(0.f, 1.f, 0.f, 1.f));
		}
	}

	const auto& vertices = debug->GetDebugLineVertices();
	for (UINT i = 0; i < vertices.size(); ++i) 
		mpCurrentFrameResource->DebugLineVB.CopyData(i, vertices[i]);

	return true;
}

bool D3D12Renderer::DrawScene() {
	std::vector<D3D12RenderItem*> staticRitems{};
	for (const auto& ritem : mRenderItems[D3D12Renderer::E_Static])
		staticRitems.push_back(ritem.get());

	std::vector<D3D12RenderItem*> skinnedRitems{};
	for (const auto& ritem : mRenderItems[D3D12Renderer::E_Skinned])
		skinnedRitems.push_back(ritem.get());

	std::vector<D3D12RenderItem*> skySphere{};
	for (const auto& ritem : mRenderItems[D3D12Renderer::E_SkySphere])
		skySphere.push_back(ritem.get());

	auto gbuffer = RENDER_PASS_MANAGER->Get<D3D12GBuffer>();
	CheckReturn(gbuffer->DrawGBuffer(
		mpCurrentFrameResource,
		mSwapChain->GetScreenViewport(),
		mSwapChain->GetScissorRect(),
		mSwapChain->GetSceneMap(),
		mSwapChain->GetSceneMapRtv(),
		mDepthStencilBuffer->GetDepthStencilBuffer(),
		mDepthStencilBuffer->GetDepthStencilBufferDsv(),
		staticRitems,
		skinnedRitems,
		0.2f, 0.1f));

	std::vector<LightData*> lights{};
	LEVEL_MANAGER->GetLightData(lights);

	auto shadow = RENDER_PASS_MANAGER->Get<D3D12Shadow>();
	CheckReturn(shadow->Run(
		mpCurrentFrameResource,
		staticRitems,
		skinnedRitems,
		lights));

	auto brdf = RENDER_PASS_MANAGER->Get<D3D12Brdf>();
	CheckReturn(brdf->ComputeBRDF(
		mpCurrentFrameResource,
		mSwapChain->GetScreenViewport(),
		mSwapChain->GetScissorRect(),
		mSwapChain->GetHdrMap(),
		mSwapChain->GetHdrMapRtv(),
		gbuffer->GetAlbedoMap(),
		gbuffer->GetAlbedoMapSrv(),
		gbuffer->GetNormalMap(),
		gbuffer->GetNormalMapSrv(),
		mDepthStencilBuffer->GetDepthStencilBuffer(),
		mDepthStencilBuffer->GetDepthStencilBufferSrv(),
		gbuffer->GetRMSMap(),
		gbuffer->GetRMSMapSrv(),
		gbuffer->GetPositionMap(),
		gbuffer->GetPositionMapSrv(),
		shadow->GetDepthMap(),
		shadow->GetDepthMapSrv()));
	CheckReturn(brdf->IntegrateIrradiance(
		mpCurrentFrameResource,
		mSwapChain->GetScreenViewport(),
		mSwapChain->GetScissorRect(),
		mSwapChain->GetHdrMap(),
		mSwapChain->GetHdrMapRtv(),
		mSwapChain->GetHdrMapCopy(),
		mSwapChain->GetHdrMapSrv(),
		gbuffer->GetAlbedoMap(),
		gbuffer->GetAlbedoMapSrv(),
		gbuffer->GetNormalMap(),
		gbuffer->GetNormalMapSrv(),
		mDepthStencilBuffer->GetDepthStencilBuffer(),
		mDepthStencilBuffer->GetDepthStencilBufferSrv(),
		gbuffer->GetRMSMap(),
		gbuffer->GetRMSMapSrv(),
		gbuffer->GetPositionMap(),
		gbuffer->GetPositionMapSrv()));
	auto environmentManager = RENDER_PASS_MANAGER->Get<D3D12EnvironmentManager>();
	CheckReturn(environmentManager->DrawSkySphere(
		mpCurrentFrameResource,
		mSwapChain->GetScreenViewport(),
		mSwapChain->GetScissorRect(),
		mSwapChain->GetHdrMap(),
		mSwapChain->GetHdrMapRtv(),
		mDepthStencilBuffer->GetDepthStencilBuffer(),
		mDepthStencilBuffer->GetDepthStencilBufferDsv(),
		skySphere));

	if (SHADER_ARGUMENT_MANAGER->Bloom.Enabled) {
		auto bloom = RENDER_PASS_MANAGER->Get<D3D12Bloom>();
		CheckReturn(bloom->ApplyBloom(
			mpCurrentFrameResource,
			mSwapChain->GetScreenViewport(),
			mSwapChain->GetScissorRect(),
			mSwapChain->GetHdrMap(),
			mSwapChain->GetHdrMapRtv(),
			mSwapChain->GetHdrMapCopy(),
			mSwapChain->GetHdrMapSrv()));
	}

	auto toneMapping = RENDER_PASS_MANAGER->Get<D3D12ToneMapping>();
	CheckReturn(toneMapping->Apply(
		mpCurrentFrameResource,
		mSwapChain->GetScreenViewport(),	
		mSwapChain->GetScissorRect(),
		mSwapChain->GetSceneMap(),
		mSwapChain->GetSceneMapRtv(),
		mSwapChain->GetHdrMap(),
		mSwapChain->GetHdrMapSrv()));

	if (SHADER_ARGUMENT_MANAGER->GammaCorrection.Enabled) {
		auto gammaCorrection = RENDER_PASS_MANAGER->Get<D3D12GammaCorrection>();
		CheckReturn(gammaCorrection->Apply(
			mpCurrentFrameResource,
			mSwapChain->GetScreenViewport(),
			mSwapChain->GetScissorRect(),
			mSwapChain->GetSceneMap(),
			mSwapChain->GetSceneMapRtv(),
			mSwapChain->GetSceneMapCopy(),
			mSwapChain->GetSceneMapCopySrv()));
	}

	if (SHADER_ARGUMENT_MANAGER->TAA.Enabled) {
		auto taa = RENDER_PASS_MANAGER->Get<D3D12Taa>();
		CheckReturn(taa->ApplyTAA(
			mpCurrentFrameResource,
			mSwapChain->GetScreenViewport(),
			mSwapChain->GetScissorRect(),
			mSwapChain->GetSceneMap(),
			mSwapChain->GetSceneMapRtv(),
			mSwapChain->GetSceneMapCopy(),
			mSwapChain->GetSceneMapCopySrv(),
			gbuffer->GetVelocityMap(),
			gbuffer->GetVelocityMapSrv()));
	}

	if (SHADER_ARGUMENT_MANAGER->Vignette.Enabled) {
		auto vignette = RENDER_PASS_MANAGER->Get<D3D12Vignette>();
		CheckReturn(vignette->ApplyVignette(
			mpCurrentFrameResource,
			mSwapChain->GetScreenViewport(),
			mSwapChain->GetScissorRect(),
			mSwapChain->GetSceneMap(),
			mSwapChain->GetSceneMapRtv(),
			mSwapChain->GetSceneMapCopy(),
			mSwapChain->GetSceneMapCopySrv()));
	}

	auto gizmo = RENDER_PASS_MANAGER->Get<D3D12Gizmo>();
	CheckReturn(gizmo->DrawAxisLine(
		mpCurrentFrameResource,
		mSwapChain->GetSceneMap(),
		mSwapChain->GetSceneMapRtv(),
		mDepthStencilBuffer->GetDepthStencilBuffer(),
		mDepthStencilBuffer->GetDepthStencilBufferDsv()));
	CheckReturn(gizmo->DrawAxisCap(
		mpCurrentFrameResource,
		mSwapChain->GetSceneMap(),
		mSwapChain->GetSceneMapRtv(),
		mDepthStencilBuffer->GetDepthStencilBuffer(),
		mDepthStencilBuffer->GetDepthStencilBufferDsv()));

	auto debug = RENDER_PASS_MANAGER->Get<D3D12Debug>();
	CheckReturn(debug->DrawDebugLines(
		mpCurrentFrameResource,
		mSwapChain->GetScreenViewport(),
		mSwapChain->GetScissorRect(),
		mSwapChain->GetSceneMap(),
		mSwapChain->GetSceneMapRtv(),
		mDepthStencilBuffer->GetDepthStencilBuffer(),
		mDepthStencilBuffer->GetDepthStencilBufferDsv()));

	return true;
}

bool D3D12Renderer::DrawEditor() {
	CheckReturn(mCommandObject->ResetDirectCommandList(
		mpCurrentFrameResource->FrameCommandAllocator(),
		nullptr));

	const auto CmdList = mCommandObject->GetDirectCommandList();
	CheckReturn(mDescriptorHeap->SetDescriptorHeap(CmdList));

	CmdList->RSSetViewports(1, &mSwapChain->GetScreenViewport());
	CmdList->RSSetScissorRects(1, &mSwapChain->GetScissorRect());

	mSwapChain->GetCurrentBackBuffer()->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

	const auto rtv = mSwapChain->GetCurrentBackBufferRtv();

	FLOAT clearValues[4] = { 0.1f, 0.1f, 0.1f, 1.f };
	CmdList->ClearRenderTargetView(rtv, clearValues, 0, nullptr);
	CmdList->OMSetRenderTargets(1, &rtv, TRUE, nullptr);

	const auto gbuffer = RENDER_PASS_MANAGER->Get<D3D12GBuffer>();
	const auto bloom = RENDER_PASS_MANAGER->Get<D3D12Bloom>();
	const auto environmentManager = RENDER_PASS_MANAGER->Get<D3D12EnvironmentManager>();

	EDITOR_MANAGER->AddDisplayTexture("AlbedoMap", static_cast<ImTextureID>(gbuffer->GetAlbedoMapSrv().ptr));
	EDITOR_MANAGER->AddDisplayTexture("NormalMap", static_cast<ImTextureID>(gbuffer->GetNormalMapSrv().ptr));
	EDITOR_MANAGER->AddDisplayTexture("VelocityMap", static_cast<ImTextureID>(gbuffer->GetVelocityMapSrv().ptr));
	EDITOR_MANAGER->AddDisplayTexture("PositionMap", static_cast<ImTextureID>(gbuffer->GetPositionMapSrv().ptr));
	
	EDITOR_MANAGER->AddDisplayTexture("HdrMap", static_cast<ImTextureID>(mSwapChain->GetHdrMapSrv().ptr));

	EDITOR_MANAGER->AddDisplayTexture("Highlights_1/4", static_cast<ImTextureID>(
		bloom->GetHighlightMapSrv(Bloom::Resource::E_4thRes).ptr));
	EDITOR_MANAGER->AddDisplayTexture("Highlights_1/16", static_cast<ImTextureID>(
		bloom->GetHighlightMapSrv(Bloom::Resource::E_16thRes).ptr));
	EDITOR_MANAGER->AddDisplayTexture("Highlights_1/64", static_cast<ImTextureID>(
		bloom->GetHighlightMapSrv(Bloom::Resource::E_64thRes).ptr));
	EDITOR_MANAGER->AddDisplayTexture("Highlights_1/256", static_cast<ImTextureID>(
		bloom->GetHighlightMapSrv(Bloom::Resource::E_256thRes).ptr));

	EDITOR_MANAGER->AddDisplayTexture("Bloom_1/4", static_cast<ImTextureID>(
		bloom->GetBloomMapSrv(Bloom::Resource::E_4thRes).ptr));
	EDITOR_MANAGER->AddDisplayTexture("Bloom_1/16", static_cast<ImTextureID>(
		bloom->GetBloomMapSrv(Bloom::Resource::E_16thRes).ptr));
	EDITOR_MANAGER->AddDisplayTexture("Bloom_1/64", static_cast<ImTextureID>(
		bloom->GetBloomMapSrv(Bloom::Resource::E_64thRes).ptr));
	EDITOR_MANAGER->AddDisplayTexture("Bloom_1/256", static_cast<ImTextureID>(
		bloom->GetBloomMapSrv(Bloom::Resource::E_256thRes).ptr));
	
	const auto shadow = RENDER_PASS_MANAGER->Get<D3D12Shadow>();
	const auto LightCount = LEVEL_MANAGER->GetLightCount();
	for (UINT lightIndex = 0; lightIndex < LightCount; ++lightIndex) {
		auto light = LEVEL_MANAGER->GetLightData(lightIndex);
		
		for (UINT offset = 0; offset < light->IndexStride; ++offset) {
			auto index = light->BaseIndex + offset;
			EDITOR_MANAGER->AddDisplayTexture(
				std::format("Shadow_DepthMap_{}", index),
				static_cast<ImTextureID>(shadow->GetDepthMapSrv(index).ptr));
		}
	}

	EDITOR_MANAGER->AddDisplayTexture("BrdfLutMap", static_cast<ImTextureID>(
		environmentManager->GetBrdfLutMapSrv().ptr));

	auto numProbes = environmentManager->GetReflectionProbeCount();
	for (UINT i = 0; i < numProbes; ++i) {
		for (UINT face = 0; face < 6; ++face) {
			EDITOR_MANAGER->AddDisplayTexture(
				std::format("ReflectionProbe_EnvCubeMap_{} Face {}", i, face),
				static_cast<ImTextureID>(environmentManager->GetReflectionProbeCapturedCubeSrv(i, face).ptr));

			EDITOR_MANAGER->AddDisplayTexture(
				std::format("ReflectionProbe_DiffuseIrradiance_{} Face {}", i, face),
				static_cast<ImTextureID>(environmentManager->GetReflectionProbeDiffuseIrradianceSrv(i, face).ptr));
		}
	}

	EDITOR_MANAGER->Draw();

	CheckReturn(mCommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12Renderer::PresentAndSignal() {
	CheckReturn(mSwapChain->ReadyToPresent(mpCurrentFrameResource));
	CheckReturn(mSwapChain->Present(mDevice->IsAllowingTearing()));
	mSwapChain->NextBackBuffer();

	mpCurrentFrameResource->mFrameFence = mCommandObject->SignalFrame();

	auto debug = RENDER_PASS_MANAGER->Get<D3D12Debug>();
	debug->ClearDebugLines();
	mDebugColliderShapes.clear();

	return true;
}
