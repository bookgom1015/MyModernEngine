#include "Renderer\D3D12\pch.h"
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

#include "Renderer/Interface/ShaderArgumentManager.hpp"
#include "backends/imgui_impl_dx12.h"

#include <cmath>

using namespace DirectX;

namespace {
	Vec3 CalcUpVector(const Vec3& dir) {
		auto up = UnitVector::UpVector;
		if (dir.z >= 0.0f) {
			const float dot = dir.Dot(up);
			if (dot > 0.99f) up = UnitVector::BackwardVector;
			else if (dot < -0.99f) up = UnitVector::ForwardVector;
		}
		else {
			up = UnitVector::DownVector;

			const float dot = dir.Dot(up);
			if (dot > 0.99f) up = UnitVector::ForwardVector;
			else if (dot < -0.99f) up = UnitVector::BackwardVector;
		}

		auto right = up.Cross(dir);
		right.Normalize();

		up = dir.Cross(right);
		up.Normalize();

		return up;
	}

	FLOAT gDeltaTime{};

	constexpr UINT ShadowMapSize = 2048;
}

D3D12Renderer::D3D12Renderer() 
	: mFrameResources{}
	, mpCurrentFrameResource{}
	, mCurrentFrameResourceIndex{}
	, mhImGuiSrv{} {
	mSceneBounds.Center = XMFLOAT3(0.f, 0.f, 0.f);
	const FLOAT WidthSquared = 16.f * 16.f;
	mSceneBounds.Radius = sqrtf(WidthSquared + WidthSquared);
}

D3D12Renderer::~D3D12Renderer() {
	if (!mbCleanedUp) 
		CleanUp();
}

bool D3D12Renderer::Initialize(
	HWND hMainWnd
	, unsigned width
	, unsigned height) {
	CheckReturn(D3D12LowRenderer::Initialize(hMainWnd, width, height));

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

	mShaderManager->CleanUp();

	D3D12LowRenderer::CleanUp();

	mbCleanedUp = true;
}

bool D3D12Renderer::Update(float deltaTime) {
	mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) 
		% D3D12FrameResource::NumFrameResources;
	mpCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();

	gDeltaTime = deltaTime;

	CheckReturn(mCommandObject->WaitFrameCompletion(mpCurrentFrameResource->mFrameFence));
	CheckReturn(mpCurrentFrameResource->ResetFrameCommandListAllocator());

	CheckReturn(mCommandObject->WaitUploadCompletion(mpCurrentFrameResource->mUploadFence));
	CheckReturn(mpCurrentFrameResource->ResetUploadCommandListAllocator());

	CheckReturn(mCommandObject->WaitImmediateCompletion(mpCurrentFrameResource->mImmediateFence));
	CheckReturn(mpCurrentFrameResource->ResetImmediateCommandListAllocator());

	if (mbNeedToResize) {
		CheckReturn(Resize(mWidth, mHeight));
		mbNeedToResize = false;
	}

	CheckReturn(ProcessPendingUploads());
	CheckReturn(CleanUpCompletedUploads());

	CheckReturn(RENDER_PASS_MANAGER->Update());

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

bool D3D12Renderer::Resize(unsigned width, unsigned height) {
	CheckReturn(mCommandObject->FlushCommandQueue());

	CheckReturn(D3D12LowRenderer::Resize(width, height));

	CheckReturn(RENDER_PASS_MANAGER->OnResize(width, height));

	return true;
}

bool D3D12Renderer::AddTexture(const std::wstring& filePath, const std::wstring& key) {
	if (key.empty())
		ReturnFalse("Texture key must not be empty");
	if (mTextures.contains(key))
		 ReturnFalse("key already exists: {}", WStrToStr(key));
	if (mPendingTextureCreates.contains(key))
		ReturnFalse("key already exists in pending creates: {}", WStrToStr(key));

	DirectX::ScratchImage image;
	CheckReturn(D3D12Texture::DecodeTextureFile(filePath, image));
	mPendingTextureCreates.try_emplace(key, std::move(image));

	return true;
}

bool D3D12Renderer::AddMesh(const std::wstring& key, const RenderMeshDesc& desc) {
	if (mStaticMeshes.contains(key) || mSkinnedMeshes.contains(key))
		ReturnFalse("key already exists: {}", WStrToStr(key));

	if (mPendingMeshCreates.contains(key))
		ReturnFalse("key already exists in pending creates: {}", WStrToStr(key));

	CreateMeshRequest req{};
	req.StaticVertices.assign(
		desc.StaticVertices.begin(), desc.StaticVertices.end());
	req.StaticIndices.assign(
		desc.StaticIndices.begin(), desc.StaticIndices.end());
	req.StaticPrimitives = desc.StaticPrimitives;

	req.SkinnedVertices.assign(
		desc.SkinnedVertices.begin(), desc.SkinnedVertices.end());
	req.SkinnedIndices.assign(
		desc.SkinnedIndices.begin(), desc.SkinnedIndices.end());
	req.SkinnedPrimitives = desc.SkinnedPrimitives;

	mPendingMeshCreates.emplace(key, std::move(req));

	return true;
}

const std::wstring& D3D12Renderer::GetGlobalDiffuseIrradianceMapPath() const {
	const auto environmentManager = RENDER_PASS_MANAGER->Get<D3D12EnvironmentManager>();
	return environmentManager->GetGlobalDiffuseIrradianceMapPath();
}

