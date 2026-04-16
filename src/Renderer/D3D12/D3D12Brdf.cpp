#include "pch.h"
#include "Renderer/D3D12/D3D12Brdf.hpp"

#include "ShaderArgumentManager.hpp"

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

#include "Renderer/D3D12/D3D12EnvironmentManager.hpp"

using namespace DirectX;

namespace {
	const WCHAR* const HLSL_ComputeBRDF = L"D3D12ComputeBRDF.hlsl";
	const WCHAR* const HLSL_IntegrateIrradiance = L"D3D12IntegrateIrradiance.hlsl";
}

D3D12Brdf::D3D12Brdf() {}

D3D12Brdf::~D3D12Brdf() {}

bool D3D12Brdf::Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	mInitData = *reinterpret_cast<InitData*>(pData);

	return true;
}

bool D3D12Brdf::CompileShaders() {
	// ComputeBRDF
	{
		const auto VS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_ComputeBRDF, L"VS", L"vs_6_5");
		const auto MS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_ComputeBRDF, L"MS", L"ms_6_5");
		const auto PS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_ComputeBRDF, L"PS", L"ps_6_5");

		CheckReturn(mInitData.ShaderManager->AddShader(VS, mShaderHashes[BRDF::Shader::VS_ComputeBRDF]));
		CheckReturn(mInitData.ShaderManager->AddShader(MS, mShaderHashes[BRDF::Shader::MS_ComputeBRDF]));
		CheckReturn(mInitData.ShaderManager->AddShader(PS, mShaderHashes[BRDF::Shader::PS_ComputeBRDF]));

	}
	// IntegrateIrradiance
	{
		const auto VS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_IntegrateIrradiance, L"VS", L"vs_6_5");
		const auto MS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_IntegrateIrradiance, L"MS", L"ms_6_5");
		const auto PS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_IntegrateIrradiance, L"PS", L"ps_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(VS, mShaderHashes[BRDF::Shader::VS_IntegrateIrradiance]));
		CheckReturn(mInitData.ShaderManager->AddShader(MS, mShaderHashes[BRDF::Shader::MS_IntegrateIrradiance]));
		CheckReturn(mInitData.ShaderManager->AddShader(PS, mShaderHashes[BRDF::Shader::PS_IntegrateIrradiance]));
	}

	return true;
}

