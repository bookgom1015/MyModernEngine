#include "pch.h"
#include "Renderer/D3D12/D3D12EnvironmentManager.hpp"

#include "AssetManager.hpp"
#include "EditorManager.hpp"

#include "Renderer/D3D12/D3D12Device.hpp"
#include "Renderer/D3D12/D3D12Util.hpp"
#include "Renderer/D3D12/D3D12ShaderManager.hpp"
#include "Renderer/D3D12/D3D12FrameResource.hpp"
#include "Renderer/D3D12/D3D12CommandObject.hpp"
#include "Renderer/D3D12/D3D12RenderItem.hpp"
#include "Renderer/D3D12/D3D12MeshData.hpp"
#include "Renderer/D3D12/D3D12MaterialData.h"
#include "Renderer/D3D12/D3D12GpuResource.hpp"

#include "Renderer/D3D12/D3D12Texture.hpp"

namespace {
	const WCHAR* const HLSL_IntegrateBrdf = L"D3D12IntegrateBrdf.hlsl";
	const WCHAR* const HLSL_DrawSkySphere = L"D3D12DrawSkySphere.hlsl";
	const WCHAR* const HLSL_CaptureEnvironment = L"D3D12CaptureEnvironment.hlsl";
	const WCHAR* const HLSL_CaptureSkySphere = L"D3D12CaptureSkySphere.hlsl";
}

D3D12EnvironmentManager::D3D12EnvironmentManager() {}

D3D12EnvironmentManager::~D3D12EnvironmentManager() {}

bool D3D12EnvironmentManager::Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	mInitData = *reinterpret_cast<InitData*>(pData);

	auto sizeF = static_cast<FLOAT>(EnvironmentManager::BrdfLutMapSize);
	auto sizeI = static_cast<INT>(EnvironmentManager::BrdfLutMapSize);
	mViewport = { 0.f, 0.f, sizeF , sizeF , 0.f, 1.f };
	mScissorRect = { 0, 0, sizeI , sizeI };

	mBrdfLutMap = std::make_unique<GpuResource>();
	mDepthBufferArray = std::make_unique<GpuResource>();

	CheckReturn(BuildResources());

	return true;
}

bool D3D12EnvironmentManager::CompileShaders() {
	// IntegrateBrdf
	{
		const auto VS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_IntegrateBrdf, L"VS", L"vs_6_5");
		const auto MS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_IntegrateBrdf, L"MS", L"ms_6_5");
		const auto PS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_IntegrateBrdf, L"PS", L"ps_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			VS, mShaderHashes[EnvironmentManager::Shader::VS_IntegrateBrdf]));
		CheckReturn(mInitData.ShaderManager->AddShader(
			MS, mShaderHashes[EnvironmentManager::Shader::MS_IntegrateBrdf]));
		CheckReturn(mInitData.ShaderManager->AddShader(
			PS, mShaderHashes[EnvironmentManager::Shader::PS_IntegrateBrdf]));
	}
	// DrawSkySphere
	{
		const auto VS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_DrawSkySphere, L"VS", L"vs_6_5");
		const auto MS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_DrawSkySphere, L"MS", L"ms_6_5");
		const auto PS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_DrawSkySphere, L"PS", L"ps_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			VS, mShaderHashes[EnvironmentManager::Shader::VS_DrawSkySphere]));
		CheckReturn(mInitData.ShaderManager->AddShader(
			MS, mShaderHashes[EnvironmentManager::Shader::MS_DrawSkySphere]));
		CheckReturn(mInitData.ShaderManager->AddShader(
			PS, mShaderHashes[EnvironmentManager::Shader::PS_DrawSkySphere]));
	}
	// CaptureEnvironment
	{
		const auto VS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_CaptureEnvironment, L"VS", L"vs_6_5");
		const auto GS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_CaptureEnvironment, L"GS", L"gs_6_5");
		const auto PS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_CaptureEnvironment, L"PS", L"ps_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			VS, mShaderHashes[EnvironmentManager::Shader::VS_CaptureEnvironment]));
		CheckReturn(mInitData.ShaderManager->AddShader(
			GS, mShaderHashes[EnvironmentManager::Shader::GS_CaptureEnvironment]));
		CheckReturn(mInitData.ShaderManager->AddShader(
			PS, mShaderHashes[EnvironmentManager::Shader::PS_CaptureEnvironment]));
	}
	// CaptureSkySphere
	{
		const auto VS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_CaptureSkySphere, L"VS", L"vs_6_5");
		const auto GS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_CaptureSkySphere, L"GS", L"gs_6_5");
		const auto PS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_CaptureSkySphere, L"PS", L"ps_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			VS, mShaderHashes[EnvironmentManager::Shader::VS_CaptureSkySphere]));
		CheckReturn(mInitData.ShaderManager->AddShader(
			GS, mShaderHashes[EnvironmentManager::Shader::GS_CaptureSkySphere]));
		CheckReturn(mInitData.ShaderManager->AddShader(
			PS, mShaderHashes[EnvironmentManager::Shader::PS_CaptureSkySphere]));
	}

	return true;
}