void D3D12Renderer::SetGlobalDiffuseIrradianceMap(const std::wstring& key) {
	std::wstring path = key;

	if (path.empty()) {
		const auto environmentManager = RENDER_PASS_MANAGER->Get<D3D12EnvironmentManager>();
		environmentManager->SetGlobalDiffuseIrradianceMap({}, nullptr);

		return;
	}

	if (!mTextures.contains(key)) {
		Logln(
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

	if (path.empty()) {
		const auto environmentManager = RENDER_PASS_MANAGER->Get<D3D12EnvironmentManager>();
		environmentManager->SetGlobalSpecularIrradianceMap({}, nullptr);

		return;
	}

	if (!mTextures.contains(path)) {
		Logln(
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

bool D3D12Renderer::HasReflectionProbe(const ReflectionProbeID& id) const {
	return RENDER_PASS_MANAGER->Get<D3D12EnvironmentManager>()->HasReflectionProbe(id);
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
	// G-버퍼
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
			.Width = static_cast<UINT>(ShadowMapSize),
			.Height = static_cast<UINT>(ShadowMapSize)
		};
		CheckReturn(brdf->Initialize(mDescriptorHeap.get(), &initData));
	}
	// 톤 매핑
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
	// 감마 보정
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
	// 기즈모
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
	// 그림자
	{
		auto shadow = RENDER_PASS_MANAGER->Get<D3D12Shadow>();
		D3D12Shadow::InitData initData{
			.Device = mDevice.get(),
			.CommandObject = mCommandObject.get(),
			.ShaderManager = mShaderManager.get(),
			.Width = ShadowMapSize,
			.Height = ShadowMapSize
		};
		CheckReturn(shadow->Initialize(mDescriptorHeap.get(), &initData));
	}
	// 시간적 안티앨리어싱(TAA)
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
	// 텍스처 스케일러
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
	// 블룸
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
	// 블러 필터
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
	// 비네트
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
	// 환경 매니저
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
	// 디버그 렌더링
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
	// 시공간 분산 유도 필터링(SVGF)
	{
		auto svgf = RENDER_PASS_MANAGER->Get<D3D12Svgf>();
		D3D12Svgf::InitData initData{
			.Device = mDevice.get(),
			.CommandObject = mCommandObject.get(),
			.ShaderManager = mShaderManager.get(),
			.Width = static_cast<UINT>(mSwapChain->GetScreenViewport().Width),
			.Height = static_cast<UINT>(mSwapChain->GetScreenViewport().Height)
		};
		CheckReturn(svgf->Initialize(mDescriptorHeap.get(), &initData));
	}
	// 볼류메트릭 라이트
	{
		auto volumetricLight = RENDER_PASS_MANAGER->Get<D3D12VolumetricLight>();
		D3D12VolumetricLight::InitData initData{
			.Device = mDevice.get(),
			.CommandObject = mCommandObject.get(),
			.ShaderManager = mShaderManager.get(),
			.Width = 160,
			.Height = 90,
			.Depth = 128
		};
		CheckReturn(volumetricLight->Initialize(mDescriptorHeap.get(), &initData));
	}
	// 레이 트레이싱 기반 앰비언트 오클루전(RTAO)
	{
		auto rtao = RENDER_PASS_MANAGER->Get<D3D12Rtao>();
		D3D12Rtao::InitData initData{
			.Device = mDevice.get(),
			.CommandObject = mCommandObject.get(),
			.ShaderManager = mShaderManager.get(),
			.Width = static_cast<UINT>(mSwapChain->GetScreenViewport().Width),
			.Height = static_cast<UINT>(mSwapChain->GetScreenViewport().Height)
		};
		CheckReturn(rtao->Initialize(mDescriptorHeap.get(), &initData));
	}
	// 화면 공간 앰비언트 오클루전(SSAO)
	{
		auto ssao = RENDER_PASS_MANAGER->Get<D3D12Ssao>();
		D3D12Ssao::InitData initData{
			.Device = mDevice.get(),
			.CommandObject = mCommandObject.get(),
			.ShaderManager = mShaderManager.get(),
			.Width = static_cast<UINT>(mSwapChain->GetScreenViewport().Width),
			.Height = static_cast<UINT>(mSwapChain->GetScreenViewport().Height)
		};
		CheckReturn(ssao->Initialize(mDescriptorHeap.get(), &initData));
	}
	// 눈 적응
	{
		auto eyeAdaption = RENDER_PASS_MANAGER->Get<D3D12EyeAdaption>();
		D3D12EyeAdaption::InitData initData{
			.Device = mDevice.get(),
			.CommandObject = mCommandObject.get(),
			.ShaderManager = mShaderManager.get(),
			.Width = static_cast<UINT>(mSwapChain->GetScreenViewport().Width),
			.Height = static_cast<UINT>(mSwapChain->GetScreenViewport().Height)
		};
		CheckReturn(eyeAdaption->Initialize(mDescriptorHeap.get(), &initData));
	}
	// 모션 블러
	{
		auto motionBlur = RENDER_PASS_MANAGER->Get<D3D12MotionBlur>();
		D3D12MotionBlur::InitData initData{
			.Device = mDevice.get(),
			.CommandObject = mCommandObject.get(),
			.ShaderManager = mShaderManager.get(),
			.Width = static_cast<UINT>(mSwapChain->GetScreenViewport().Width),
			.Height = static_cast<UINT>(mSwapChain->GetScreenViewport().Height)
		};
		CheckReturn(motionBlur->Initialize(mDescriptorHeap.get(), &initData));
	}
	// 화면 공간 반사(SSR)
	{
		auto ssr = RENDER_PASS_MANAGER->Get<D3D12Ssr>();
		D3D12Ssr::InitData initData{
			.Device = mDevice.get(),
			.CommandObject = mCommandObject.get(),
			.ShaderManager = mShaderManager.get(),
			.Width = static_cast<UINT>(mSwapChain->GetScreenViewport().Width),
			.Height = static_cast<UINT>(mSwapChain->GetScreenViewport().Height)
		};
		CheckReturn(ssr->Initialize(mDescriptorHeap.get(), &initData));
	}

	CheckReturn(RENDER_PASS_MANAGER->CompileShaders(mShaderManager.get(), mShaderDirectory));
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
		CheckReturn(D3D12Texture::RecordUpload(
			mDevice->GetD3DDevice(),
			CmdList,
			req.second,
			texture.get()));

		CheckReturn(mDescriptorHeap->AllocateCbvSrvUav(1, texture->Allocation));

		CheckReturn(D3D12Texture::BuildTextureShaderResourceView(
			mDevice->GetD3DDevice(),
			texture.get(),
			mDescriptorHeap->GetCpuHandle(texture->Allocation)));

		// GPU가 복사를 끝낼 때까지 업로드 버퍼를 유지하도록 텍스처를 보관합니다.
		// 펜스 값으로 복사 완료 여부를 추적하고 완료 후 업로드 버퍼를 해제합니다.
		mTextures[req.first] = std::move(texture);
	}

	for (const auto& req : mPendingMeshCreates) {
		auto mesh = req.second;

		// 정적 정점이 있으면 정적 정점과 인덱스를 업로드합니다.
		if (mesh.StaticVertices.size() > 0) {
			const UINT VerticesByteSize =
				static_cast<UINT>(sizeof(RenderVertex) * mesh.StaticVertices.size());
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
			data->VertexByteStride = static_cast<UINT>(sizeof(RenderVertex));
			data->VertexBufferByteSize = VerticesByteSize;

			data->TotalIndexCount = static_cast<UINT>(mesh.StaticIndices.size());
			data->IndexBufferByteSize = IndicesByteSize;
			data->IndexByteStride = sizeof(std::uint32_t);
			data->IndexFormat = DXGI_FORMAT_R32_UINT;

			data->Primitives = mesh.StaticPrimitives;

			mStaticMeshes[req.first] = std::move(data);
		}
		// 스키닝 정점이 있으면 스키닝 정점과 인덱스를 업로드합니다.
		if (mesh.SkinnedVertices.size() > 0) {
			const UINT VerticesByteSize = 
				static_cast<UINT>(sizeof(RenderSkinnedVertex) * mesh.SkinnedVertices.size());
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
			data->VertexByteStride = static_cast<UINT>(sizeof(RenderSkinnedVertex));
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

	UINT64 fenceValue{};
	CheckReturn(mCommandObject->SignalUpload(fenceValue));
	mpCurrentFrameResource->mUploadFence = fenceValue;

	// 방금 생성한 리소스의 업로드 버퍼를 나중에 해제하도록 예약합니다.
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
	// 복사가 완료된 텍스처의 업로드 버퍼를 해제합니다.
	if (!mPendingUploads.empty()) {
		const UINT64 completed = mCommandObject->GetCompletedUploadFenceValue();
		if (completed == UINT64_MAX)
			ReturnFalse("The D3D12 device was removed while checking pending uploads");
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
	for (auto& layer : mRenderItems) 
		layer.clear();

	mMaterials.clear();
	mCurrentFrameBonePalette.clear();
	mPreviousFrameBonePalette.clear();
	mObjectCBCount = 0;

	const auto findTexture = [this](const std::wstring& key) -> D3D12Texture* {
		const auto it = mTextures.find(key);
		return it != mTextures.end() ? it->second.get() : nullptr;
	};

	for (const auto& object : mRenderObjectDescs[ERenderObjectDomain::E_Opaque]) {
		const bool skinned = object.SkinnedPrimitiveIndex >= 0;
		// 메쉬과 스켈레톤은 편집 중에 별도로 할당될 수 있습니다.
		// 프리미티브는 스키닝 데이터가 준비될 때까지 렌더링을 연기합니다.
		if (skinned && (object.SkinIndex < 0 || object.BonePalette.empty()))
			continue;

		const int primitiveIndex = skinned ? object.SkinnedPrimitiveIndex : object.StaticPrimitiveIndex;
		if (primitiveIndex < 0) ReturnFalse("Render primitive index is invalid");

		const auto& meshes = skinned ? mSkinnedMeshes : mStaticMeshes;
		const auto& key = skinned ? object.SkinnedMeshKey : object.StaticMeshKey;
		const auto mesh = meshes.find(key);
		if (mesh == meshes.end()) ReturnFalse("Mesh not found");

		const auto meshData = mesh->second.get();
		if (static_cast<size_t>(primitiveIndex) >= meshData->Primitives.size())
			ReturnFalse("Render primitive index is out of range");

		const auto& primitive = meshData->Primitives[primitiveIndex];
		const auto& material = object.Material;
		const int materialIndex = static_cast<int>(mMaterials.size());

		D3D12MaterialData materialData{};
		materialData.MaterialCBIndex = materialIndex;
		materialData.Albedo = material.Albedo;
		materialData.Roughness = material.Roughness;
		materialData.Metalness = material.Metalness;
		materialData.Specular = material.Specular;
		materialData.MatTransform = Identity4x4;
		mMaterials.push_back(materialData);

		auto item = std::make_unique<D3D12RenderItem>();
		item->ObjectCBIndex = mObjectCBCount++;
		item->MaterialCBIndex = materialIndex;
		item->AlbedoMap = findTexture(material.AlbedoMapKey);
		item->NormalMap = findTexture(material.NormalMapKey);
		item->MeshData = meshData;
		item->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		item->World = object.World;
		item->PrevWorld = object.PrevWorld;
		item->IndexCount = primitive.IndexCount;
		item->StartIndexLocation = primitive.StartIndexLocation;
		item->BaseVertexLocation = primitive.BaseVertexLocation;

		if (skinned) {

			item->SkinIndex = object.SkinIndex;
			item->BonePaletteOffset = static_cast<UINT>(mCurrentFrameBonePalette.size());

			// 프리미티브별로 이전 프레임과 현재 프레임의 뼈대 팔레트가 다를 수 있으므로, 이전 프레임의 뼈대 팔레트를 저장합니다.
			const auto& previous = object.PrevBonePalette.size() == object.BonePalette.size()
				? object.PrevBonePalette : object.BonePalette;

			mCurrentFrameBonePalette.insert(mCurrentFrameBonePalette.end(), object.BonePalette.begin(), object.BonePalette.end());
			mPreviousFrameBonePalette.insert(mPreviousFrameBonePalette.end(), previous.begin(), previous.end());
		}

		mRenderItems[skinned ? RenderLayer::E_Skinned : RenderLayer::E_Static].push_back(std::move(item));
	}

	for (const auto& object : mRenderObjectDescs[ERenderObjectDomain::E_SkySphere]) {
		const auto mesh = mStaticMeshes.find(object.StaticMeshKey);
		if (mesh == mStaticMeshes.end()) ReturnFalse("Sky sphere mesh not found");

		const auto meshData = mesh->second.get();
		if (object.StaticPrimitiveIndex < 0 || static_cast<size_t>(object.StaticPrimitiveIndex) >= meshData->Primitives.size())
			ReturnFalse("Sky sphere primitive index is out of range");

		const auto& primitive = meshData->Primitives[object.StaticPrimitiveIndex];

		auto item = std::make_unique<D3D12RenderItem>();
		item->ObjectCBIndex = mObjectCBCount++;
		item->MaterialCBIndex = -1;
		item->EnvironmentMap = findTexture(object.EnvironmentMapKey);
		item->MeshData = meshData;
		item->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		item->World = object.World;
		item->PrevWorld = object.PrevWorld;
		item->IndexCount = primitive.IndexCount;
		item->StartIndexLocation = primitive.StartIndexLocation;
		item->BaseVertexLocation = primitive.BaseVertexLocation;

		mRenderItems[RenderLayer::E_SkySphere].push_back(std::move(item));
	}

	return true;
}

bool D3D12Renderer::UpdateConstantBuffers() {
	CheckReturn(UpdatePassCB());
	CheckReturn(UpdateLightCB());
	CheckReturn(UpdateGizmoCB());
	CheckReturn(UpdateObjectCB());
	CheckReturn(UpdateMaterialCB());
	CheckReturn(UpdateProjectToCubeCB());
	CheckReturn(UpdateAmbientOcclusionCB());
	CheckReturn(UpdateCalcLocalMeanVarianceCB());
	CheckReturn(UpdateBlendWithCurrentFrameCB());
	CheckReturn(UpdateCrossBilateralFilterCB());
	CheckReturn(UpdateAtrousWaveletTransformFilterCB());
	CheckReturn(UpdateBoneSB());
	CheckReturn(UpdateProbeSB());
	CheckReturn(UpdateDebugLineVB());
	CheckReturn(UpdateScreenSpaceReflectionCB());

	return true;
}

bool D3D12Renderer::UpdatePassCB() {
	static PassCB passCB{
		.ViewProj = Identity4x4
	};

	// NDC의 [-1, +1]² 범위를 텍스처 좌표의 [0, 1]² 범위로 변환합니다.
	const XMMATRIX T(
		0.5f, 0.f, 0.f, 0.f,
		0.f, -0.5f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.5f, 0.5f, 0.f, 1.f
	);

	const auto View = mCamera.ViewMatrix;
	const auto Proj = mCamera.ProjMatrix;
	const auto ViewProj = XMMatrixMultiply(View, Proj);

	const auto ViewDet = XMMatrixDeterminant(View);
	const auto InvView = XMMatrixInverse(const_cast<XMVECTOR*>(&ViewDet), View);

	const auto ProjDet = XMMatrixDeterminant(Proj);
	const auto InvProj = XMMatrixInverse(const_cast<XMVECTOR*>(&ProjDet), Proj	);

	const auto ViewProjDet = XMMatrixDeterminant(ViewProj);
	const auto InvViewProj = XMMatrixInverse(const_cast<XMVECTOR*>(&ViewProjDet), ViewProj);

	const auto ViewProjTex = XMMatrixMultiply(ViewProj, T);

	passCB.PrevViewProj = passCB.ViewProj;
	passCB.View = XMMatrixTranspose(View);
	passCB.InvView = XMMatrixTranspose(InvView);
	passCB.Proj = XMMatrixTranspose(Proj);
	passCB.InvProj = XMMatrixTranspose(InvProj);
	passCB.ViewProj = XMMatrixTranspose(ViewProj);
	passCB.InvViewProj = XMMatrixTranspose(InvViewProj);
	passCB.ViewProjTex = XMMatrixTranspose(ViewProjTex);
	passCB.EyePosW = mCamera.Position;

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

	const auto LightCount = static_cast<UINT>(mLights.size());

	ligthCB.LightCount = LightCount;

	const XMMATRIX T(
		0.5f, 0.f, 0.f, 0.f,
		0.f, -0.5f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.5f, 0.5f, 0.f, 1.f
	);

	for (UINT i = 0, idx = 0; i < LightCount; ++i) {
		auto light = &mLights[i];

		if (light->Type == ELight::E_Directional) {
			const XMVECTOR lightDir = light->Direction;
			const XMVECTOR lightPos = -2.f * mSceneBounds.Radius * lightDir;
			const XMVECTOR targetPos = XMLoadFloat3(&mSceneBounds.Center);
			const XMVECTOR lightUp = UnitVector::UpVector;
			const XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

			// 경계 구를 광원 공간으로 변환합니다.
			Vec3 sphereCenterLS = XMVector3TransformCoord(targetPos, lightView);

			// 광원 공간의 직교 절두체가 장면을 감쌉니다.
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

			light->Position = lightPos;

			light->BaseIndex = idx;
			light->IndexStride = 1;
			idx += 1;
		}
		else if (light->Type == ELight::E_Point || light->Type == ELight::E_Tube) {
			// 점광원은 PCF 탭과 선형 비교 필터가 면 경계를 넘지 않도록 각 가장자리에 여유를 둡니다.
			constexpr float PointShadowBorderTexels = 2.f;
			constexpr float PointShadowProjectionScale =
				1.f - 2.f * PointShadowBorderTexels / static_cast<float>(ShadowMapSize);
			const float fov = light->Type == ELight::E_Point
				? 2.f * std::atan(1.f / PointShadowProjectionScale)
				: XM_PIDIV2;
			const auto proj = XMMatrixPerspectiveFovLH(fov, 1.f, 0.1f, 50.f);

			XMVECTOR pos;
			if (light->Type == ELight::E_Tube) {
				const auto Pos0 = light->Position;
				const auto Pos1 = light->Position1;

				pos = (Pos0 + Pos1) * 0.5f;
			}
			else {
				pos = light->Position;
			}

			// +X 방향
			{
				const auto target = pos + XMVectorSet(1.f, 0.f, 0.f, 0.f);
				const auto view_px = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 1.f, 0.f, 0.f));
				const auto vp_px = view_px * proj;
				light->Matrix0 = XMMatrixTranspose(vp_px);
			}
			// -X 방향
			{
				const auto target = pos + XMVectorSet(-1.f, 0.f, 0.f, 0.f);
				const auto view_nx = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 1.f, 0.f, 0.f));
				const auto vp_nx = view_nx * proj;
				light->Matrix1 = XMMatrixTranspose(vp_nx);
			}
			// +Y 방향
			{
				const auto target = pos + XMVectorSet(0.f, 1.f, 0.f, 0.f);
				const auto view_py = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 0.f, -1.f, 0.f));
				const auto vp_py = view_py * proj;
				light->Matrix2 = XMMatrixTranspose(vp_py);
			}
			// -Y 방향
			{
				const auto target = pos + XMVectorSet(0.f, -1.f, 0.f, 0.f);
				const auto view_ny = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 0.f, 1.f, 0.f));
				const auto vp_ny = view_ny * proj;
				light->Matrix3 = XMMatrixTranspose(vp_ny);
			}
			// +Z 방향
			{
				const auto target = pos + XMVectorSet(0.f, 0.f, 1.f, 0.f);
				const auto view_pz = XMMatrixLookAtLH(pos, target, XMVectorSet(0.f, 1.f, 0.f, 0.f));
				const auto vp_pz = view_pz * proj;
				light->Matrix4 = XMMatrixTranspose(vp_pz);
			}
			// -Z 방향
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
			const auto Proj = XMMatrixPerspectiveFovLH(light->OuterConeAngle * 2.f * DegToRad, 1.f, 0.1f, light->AttenuationRadius);
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
			const Vec3 lightDir = light->Direction;
			const Vec3 lightUp = light->Direction;
			const Vec3 lightRight = XMVector3Cross(lightUp, lightDir);
			light->Up = lightUp;
			light->Right = lightRight;

			const FLOAT HalfSizeX = light->RectSize.x * 0.5f;
			const FLOAT HalfSizeY = light->RectSize.y * 0.5f;

			const Vec3 LightCenter = XMLoadFloat3(&light->Center);
			const Vec3 LightPos0 = LightCenter + lightUp * HalfSizeY + lightRight * HalfSizeX;
			const Vec3 LightPos1 = LightCenter + lightUp * HalfSizeY - lightRight * HalfSizeX;
			const Vec3 LightPos2 = LightCenter - lightUp * HalfSizeY - lightRight * HalfSizeX;
			const Vec3 LightPos3 = LightCenter - lightUp * HalfSizeY + lightRight * HalfSizeX;
			light->Position = LightPos0;
			light->Position1 = LightPos1;
			light->Position2 = LightPos2;
			light->Position3 = LightPos3;

			light->BaseIndex = idx;
			light->IndexStride = 1;
			idx += 1;
		}

		ligthCB.Lights[i] = *light;
	}

	mpCurrentFrameResource->LightCB.CopyCB(ligthCB);

	return true;
}

