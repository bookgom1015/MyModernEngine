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

#include "CTransform.hpp"

#include "AMesh.hpp"

using namespace DirectX;

D3D12Renderer::D3D12Renderer() 
	: mFrameResources{}
	, mpCurrentFrameResource{}
	, mCurrentFrameResourceIndex{}
	, mhImGuiSrv{} {
	mDefaultMaterialData = {
		.AlbedoMapIndex = -1,
		.NormalMapIndex = -1,
		.AlphaMapIndex = -1,
		.RoughnessMapIndex = -1,
		.MetalnessMapIndex = -1,
		.SpecularMapIndex = -1,
		.MaterialCBIndex = 0,
		.NumFramesDirty = D3D12FrameResource::NumFrameResources,
		.Albedo = Vec4(1.f),
		.Roughness = 0.5f,
		.Specular = Vec3(0.08f),
		.Metalness = 0.0f
	};

	mSceneBounds.Center = XMFLOAT3(0.f, 0.f, 0.f);
	const FLOAT WidthSquared = 128.f * 128.f;
	mSceneBounds.Radius = sqrtf(WidthSquared + WidthSquared);
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

	CheckReturn(EDITOR_MANAGER->Initialize());
		
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

	CheckReturn(EDITOR_MANAGER->Update());

	CheckReturn(UpdateConstantBuffers());

	if (mpEditorCamera != nullptr) mpEditorCamera->SortObjects();

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
	CheckReturn(mDescriptorHeap->SetDescriptorHeap(CmdList));

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

bool D3D12Renderer::RegisterRenderItem(
	const std::wstring& key
	, const std::wstring& meshKey
	, const std::wstring& matKey) {
	const auto meshIter = mMeshes.find(meshKey);
	if (meshIter == mMeshes.end()) ReturnFalse("Mesh not found");

	auto ritem = std::make_unique<D3D12RenderItem>();
	ritem->NumFramesDirty = D3D12FrameResource::NumFrameResources;
	ritem->ObjectCBIndex = mRenderItemIndexCounter++;

	ritem->MeshData = &meshIter->second;

	const auto matIter = mMaterials.find(matKey);
	if (matIter != mMaterials.end()) 
		ritem->MaterialData = &matIter->second;
	else 
		ritem->MaterialData = &mDefaultMaterialData;

	ritem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	ritem->IndexCount = meshIter->second.IndexBufferByteSize / meshIter->second.IndexByteStride;
	ritem->StartIndexLocation = 0;
	ritem->BaseVertexLocation = 0;

	mRenderItems[key] = std::move(ritem);

	return true;
}

bool D3D12Renderer::UpdateRenderItemTransform(const std::wstring& key, class CTransform* const pTransform) {
	const auto iter = mRenderItems.find(key);
	if (iter == mRenderItems.end()) ReturnFalse("Render item not found");

	auto& ritem = iter->second;
	ritem->NumFramesDirty = D3D12FrameResource::NumFrameResources;
	ritem->PrevWorld = ritem->World;

	auto rot = pTransform->GetRelativeRotation();
	auto quat = XMQuaternionRotationRollPitchYaw(rot.x, rot.y, rot.z);

	ritem->World = XMMatrixAffineTransformation(
		pTransform->GetRelativeScale(),
		XMVectorSet(0.f, 0.f, 0.f, 1.f),
		quat,
		pTransform->GetRelativePosition()
	);
	

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

	CheckReturn(RENDER_PASS_MANAGER->CompileShaders(mShaderManager.get()));
	CheckReturn(RENDER_PASS_MANAGER->BuildRootSignatures());
	CheckReturn(RENDER_PASS_MANAGER->BuildPipelineStates());
	CheckReturn(RENDER_PASS_MANAGER->AllocateDescriptors());

	return true;
}

bool D3D12Renderer::UpdateConstantBuffers() {
	CheckReturn(UpdatePassCB());
	CheckReturn(UpdateLightCB());
	CheckReturn(UpdateObjectCB());
	CheckReturn(UpdateMaterialCB());

	return true;
}

bool D3D12Renderer::UpdatePassCB() {
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

	const XMMATRIX view = XMLoadFloat4x4(&mpEditorCamera->GetViewMatrix());
	const XMMATRIX proj = XMLoadFloat4x4(&mpEditorCamera->GetProjMatrix());
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
	XMStoreFloat3(&passCB.EyePosW, mpEditorCamera->GetCameraPosition());

	//const auto taa = mShadingObjectManager->Get<Shading::TAA::TAAClass>();
	//
	//if (mpShadingArgumentSet->TAA.Enabled) {
	//	const auto OffsetIndex = static_cast<UINT>(
	//		mCommandObject->CurrentFence() % taa->HaltonSequenceSize());
	//	passCB.JitteredOffset = taa->HaltonSequence(OffsetIndex);
	//}
	//else {
	//	passCB.JitteredOffset = { 0.f, 0.f };
	//}
	passCB.JitteredOffset = { 0.f, 0.f };

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
			light->Mat[0] = XMMatrixTranspose(viewProj);

			const XMMATRIX S = lightView * lightProj * T;
			light->Mat[1] = XMMatrixTranspose(S);

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
				light->Mat[0] = XMMatrixTranspose(vp_px);
			}
			// Positive -X
			{
				const auto target = pos + XMVectorSet(-1.f, 0.f, 0.f, 0.f);
				const auto view_nx = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 1.f, 0.f, 0.f));
				const auto vp_nx = view_nx * proj;
				light->Mat[1] = XMMatrixTranspose(vp_nx);
			}
			// Positive +Y
			{
				const auto target = pos + XMVectorSet(0.f, 1.f, 0.f, 0.f);
				const auto view_py = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 0.f, -1.f, 0.f));
				const auto vp_py = view_py * proj;
				light->Mat[2] = XMMatrixTranspose(vp_py);
			}
			// Positive -Y
			{
				const auto target = pos + XMVectorSet(0.f, -1.f, 0.f, 0.f);
				const auto view_ny = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 0.f, 1.f, 0.f));
				const auto vp_ny = view_ny * proj;
				light->Mat[3] = XMMatrixTranspose(vp_ny);
			}
			// Positive +Z
			{
				const auto target = pos + XMVectorSet(0.f, 0.f, 1.f, 0.f);
				const auto view_pz = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 1.f, 0.f, 0.f));
				const auto vp_pz = view_pz * proj;
				light->Mat[4] = XMMatrixTranspose(vp_pz);
			}
			// Positive -Z
			{
				const auto target = pos + XMVectorSet(0.f, 0.f, -1.f, 0.f);
				const auto view_nz = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 1.f, 0.f, 0.f));
				const auto vp_nz = view_nz * proj;
				light->Mat[5] = XMMatrixTranspose(vp_nz);
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
			light->Mat[0] = XMMatrixTranspose(ViewProj);

			const auto S = View * Proj * T;
			light->Mat[1] = XMMatrixTranspose(S);

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