bool D3D12EnvironmentManager::BuildRootSignatures() {
	decltype(auto) samplers = D3D12Util::GetStaticSamplers();

	// IntegrateBrdf
	{
		CD3DX12_ROOT_PARAMETER slotRootParameter[EnvironmentManager::RootSignature::IntegrateBrdf::Count]{};
		slotRootParameter[EnvironmentManager::RootSignature::IntegrateBrdf::CB_Pass].InitAsConstantBufferView(0);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[EnvironmentManager::RootSignature::GR_IntegrateBrdf]),
			L"EnvironmentManager_GR_IntegrateBrdf"));
	}
	// DrawSkySphere 
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[1]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[EnvironmentManager::RootSignature::DrawSkySphere::Count]{};
		slotRootParameter[EnvironmentManager::RootSignature::DrawSkySphere::CB_Pass]
			.InitAsConstantBufferView(0);
		slotRootParameter[EnvironmentManager::RootSignature::DrawSkySphere::CB_Object]
			.InitAsConstantBufferView(1);
		slotRootParameter[EnvironmentManager::RootSignature::DrawSkySphere::RC_Consts]
			.InitAsConstants(EnvironmentManager::RootConstant::DrawSkySphere::Count, 2);
		slotRootParameter[EnvironmentManager::RootSignature::DrawSkySphere::SB_VertexBuffer]
			.InitAsShaderResourceView(0);
		slotRootParameter[EnvironmentManager::RootSignature::DrawSkySphere::SB_IndexBuffer]
			.InitAsShaderResourceView(1);
		slotRootParameter[EnvironmentManager::RootSignature::DrawSkySphere::SI_EnvCubeMap]
			.InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[EnvironmentManager::RootSignature::GR_DrawSkySphere]),
			L"EnvironmentMap_GR_DrawSkySphere"));
	}
	// CaptureEnvironment
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[3]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 1);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[EnvironmentManager::RootSignature::CaptureEnvironment::Count]{};
		slotRootParameter[EnvironmentManager::RootSignature::CaptureEnvironment::CB_Pass]
			.InitAsConstantBufferView(0);
		slotRootParameter[EnvironmentManager::RootSignature::CaptureEnvironment::CB_ProjectToCube]
			.InitAsConstantBufferView(1);
		slotRootParameter[EnvironmentManager::RootSignature::CaptureEnvironment::CB_Light]
			.InitAsConstantBufferView(2);
		slotRootParameter[EnvironmentManager::RootSignature::CaptureEnvironment::CB_Object]
			.InitAsConstantBufferView(3);
		slotRootParameter[EnvironmentManager::RootSignature::CaptureEnvironment::CB_Material]
			.InitAsConstantBufferView(4);
		slotRootParameter[EnvironmentManager::RootSignature::CaptureEnvironment::RC_Consts]
			.InitAsConstants(EnvironmentManager::RootConstant::CaptureEnvironment::Count, 5);
		slotRootParameter[EnvironmentManager::RootSignature::CaptureEnvironment::SI_ShadowMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[EnvironmentManager::RootSignature::CaptureEnvironment::SI_Textures_AlbedoMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[EnvironmentManager::RootSignature::CaptureEnvironment::SI_Textures_NormalMap]
			.InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[EnvironmentManager::RootSignature::GR_CaptureEnvironment]),
			L"EnvironmentMap_GR_CaptureEnvironment"));
	}
	// CaptureSkySphere
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[1]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[EnvironmentManager::RootSignature::CaptureSkySphere::Count]{};
		slotRootParameter[EnvironmentManager::RootSignature::CaptureSkySphere::CB_Pass]
			.InitAsConstantBufferView(0);
		slotRootParameter[EnvironmentManager::RootSignature::CaptureSkySphere::CB_ProjectToCube]
			.InitAsConstantBufferView(1);
		slotRootParameter[EnvironmentManager::RootSignature::CaptureSkySphere::CB_Object]
			.InitAsConstantBufferView(2);		
		slotRootParameter[EnvironmentManager::RootSignature::CaptureSkySphere::SI_EnvCubeMap]
			.InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[EnvironmentManager::RootSignature::GR_CaptureSkySphere]),
			L"EnvironmentMap_GR_CaptureSkySphere"));
	}

	return true;
}