bool D3D12Renderer::UpdateGizmoCB() {
	auto view = mCamera.UnitViewMatrix;

	auto det = XMMatrixDeterminant(view);
	auto invView = XMMatrixInverse(&det, view);

	auto proj = mCamera.OrthoProjMatrix;
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

			objCB.PrevWorld = XMMatrixTranspose(ritem->PrevWorld);
			objCB.World = XMMatrixTranspose(ritem->World);
			objCB.TexTransform = XMMatrixTranspose(ritem->TexTransform);

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

bool D3D12Renderer::UpdateProjectToCubeCB() {
	auto environmentManager = RENDER_PASS_MANAGER->Get<D3D12EnvironmentManager>();
	auto size = environmentManager->GetReflectionProbeCount();

	for (UINT i = 0; i < size; ++i) {
		ProjectToCubeCB  projToCubeCB{};

		auto reflectionProbe = environmentManager->GetReflectionProbe(i);
		if (reflectionProbe == nullptr) continue;

		auto length = reflectionProbe->BoxExtents.Length();
		auto world = reflectionProbe->World;

		// 프로브의 월드 위치입니다.
		const Vec4 eye = XMVector3TransformCoord(XMVectorSet(0.f, 0.f, 0.f, 1.f), world);

		// 프로브의 회전을 반영한 월드 축입니다.
		const Vec4 axisX = XMVector3Normalize(
			XMVector3TransformNormal(UnitVector::RightVector, world));
		const Vec4 axisY = XMVector3Normalize(
			XMVector3TransformNormal(UnitVector::UpVector, world));
		const Vec4 axisZ = XMVector3Normalize(
			XMVector3TransformNormal(UnitVector::ForwardVector, world));

		projToCubeCB.Proj = XMMatrixTranspose(XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.f, 0.1f, 100.f));

		// +X
		projToCubeCB.Views[0] = XMMatrixTranspose(XMMatrixLookAtLH(eye, eye + axisX, axisY));
		// -X
		projToCubeCB.Views[1] = XMMatrixTranspose(XMMatrixLookAtLH(eye, eye - axisX, axisY));
		// +Y
		projToCubeCB.Views[2] = XMMatrixTranspose(XMMatrixLookAtLH(eye, eye + axisY, -axisZ));
		// -Y
		projToCubeCB.Views[3] = XMMatrixTranspose(XMMatrixLookAtLH(eye, eye - axisY, axisZ));
		// +Z
		projToCubeCB.Views[4] = XMMatrixTranspose(XMMatrixLookAtLH(eye, eye + axisZ, axisY));
		// -Z
		projToCubeCB.Views[5] = XMMatrixTranspose(XMMatrixLookAtLH(eye, eye - axisZ, axisY));

		mpCurrentFrameResource->ProjectToCubeCB.CopyCB(projToCubeCB, i);
	}

	return true;
}

