#include "pch.h"
#include "Renderer/D3D12/D3D12Vignette.hpp"

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

namespace {
	const WCHAR* const HLSL_Vignette = L"D3D12Vignette.hlsl";
}

D3D12Vignette::D3D12Vignette() {}

D3D12Vignette::~D3D12Vignette() {}

bool D3D12Vignette::Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	mInitData = *reinterpret_cast<InitData*>(pData);

	return true;
}

bool D3D12Vignette::CompileShaders() {
	const auto VS = D3D12ShaderManager::D3D12ShaderInfo(
		HLSL_Vignette, L"VS", L"vs_6_5");
	const auto MS = D3D12ShaderManager::D3D12ShaderInfo(
		HLSL_Vignette, L"MS", L"ms_6_5");
	const auto PS = D3D12ShaderManager::D3D12ShaderInfo(
		HLSL_Vignette, L"PS", L"ps_6_5");
	CheckReturn(mInitData.ShaderManager->AddShader(
		VS, mShaderHashes[Vignette::Shader::VS_Vignette]));
	CheckReturn(mInitData.ShaderManager->AddShader(
		MS, mShaderHashes[Vignette::Shader::MS_Vignette]));
	CheckReturn(mInitData.ShaderManager->AddShader(
		PS, mShaderHashes[Vignette::Shader::PS_Vignette]));

	return true;
}

bool D3D12Vignette::BuildRootSignatures() {
	decltype(auto) samplers = D3D12Util::GetStaticSamplers();

	CD3DX12_DESCRIPTOR_RANGE texTables[1]{}; UINT index = 0;
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

	index = 0;

	CD3DX12_ROOT_PARAMETER slotRootParameter[Vignette::RootSignature::Default::Count]{};
	slotRootParameter[Vignette::RootSignature::Default::RC_Consts]
		.InitAsConstants(Vignette::RootConstant::Default::Count, 0);
	slotRootParameter[Vignette::RootSignature::Default::SI_BackBuffer]
		.InitAsDescriptorTable(1, &texTables[index++]);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		D3D12Util::StaticSamplerCount, samplers,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	CheckReturn(D3D12Util::CreateRootSignature(
		mInitData.Device,
		rootSigDesc,
		IID_PPV_ARGS(&mRootSignature),
		L"Vignette_GR_Default"));

	return true;
}

bool D3D12Vignette::BuildPipelineStates() {
	if (mInitData.Device->IsMeshShaderSupported()) {
		auto psoDesc = D3D12Util::FitToScreenMeshPsoDesc();
		psoDesc.pRootSignature = mRootSignature.Get();
		{
			const auto MS = mInitData.ShaderManager->GetShader(
				mShaderHashes[Vignette::Shader::MS_Vignette]);
			NullCheck(MS);
			const auto PS = mInitData.ShaderManager->GetShader(
				mShaderHashes[Vignette::Shader::PS_Vignette]);
			NullCheck(PS);
			psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.RTVFormats[0] = HDR_FORMAT;

		CheckReturn(D3D12Util::CreatePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[Vignette::PipelineState::MP_Vignette]),
			L"Vignette_MP_Default"));
	}
	else {
		auto psoDesc = D3D12Util::FitToScreenPsoDesc();
		psoDesc.pRootSignature = mRootSignature.Get();
		{
			const auto VS = mInitData.ShaderManager->GetShader(
				mShaderHashes[Vignette::Shader::VS_Vignette]);
			NullCheck(VS);
			const auto PS = mInitData.ShaderManager->GetShader(
				mShaderHashes[Vignette::Shader::PS_Vignette]);
			NullCheck(PS);
			psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.RTVFormats[0] = HDR_FORMAT;

		CheckReturn(D3D12Util::CreateGraphicsPipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[Vignette::PipelineState::GP_Vignette]),
			L"Vignette_GP_Default"));
	}

	return true;
}

bool D3D12Vignette::ApplyVignette(
	D3D12FrameResource* const pFrameResource
	, const D3D12_VIEWPORT& viewport
	, const D3D12_RECT& scissorRect
	, GpuResource* const pBackBuffer
	, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer
	, GpuResource* const pBackBufferCopy
	, D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[mInitData.Device->IsMeshShaderSupported()
		? Vignette::PipelineState::MP_Vignette
		: Vignette::PipelineState::GP_Vignette].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetGraphicsRootSignature(mRootSignature.Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_DEST);

		CmdList->CopyResource(pBackBufferCopy->Resource(), pBackBuffer->Resource());

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, nullptr);

		Vignette::RootConstant::Default::Struct rc;
		rc.gStrength = SHADER_ARGUMENT_MANAGER->Vignette.Strength;

		D3D12Util::SetRoot32BitConstants<
			Vignette::RootConstant::Default::Struct>(
				Vignette::RootSignature::Default::RC_Consts,
				Vignette::RootConstant::Default::Count,
				&rc,
				0,
				CmdList,
				FALSE);

		CmdList->SetGraphicsRootDescriptorTable(
			Vignette::RootSignature::Default::SI_BackBuffer, si_backBufferCopy);

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