bool D3D12Renderer::UpdateObjectCB() {
	for (auto& pair : mRenderItems) {
		auto& ritem = pair.second;
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if (ritem->NumFramesDirty > 0) {
			const XMMATRIX PrevWorld = XMLoadFloat4x4(&ritem->PrevWorld);
			const XMMATRIX World = XMLoadFloat4x4(&ritem->World);
			const XMMATRIX TexTransform = XMLoadFloat4x4(&ritem->TexTransform);

			ObjectCB objCB;

			XMStoreFloat4x4(&objCB.PrevWorld, XMMatrixTranspose(PrevWorld));
			XMStoreFloat4x4(&objCB.World, XMMatrixTranspose(World));
			XMStoreFloat4x4(&objCB.TexTransform, XMMatrixTranspose(TexTransform));

			mpCurrentFrameResource->ObjectCB.CopyCB(objCB, ritem->ObjectCBIndex);

			ritem->PrevWorld = ritem->World;

			// Next FrameResource need to be updated too.
			--ritem->NumFramesDirty;
		}
	}

	return true;
}

bool D3D12Renderer::UpdateMaterialCB() {
	if (mDefaultMaterialData.NumFramesDirty > 0) {
		MaterialCB matCB{};

		matCB.Albedo = mDefaultMaterialData.Albedo;
		matCB.Roughness = mDefaultMaterialData.Roughness;
		matCB.Metalness = mDefaultMaterialData.Metalness;
		matCB.Specular = mDefaultMaterialData.Specular;
		matCB.MatTransform = mDefaultMaterialData.MatTransform;

		matCB.AlbedoMapIndex = mDefaultMaterialData.AlbedoMapIndex;
		matCB.NormalMapIndex = mDefaultMaterialData.NormalMapIndex;
		matCB.AlphaMapIndex = mDefaultMaterialData.AlphaMapIndex;
		matCB.RoughnessMapIndex = mDefaultMaterialData.RoughnessMapIndex;
		matCB.MetalnessMapIndex = mDefaultMaterialData.MetalnessMapIndex;
		matCB.SpecularMapIndex = mDefaultMaterialData.SpecularMapIndex;

		mpCurrentFrameResource->MaterialCB.CopyCB(matCB, mDefaultMaterialData.MaterialCBIndex);

		--mDefaultMaterialData.NumFramesDirty;
	}

	for (auto& pair : mMaterials) {
		auto& matData = pair.second;

		if (matData.NumFramesDirty > 0) {
			MaterialCB matCB{};

			matCB.Albedo = matData.Albedo;
			matCB.Roughness = matData.Roughness;
			matCB.Metalness = matData.Metalness;
			matCB.Specular = matData.Specular;
			matCB.MatTransform = matData.MatTransform;

			matCB.AlbedoMapIndex = matData.AlbedoMapIndex;
			matCB.NormalMapIndex = matData.NormalMapIndex;
			matCB.AlphaMapIndex = matData.AlphaMapIndex;
			matCB.RoughnessMapIndex = matData.RoughnessMapIndex;
			matCB.MetalnessMapIndex = matData.MetalnessMapIndex;
			matCB.SpecularMapIndex = matData.SpecularMapIndex;

			mpCurrentFrameResource->MaterialCB.CopyCB(matCB, matData.MaterialCBIndex);

			--matData.NumFramesDirty;
		}
	}

	return true;
}