bool D3D12Renderer::UpdateAmbientOcclusionCB() {
	const auto ssao = RENDER_PASS_MANAGER->Get<D3D12Ssao>();;

	const auto View = mCamera.ViewMatrix;
	const auto Proj = mCamera.ProjMatrix;

	auto det = XMMatrixDeterminant(Proj);
	const Mat4 invProj = XMMatrixInverse(&det, Proj);

	AmbientOcclusionCB aoCB{};
	aoCB.View = XMMatrixTranspose(View);
	aoCB.Proj = XMMatrixTranspose(Proj);
	aoCB.InvProj = XMMatrixTranspose(invProj);

	// NDC의 [-1, +1]² 범위를 텍스처 좌표의 [0, 1]² 범위로 변환합니다.
	const Mat4 T(
		0.5f,  0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f,  0.0f, 1.0f, 0.0f,
		0.5f,  0.5f, 0.0f, 1.0f
	);
	XMStoreFloat4x4(&aoCB.ProjTex, XMMatrixTranspose(Proj * T));

	//if (mpShadingArgumentSet->RaytracingEnabled) {
	//	const BOOL CheckboardRayGeneration = mpShadingArgumentSet->RTAO.CheckboardRayGeneration;
	//	const UINT PixelStepX = CheckboardRayGeneration ? 2 : 1;
	//
	//	aoCB.TextureDim = { Foundation::Util::D3D12Util::CeilDivide(mClientWidth, PixelStepX), mClientHeight };
	//
	//	aoCB.OcclusionRadius = mpShadingArgumentSet->RTAO.OcclusionRadius;
	//	aoCB.OcclusionFadeStart = mpShadingArgumentSet->RTAO.OcclusionFadeStart;
	//	aoCB.OcclusionFadeEnd = mpShadingArgumentSet->RTAO.OcclusionFadeEnd;
	//	aoCB.SurfaceEpsilon = mpShadingArgumentSet->RTAO.SurfaceEpsilon;
	//	aoCB.SampleCount = mpShadingArgumentSet->RTAO.SampleCount;
	//	aoCB.CheckerboardRayGenEnabled = CheckboardRayGeneration;
	//	aoCB.EvenPixelsActivated = mpShadingArgumentSet->RTAO.CheckerboardGenerateRaysForEvenPixels;
	//}
	//else 
	{
		aoCB.TextureDim = { 
			static_cast<UINT>(mSwapChain->GetScreenViewport().Width), 
			static_cast<UINT>(mSwapChain->GetScreenViewport().Height) };

		const auto& ssao = SHADER_ARGUMENT_MANAGER->SSAO;

		aoCB.OcclusionRadius = ssao.OcclusionRadius;
		aoCB.OcclusionFadeStart = ssao.OcclusionFadeStart;
		aoCB.OcclusionFadeEnd = ssao.OcclusionFadeEnd;
		aoCB.OcclusionStrength = ssao.OcclusionStrength;
		aoCB.SurfaceEpsilon = ssao.SurfaceEpsilon;
		aoCB.SampleCount = ssao.SampleCount;
		aoCB.CheckerboardRayGenEnabled = FALSE;
		aoCB.EvenPixelsActivated = FALSE;
	}

	aoCB.FrameCount = static_cast<UINT>(mpCurrentFrameResource->mFrameFence);

	mpCurrentFrameResource->AmbientOcclusionCB.CopyCB(aoCB);

	return true;
}