bool D3D12Brdf::BuildRootSignatures() {
	decltype(auto) samplers = D3D12Util::GetStaticSamplers();

	// ComputeBRDF
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[6]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[BRDF::RootSignature::ComputeBRDF::Count]{};
		slotRootParameter[BRDF::RootSignature::ComputeBRDF::CB_Pass].
			InitAsConstantBufferView(0);
		slotRootParameter[BRDF::RootSignature::ComputeBRDF::CB_Light].
			InitAsConstantBufferView(1);
		slotRootParameter[BRDF::RootSignature::ComputeBRDF::RC_Consts].
			InitAsConstants(BRDF::RootConstant::ComputeBRDF::Count, 2);
		slotRootParameter[BRDF::RootSignature::ComputeBRDF::SI_AlbedoMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[BRDF::RootSignature::ComputeBRDF::SI_NormalMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[BRDF::RootSignature::ComputeBRDF::SI_DepthMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[BRDF::RootSignature::ComputeBRDF::SI_RMSMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[BRDF::RootSignature::ComputeBRDF::SI_PositionMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[BRDF::RootSignature::ComputeBRDF::SI_ShadowMap]
			.InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[BRDF::RootSignature::GR_ComputeBRDF]),
			L"BRDF_GR_ComputeBRDF"));
	}
	// IntegrateIrradiance
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[12]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 7, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 8, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 9, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 32, 0, 1);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 32, 32, 1);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[BRDF::RootSignature::IntegrateIrradiance::Count]{};
		slotRootParameter[BRDF::RootSignature::IntegrateIrradiance::CB_Pass]
			.InitAsConstantBufferView(0);
		slotRootParameter[BRDF::RootSignature::IntegrateIrradiance::SB_ReflectionProbeMetaData]
			.InitAsShaderResourceView(0, 2);
		slotRootParameter[BRDF::RootSignature::IntegrateIrradiance::RC_Consts]
			.InitAsConstants(BRDF::RootConstant::IntegrateIrradiance::Count, 1);
		slotRootParameter[BRDF::RootSignature::IntegrateIrradiance::SI_BackBuffer]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[BRDF::RootSignature::IntegrateIrradiance::SI_AlbedoMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[BRDF::RootSignature::IntegrateIrradiance::SI_NormalMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[BRDF::RootSignature::IntegrateIrradiance::SI_DepthMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[BRDF::RootSignature::IntegrateIrradiance::SI_RMSMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[BRDF::RootSignature::IntegrateIrradiance::SI_PositionMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[BRDF::RootSignature::IntegrateIrradiance::SI_AOMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[BRDF::RootSignature::IntegrateIrradiance::SI_BrdfLutMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[BRDF::RootSignature::IntegrateIrradiance::SI_GlobalDiffuseIrradianceMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[BRDF::RootSignature::IntegrateIrradiance::SI_GlobalSpecularIrradianceMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[BRDF::RootSignature::IntegrateIrradiance::SI_LocalDiffuseIrradianceMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[BRDF::RootSignature::IntegrateIrradiance::SI_LocalSpecularIrradianceMap]
			.InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[BRDF::RootSignature::GR_IntegrateIrradiance]),
			L"BRDF_GR_IntegrateIrradiance"));
	}

	return true;
}

bool D3D12Brdf::BuildPipelineStates() {
	// ComputeBRDF
	{
		if (mInitData.Device->IsMeshShaderSupported()) {
			auto psoDesc = D3D12Util::FitToScreenMeshPsoDesc();
			psoDesc.pRootSignature = mRootSignatures[BRDF::RootSignature::GR_ComputeBRDF].Get();
			{
				const auto MS = mInitData.ShaderManager->GetShader(
					mShaderHashes[BRDF::Shader::MS_ComputeBRDF]);
				NullCheck(MS);
				const auto PS = mInitData.ShaderManager->GetShader(
					mShaderHashes[BRDF::Shader::PS_ComputeBRDF]);
				NullCheck(PS);
				psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;
			
			CheckReturn(D3D12Util::CreatePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[BRDF::PipelineState::MP_ComputeBRDF]),
				L"BRDF_MP_ComputeBRDF"));
		}
		else {
			auto psoDesc = D3D12Util::FitToScreenPsoDesc();
			psoDesc.pRootSignature = mRootSignatures[BRDF::RootSignature::GR_ComputeBRDF].Get();
			{
				const auto VS = mInitData.ShaderManager->GetShader(
					mShaderHashes[BRDF::Shader::VS_ComputeBRDF]);
				NullCheck(VS);
				const auto PS = mInitData.ShaderManager->GetShader(
					mShaderHashes[BRDF::Shader::PS_ComputeBRDF]);
				NullCheck(PS);
				psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			CheckReturn(D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[BRDF::PipelineState::GP_ComputeBRDF]),
				L"BRDF_GP_ComputeBRDF"));
		}

	}
	// IntegrateIrradiance
	{
		if (mInitData.Device->IsMeshShaderSupported()) {
			auto psoDesc = D3D12Util::FitToScreenMeshPsoDesc();
			psoDesc.pRootSignature = mRootSignatures[BRDF::RootSignature::GR_IntegrateIrradiance].Get();
			{
				const auto MS = mInitData.ShaderManager->GetShader(
					mShaderHashes[BRDF::Shader::MS_IntegrateIrradiance]);
				NullCheck(MS);
				const auto PS = mInitData.ShaderManager->GetShader(
					mShaderHashes[BRDF::Shader::PS_IntegrateIrradiance]);
				NullCheck(PS);
				psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			CheckReturn(D3D12Util::CreatePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[BRDF::PipelineState::MP_IntegrateIrradiance]),
				L"BRDF_MP_IntegrateIrradiance"));
		}
		else {
			auto psoDesc = D3D12Util::FitToScreenPsoDesc();
			psoDesc.pRootSignature = mRootSignatures[BRDF::RootSignature::GR_IntegrateIrradiance].Get();
			{
				const auto VS = mInitData.ShaderManager->GetShader(
					mShaderHashes[BRDF::Shader::VS_IntegrateIrradiance]);
				NullCheck(VS);
				const auto PS = mInitData.ShaderManager->GetShader(
					mShaderHashes[BRDF::Shader::PS_IntegrateIrradiance]);
				NullCheck(PS);
				psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			CheckReturn(D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[BRDF::PipelineState::GP_IntegrateIrradiance]),
				L"BRDF_GP_IntegrateIrradiance"));
		}

	}

	return true;
}

bool D3D12Brdf::ComputeBRDF(
	D3D12FrameResource* const pFrameResource
	, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect
	, GpuResource* const pBackBuffer, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer
	, GpuResource* const pAlbedoMap, D3D12_GPU_DESCRIPTOR_HANDLE si_albedoMap
	, GpuResource* const pNormalMap, D3D12_GPU_DESCRIPTOR_HANDLE si_normalMap
	, GpuResource* const pDepthMap, D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap
	, GpuResource* const pRMSMap, D3D12_GPU_DESCRIPTOR_HANDLE si_rmsMap
	, GpuResource* const pPositionMap, D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap
	, GpuResource* const pShadowMap, D3D12_GPU_DESCRIPTOR_HANDLE si_shadowMap) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[mInitData.Device->IsMeshShaderSupported() 
		? BRDF::PipelineState::MP_ComputeBRDF : BRDF::PipelineState::GP_ComputeBRDF].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetGraphicsRootSignature(mRootSignatures[BRDF::RootSignature::GR_ComputeBRDF].Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		pAlbedoMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pNormalMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pRMSMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pPositionMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pShadowMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, nullptr);

		CmdList->SetGraphicsRootConstantBufferView(
			BRDF::RootSignature::ComputeBRDF::CB_Pass, pFrameResource->PassCB.CBAddress());
		CmdList->SetGraphicsRootConstantBufferView(
			BRDF::RootSignature::ComputeBRDF::CB_Light, pFrameResource->LightCB.CBAddress());

		BRDF::RootConstant::ComputeBRDF::Struct rc;
		rc.gInvTexDim = { 
			1.f / static_cast<FLOAT>(mInitData.Width),
			1.f / static_cast<FLOAT>(mInitData.Height) };
		rc.gShadowEnabled = true;

		D3D12Util::SetRoot32BitConstants<BRDF::RootConstant::ComputeBRDF::Struct>(
			BRDF::RootSignature::ComputeBRDF::RC_Consts,
			BRDF::RootConstant::ComputeBRDF::Count,
			&rc,
			0,
			CmdList,
			FALSE);

		CmdList->SetGraphicsRootDescriptorTable(
			BRDF::RootSignature::ComputeBRDF::SI_AlbedoMap, si_albedoMap);
		CmdList->SetGraphicsRootDescriptorTable(
			BRDF::RootSignature::ComputeBRDF::SI_NormalMap, si_normalMap);
		CmdList->SetGraphicsRootDescriptorTable(
			BRDF::RootSignature::ComputeBRDF::SI_DepthMap, si_depthMap);
		CmdList->SetGraphicsRootDescriptorTable(
			BRDF::RootSignature::ComputeBRDF::SI_RMSMap, si_rmsMap);
		CmdList->SetGraphicsRootDescriptorTable(
			BRDF::RootSignature::ComputeBRDF::SI_PositionMap, si_positionMap);
		CmdList->SetGraphicsRootDescriptorTable(
			BRDF::RootSignature::ComputeBRDF::SI_ShadowMap, si_shadowMap);

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

bool D3D12Brdf::IntegrateIrradiance(
	D3D12FrameResource* const pFrameResource
	, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect
	, GpuResource* const pBackBuffer, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer
	, GpuResource* const pBackBufferCopy, D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy
	, GpuResource* const pAlbedoMap, D3D12_GPU_DESCRIPTOR_HANDLE si_albedoMap
	, GpuResource* const pNormalMap, D3D12_GPU_DESCRIPTOR_HANDLE si_normalMap
	, GpuResource* const pDepthMap, D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap
	, GpuResource* const pRMSMap, D3D12_GPU_DESCRIPTOR_HANDLE si_rmsMap
	, GpuResource* const pPositionMap, D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap
	, GpuResource* const pAOMap, D3D12_GPU_DESCRIPTOR_HANDLE si_aoMap) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[mInitData.Device->IsMeshShaderSupported() ?
		BRDF::PipelineState::MP_IntegrateIrradiance : BRDF::PipelineState::GP_IntegrateIrradiance].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetGraphicsRootSignature(mRootSignatures[
			BRDF::RootSignature::GR_IntegrateIrradiance].Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_DEST);

		CmdList->CopyResource(pBackBufferCopy->Resource(), pBackBuffer->Resource());

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		auto environmentManager = RENDER_PASS_MANAGER->Get<D3D12EnvironmentManager>();

		auto brdfLutMap = environmentManager->GetBrdfLutMap();
		auto si_brdfLutMap = environmentManager->GetBrdfLutMapSrv();

		auto diffuseIrradianceMap = environmentManager->GetGlobalDiffuseIrradianceMap();
		auto specularIrradianceMap = environmentManager->GetGlobalSpecularIrradianceMap();

		auto si_diffuseIrradianceMap = diffuseIrradianceMap ?
			mpDescHeap->GetGpuHandle(diffuseIrradianceMap->Allocation) : D3D12_GPU_DESCRIPTOR_HANDLE{};
		auto si_specularIrradianceMap = specularIrradianceMap ?
			mpDescHeap->GetGpuHandle(specularIrradianceMap->Allocation) : D3D12_GPU_DESCRIPTOR_HANDLE{};

		pAlbedoMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pNormalMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pRMSMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pPositionMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pAOMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		brdfLutMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		if (diffuseIrradianceMap) 
			diffuseIrradianceMap->Resource.Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		if (specularIrradianceMap) 
			specularIrradianceMap->Resource.Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		auto numProbes = environmentManager->GetReflectionProbeCount();
		for (UINT i = 0; i < numProbes; ++i) {
			auto diffuseIrradiance = environmentManager->GetReflectionProbeDiffuseIrradiance(i);
			if (diffuseIrradiance) diffuseIrradiance->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			auto specularIrradiance = environmentManager->GetReflectionProbeSpecularIrradiance(i);
			if (specularIrradiance) specularIrradiance->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}

		CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, nullptr);

		CmdList->SetGraphicsRootConstantBufferView(
			BRDF::RootSignature::IntegrateIrradiance::CB_Pass,
			pFrameResource->PassCB.CBAddress());

		CmdList->SetGraphicsRootShaderResourceView(
			BRDF::RootSignature::IntegrateIrradiance::SB_ReflectionProbeMetaData,
			pFrameResource->ProbeSB.Resource()->GetGPUVirtualAddress());

		BRDF::RootConstant::IntegrateIrradiance::Struct rc;
		rc.gAoEnabled = SHADER_ARGUMENT_MANAGER->AOEnabled;

		D3D12Util::SetRoot32BitConstants<BRDF::RootConstant::IntegrateIrradiance::Struct>(
			BRDF::RootSignature::IntegrateIrradiance::RC_Consts,
			BRDF::RootConstant::IntegrateIrradiance::Count,
			&rc,
			0,
			CmdList,
			FALSE);

		CmdList->SetGraphicsRootDescriptorTable(
			BRDF::RootSignature::IntegrateIrradiance::SI_BackBuffer, si_backBufferCopy);
		CmdList->SetGraphicsRootDescriptorTable(
			BRDF::RootSignature::IntegrateIrradiance::SI_AlbedoMap, si_albedoMap);
		CmdList->SetGraphicsRootDescriptorTable(
			BRDF::RootSignature::IntegrateIrradiance::SI_NormalMap, si_normalMap);
		CmdList->SetGraphicsRootDescriptorTable(
			BRDF::RootSignature::IntegrateIrradiance::SI_DepthMap, si_depthMap);
		CmdList->SetGraphicsRootDescriptorTable(
			BRDF::RootSignature::IntegrateIrradiance::SI_RMSMap, si_rmsMap);
		CmdList->SetGraphicsRootDescriptorTable(
			BRDF::RootSignature::IntegrateIrradiance::SI_PositionMap, si_positionMap);
		CmdList->SetGraphicsRootDescriptorTable(
			BRDF::RootSignature::IntegrateIrradiance::SI_AOMap, si_aoMap);
		CmdList->SetGraphicsRootDescriptorTable(
			BRDF::RootSignature::IntegrateIrradiance::SI_BrdfLutMap, si_brdfLutMap);
		if (diffuseIrradianceMap) 
			CmdList->SetGraphicsRootDescriptorTable(
				BRDF::RootSignature::IntegrateIrradiance::SI_GlobalDiffuseIrradianceMap, 
				si_diffuseIrradianceMap);
		if (specularIrradianceMap)
			CmdList->SetGraphicsRootDescriptorTable(
				BRDF::RootSignature::IntegrateIrradiance::SI_GlobalSpecularIrradianceMap, 
				si_specularIrradianceMap);

		auto si_localDiffuseIrradianceMap = environmentManager->GetReflectionProbeDiffuseIrradianceSrvs();
		auto si_localSpecularIrradianceMap = environmentManager->GetReflectionProbeSpecularIrradianceSrvs();

		CmdList->SetGraphicsRootDescriptorTable(
			BRDF::RootSignature::IntegrateIrradiance::SI_LocalDiffuseIrradianceMap,
			si_localDiffuseIrradianceMap);
		CmdList->SetGraphicsRootDescriptorTable(
			BRDF::RootSignature::IntegrateIrradiance::SI_LocalSpecularIrradianceMap,
			si_localSpecularIrradianceMap);

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