bool D3D12EnvironmentManager::BuildPipelineStates() {
	// IntegrateBrdf
	{
		if (mInitData.Device->IsMeshShaderSupported()) {
			auto psoDesc = D3D12Util::FitToScreenMeshPsoDesc();

			psoDesc.pRootSignature = mRootSignatures[EnvironmentManager::RootSignature::GR_IntegrateBrdf].Get();
			{
				const auto MS = mInitData.ShaderManager->GetShader(
					mShaderHashes[EnvironmentManager::Shader::MS_IntegrateBrdf]);
				NullCheck(MS);
				const auto PS = mInitData.ShaderManager->GetShader(
					mShaderHashes[EnvironmentManager::Shader::PS_IntegrateBrdf]);
				NullCheck(PS);
				psoDesc.MS = {
					reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
				psoDesc.PS = {
					reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = EnvironmentManager::BrdfLutMapFormat;
			psoDesc.DepthStencilState.DepthEnable = FALSE;

			CheckReturn(D3D12Util::CreatePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[EnvironmentManager::PipelineState::MP_IntegrateBrdf]),
				L"EnvironmentMap_MP_IntegrateBrdf"));
		}
		else {
			auto psoDesc = D3D12Util::FitToScreenPsoDesc();

			psoDesc.pRootSignature = mRootSignatures[EnvironmentManager::RootSignature::GR_IntegrateBrdf].Get();
			{
				const auto VS = mInitData.ShaderManager->GetShader(
					mShaderHashes[EnvironmentManager::Shader::VS_IntegrateBrdf]);
				NullCheck(VS);
				const auto PS = mInitData.ShaderManager->GetShader(
					mShaderHashes[EnvironmentManager::Shader::PS_IntegrateBrdf]);
				NullCheck(PS);
				psoDesc.VS = {
					reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
				psoDesc.PS = {
					reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = EnvironmentManager::BrdfLutMapFormat;
			psoDesc.DepthStencilState.DepthEnable = FALSE;

			CheckReturn(D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[EnvironmentManager::PipelineState::GP_IntegrateBrdf]),
				L"EnvironmentMap_GP_IntegrateBrdf"));
		}
	}
	// DrawSkySphere
	{
		if (mInitData.Device->IsMeshShaderSupported()) {
			auto psoDesc = D3D12Util::DefaultMeshPsoDesc(DepthStencilBuffer::DepthStencilBufferFormat);

			psoDesc.pRootSignature = mRootSignatures[EnvironmentManager::RootSignature::GR_DrawSkySphere].Get();
			{
				const auto MS = mInitData.ShaderManager->GetShader(
					mShaderHashes[EnvironmentManager::Shader::MS_DrawSkySphere]);
				NullCheck(MS);
				const auto PS = mInitData.ShaderManager->GetShader(
					mShaderHashes[EnvironmentManager::Shader::PS_DrawSkySphere]);
				NullCheck(PS);
				psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;
			psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
			psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

			CheckReturn(D3D12Util::CreatePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[EnvironmentManager::PipelineState::MP_DrawSkySphere]),
				L"EnvironmentMap_MP_DrawSkySphere"));
		}
		else {
			const auto inputLayout = D3D12Util::StaticVertexInputLayoutDesc();
			auto psoDesc = D3D12Util::DefaultPsoDesc(inputLayout, DepthStencilBuffer::DepthStencilBufferFormat);

			psoDesc.pRootSignature = mRootSignatures[EnvironmentManager::RootSignature::GR_DrawSkySphere].Get();
			{
				const auto VS = mInitData.ShaderManager->GetShader(
					mShaderHashes[EnvironmentManager::Shader::VS_DrawSkySphere]);
				NullCheck(VS);
				const auto PS = mInitData.ShaderManager->GetShader(
					mShaderHashes[EnvironmentManager::Shader::PS_DrawSkySphere]);
				NullCheck(PS);
				psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;
			psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
			psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

			CheckReturn(D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[EnvironmentManager::PipelineState::GP_DrawSkySphere]),
				L"EnvironmentMap_GP_DrawSkySphere"));
		}
	}
	// CaptureEnvironment
	{
		auto psoDesc = D3D12Util::DefaultPsoDesc(D3D12Util::StaticVertexInputLayoutDesc(), DepthStencilBuffer::DepthStencilBufferFormat);
		psoDesc.pRootSignature = mRootSignatures[EnvironmentManager::RootSignature::GR_CaptureEnvironment].Get();
		{
			const auto VS = mInitData.ShaderManager->GetShader(
				mShaderHashes[EnvironmentManager::Shader::VS_CaptureEnvironment]);
			NullCheck(VS);
			const auto GS = mInitData.ShaderManager->GetShader(
				mShaderHashes[EnvironmentManager::Shader::GS_CaptureEnvironment]);
			NullCheck(GS);
			const auto PS = mInitData.ShaderManager->GetShader(
				mShaderHashes[EnvironmentManager::Shader::PS_CaptureEnvironment]);
			NullCheck(PS);
			psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
			psoDesc.GS = { reinterpret_cast<BYTE*>(GS->GetBufferPointer()), GS->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = HDR_FORMAT;
		psoDesc.DSVFormat = EnvironmentManager::DepthBufferArrayFormat;

		CheckReturn(D3D12Util::CreateGraphicsPipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[EnvironmentManager::PipelineState::GP_CaptureEnvironment]),
			L"EnvironmentMap_GP_CaptureEnvironment"));
	}
	// CaptureSkySphere
	{
		auto psoDesc = D3D12Util::DefaultPsoDesc(D3D12Util::StaticVertexInputLayoutDesc(), DepthStencilBuffer::DepthStencilBufferFormat);
		psoDesc.pRootSignature = mRootSignatures[EnvironmentManager::RootSignature::GR_CaptureSkySphere].Get();
		{
			const auto VS = mInitData.ShaderManager->GetShader(
				mShaderHashes[EnvironmentManager::Shader::VS_CaptureSkySphere]);
			NullCheck(VS);
			const auto GS = mInitData.ShaderManager->GetShader(
				mShaderHashes[EnvironmentManager::Shader::GS_CaptureSkySphere]);
			NullCheck(GS);
			const auto PS = mInitData.ShaderManager->GetShader(
				mShaderHashes[EnvironmentManager::Shader::PS_CaptureSkySphere]);
			NullCheck(PS);
			psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
			psoDesc.GS = { reinterpret_cast<BYTE*>(GS->GetBufferPointer()), GS->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.NumRenderTargets = 1;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
		psoDesc.RTVFormats[0] = HDR_FORMAT;
		psoDesc.DSVFormat = EnvironmentManager::DepthBufferArrayFormat;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

		CheckReturn(D3D12Util::CreateGraphicsPipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[EnvironmentManager::PipelineState::GP_CaptureSkySphere]),
			L"EnvironmentMap_GP_CaptureSkySphere"));
	}

	return true;
}