bool D3D12Renderer::UpdateCalcLocalMeanVarianceCB() {
	CalcLocalMeanVarianceCB localMeanCB{};

	const auto& rtao = SHADER_ARGUMENT_MANAGER->RTAO;

	const BOOL CheckboardRayGeneration = SHADER_ARGUMENT_MANAGER->RaytracingEnabled ?
		rtao.CheckboardRayGeneration : FALSE;
	const UINT PixelStepY = CheckboardRayGeneration ? 2 : 1;

	localMeanCB.TextureDim = {
		static_cast<UINT>(mSwapChain->GetScreenViewport().Width),
		static_cast<UINT>(mSwapChain->GetScreenViewport().Height) };
	localMeanCB.KernelWidth = 9;
	localMeanCB.KernelRadius = 9 >> 1;
	localMeanCB.CheckerboardSamplingEnabled = CheckboardRayGeneration;
	localMeanCB.EvenPixelActivated = rtao.CheckerboardGenerateRaysForEvenPixels;
	localMeanCB.PixelStepY = PixelStepY;

	mpCurrentFrameResource->CalcLocalMeanVarianceCB.CopyCB(localMeanCB);

	return true;
}

bool D3D12Renderer::UpdateBlendWithCurrentFrameCB() {
	BlendWithCurrentFrameCB blendFrameCB{};

	const auto raytracing = SHADER_ARGUMENT_MANAGER->RaytracingEnabled;

	const auto& rtao = SHADER_ARGUMENT_MANAGER->RTAO;
	const auto& blend = rtao.BlendWithCurrentFrame;

	blendFrameCB.StdDevGamma = blend.StdDevGamma;
	blendFrameCB.ClampCachedValues = blend.UseClamping;
	blendFrameCB.ClampingMinStdDevTolerance = blend.MinStdDevTolerance;

	blendFrameCB.ClampDifferenceToTsppScale = blend.ClampDifferenceToTSPPScale;
	blendFrameCB.ForceUseMinSmoothingFactor = FALSE;
	blendFrameCB.MinSmoothingFactor = 1.f / rtao.MaxTSPP;
	blendFrameCB.MinTsppToUseTemporalVariance = blend.MinTSPPToUseTemporalVariance;

	blendFrameCB.BlurStrengthMaxTspp = blend.LowTSPPMaxTSPP;
	blendFrameCB.BlurDecayStrength = blend.LowTSPPDecayConstant;
	blendFrameCB.CheckerboardEnabled = raytracing ? rtao.CheckboardRayGeneration : FALSE;
	blendFrameCB.CheckerboardEvenPixelActivated = raytracing ? 
		rtao.CheckerboardGenerateRaysForEvenPixels : FALSE;

	mpCurrentFrameResource->BlendWithCurrentFrameCB.CopyCB(blendFrameCB);

	return true;
}

bool D3D12Renderer::UpdateCrossBilateralFilterCB() {
	CrossBilateralFilterCB filterCB{};

	filterCB.DepthNumMantissaBits = D3D12Util::NumMantissaBitsInFloatFormat(16);
	filterCB.DepthSigma = 1.f;

	mpCurrentFrameResource->CrossBilateralFilterCB.CopyCB(filterCB);

	return true;
}

bool D3D12Renderer::UpdateAtrousWaveletTransformFilterCB() {
	AtrousWaveletTransformFilterCB filterCB{};

	const auto& rtao = SHADER_ARGUMENT_MANAGER->RTAO;
	const auto& atrous = rtao.AtrousWaveletTransformFilter;

	filterCB.TextureDim = {
		static_cast<UINT>(mSwapChain->GetScreenViewport().Width),
		static_cast<UINT>(mSwapChain->GetScreenViewport().Height) };
	filterCB.DepthWeightCutoff = atrous.DepthWeightCutoff;
	filterCB.UsingBilateralDownsamplingBuffers = FALSE;

	// 적응형 커널 반경에 적용할 회전을 설정합니다.
	FLOAT kernelRadiusLerfCoef = 0;
	if (atrous.KernelRadiusRotateKernelEnabled) {
		static UINT frameID = 0;
		UINT i = frameID++ % atrous.KernelRadiusRotateKernelNumCycles;
		kernelRadiusLerfCoef = i / static_cast<FLOAT>(atrous.KernelRadiusRotateKernelNumCycles);
	}

	filterCB.UseAdaptiveKernelSize = atrous.UseAdaptiveKernelSize;
	filterCB.KernelRadiusLerfCoef = kernelRadiusLerfCoef;
	filterCB.MinKernelWidth = atrous.FilterMinKernelWidth;
	filterCB.MaxKernelWidth = static_cast<UINT>((atrous.FilterMaxKernelWidthPercentage / 100.f)
		* static_cast<UINT>(mSwapChain->GetScreenViewport().Height));

	filterCB.PerspectiveCorrectDepthInterpolation = atrous.PerspectiveCorrectDepthInterpolation;
	filterCB.MinVarianceToDenoise = atrous.MinVarianceToDenoise;

	filterCB.ValueSigma = atrous.ValueSigma;
	filterCB.DepthSigma = atrous.DepthSigma;
	filterCB.NormalSigma = atrous.NormalSigma;
	filterCB.FovY = mCamera.FovY;

	filterCB.DepthNumMantissaBits = D3D12Util::NumMantissaBitsInFloatFormat(16);

	mpCurrentFrameResource->AtrousWaveletTransformFilterCB.CopyCB(filterCB);

	return true;
}

bool D3D12Renderer::UpdateBoneSB() {
	const UINT capacity = 1024; // 방금 생성한 리소스와 크기가 같습니다.
	const UINT count = static_cast<UINT>(mCurrentFrameBonePalette.size());

	if (count > capacity)
		ReturnFalse("Bone palette overflow: {} > {}", count, capacity);

	for (UINT i = 0; i < count; ++i) {
		mpCurrentFrameResource->BoneSB[D3D12FrameResource::CurrentBonePaletteIndex]
			.CopyData(i, XMMatrixTranspose(mCurrentFrameBonePalette[i]));

		mpCurrentFrameResource->BoneSB[D3D12FrameResource::PreviousBonePaletteIndex]
			.CopyData(i, XMMatrixTranspose(mPreviousFrameBonePalette[i]));
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
		probeData.World = XMMatrixTranspose(reflectionProbe.World);
		probeData.InvWorld = XMMatrixTranspose(reflectionProbe.World.Invert());

		probeData.BoxExtents = reflectionProbe.BoxExtents;
		probeData.Radius = reflectionProbe.Radius;

		probeData.Shape = reflectionProbe.Shape;
		probeData.IBLIndex = reflectionProbeSlot->TextureIndex;
		probeData.Priority = reflectionProbe.Priority;
		probeData.Flags = reflectionProbe.Enabled ? 1 : 0;

		probeData.BlendDistance = reflectionProbe.BlendDistance;
		probeData.UseBoxProjection = reflectionProbe.UseBoxProjection;

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
		auto status = shape.Flags & (EDebugColliderFlags::E_Trigger | EDebugColliderFlags::E_Collided);
		auto color = status ? Vec4(1.f, 0.f, 0.f, 1.f) : Vec4(0.f, 1.f, 0.f, 1.f);
		if (shape.Type == ECollider::E_Box) {
			debug->AddWireBox(shape.World, shape.HalfExtents, color);
		}
		else if (shape.Type == ECollider::E_Sphere) {
			debug->AddWireSphere(shape.World, shape.Radius, color, 8, 16);
		}
		else if (shape.Type == ECollider::E_Capsule) {
			debug->AddWireCapsule(shape.World, shape.Radius, shape.HalfSegment, color);
		}
	}

	const auto& vertices = debug->GetDebugLineVertices();
	for (UINT i = 0; i < vertices.size(); ++i) 
		mpCurrentFrameResource->DebugLineVB.CopyData(i, vertices[i]);

	return true;
}

