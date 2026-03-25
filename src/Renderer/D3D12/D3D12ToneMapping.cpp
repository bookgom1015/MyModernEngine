#include "pch.h"
#include "Renderer/D3D12/D3D12ToneMapping.hpp"

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
	const WCHAR* const HLSL_ToneMapping = L"D3D12ToneMapping.hlsl";
}

D3D12ToneMapping::D3D12ToneMapping() {}

D3D12ToneMapping::~D3D12ToneMapping() {}

bool D3D12ToneMapping::Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	mInitData = *reinterpret_cast<InitData*>(pData);
	
	return true;
}

bool D3D12ToneMapping::CompileShaders() {
	const auto VS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_ToneMapping, L"VS", L"vs_6_5");
	const auto MS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_ToneMapping, L"MS", L"ms_6_5");
	const auto PS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_ToneMapping, L"PS", L"ps_6_5");
	CheckReturn(mInitData.ShaderManager->AddShader(VS, mShaderHashes[ToneMapping::Shader::VS_ToneMapping]));
	CheckReturn(mInitData.ShaderManager->AddShader(MS, mShaderHashes[ToneMapping::Shader::MS_ToneMapping]));
	CheckReturn(mInitData.ShaderManager->AddShader(PS, mShaderHashes[ToneMapping::Shader::PS_ToneMapping]));

	return true;
}

bool D3D12ToneMapping::BuildRootSignatures() {
	decltype(auto) samplers = D3D12Util::GetStaticSamplers();

	CD3DX12_DESCRIPTOR_RANGE texTables[1]{}; UINT index = 0;
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

	index = 0;

	CD3DX12_ROOT_PARAMETER slotRootParameter[ToneMapping::RootSignature::Default::Count]{};
	slotRootParameter[ToneMapping::RootSignature::Default::RC_Cosnts]
		.InitAsConstants(ToneMapping::RootConstant::Default::Count, 0);
	slotRootParameter[ToneMapping::RootSignature::Default::SI_Intermediate]
		.InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[ToneMapping::RootSignature::Default::UI_AvgLogLuminance]
		.InitAsUnorderedAccessView(0);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		D3D12Util::StaticSamplerCount, samplers,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	CheckReturn(D3D12Util::CreateRootSignature(
		mInitData.Device,
		rootSigDesc,
		IID_PPV_ARGS(&mRootSignature),
		L"ToneMapping_GR_Default"));

	return true;
}

bool D3D12ToneMapping::BuildPipelineStates() {
	// ToneMapping
	{
		if (mInitData.Device->IsMeshShaderSupported()) {
			auto psoDesc = D3D12Util::FitToScreenMeshPsoDesc();
			psoDesc.pRootSignature = mRootSignature.Get();
			{
				const auto MS = mInitData.ShaderManager->GetShader(mShaderHashes[
					ToneMapping::Shader::MS_ToneMapping]);
				NullCheck(MS);
				const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[
					ToneMapping::Shader::PS_ToneMapping]);
				NullCheck(PS);
				psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.RTVFormats[0] = SDR_FORMAT;

			CheckReturn(D3D12Util::CreatePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[ToneMapping::PipelineState::MP_ToneMapping]),
				L"ToneMapping_MP_ToneMapping"));
		}
		else {
			auto psoDesc = D3D12Util::FitToScreenPsoDesc();
			psoDesc.pRootSignature = mRootSignature.Get();
			{
				const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[
					ToneMapping::Shader::VS_ToneMapping]);
				NullCheck(VS);
				const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[
					ToneMapping::Shader::PS_ToneMapping]);
				NullCheck(PS);
				psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.RTVFormats[0] = SDR_FORMAT;

			CheckReturn(D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[ToneMapping::PipelineState::GP_ToneMapping]),
				L"ToneMapping_GP_ToneMapping"));
		}

	}
	return true;
}

bool D3D12ToneMapping::Apply(
	D3D12FrameResource* const pFrameResource
	, const D3D12_VIEWPORT& viewport
	, const D3D12_RECT& scissorRect
	, GpuResource* const pBackBuffer
	, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer
	, GpuResource* const pHdrMap
	, D3D12_GPU_DESCRIPTOR_HANDLE si_HdrMapSrv) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->CommandAllocator(),
		mPipelineStates[mInitData.Device->IsMeshShaderSupported() 
		? ToneMapping::PipelineState::MP_ToneMapping : ToneMapping::PipelineState::GP_ToneMapping].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetGraphicsRootSignature(mRootSignature.Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		pHdrMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, nullptr);

		ToneMapping::RootConstant::Default::Struct rc;
		rc.gExposure = 0.f;
		rc.gMiddleGrayKey = 0.f;
		rc.gTonemapperType = ToneMapping::E_ACES;


		D3D12Util::SetRoot32BitConstants<ToneMapping::RootConstant::Default::Struct>(
			ToneMapping::RootSignature::Default::RC_Cosnts,
			ToneMapping::RootConstant::Default::Count,
			&rc,
			0,
			CmdList,
			FALSE);

		CmdList->SetGraphicsRootDescriptorTable(
			ToneMapping::RootSignature::Default::SI_Intermediate, si_HdrMapSrv);
		//CmdList->SetGraphicsRootUnorderedAccessView(
		//	ToneMapping::RootSignature::Default::UI_AvgLogLuminance,
		//	pAvgLogLuminance->Resource()->GetGPUVirtualAddress());


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
}