bool D3D12EnvironmentManager::AllocateDescriptors() {
	CheckReturn(mpDescHeap->AllocateCbvSrvUav(1, mhBrdfLutMapSrv));
	CheckReturn(mpDescHeap->AllocateRtv(1, mhBrdfLutMapRtv));
	CheckReturn(mpDescHeap->AllocateDsv(1, mhDepthBufferArrayDsv));

	mhReflectionProbeCapturedCubeSrvs.resize(32);
	mhReflectionProbeCapturedCubeRtvs.resize(32);
	for (UINT i = 0; i < 32; ++i) {
		CheckReturn(mpDescHeap->AllocateCbvSrvUav(1, mhReflectionProbeCapturedCubeSrvs[i]));
		CheckReturn(mpDescHeap->AllocateRtv(1, mhReflectionProbeCapturedCubeRtvs[i]));
	}

	CheckReturn(BuildDescriptors());

	return true;
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12EnvironmentManager::GetReflectionProbeCapturedCubeSrv(ReflectionProbeID id) const {
	assert(id.Slot < mReflectionProbes.size());

	auto& slot = mReflectionProbes[id.Slot];
	assert(slot->Alive && "Invalid reflection probe id");
	assert(slot->Generation == id.Generation && "Stale reflection probe id");

	return mpDescHeap->GetGpuHandle(slot->CapturedCubeSrv);
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12EnvironmentManager::GetReflectionProbeCapturedCubeSrv(size_t index) const {
	assert(index < mReflectionProbes.size());

	return mpDescHeap->GetGpuHandle(mReflectionProbes[index]->CapturedCubeSrv);
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12EnvironmentManager::GetReflectionProbeCapturedCubeSrv(size_t index, UINT face) const {
	assert(index < mReflectionProbes.size());
	assert(face < 6);

	return mpDescHeap->GetGpuHandle(mReflectionProbes[index]->CapturedCubeSrvFace[face]);
}

ReflectionProbeID D3D12EnvironmentManager::AddReflectionProbe(const ReflectionProbeDesc& desc) {
	std::uint32_t slot = UINT32_MAX;

	if (!mFreeProbeSlots.empty())
	{
		slot = mFreeProbeSlots.back();
		mFreeProbeSlots.pop_back();

		auto& probeSlot = mReflectionProbes[slot];
		probeSlot->Desc = desc;
		probeSlot->Alive = true;

		return { slot, probeSlot->Generation };
	}

	slot = static_cast<std::uint32_t>(mReflectionProbes.size());

	auto newSlot = std::make_unique<D3D12ReflectionProbeSlot>();
	BuildReflectionProbeResources(newSlot.get(), desc, slot);

	mReflectionProbes.push_back(std::move(newSlot));

	return { slot, 1 };
}

void D3D12EnvironmentManager::UpdateReflectionProbe(ReflectionProbeID id, const ReflectionProbeDesc& desc) {
	if (id.Slot >= mReflectionProbes.size()) return;

	auto& slot = mReflectionProbes[id.Slot];
	if (!slot->Alive) return;
	if (slot->Generation != id.Generation) return;

	slot->Desc = desc;
}

void D3D12EnvironmentManager::RemoveReflectionProbe(ReflectionProbeID id) {
	if (id.Slot >= mReflectionProbes.size()) return;

	auto& slot = mReflectionProbes[id.Slot];
	if (!slot->Alive) return;
	if (slot->Generation != id.Generation) return;

	slot->Alive = false;
	++slot->Generation; // 이전 핸들 무효화
	mFreeProbeSlots.push_back(id.Slot);
}

ReflectionProbeDesc* D3D12EnvironmentManager::GetReflectionProbe(ReflectionProbeID id) {
	if (id.Slot >= mReflectionProbes.size()) return nullptr;

	auto& slot = mReflectionProbes[id.Slot];
	if (!slot->Alive) return nullptr;
	if (slot->Generation != id.Generation) return nullptr;

	return &slot->Desc;
}

const ReflectionProbeDesc* D3D12EnvironmentManager::GetReflectionProbe(ReflectionProbeID id) const {
	if (id.Slot >= mReflectionProbes.size()) return nullptr;

	auto& slot = mReflectionProbes[id.Slot];
	if (!slot->Alive) return nullptr;
	if (slot->Generation != id.Generation) return nullptr;

	return &slot->Desc;
}

ReflectionProbeDesc* D3D12EnvironmentManager::GetReflectionProbe(size_t index) {
	if (index >= mReflectionProbes.size()) return nullptr;

	auto& slot = mReflectionProbes[index];
	if (!slot->Alive) return nullptr;

	return &slot->Desc;
}

const ReflectionProbeDesc* D3D12EnvironmentManager::GetReflectionProbe(size_t index) const {
	if (index >= mReflectionProbes.size()) return nullptr;

	auto& slot = mReflectionProbes[index];
	if (!slot->Alive) return nullptr;

	return &slot->Desc;
}

D3D12ReflectionProbeSlot* D3D12EnvironmentManager::GetReflectionProbeSlot(size_t index) {
	if (index >= mReflectionProbes.size()) return nullptr;

	auto& slot = mReflectionProbes[index];
	if (!slot->Alive) return nullptr;

	return slot.get();
}

const D3D12ReflectionProbeSlot* D3D12EnvironmentManager::GetReflectionProbeSlot(size_t index) const {
	if (index >= mReflectionProbes.size()) return nullptr;

	auto& slot = mReflectionProbes[index];
	if (!slot->Alive) return nullptr;

	return slot.get();
}

bool D3D12EnvironmentManager::BakeReflectionProbes(
	D3D12FrameResource* const pFrameResource
	, const std::vector<struct D3D12RenderItem*>& ritems) {
	CheckReturn(mInitData.CommandObject->ResetImmediateCommandList(
		pFrameResource->ImmediateCommandAllocator(),
		mPipelineStates[EnvironmentManager::PipelineState::GP_CaptureEnvironment].Get()));

	const auto CmdList = mInitData.CommandObject->GetImmediateCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetGraphicsRootSignature(
			mRootSignatures[EnvironmentManager::RootSignature::GR_CaptureEnvironment].Get());

		CmdList->RSSetViewports(1, &mViewport);
		CmdList->RSSetScissorRects(1, &mScissorRect);

		CmdList->SetGraphicsRootConstantBufferView(
			EnvironmentManager::RootSignature::CaptureEnvironment::CB_Pass,
			pFrameResource->PassCB.CBAddress());
		CmdList->SetGraphicsRootConstantBufferView(
			EnvironmentManager::RootSignature::CaptureEnvironment::CB_Light,
			pFrameResource->LightCB.CBAddress());

		for (UINT i = 0, end = static_cast<UINT>(mReflectionProbes.size()); i < end; ++i) {
			CmdList->SetGraphicsRootConstantBufferView(
				EnvironmentManager::RootSignature::CaptureEnvironment::CB_ProjectToCube,
				pFrameResource->ProjectToCubeCB.CBAddress(i));

			auto& probe = mReflectionProbes[i];			

			probe->CapturedCube->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
			auto rtv = mpDescHeap->GetCpuHandle(probe->CapturedCubeRtv);
			CmdList->ClearRenderTargetView(rtv, EnvironmentManager::EnvironmentCubeMapClearValues, 0, nullptr);

			mDepthBufferArray->Transite(CmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			auto dsv = mpDescHeap->GetCpuHandle(mhDepthBufferArrayDsv);
			CmdList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

			CmdList->OMSetRenderTargets(1, &rtv, TRUE, &dsv);

			CheckReturn(DrawRenderItems(pFrameResource, CmdList, ritems, 0));
		}
	}

	CheckReturn(mInitData.CommandObject->ExecuteImmediateCommandList());
	pFrameResource->mImmediateFence = mInitData.CommandObject->SignalImmediate();

	return true;
}

bool D3D12EnvironmentManager::BakeReflectionProbesWithSkySphere(
	D3D12FrameResource* const pFrameResource
	, const std::vector<struct D3D12RenderItem*>& ritems) {
	CheckReturn(mInitData.CommandObject->ResetImmediateCommandList(
		pFrameResource->ImmediateCommandAllocator(),
		mPipelineStates[EnvironmentManager::PipelineState::GP_CaptureSkySphere].Get()));

	const auto CmdList = mInitData.CommandObject->GetImmediateCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetGraphicsRootSignature(
			mRootSignatures[EnvironmentManager::RootSignature::GR_CaptureSkySphere].Get());

		CmdList->RSSetViewports(1, &mViewport);
		CmdList->RSSetScissorRects(1, &mScissorRect);

		CmdList->SetGraphicsRootConstantBufferView(
			EnvironmentManager::RootSignature::CaptureSkySphere::CB_Pass,
			pFrameResource->PassCB.CBAddress());

		for (UINT i = 0, end = static_cast<UINT>(mReflectionProbes.size()); i < end; ++i) {
			CmdList->SetGraphicsRootConstantBufferView(
				EnvironmentManager::RootSignature::CaptureSkySphere::CB_ProjectToCube,
				pFrameResource->ProjectToCubeCB.CBAddress(i));

			auto& probe = mReflectionProbes[i];

			probe->CapturedCube->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
			auto rtv = mpDescHeap->GetCpuHandle(probe->CapturedCubeRtv);

			mDepthBufferArray->Transite(CmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			auto dsv = mpDescHeap->GetCpuHandle(mhDepthBufferArrayDsv);

			CmdList->OMSetRenderTargets(1, &rtv, TRUE, &dsv);

			CheckReturn(DrawSkySphereRenderItems(pFrameResource, CmdList, ritems));
		}
	}

	CheckReturn(mInitData.CommandObject->ExecuteImmediateCommandList());
	pFrameResource->mImmediateFence = mInitData.CommandObject->SignalImmediate();

	return true;
}

ProbeSampleResult D3D12EnvironmentManager::FindBestProbe(const Mat4& world) const {
	ProbeSampleResult result{};

	const Vec3 pos = ExtractWorldPosition(world);

	float bestScore = FLT_MAX;
	int bestPriority = INT_MIN;
	std::uint32_t bestSlot = UINT32_MAX;

	for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(mReflectionProbes.size()); ++i) {
		const auto& probe = mReflectionProbes[i];
		if (!probe || !probe->Alive)
			continue;

		const auto& desc = probe->Desc;
		if (!desc.Enabled)
			continue;

		if (!Contains(desc, pos))
			continue;

		const float score = CalcScore(desc, pos);

		// 우선 Priority 높은 것 우선, 같으면 더 가까운 것
		if (desc.Priority > bestPriority ||
			(desc.Priority == bestPriority && score < bestScore)) {
			bestPriority = desc.Priority;
			bestScore = score;
			bestSlot = i;
		}
	}

	if (bestSlot != UINT32_MAX) {
		result.ProbeSlot = bestSlot;
		result.Blend = 1.f;     // 나중에 BlendDistance 반영 가능
		result.UseGlobal = false;
	}
	else {
		result.ProbeSlot = UINT32_MAX;
		result.Blend = 1.f;
		result.UseGlobal = true;
	}

	return result;
}

bool D3D12EnvironmentManager::DrawBrdfLutMap(D3D12FrameResource* const pFrameResource) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[
			mInitData.Device->IsMeshShaderSupported()
				? EnvironmentManager::PipelineState::MP_IntegrateBrdf
				: EnvironmentManager::PipelineState::GP_IntegrateBrdf].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetGraphicsRootSignature(
			mRootSignatures[EnvironmentManager::RootSignature::GR_IntegrateBrdf].Get());

		CmdList->RSSetViewports(1, &mViewport);
		CmdList->RSSetScissorRects(1, &mScissorRect);

		mBrdfLutMap->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

		auto rtv = mpDescHeap->GetCpuHandle(mhBrdfLutMapRtv);
		CmdList->OMSetRenderTargets(1, &rtv, TRUE, nullptr);

		CmdList->SetGraphicsRootConstantBufferView(
			EnvironmentManager::RootSignature::IntegrateBrdf::CB_Pass, 
			pFrameResource->PassCB.CBAddress());

		if (mInitData.Device->IsMeshShaderSupported()) {
			CmdList->DispatchMesh(1, 1, 1);
		}
		else {
			CmdList->IASetVertexBuffers(0, 0, nullptr);
			CmdList->IASetIndexBuffer(nullptr);
			CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			CmdList->DrawInstanced(6, 1, 0, 0);
		}
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12EnvironmentManager::DrawSkySphere(
	D3D12FrameResource* const pFrameResource
	, D3D12_VIEWPORT viewport
	, D3D12_RECT scissorRect
	, GpuResource* const backBuffer
	, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer
	, GpuResource* const depthBuffer
	, D3D12_CPU_DESCRIPTOR_HANDLE dio_depthStencil
	, const std::vector<D3D12RenderItem*>& ritems) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[mInitData.Device->IsMeshShaderSupported() ?
		EnvironmentManager::PipelineState::MP_DrawSkySphere: EnvironmentManager::PipelineState::GP_DrawSkySphere].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetGraphicsRootSignature(mRootSignatures[EnvironmentManager::RootSignature::GR_DrawSkySphere].Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		backBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		depthBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, &dio_depthStencil);

		CmdList->SetGraphicsRootConstantBufferView(
			EnvironmentManager::RootSignature::DrawSkySphere::CB_Pass,
			pFrameResource->PassCB.CBAddress());

		CheckReturn(DrawRenderItems(pFrameResource, CmdList, ritems));
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12EnvironmentManager::DrawRenderItems(
	D3D12FrameResource* const pFrameResource
	, ID3D12GraphicsCommandList6* const pCmdList
	, const std::vector<D3D12RenderItem*>& ritems) {
	for (size_t i = 0, end = ritems.size(); i < end; ++i) {
		const auto ri = ritems[i];

		pCmdList->SetGraphicsRootConstantBufferView(
			EnvironmentManager::RootSignature::DrawSkySphere::CB_Object,
			pFrameResource->ObjectCB.CBAddress(ri->ObjectCBIndex));

		if (ri->EnvironmentMap) {
			const auto envMapSrv = mpDescHeap->GetGpuHandle(ri->EnvironmentMap->Allocation);
			pCmdList->SetGraphicsRootDescriptorTable(
				EnvironmentManager::RootSignature::DrawSkySphere::SI_EnvCubeMap,
				envMapSrv);
		}

		EnvironmentManager::RootConstant::DrawSkySphere::Struct rc;
		rc.gIndexCount = ri->IndexCount;

		D3D12Util::SetRoot32BitConstants<EnvironmentManager::RootConstant::DrawSkySphere::Struct>(
			EnvironmentManager::RootSignature::DrawSkySphere::RC_Consts,
			EnvironmentManager::RootConstant::DrawSkySphere::Count,
			&rc,
			0,
			pCmdList,
			FALSE);	

		if (mInitData.Device->IsMeshShaderSupported()) {
			pCmdList->SetGraphicsRootShaderResourceView(
				EnvironmentManager::RootSignature::DrawSkySphere::SB_VertexBuffer
				, ri->MeshData->VertexBufferGPU->GetGPUVirtualAddress());
			pCmdList->SetGraphicsRootShaderResourceView(
				EnvironmentManager::RootSignature::DrawSkySphere::SB_IndexBuffer
				, ri->MeshData->IndexBufferGPU->GetGPUVirtualAddress());

			const UINT PrimCount = ri->IndexCount / 3;

			pCmdList->DispatchMesh(
				D3D12Util::CeilDivide(PrimCount, GBuffer::ThreadGroup::MeshShader::ThreadsPerGroup),
				1,
				1);
		}
		else {
			auto vb = ri->MeshData->VertexBufferView();
			auto iv = ri->MeshData->IndexBufferView();

			pCmdList->IASetVertexBuffers(0, 1, &vb);
			pCmdList->IASetIndexBuffer(&iv);
			pCmdList->IASetPrimitiveTopology(ri->PrimitiveType);

			pCmdList->DrawIndexedInstanced(
				ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
		}
	}

	return true;
}

bool D3D12EnvironmentManager::DrawRenderItems(
	D3D12FrameResource* const pFrameResource
	, ID3D12GraphicsCommandList6* const pCmdList
	, const std::vector<D3D12RenderItem*>& ritems
	, UINT) {
	for (size_t i = 0, end = ritems.size(); i < end; ++i) {
		const auto ri = ritems[i];

		pCmdList->SetGraphicsRootConstantBufferView(
			EnvironmentManager::RootSignature::CaptureEnvironment::CB_Object,
			pFrameResource->ObjectCB.CBAddress(ri->ObjectCBIndex));

		pCmdList->SetGraphicsRootConstantBufferView(
			EnvironmentManager::RootSignature::CaptureEnvironment::CB_Material,
			pFrameResource->MaterialCB.CBAddress(ri->MaterialCBIndex));

		EnvironmentManager::RootConstant::CaptureEnvironment::Struct rc;
		rc.gInvTexDim = { 1.f / mViewport.Width, 1.f / mViewport.Height };
		rc.gHasAlbedoMap = ri->AlbedoMap != nullptr;
		rc.gHasNormalMap = ri->NormalMap != nullptr;

		D3D12Util::SetRoot32BitConstants<EnvironmentManager::RootConstant::CaptureEnvironment::Struct>(
			EnvironmentManager::RootSignature::CaptureEnvironment::RC_Consts,
			EnvironmentManager::RootConstant::CaptureEnvironment::Count,
			&rc,
			0,
			pCmdList,
			FALSE);

		if (ri->AlbedoMap) {
			const auto albedoMapSrv = mpDescHeap->GetGpuHandle(ri->AlbedoMap->Allocation);
			pCmdList->SetGraphicsRootDescriptorTable(
				EnvironmentManager::RootSignature::CaptureEnvironment::SI_Textures_AlbedoMap,
				albedoMapSrv);
		}
		if (ri->NormalMap) {
			const auto normalMapSrv = mpDescHeap->GetGpuHandle(ri->NormalMap->Allocation);
			pCmdList->SetGraphicsRootDescriptorTable(
				EnvironmentManager::RootSignature::CaptureEnvironment::SI_Textures_NormalMap,
				normalMapSrv);
		}

		auto vb = ri->MeshData->VertexBufferView();
		auto iv = ri->MeshData->IndexBufferView();

		pCmdList->IASetVertexBuffers(0, 1, &vb);
		pCmdList->IASetIndexBuffer(&iv);
		pCmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		pCmdList->DrawIndexedInstanced(
			ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}

	return true;
}

bool D3D12EnvironmentManager::DrawSkySphereRenderItems(
	D3D12FrameResource* const pFrameResource
	, ID3D12GraphicsCommandList6* const pCmdList
	, const std::vector<D3D12RenderItem*>& ritems) {
	for (size_t i = 0, end = ritems.size(); i < end; ++i) {
		const auto ri = ritems[i];

		pCmdList->SetGraphicsRootConstantBufferView(
			EnvironmentManager::RootSignature::CaptureSkySphere::CB_Object,
			pFrameResource->ObjectCB.CBAddress(ri->ObjectCBIndex));

		if (ri->EnvironmentMap) {
			const auto envMapSrv = mpDescHeap->GetGpuHandle(ri->EnvironmentMap->Allocation);
			pCmdList->SetGraphicsRootDescriptorTable(
				EnvironmentManager::RootSignature::CaptureSkySphere::SI_EnvCubeMap,
				envMapSrv);
		}

		auto vb = ri->MeshData->VertexBufferView();
		auto iv = ri->MeshData->IndexBufferView();

		pCmdList->IASetVertexBuffers(0, 1, &vb);
		pCmdList->IASetIndexBuffer(&iv);
		pCmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		pCmdList->DrawIndexedInstanced(
			ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}

	return true;
}

bool D3D12EnvironmentManager::BuildResources() {
	D3D12_RESOURCE_DESC rscDesc;
	ZeroMemory(&rscDesc, sizeof(D3D12_RESOURCE_DESC));
	rscDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rscDesc.Alignment = 0;
	rscDesc.Width = EnvironmentManager::BrdfLutMapSize;
	rscDesc.Height = EnvironmentManager::BrdfLutMapSize;
	rscDesc.MipLevels = 1;
	rscDesc.SampleDesc.Count = 1;
	rscDesc.SampleDesc.Quality = 0;
	rscDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	const auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	// BrdfLutMap
	{
		rscDesc.Format = EnvironmentManager::BrdfLutMapFormat;
		rscDesc.DepthOrArraySize = 1;
		rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		CheckReturn(mBrdfLutMap->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"EnvironmentMap_BrdfLutMap"));
	}
	// DepthBufferArray
	{
		rscDesc.Format = EnvironmentManager::DepthBufferArrayFormat;
		rscDesc.DepthOrArraySize = 6;
		rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE optClear{};
		optClear.Format = Shadow::DepthMapFormat;
		optClear.DepthStencil.Depth = 1.f;
		optClear.DepthStencil.Stencil = 0;

		CheckReturn(mDepthBufferArray->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&optClear,
			L"EnvironmentMap_DepthBufferArray"));
	}

	return true;
}

bool D3D12EnvironmentManager::BuildDescriptors() {
	// Srv
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Format = EnvironmentManager::BrdfLutMapFormat;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.f;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.MipLevels = 1;

		D3D12Util::CreateShaderResourceView(
			mInitData.Device,
			mBrdfLutMap->Resource(),
			&srvDesc,
			mpDescHeap->GetCpuHandle(mhBrdfLutMapSrv));
	}
	// Rtv
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Format = EnvironmentManager::BrdfLutMapFormat;
		rtvDesc.Texture2D.PlaneSlice = 0;
		rtvDesc.Texture2D.MipSlice = 0;

		D3D12Util::CreateRenderTargetView(
			mInitData.Device,
			mBrdfLutMap->Resource(),
			&rtvDesc,
			mpDescHeap->GetCpuHandle(mhBrdfLutMapRtv));
	}
	// Dsv
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Format = Shadow::DepthMapFormat;
		dsvDesc.Texture2DArray.ArraySize = 6;
		dsvDesc.Texture2DArray.FirstArraySlice = 0;
		dsvDesc.Texture2DArray.MipSlice = 0;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

		D3D12Util::CreateDepthStencilView(
			mInitData.Device,
			mDepthBufferArray->Resource(),
			&dsvDesc,
			mpDescHeap->GetCpuHandle(mhDepthBufferArrayDsv));
	}

	return true;
}