bool D3D12Renderer::UpdateScreenSpaceReflectionCB() {
	ScreenSpaceReflectionCB ssrCB{};

	ssrCB.RayLength = SHADER_ARGUMENT_MANAGER->SSR.RayLength;
	ssrCB.NoiseIntensity = SHADER_ARGUMENT_MANAGER->SSR.NoiseIntensity;
	ssrCB.StepCount = SHADER_ARGUMENT_MANAGER->SSR.StepCount;
	ssrCB.BackStepCount = SHADER_ARGUMENT_MANAGER->SSR.BackStepCount;

	ssrCB.DepthThreshold = SHADER_ARGUMENT_MANAGER->SSR.DepthThreshold;
	ssrCB.Thickness = SHADER_ARGUMENT_MANAGER->SSR.Thickness;
	ssrCB.Resolution = SHADER_ARGUMENT_MANAGER->SSR.Resolution;
	ssrCB.MaxDistance = SHADER_ARGUMENT_MANAGER->SSR.MaxDistance;

	mpCurrentFrameResource->ScreenSpaceReflectionCB.CopyCB(ssrCB);

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

	const auto svgf = RENDER_PASS_MANAGER->Get<D3D12Svgf>();
	CheckReturn(svgf->CalculateDepthParticalDerivative(
		mpCurrentFrameResource,
		mDepthStencilBuffer->GetDepthStencilBuffer(),
		mDepthStencilBuffer->GetDepthStencilBufferSrv()));

	std::vector<const LightData*> lights{};
	for (const auto& light : mLights) lights.push_back(&light);

	auto shadow = RENDER_PASS_MANAGER->Get<D3D12Shadow>();
	CheckReturn(shadow->Run(
		mpCurrentFrameResource,
		staticRitems,
		skinnedRitems,
		lights));

	if (SHADER_ARGUMENT_MANAGER->AOEnabled) 
		CheckReturn(DrawAO());

	CheckReturn(ComputeBRDF());

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

	auto volumetricLight = RENDER_PASS_MANAGER->Get<D3D12VolumetricLight>();
	if (SHADER_ARGUMENT_MANAGER->VolumetricLight.Enabled) {
		CheckReturn(volumetricLight->BuildFog(
			mpCurrentFrameResource,
			shadow->GetDepthMap(),
			shadow->GetDepthMapSrv(),
			mCamera.NearZ,
			mCamera.FarZ));

		CheckReturn(volumetricLight->ApplyFog(
			mpCurrentFrameResource,
			mSwapChain->GetHdrMap(),
			mSwapChain->GetHdrMapRtv(),
			gbuffer->GetPositionMap(),
			gbuffer->GetPositionMapSrv(),
			mSwapChain->GetScreenViewport(),
			mSwapChain->GetScissorRect(),
			mCamera.NearZ,
			mCamera.FarZ));
	}

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

	CheckReturn(ApplyEyeAdaption());

	auto toneMapping = RENDER_PASS_MANAGER->Get<D3D12ToneMapping>();
	auto eyeAdaption = RENDER_PASS_MANAGER->Get<D3D12EyeAdaption>();
	CheckReturn(toneMapping->Apply(
		mpCurrentFrameResource,
		mSwapChain->GetScreenViewport(),
		mSwapChain->GetScissorRect(),
		mSwapChain->GetSceneMap(),
		mSwapChain->GetSceneMapRtv(),
		mSwapChain->GetHdrMap(),
		mSwapChain->GetHdrMapSrv(),
		eyeAdaption->GetLuminanceMap()));

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

	if (SHADER_ARGUMENT_MANAGER->MotionBlur.Enabled) {
		auto motionBlur = RENDER_PASS_MANAGER->Get<D3D12MotionBlur>();
		CheckReturn(motionBlur->ApplyMotionBlur(
			mpCurrentFrameResource,
			mSwapChain->GetScreenViewport(),
			mSwapChain->GetScissorRect(),
			mSwapChain->GetSceneMap(),
			mSwapChain->GetSceneMapRtv(),
			mSwapChain->GetSceneMapCopy(),
			mSwapChain->GetSceneMapCopySrv(),
			mDepthStencilBuffer->GetDepthStencilBuffer(),
			mDepthStencilBuffer->GetDepthStencilBufferSrv(),
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

	AddDisplayTexture("AlbedoMap", static_cast<std::uint64_t>(gbuffer->GetAlbedoMapSrv().ptr));
	AddDisplayTexture("NormalMap", static_cast<std::uint64_t>(gbuffer->GetNormalMapSrv().ptr));
	AddDisplayTexture("VelocityMap", static_cast<std::uint64_t>(gbuffer->GetVelocityMapSrv().ptr));
	AddDisplayTexture("PositionMap", static_cast<std::uint64_t>(gbuffer->GetPositionMapSrv().ptr));

	AddDisplayTexture("HdrMap", static_cast<std::uint64_t>(mSwapChain->GetHdrMapSrv().ptr));

	AddDisplayTexture("Highlights_1/4", static_cast<std::uint64_t>(
		bloom->GetHighlightMapSrv(Bloom::Resource::E_4thRes).ptr));
	AddDisplayTexture("Highlights_1/16", static_cast<std::uint64_t>(
		bloom->GetHighlightMapSrv(Bloom::Resource::E_16thRes).ptr));
	AddDisplayTexture("Highlights_1/64", static_cast<std::uint64_t>(
		bloom->GetHighlightMapSrv(Bloom::Resource::E_64thRes).ptr));
	AddDisplayTexture("Highlights_1/256", static_cast<std::uint64_t>(
		bloom->GetHighlightMapSrv(Bloom::Resource::E_256thRes).ptr));

	AddDisplayTexture("Bloom_1/4", static_cast<std::uint64_t>(
		bloom->GetBloomMapSrv(Bloom::Resource::E_4thRes).ptr));
	AddDisplayTexture("Bloom_1/16", static_cast<std::uint64_t>(
		bloom->GetBloomMapSrv(Bloom::Resource::E_16thRes).ptr));
	AddDisplayTexture("Bloom_1/64", static_cast<std::uint64_t>(
		bloom->GetBloomMapSrv(Bloom::Resource::E_64thRes).ptr));
	AddDisplayTexture("Bloom_1/256", static_cast<std::uint64_t>(
		bloom->GetBloomMapSrv(Bloom::Resource::E_256thRes).ptr));

	const auto shadow = RENDER_PASS_MANAGER->Get<D3D12Shadow>();
	const auto LightCount = static_cast<UINT>(mLights.size());
	for (UINT lightIndex = 0; lightIndex < LightCount; ++lightIndex) {
		auto light = &mLights[lightIndex];

		for (UINT offset = 0; offset < light->IndexStride; ++offset) {
			auto index = light->BaseIndex + offset;
			AddDisplayTexture(
				std::format("Shadow_DepthMap_{}", index),
				static_cast<std::uint64_t>(shadow->GetDepthMapSrv(index).ptr));
		}
	}

	AddDisplayTexture("BrdfLutMap", static_cast<std::uint64_t>(
		environmentManager->GetBrdfLutMapSrv().ptr));

	auto numProbes = environmentManager->GetReflectionProbeCount();
	for (UINT i = 0; i < numProbes; ++i) {
		for (UINT face = 0; face < 6; ++face) {
			AddDisplayTexture(
				std::format("ReflectionProbe_EnvCubeMap_{} Face {}", i, face),
				static_cast<std::uint64_t>(environmentManager->GetReflectionProbeCapturedCubeSrv(i, face).ptr));

			AddDisplayTexture(
				std::format("ReflectionProbe_DiffuseIrradiance_{} Face {}", i, face),
				static_cast<std::uint64_t>(environmentManager->GetReflectionProbeDiffuseIrradianceSrv(i, face).ptr));
		}
	}

	auto ssao = RENDER_PASS_MANAGER->Get<D3D12Ssao>();
	AddDisplayTexture("SSAO_AO", static_cast<std::uint64_t>(
		ssao->GetAOMapSrv().ptr));
	AddDisplayTexture("SSAO_Temporal_AO", static_cast<std::uint64_t>(
		ssao->GetTemporalAOMapSrv().ptr));

	auto ssr = RENDER_PASS_MANAGER->Get<D3D12Ssr>();
	AddDisplayTexture(
		"SSR_Reflection_0", 
		static_cast<std::uint64_t>(ssr->GetSrv(Ssr::Descriptor::Srv::E_ReflectionMap0).ptr));
	AddDisplayTexture(
		"SSR_Reflection_1",
		static_cast<std::uint64_t>(ssr->GetSrv(Ssr::Descriptor::Srv::E_ReflectionMap1).ptr));

	if (mDrawEditor) CheckReturn(mDrawEditor());

	CheckReturn(mCommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12Renderer::DrawAO() {
	auto gbuffer = RENDER_PASS_MANAGER->Get<D3D12GBuffer>();
	auto svgf = RENDER_PASS_MANAGER->Get<D3D12Svgf>();
	auto ssao = RENDER_PASS_MANAGER->Get<D3D12Ssao>();

	if (SHADER_ARGUMENT_MANAGER->RaytracingEnabled) {

	}
	else {
		CheckReturn(ssao->DrawAO(
			mpCurrentFrameResource,
			gbuffer->GetNormalDepthMap(),
			gbuffer->GetNormalDepthMapSrv(),
			gbuffer->GetPositionMap(),
			gbuffer->GetPositionMapSrv()));

		// 시공간 분산 유도 필터링으로 노이즈를 제거합니다.
		{
			// 시간적 슈퍼샘플링
			{
				// 1단계. 역재투영을 수행합니다.
				{
					const auto PrevTemporalCacheFrameIndex = ssao->CurrentTemporalCacheFrameIndex();
					const auto CurrTemporalCacheFrameIndex = ssao->MoveToNextTemporalCacheFrame();

					const auto PrevTemporalAOFrameIndex = ssao->CurrentTemporalAOFrameIndex();
					const auto CurrTemporalAOFrameIndex = ssao->MoveToNextTemporalAOFrame();

					// 역재투영으로 이전 프레임의 값을 가져옵니다.
					CheckReturn(svgf->ReverseReprojectPreviousFrame(
						mpCurrentFrameResource,
						gbuffer->GetNormalDepthMap(),
						gbuffer->GetNormalDepthMapSrv(),
						gbuffer->GetReprojNormalDepthMap(),
						gbuffer->GetReprojNormalDepthMapSrv(),
						gbuffer->GetCachedNormalDepthMap(),
						gbuffer->GetCachedNormalDepthMapSrv(),
						gbuffer->GetVelocityMap(),
						gbuffer->GetVelocityMapSrv(),
						ssao->GetTemporalAOCoefficientResource(PrevTemporalAOFrameIndex),
						ssao->GetTemporalAOCoefficientSrv(PrevTemporalAOFrameIndex),
						ssao->GetTemporalCacheResource(
							Ssao::Resource::TemporalCache::E_AOCoefficientSquaredMean,
							PrevTemporalCacheFrameIndex),
						ssao->GetTemporalCacheDescriptor(
							Ssao::Descriptor::TemporalCache::ES_AOCoefficientSquaredMean, 
							PrevTemporalCacheFrameIndex),
						ssao->GetTemporalCacheResource(
							Ssao::Resource::TemporalCache::E_RayHitDistance,
							PrevTemporalCacheFrameIndex),
						ssao->GetTemporalCacheDescriptor(
							Ssao::Descriptor::TemporalCache::ES_RayHitDistance,
							PrevTemporalCacheFrameIndex),
						ssao->GetTemporalCacheResource(
							Ssao::Resource::TemporalCache::E_TSPP,
							PrevTemporalCacheFrameIndex),
						ssao->GetTemporalCacheDescriptor(
							Ssao::Descriptor::TemporalCache::ES_TSPP,
							PrevTemporalCacheFrameIndex),
						ssao->GetTemporalCacheResource(
							Ssao::Resource::TemporalCache::E_TSPP,
							CurrTemporalCacheFrameIndex),
						ssao->GetTemporalCacheDescriptor(
							Ssao::Descriptor::TemporalCache::EU_TSPP, 
							CurrTemporalCacheFrameIndex)));
				}
				// 2단계. 현재 프레임 값과 재투영한 캐시 값을 혼합합니다.
				{
					// 혼합 시 값을 제한하는 데 사용할 국소 평균과 분산을 계산합니다.
					CheckReturn(svgf->CalculateLocalMeanVariance(
						mpCurrentFrameResource,
						ssao->GetAOCoefficientResource(Ssao::Resource::AO::E_AOCoefficient),
						ssao->GetAOCoefficientDescriptor(Ssao::Descriptor::AO::ES_AOCoefficient),
						FALSE));

					// 재투영한 값과 현재 프레임 값을 혼합합니다.
					// 체커보드 샘플링에서는 혼합 전에 활성 이웃 픽셀을 사용해 비활성 픽셀을 필터링합니다.
					{
						const auto CurrTemporalCacheFrameIndex = ssao->MoveToNextTemporalCacheFrame();
						const auto CurrAOResourceFrameIndex = ssao->MoveToNextTemporalAOFrame();

						CheckReturn(svgf->BlendWithCurrentFrame(
							mpCurrentFrameResource,
							ssao->GetAOCoefficientResource(Ssao::Resource::AO::E_AOCoefficient),
							ssao->GetAOCoefficientDescriptor(Ssao::Descriptor::AO::ES_AOCoefficient),
							ssao->GetAOCoefficientResource(Ssao::Resource::AO::E_RayHitDistance),
							ssao->GetAOCoefficientDescriptor(Ssao::Descriptor::AO::ES_RayHitDistance),
							ssao->GetTemporalAOCoefficientResource(CurrAOResourceFrameIndex),
							ssao->GetTemporalAOCoefficientUav(CurrAOResourceFrameIndex),
							ssao->GetTemporalCacheResource(
								Ssao::Resource::TemporalCache::E_AOCoefficientSquaredMean, 
								CurrTemporalCacheFrameIndex),
							ssao->GetTemporalCacheDescriptor(
								Ssao::Descriptor::TemporalCache::EU_AOCoefficientSquaredMean, 
								CurrTemporalCacheFrameIndex),
							ssao->GetTemporalCacheResource(
								Ssao::Resource::TemporalCache::E_RayHitDistance, 
								CurrTemporalCacheFrameIndex),
							ssao->GetTemporalCacheDescriptor(
								Ssao::Descriptor::TemporalCache::EU_RayHitDistance, 
								CurrTemporalCacheFrameIndex),
							ssao->GetTemporalCacheResource(
								Ssao::Resource::TemporalCache::E_TSPP, 
								CurrTemporalCacheFrameIndex),
							ssao->GetTemporalCacheDescriptor(
								Ssao::Descriptor::TemporalCache::EU_TSPP, 
								CurrTemporalCacheFrameIndex)));
					}
				}
			}
			// 필터링
			{
				// 1단계. 아트루 웨이블릿 변환 필터를 한 번 적용합니다.
				if (SHADER_ARGUMENT_MANAGER->SSAO.Denoiser.FullscreenBlurEnabaled) {
					const auto CurrTemporalCacheFrameIndex = ssao->CurrentTemporalCacheFrameIndex();
					const auto InputAOResourceFrameIndex = ssao->CurrentTemporalAOFrameIndex();
					const auto OutputAOResourceFrameIndex = ssao->MoveToNextTemporalAOFrame();

					const FLOAT RayHitDistToKernelWidthScale = 22 / SHADER_ARGUMENT_MANAGER->SSAO.OcclusionRadius *
						SHADER_ARGUMENT_MANAGER->SSAO.AtrousWaveletTransformFilter.AdaptiveKernelSizeRayHitDistanceScaleFactor;
					const FLOAT RayHitDistToKernelSizeScaleExp = D3D12Util::Lerp(
						1,
						SHADER_ARGUMENT_MANAGER->SSAO.AtrousWaveletTransformFilter.AdaptiveKernelSizeRayHitDistanceScaleExponent,
						D3D12Util::RelativeCoef(SHADER_ARGUMENT_MANAGER->SSAO.OcclusionRadius, 4, 22));

					CheckReturn(svgf->ApplyAtrousWaveletTransformFilter(
						mpCurrentFrameResource,
						gbuffer->GetNormalDepthMap(),
						gbuffer->GetNormalDepthMapSrv(),
						ssao->GetTemporalCacheResource(
							Ssao::Resource::TemporalCache::E_RayHitDistance, 
							CurrTemporalCacheFrameIndex),
						ssao->GetTemporalCacheDescriptor(
							Ssao::Descriptor::TemporalCache::ES_RayHitDistance, 
							CurrTemporalCacheFrameIndex),
						ssao->GetTemporalCacheResource(
							Ssao::Resource::TemporalCache::E_TSPP, 
							CurrTemporalCacheFrameIndex),
						ssao->GetTemporalCacheDescriptor(
							Ssao::Descriptor::TemporalCache::ES_TSPP, 
							CurrTemporalCacheFrameIndex),
						ssao->GetTemporalAOCoefficientResource(InputAOResourceFrameIndex),
						ssao->GetTemporalAOCoefficientSrv(InputAOResourceFrameIndex),
						ssao->GetTemporalAOCoefficientResource(OutputAOResourceFrameIndex),
						ssao->GetTemporalAOCoefficientUav(OutputAOResourceFrameIndex),
						RayHitDistToKernelWidthScale,
						RayHitDistToKernelSizeScaleExp));
				}
				// 2단계. 가려졌다가 드러난 픽셀에는 깊이 제약을 완화한 3×3 블러를 여러 번 적용합니다.
				if (SHADER_ARGUMENT_MANAGER->SSAO.Denoiser.DisocclusionBlurEnabled) {
					const auto CurrAOResourceFrameIndex = ssao->CurrentTemporalAOFrameIndex();

					CheckReturn(svgf->BlurDisocclusion(
						mpCurrentFrameResource,
						mDepthStencilBuffer->GetDepthStencilBuffer(),
						mDepthStencilBuffer->GetDepthStencilBufferSrv(),
						gbuffer->GetRMSMap(),
						gbuffer->GetRMSMapSrv(),
						ssao->GetTemporalAOCoefficientResource(CurrAOResourceFrameIndex),
						ssao->GetTemporalAOCoefficientUav(CurrAOResourceFrameIndex),
						SHADER_ARGUMENT_MANAGER->SSAO.Denoiser.LowTsppBlurPassCount));
				}
			}
		}
	}

	return true;
}

bool D3D12Renderer::ComputeBRDF() {
	auto brdf = RENDER_PASS_MANAGER->Get<D3D12Brdf>();
	auto gbuffer = RENDER_PASS_MANAGER->Get<D3D12GBuffer>();
	auto shadow = RENDER_PASS_MANAGER->Get<D3D12Shadow>();
	auto ssao = RENDER_PASS_MANAGER->Get<D3D12Ssao>();
	auto ssr = RENDER_PASS_MANAGER->Get<D3D12Ssr>();

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

	if (SHADER_ARGUMENT_MANAGER->SSR.Enabled) {
		CheckReturn(ssr->Run(
			mpCurrentFrameResource,
			mSwapChain->GetScreenViewport(),
			mSwapChain->GetScissorRect(),
			mSwapChain->GetHdrMap(),
			mSwapChain->GetHdrMapSrv(),
			mDepthStencilBuffer->GetDepthStencilBuffer(),
			mDepthStencilBuffer->GetDepthStencilBufferSrv(),
			gbuffer->GetNormalMap(),
			gbuffer->GetNormalMapSrv(),
			gbuffer->GetRMSMap(),
			gbuffer->GetRMSMapSrv(),
			gbuffer->GetPositionMap(),
			gbuffer->GetPositionMapSrv(),
			SHADER_ARGUMENT_MANAGER->SSR.UseScreenspaceReflection ? Ssr::Space::E_Screen : Ssr::Space::E_View));

		for (size_t i = 0; i < 2; ++i) {
			auto blurFilter = RENDER_PASS_MANAGER->Get<D3D12BlurFilter>();
			blurFilter->GaussianBlur(
				mpCurrentFrameResource,
				BlurFilter::PipelineState::CP_GaussianBlurFilterRGBANxN5x5,
				ssr->GetResource(Ssr::Resource::E_ReflectionMap0),
				ssr->GetSrv(Ssr::Descriptor::Srv::E_ReflectionMap0),
				ssr->GetResource(Ssr::Resource::E_ReflectionMap1),
				ssr->GetUav(Ssr::Descriptor::Uav::E_ReflectionMap1),
				ssr->GetWidth(),
				ssr->GetHeight(),
				8);
			blurFilter->GaussianBlur(
				mpCurrentFrameResource,
				BlurFilter::PipelineState::CP_GaussianBlurFilterRGBANxN5x5,
				ssr->GetResource(Ssr::Resource::E_ReflectionMap1),
				ssr->GetSrv(Ssr::Descriptor::Srv::E_ReflectionMap1),
				ssr->GetResource(Ssr::Resource::E_ReflectionMap0),
				ssr->GetUav(Ssr::Descriptor::Uav::E_ReflectionMap0),
				ssr->GetWidth(),
				ssr->GetHeight(),
				8);
		}
	}

	CheckReturn(brdf->IntegrateIrradiance(
		mpCurrentFrameResource,
		mSwapChain->GetScreenViewport(),
		mSwapChain->GetScissorRect(),
		mSwapChain->GetHdrMap(),
		mSwapChain->GetHdrMapRtv(),
		mSwapChain->GetHdrMapCopy(),
		mSwapChain->GetHdrMapCopySrv(),
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
		ssr->GetResource(Ssr::Resource::E_ReflectionMap0),
		ssr->GetSrv(Ssr::Descriptor::Srv::E_ReflectionMap0),
		ssao->GetTemporalAOMap(),
		ssao->GetTemporalAOMapSrv()));

	return true;
}

bool D3D12Renderer::ApplyEyeAdaption() {
	const auto eyeAdaption = RENDER_PASS_MANAGER->Get<D3D12EyeAdaption>();

	CheckReturn(eyeAdaption->ClearHistogram(
		mpCurrentFrameResource));

	CheckReturn(eyeAdaption->BuildLuminanceHistogram(
		mpCurrentFrameResource,
		mSwapChain->GetHdrMap(),
		mSwapChain->GetHdrMapSrv()));

	CheckReturn(eyeAdaption->PercentileExtract(
		mpCurrentFrameResource));

	CheckReturn(eyeAdaption->TemporalSmoothing(
		mpCurrentFrameResource,
		gDeltaTime));

	return true;
}

bool D3D12Renderer::PresentAndSignal() {
	CheckReturn(mSwapChain->ReadyToPresent(mpCurrentFrameResource));
	CheckReturn(mSwapChain->Present(mDevice->IsAllowingTearing()));
	mSwapChain->NextBackBuffer();

	CheckReturn(mCommandObject->SignalFrame(mpCurrentFrameResource->mFrameFence));

	auto debug = RENDER_PASS_MANAGER->Get<D3D12Debug>();
	debug->ClearDebugLines();
	mDebugColliderShapes.clear();

	return true;
}