bool D3D12Renderer::DrawScene() {	
	std::vector<std::wstring> mOpaqueRenderItemKeys{};
	mpEditorCamera->GetOpaqueObjectKeys(mOpaqueRenderItemKeys);

	std::vector<D3D12RenderItem*> ritems{};
	for (const auto& key : mOpaqueRenderItemKeys) {
		auto iter = mRenderItems.find(key);
		assert(iter != mRenderItems.end() && "Render item not found for opaque object key");

		ritems.push_back(iter->second.get());
	}

	auto gbuffer = RENDER_PASS_MANAGER->Get<D3D12GBuffer>();
	CheckReturn(gbuffer->DrawGBuffer(
		mpCurrentFrameResource,
		mSwapChain->GetScreenViewport(),
		mSwapChain->GetScissorRect(),
		mSwapChain->GetSceneMap(),
		mSwapChain->GetSceneMapRtv(),
		mDepthStencilBuffer->GetDepthStencilBuffer(),
		mDepthStencilBuffer->GetDepthStencilBufferDsv(),
		ritems,
		0.5f, 0.1f));

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
		gbuffer->GetSpecularMap(),
		gbuffer->GetSpecularMapSrv(),
		gbuffer->GetRoughnessMetalnessMap(),
		gbuffer->GetRoughnessMetalnessMapSrv(),
		gbuffer->GetPositionMap(),
		gbuffer->GetPositionMapSrv()));
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
		gbuffer->GetSpecularMap(),
		gbuffer->GetSpecularMapSrv(),
		gbuffer->GetRoughnessMetalnessMap(),
		gbuffer->GetRoughnessMetalnessMapSrv(),
		gbuffer->GetPositionMap(),
		gbuffer->GetPositionMapSrv()));

	auto toneMapping = RENDER_PASS_MANAGER->Get<D3D12ToneMapping>();
	CheckReturn(toneMapping->Apply(
		mpCurrentFrameResource,
		mSwapChain->GetScreenViewport(),	
		mSwapChain->GetScissorRect(),
		mSwapChain->GetSceneMap(),
		mSwapChain->GetSceneMapRtv(),
		mSwapChain->GetHdrMap(),
		mSwapChain->GetHdrMapSrv()));

	auto gammaCorrection = RENDER_PASS_MANAGER->Get<D3D12GammaCorrection>();
	CheckReturn(gammaCorrection->Apply(
		mpCurrentFrameResource,
		mSwapChain->GetScreenViewport(),
		mSwapChain->GetScissorRect(),
		mSwapChain->GetSceneMap(),
		mSwapChain->GetSceneMapRtv(),
		mSwapChain->GetSceneMapCopy(),
		mSwapChain->GetSceneMapCopySrv(),
		2.2f));

	return true;
}

bool D3D12Renderer::DrawEditor() {
	CheckReturn(mCommandObject->ResetDirectCommandList(
		mpCurrentFrameResource->CommandAllocator(),
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

	auto gbuffer = RENDER_PASS_MANAGER->Get<D3D12GBuffer>();

	EDITOR_MANAGER->AddDisplayTexture("AlbedoMap", static_cast<ImTextureID>(gbuffer->GetAlbedoMapSrv().ptr));
	EDITOR_MANAGER->AddDisplayTexture("NormalMap", static_cast<ImTextureID>(gbuffer->GetNormalMapSrv().ptr));
	EDITOR_MANAGER->AddDisplayTexture("SpecularMap", static_cast<ImTextureID>(gbuffer->GetSpecularMapSrv().ptr));
	EDITOR_MANAGER->AddDisplayTexture("RoughnessMetalnessMap", static_cast<ImTextureID>(gbuffer->GetRoughnessMetalnessMapSrv().ptr));
	EDITOR_MANAGER->AddDisplayTexture("VelocityMap", static_cast<ImTextureID>(gbuffer->GetVelocityMapSrv().ptr));
	EDITOR_MANAGER->AddDisplayTexture("PositionMap", static_cast<ImTextureID>(gbuffer->GetPositionMapSrv().ptr));

	EDITOR_MANAGER->AddDisplayTexture("HdrMap", static_cast<ImTextureID>(mSwapChain->GetHdrMapSrv().ptr));

	EDITOR_MANAGER->Draw();

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