bool D3D12EnvironmentManager::BuildReflectionProbeResources(
	D3D12ReflectionProbeSlot* pSlot
	, const ReflectionProbeDesc& desc
	, std::uint32_t slot) {
	pSlot->Desc = desc;
	pSlot->Alive = true;
	pSlot->Generation = 1;
	pSlot->TextureIndex = slot;

	D3D12_RESOURCE_DESC rscDesc{};
	rscDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rscDesc.Alignment = 0;
	rscDesc.Width = EnvironmentManager::CubeMapSize;
	rscDesc.Height = EnvironmentManager::CubeMapSize;
	rscDesc.DepthOrArraySize = 6;
	rscDesc.SampleDesc.Count = 1;
	rscDesc.SampleDesc.Quality = 0;
	rscDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	const auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
	rtvDesc.Texture2DArray.ArraySize = 6;
	rtvDesc.Texture2DArray.PlaneSlice = 0;
	rtvDesc.Texture2DArray.FirstArraySlice = 0;

	// CapturedCube
	{
		pSlot->CapturedCube = std::make_unique<GpuResource>();
		
		rscDesc.Format = EnvironmentManager::EnvironmentCubeMapFormat;
		rscDesc.MipLevels = 1;

		const CD3DX12_CLEAR_VALUE optClear(
			EnvironmentManager::EnvironmentCubeMapFormat,
			EnvironmentManager::EnvironmentCubeMapClearValues);

		CheckReturn(pSlot->CapturedCube->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&optClear,
			L"EnvironmentManager_ReflectionProbe_CapturedCube"));

		pSlot->CapturedCubeSrv = mhReflectionProbeCapturedCubeSrvs[slot];
		pSlot->CapturedCubeRtv = mhReflectionProbeCapturedCubeRtvs[slot];

		srvDesc.Format = EnvironmentManager::EnvironmentCubeMapFormat;
		srvDesc.TextureCube.MipLevels = 1;
		D3D12Util::CreateShaderResourceView(
			mInitData.Device,
			pSlot->CapturedCube->Resource(),
			&srvDesc,
			mpDescHeap->GetCpuHandle(pSlot->CapturedCubeSrv));

		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvFaceDesc{};
			srvFaceDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvFaceDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			srvFaceDesc.Texture2DArray.MostDetailedMip = 0;
			srvFaceDesc.Texture2DArray.MipLevels = 1;
			srvFaceDesc.Texture2DArray.ArraySize = 1;
			
			for (UINT i = 0; i < 6; ++i) {
				mpDescHeap->AllocateCbvSrvUav(1, pSlot->CapturedCubeSrvFace[i]);

				srvFaceDesc.Texture2DArray.FirstArraySlice = i;

				D3D12Util::CreateShaderResourceView(
					mInitData.Device,
					pSlot->CapturedCube->Resource(),
					&srvFaceDesc,
					mpDescHeap->GetCpuHandle(pSlot->CapturedCubeSrvFace[i]));
			}
		}

		rtvDesc.Format = EnvironmentManager::EnvironmentCubeMapFormat;
		rtvDesc.Texture2DArray.MipSlice = 0;
		D3D12Util::CreateRenderTargetView(
			mInitData.Device,
			pSlot->CapturedCube->Resource(),
			&rtvDesc,
			mpDescHeap->GetCpuHandle(pSlot->CapturedCubeRtv));
	}
	// DiffuseIrradiance
	{
		pSlot->DiffuseIrradiance = std::make_unique<GpuResource>();

		rscDesc.Format = EnvironmentManager::DiffuseIrradianceCubeMapFormat;
		rscDesc.MipLevels = 1;

		CheckReturn(pSlot->DiffuseIrradiance->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"EnvironmentManager_ReflectionProbe_CapturedCube"));

		mpDescHeap->AllocateCbvSrvUav(1, pSlot->DiffuseIrradianceSrv);
		mpDescHeap->AllocateRtv(1, pSlot->DiffuseIrradianceRtv);

		srvDesc.Format = EnvironmentManager::DiffuseIrradianceCubeMapFormat;
		srvDesc.TextureCube.MipLevels = 1;
		D3D12Util::CreateShaderResourceView(
			mInitData.Device,
			pSlot->DiffuseIrradiance->Resource(),
			&srvDesc,
			mpDescHeap->GetCpuHandle(pSlot->DiffuseIrradianceSrv));

		rtvDesc.Format = EnvironmentManager::DiffuseIrradianceCubeMapFormat;
		rtvDesc.Texture2DArray.MipSlice = 0;
		D3D12Util::CreateRenderTargetView(
			mInitData.Device,
			pSlot->DiffuseIrradiance->Resource(),
			&rtvDesc,
			mpDescHeap->GetCpuHandle(pSlot->DiffuseIrradianceRtv));
	}
	// SpecularIrradiance
	{
		pSlot->SpecularIrradiance = std::make_unique<GpuResource>();

		rscDesc.Format = EnvironmentManager::SpecularIrradianceCubeMapFormat;
		rscDesc.MipLevels = 5;

		CheckReturn(pSlot->SpecularIrradiance->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"EnvironmentManager_ReflectionProbe_CapturedCube"));

		mpDescHeap->AllocateCbvSrvUav(1, pSlot->SpecularIrradianceSrv);
		for (UINT mip = 0; mip < 5; ++mip) 
			mpDescHeap->AllocateRtv(1, pSlot->SpecularIrradianceRtvs[mip]);

		srvDesc.Format = EnvironmentManager::SpecularIrradianceCubeMapFormat;
		srvDesc.TextureCube.MipLevels = 5;
		D3D12Util::CreateShaderResourceView(
			mInitData.Device,
			pSlot->SpecularIrradiance->Resource(),
			&srvDesc,
			mpDescHeap->GetCpuHandle(pSlot->SpecularIrradianceSrv));

		rtvDesc.Format = EnvironmentManager::SpecularIrradianceCubeMapFormat;
		for (UINT mip = 0; mip < 5; ++mip) {
			rtvDesc.Texture2DArray.MipSlice = mip;
			D3D12Util::CreateRenderTargetView(
				mInitData.Device,
				pSlot->SpecularIrradiance->Resource(),
				&rtvDesc,
				mpDescHeap->GetCpuHandle(pSlot->SpecularIrradianceRtvs[mip]));
		}
	}

	return true;
}

