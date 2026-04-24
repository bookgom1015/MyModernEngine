#include "pch.h"
#include "Renderer/D3D12/D3D12MotionBlur.hpp"

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
	const WCHAR* const HLSL_MotionBlur = L"D3D12MotionBlur.hlsl";
}

D3D12MotionBlur::D3D12MotionBlur() {}

D3D12MotionBlur::~D3D12MotionBlur() {}

bool D3D12MotionBlur::Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	mInitData = *reinterpret_cast<InitData*>(pData);

	return true;
}

bool D3D12MotionBlur::CompileShaders() {
	const auto VS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_MotionBlur, L"VS", L"vs_6_5");
	CheckReturn(mInitData.ShaderManager->AddShader(VS, mShaderHashes[MotionBlur::Shader::VS_MotionBlur]));

	const auto MS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_MotionBlur, L"MS", L"ms_6_5");
	CheckReturn(mInitData.ShaderManager->AddShader(MS, mShaderHashes[MotionBlur::Shader::MS_MotionBlur]));

	const auto PS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_MotionBlur, L"PS", L"ps_6_5");
	CheckReturn(mInitData.ShaderManager->AddShader(PS, mShaderHashes[MotionBlur::Shader::PS_MotionBlur]));

	return true;
}

bool D3D12MotionBlur::BuildRootSignatures() {
	decltype(auto) samplers = D3D12Util::GetStaticSamplers();

	CD3DX12_DESCRIPTOR_RANGE texTables[3]{}; UINT index = 0;
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);

	index = 0;

	CD3DX12_ROOT_PARAMETER slotRootParameter[MotionBlur::RootSignature::Default::Count]{};
	slotRootParameter[MotionBlur::RootSignature::Default::RC_Consts]
		.InitAsConstants(MotionBlur::RootConstant::Default::Count, 0);
	slotRootParameter[MotionBlur::RootSignature::Default::SI_BackBuffer]
		.InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[MotionBlur::RootSignature::Default::SI_DepthMap]
		.InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[MotionBlur::RootSignature::Default::SI_VelocityMap]
		.InitAsDescriptorTable(1, &texTables[index++]);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		D3D12Util::StaticSamplerCount, samplers,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	CheckReturn(D3D12Util::CreateRootSignature(
		mInitData.Device,
		rootSigDesc,
		IID_PPV_ARGS(&mRootSignature),
		L"MotionBlur_GR_Default"));

	return true;
}

bool D3D12MotionBlur::BuildPipelineStates() {
	if (mInitData.Device->IsMeshShaderSupported()) {
		auto psoDesc = D3D12Util::FitToScreenMeshPsoDesc();
		psoDesc.pRootSignature = mRootSignature.Get();
		{
			const auto MS = mInitData.ShaderManager->GetShader(
				mShaderHashes[MotionBlur::Shader::MS_MotionBlur]);
			NullCheck(MS);
			const auto PS = mInitData.ShaderManager->GetShader(
				mShaderHashes[MotionBlur::Shader::PS_MotionBlur]);
			NullCheck(PS);
			psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = SDR_FORMAT;

		CheckReturn(D3D12Util::CreatePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[MotionBlur::PipelineState::MP_MotionBlur]),
			L"MotionBlur_MP_Default"));
	}
	else {
		auto psoDesc = D3D12Util::FitToScreenPsoDesc();
		psoDesc.pRootSignature = mRootSignature.Get();
		{
			const auto VS = mInitData.ShaderManager->GetShader(
				mShaderHashes[MotionBlur::Shader::VS_MotionBlur]);
			NullCheck(VS);
			const auto PS = mInitData.ShaderManager->GetShader(
				mShaderHashes[MotionBlur::Shader::PS_MotionBlur]);				
			NullCheck(PS);
			psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = SDR_FORMAT;

		CheckReturn(D3D12Util::CreateGraphicsPipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[MotionBlur::PipelineState::GP_MotionBlur]),
			L"MotionBlur_GP_Default"));
	}

	return true;
}

bool D3D12MotionBlur::ApplyMotionBlur(
	D3D12FrameResource* const pFrameResource
	, const D3D12_VIEWPORT& viewport
	, const D3D12_RECT& scissorRect
	, GpuResource* const pBackBuffer, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer
	, GpuResource* const pBackBufferCopy, D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy
	, GpuResource* const pDepthMap, D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap
	, GpuResource* const pVelocityMap, D3D12_GPU_DESCRIPTOR_HANDLE si_velocityMap) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[mInitData.Device->IsMeshShaderSupported()
		? MotionBlur::PipelineState::MP_MotionBlur : MotionBlur::PipelineState::GP_MotionBlur].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetGraphicsRootSignature(mRootSignature.Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_DEST);
		pDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_DEPTH_READ);
		pVelocityMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->CopyResource(pBackBufferCopy->Resource(), pBackBuffer->Resource());

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, nullptr);

		MotionBlur::RootConstant::Default::Struct rc;
		rc.gIntensity = SHADER_ARGUMENT_MANAGER->MotionBlur.Intensity;
		rc.gLimit = SHADER_ARGUMENT_MANAGER->MotionBlur.Limit;
		rc.gDepthBias = SHADER_ARGUMENT_MANAGER->MotionBlur.DepthBias;
		rc.gSampleCount = SHADER_ARGUMENT_MANAGER->MotionBlur.SampleCount;

		D3D12Util::SetRoot32BitConstants<MotionBlur::RootConstant::Default::Struct>(
			MotionBlur::RootSignature::Default::RC_Consts,
			MotionBlur::RootConstant::Default::Count,
			&rc,
			0,
			CmdList,
			FALSE);

		CmdList->SetGraphicsRootDescriptorTable(MotionBlur::RootSignature::Default::SI_BackBuffer, si_backBufferCopy);
		CmdList->SetGraphicsRootDescriptorTable(MotionBlur::RootSignature::Default::SI_DepthMap, si_depthMap);
		CmdList->SetGraphicsRootDescriptorTable(MotionBlur::RootSignature::Default::SI_VelocityMap, si_velocityMap);

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