bool D3D12EnvironmentManager::Contains(const ReflectionProbeDesc& probe, const Vec3& pos) const {
	switch (probe.Shape) {
	case EProbeShape::E_Sphere: return ContainsSphere(probe, pos);
	case EProbeShape::E_Box: return ContainsBox(probe, pos);
	default: return false;
	}
}

float D3D12EnvironmentManager::CalcScore(const ReflectionProbeDesc& probe, const Vec3& pos) const {
	auto center = ExtractWorldPosition(probe.World);

	const float dx = pos.x - center.x;
	const float dy = pos.y - center.y;
	const float dz = pos.z - center.z;

	const float distSq = dx * dx + dy * dy + dz * dz;

	// 점수가 작을수록 더 적합
	return distSq;
}

bool D3D12EnvironmentManager::ContainsBox(const ReflectionProbeDesc& probe, const Vec3& pos) const {
	auto center = ExtractWorldPosition(probe.World);

	const Vec3 minP = {
		center.x - probe.BoxExtents.x,
		center.y - probe.BoxExtents.y,
		center.z - probe.BoxExtents.z
	};

	const Vec3 maxP = {
		center.x + probe.BoxExtents.x,
		center.y + probe.BoxExtents.y,
		center.z + probe.BoxExtents.z
	};

	return
		pos.x >= minP.x && pos.x <= maxP.x &&
		pos.y >= minP.y && pos.y <= maxP.y &&
		pos.z >= minP.z && pos.z <= maxP.z;
}

bool D3D12EnvironmentManager::ContainsSphere(const ReflectionProbeDesc& probe, const Vec3& pos) const {
	auto center = ExtractWorldPosition(probe.World);

	const float dx = pos.x - center.x;
	const float dy = pos.y - center.y;
	const float dz = pos.z - center.z;

	const float distSq = dx * dx + dy * dy + dz * dz;

	return distSq <= probe.Radius * probe.Radius;
}