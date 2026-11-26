#include "Renderer/D3D12/pch.h"
#include "Renderer/D3D12/D3D12Dof.hpp"

#include "Renderer/Interface/ShaderArgumentManager.hpp"

#include "Renderer/D3D12/D3D12Device.hpp"
#include "Renderer/D3D12/D3D12Util.hpp"
#include "Renderer/D3D12/D3D12ShaderManager.hpp"
#include "Renderer/D3D12/D3D12FrameResource.hpp"
#include "Renderer/D3D12/D3D12CommandObject.hpp"
#include "Renderer/D3D12/D3D12RenderItem.hpp"
#include "Renderer/D3D12/D3D12GpuResource.hpp"

namespace {
	const WCHAR* const HLSL_CalcFocalDistance = L"CalcFocalDistance.hlsl";
	const WCHAR* const HLSL_CircleOfConfusion = L"CircleOfConfusion.hlsl";
	const WCHAR* const HLSL_Bokeh = L"Bokeh.hlsl";
	const WCHAR* const HLSL_BokehBlurNxN = L"BokehBlurNxN.hlsl";
}

D3D12Dof::D3D12Dof() {}

D3D12Dof::~D3D12Dof() {}

bool D3D12Dof::Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	mInitData = *reinterpret_cast<InitData*>(pData);

	for (size_t i = 0; i < Dof::Resource::Count; ++i)
		mResources[i] = std::make_unique<GpuResource>();

	CheckReturn(BuildSizeDependentResources());
	CheckReturn(BuildSizeIndependentResources());

	return true;
}

bool D3D12Dof::CompileShaders() {
	// CalcFocalDistance CS
	{
		const auto CS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_CalcFocalDistance, L"CS", L"cs_6_5");

		CheckReturn(mInitData.ShaderManager->AddShader(CS, mShaderHashes[Dof::Shader::CS_CaclFocalDistance]));
	}
	// CircleOfConfusion CS
	{
		const auto CS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_CircleOfConfusion, L"CS", L"cs_6_5");

		CheckReturn(mInitData.ShaderManager->AddShader(CS, mShaderHashes[Dof::Shader::CS_CircleOfConfusion]));
	}
	// Bokeh VS, MS, PS
	{
		const auto VS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_Bokeh, L"VS", L"vs_6_5");
		const auto MS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_Bokeh, L"MS", L"ms_6_5");
		const auto PS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_Bokeh, L"PS", L"ps_6_5");

		CheckReturn(mInitData.ShaderManager->AddShader(VS, mShaderHashes[Dof::Shader::VS_Bokeh]));
		CheckReturn(mInitData.ShaderManager->AddShader(MS, mShaderHashes[Dof::Shader::MS_Bokeh]));
		CheckReturn(mInitData.ShaderManager->AddShader(PS, mShaderHashes[Dof::Shader::PS_Bokeh]));
	}
	// BokehBlurNxN VS, MS, PS
	{
		const auto VS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_BokehBlurNxN, L"VS", L"vs_6_5");
		const auto MS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_BokehBlurNxN, L"MS", L"ms_6_5");
		const auto PS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_BokehBlurNxN, L"PS", L"ps_6_5");

		CheckReturn(mInitData.ShaderManager->AddShader(VS, mShaderHashes[Dof::Shader::VS_BokehBlurNxN]));
		CheckReturn(mInitData.ShaderManager->AddShader(MS, mShaderHashes[Dof::Shader::MS_BokehBlurNxN]));
		CheckReturn(mInitData.ShaderManager->AddShader(PS, mShaderHashes[Dof::Shader::PS_BokehBlurNxN]));
	}

	return true;
}

bool D3D12Dof::BuildRootSignatures() {
	decltype(auto) samplers = D3D12Util::GetStaticSamplers();

	// CalcFocalDistance
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[1]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[Dof::RootSignature::CalcFocalDistance::Count]{};
		slotRootParameter[Dof::RootSignature::CalcFocalDistance::CB_Pass]
			.InitAsConstantBufferView(0);
		slotRootParameter[Dof::RootSignature::CalcFocalDistance::SI_PositionMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[Dof::RootSignature::CalcFocalDistance::UO_FocalDistanceBuffer]
			.InitAsUnorderedAccessView(0);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[Dof::RootSignature::GR_CalcFocalDistance]),
			L"DOF_GR_CalcFocalDistance"));
	}
	// CircleOfConfusion
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[2]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[Dof::RootSignature::CircleOfConfusion::Count]{};
		slotRootParameter[Dof::RootSignature::CircleOfConfusion::CB_Pass]
			.InitAsConstantBufferView(0);
		slotRootParameter[Dof::RootSignature::CircleOfConfusion::RC_Consts]
			.InitAsConstants(Dof::RootConstant::CircleOfConfusion::Count, 1);
		slotRootParameter[Dof::RootSignature::CircleOfConfusion::SI_DepthMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[Dof::RootSignature::CircleOfConfusion::UI_FocalDistanceBuffer]
			.InitAsUnorderedAccessView(0);
		slotRootParameter[Dof::RootSignature::CircleOfConfusion::UO_CircleOfConfusionMap]
			.InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[Dof::RootSignature::GR_CircleOfConfusion]),
			L"DOF_GR_CircleOfConfusion"));
	}
	// Bokeh
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[3]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[Dof::RootSignature::Bokeh::Count]{};
		slotRootParameter[Dof::RootSignature::Bokeh::RC_Consts].InitAsConstants(Dof::RootConstant::Bokeh::Count, 0);
		slotRootParameter[Dof::RootSignature::Bokeh::CB_Pass].InitAsConstantBufferView(1);
		slotRootParameter[Dof::RootSignature::Bokeh::SI_BackBuffer].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[Dof::RootSignature::Bokeh::SI_CoCMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[Dof::RootSignature::Bokeh::SI_DepthMap].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[Dof::RootSignature::GR_Bokeh]),
			L"DOF_GR_Bokeh"));
	}
	// BokehBlurNxN
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[3]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[Dof::RootSignature::BokehBlurNxN::Count]{};
		slotRootParameter[Dof::RootSignature::BokehBlurNxN::RC_Consts]
			.InitAsConstants(Dof::RootConstant::BokehBlur3x3::Count, 0);
		slotRootParameter[Dof::RootSignature::BokehBlurNxN::CB_Pass].InitAsConstantBufferView(1);
		slotRootParameter[Dof::RootSignature::BokehBlurNxN::SI_InputMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[Dof::RootSignature::BokehBlurNxN::SI_CoCMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[Dof::RootSignature::BokehBlurNxN::SI_DepthMap]
			.InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[Dof::RootSignature::GR_BokehBlurNxN]),
			L"DOF_GR_BokehBlurNxN"));
	}

	return true;
}

bool D3D12Dof::BuildPipelineStates() {
	// CalcFocalDistance
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[Dof::RootSignature::GR_CalcFocalDistance].Get();
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		{
			const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Dof::Shader::CS_CaclFocalDistance]);
			NullCheck(CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}

		CheckReturn(D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[Dof::PipelineState::CP_CalcFocalDistance]),
			L"DOF_CP_CalcFocalDistance"));
	}
	// CircleOfConfusion
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[Dof::RootSignature::GR_CircleOfConfusion].Get();
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		{
			const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Dof::Shader::CS_CircleOfConfusion]);
			NullCheck(CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}

		CheckReturn(D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[Dof::PipelineState::CP_CircleOfConfusion]),
			L"DOF_CP_CircleOfConfusion"));
	}
	// Bokeh
	{
		if (mInitData.Device->IsMeshShaderSupported()) {
			auto psoDesc = D3D12Util::FitToScreenMeshPsoDesc();
			psoDesc.pRootSignature = mRootSignatures[Dof::RootSignature::GR_Bokeh].Get();
			{
				const auto MS = mInitData.ShaderManager->GetShader(mShaderHashes[Dof::Shader::MS_Bokeh]);
				NullCheck(MS);
				const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Dof::Shader::PS_Bokeh]);
				NullCheck(PS);
				psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			CheckReturn(D3D12Util::CreatePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[Dof::PipelineState::MP_Bokeh]),
				L"DOF_MP_Bokeh"));
		}
		else {
			auto psoDesc = D3D12Util::FitToScreenPsoDesc();
			psoDesc.pRootSignature = mRootSignatures[Dof::RootSignature::GR_Bokeh].Get();
			{
				const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[Dof::Shader::VS_Bokeh]);
				NullCheck(VS);
				const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Dof::Shader::PS_Bokeh]);
				NullCheck(PS);
				psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			CheckReturn(D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[Dof::PipelineState::GP_Bokeh]),
				L"DOF_GP_Bokeh"));
		}
	}
	// BokehBlurNxN
	{
		if (mInitData.Device->IsMeshShaderSupported()) {
			auto psoDesc = D3D12Util::FitToScreenMeshPsoDesc();
			psoDesc.pRootSignature = mRootSignatures[Dof::RootSignature::GR_BokehBlurNxN].Get();
			{
				const auto MS = mInitData.ShaderManager->GetShader(mShaderHashes[Dof::Shader::MS_BokehBlurNxN]);
				NullCheck(MS);
				const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Dof::Shader::PS_BokehBlurNxN]);
				NullCheck(PS);
				psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			CheckReturn(D3D12Util::CreatePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[Dof::PipelineState::MP_BokehBlurNxN]),
				L"DOF_MP_BokehBlurNxN"));
		}
		else {
			auto psoDesc = D3D12Util::FitToScreenPsoDesc();
			psoDesc.pRootSignature = mRootSignatures[Dof::RootSignature::GR_BokehBlurNxN].Get();
			{
				const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[Dof::Shader::VS_BokehBlurNxN]);
				NullCheck(VS);
				const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Dof::Shader::PS_BokehBlurNxN]);
				NullCheck(PS);
				psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			CheckReturn(D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[Dof::PipelineState::GP_BokehBlurNxN]),
				L"DOF_GP_BokehBlurNxN"));
		}
	}

	return true;
}

bool D3D12Dof::AllocateDescriptors() {
	CheckReturn(mpDescHeap->AllocateCbvSrvUav(1, mhCircleOfConfusionMapSrv));
	CheckReturn(mpDescHeap->AllocateCbvSrvUav(1, mhCircleOfConfusionMapUav));

	CheckReturn(BuildDescriptors());

	return true;
}

bool D3D12Dof::OnResize(unsigned width, unsigned height) {
	mInitData.Width = width;
	mInitData.Height = height;

	CheckReturn(BuildSizeDependentResources());
	CheckReturn(BuildDescriptors());

	return true;
}

bool D3D12Dof::CalcFocalDistance(
	D3D12FrameResource* const pFrameResource
	, GpuResource* const pPositionMap, D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[Dof::PipelineState::CP_CalcFocalDistance].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetComputeRootSignature(mRootSignatures[Dof::RootSignature::GR_CalcFocalDistance].Get());

		CmdList->SetComputeRootConstantBufferView(Dof::RootSignature::CalcFocalDistance::CB_Pass, pFrameResource->PassCB.CBAddress());

		pPositionMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		const auto FocalDistBuffer = mResources[Dof::Resource::E_FocalDistanceBuffer].get();
		FocalDistBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Util::UavBarrier(CmdList, FocalDistBuffer);

		CmdList->SetComputeRootDescriptorTable(Dof::RootSignature::CalcFocalDistance::SI_PositionMap, si_positionMap);

		CmdList->SetComputeRootUnorderedAccessView(
			Dof::RootSignature::CalcFocalDistance::UO_FocalDistanceBuffer,
			mResources[Dof::Resource::E_FocalDistanceBuffer]->Resource()->GetGPUVirtualAddress());

		CmdList->Dispatch(
			D3D12Util::CeilDivide(mInitData.Width, Dof::ThreadGroup::Default::Width),
			D3D12Util::CeilDivide(mInitData.Height, Dof::ThreadGroup::Default::Height),
			Dof::ThreadGroup::Default::Depth);
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12Dof::CircleOfConfusion(
	D3D12FrameResource* const pFrameResource,
	GpuResource* const pDepthMap, D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap
	, float focusRange) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[Dof::PipelineState::CP_CircleOfConfusion].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetComputeRootSignature(mRootSignatures[Dof::RootSignature::GR_CircleOfConfusion].Get());

		CmdList->SetComputeRootConstantBufferView(Dof::RootSignature::CircleOfConfusion::CB_Pass, pFrameResource->PassCB.CBAddress());

		Dof::RootConstant::CircleOfConfusion::Struct rc{};
		rc.gFocusRange = focusRange;

		D3D12Util::SetRoot32BitConstants<
			Dof::RootConstant::CircleOfConfusion::Struct>(
				Dof::RootSignature::CircleOfConfusion::RC_Consts,
				Dof::RootConstant::CircleOfConfusion::Count,
				&rc,
				0,
				CmdList,
				TRUE);

		pDepthMap->Transite(CmdList,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		const auto FocalDistBuffer = mResources[Dof::Resource::E_FocalDistanceBuffer].get();
		FocalDistBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Util::UavBarrier(CmdList, FocalDistBuffer);

		CmdList->SetComputeRootDescriptorTable(Dof::RootSignature::CircleOfConfusion::SI_DepthMap, si_depthMap);

		CmdList->SetComputeRootUnorderedAccessView(
			Dof::RootSignature::CircleOfConfusion::UI_FocalDistanceBuffer,
			mResources[Dof::Resource::E_FocalDistanceBuffer]->Resource()->GetGPUVirtualAddress());

		CmdList->SetComputeRootDescriptorTable(
			Dof::RootSignature::CircleOfConfusion::UO_CircleOfConfusionMap,
			mpDescHeap->GetGpuHandle(mhCircleOfConfusionMapUav));

		CmdList->Dispatch(
			D3D12Util::CeilDivide(mInitData.Width, Dof::ThreadGroup::Default::Width),
			D3D12Util::CeilDivide(mInitData.Height, Dof::ThreadGroup::Default::Height),
			Dof::ThreadGroup::Default::Depth);
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12Dof::Bokeh(
	D3D12FrameResource* const pFrameResource
	, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect
	, GpuResource* const pBackBuffer,	D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer
	, GpuResource* const pBackBufferCopy, D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy
	, D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap
	, UINT numSamples, float bokehRadius, float threshold, float highlightPower, UINT bokehShape
	, bool protectDepthEdges) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[mInitData.Device->IsMeshShaderSupported() 
		? Dof::PipelineState::MP_Bokeh : Dof::PipelineState::GP_Bokeh].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetGraphicsRootSignature(mRootSignatures[Dof::RootSignature::GR_Bokeh].Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_DEST);

		CmdList->CopyResource(pBackBufferCopy->Resource(), pBackBuffer->Resource());

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		mResources[Dof::Resource::E_CircleOfConfusionMap]->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, nullptr);

		Dof::RootConstant::Bokeh::Struct rc{};
		rc.gSampleCount = static_cast<UINT>(numSamples);
		rc.gBokehRadius = bokehRadius;
		rc.gThreshold = threshold;
		rc.gHighlightPower = highlightPower;
		rc.gBokehShape = bokehShape;
		rc.gProtectDepthEdges = protectDepthEdges;

		D3D12Util::SetRoot32BitConstants<Dof::RootConstant::Bokeh::Struct>(
			Dof::RootSignature::Bokeh::RC_Consts,
			Dof::RootConstant::Bokeh::Count,
			&rc,
			0,
			CmdList,
			FALSE);

		CmdList->SetGraphicsRootConstantBufferView(
			Dof::RootSignature::Bokeh::CB_Pass, pFrameResource->PassCB.CBAddress());
		CmdList->SetGraphicsRootDescriptorTable(Dof::RootSignature::Bokeh::SI_BackBuffer, si_backBufferCopy);
		CmdList->SetGraphicsRootDescriptorTable(
			Dof::RootSignature::Bokeh::SI_CoCMap, mpDescHeap->GetGpuHandle(mhCircleOfConfusionMapSrv));
		CmdList->SetGraphicsRootDescriptorTable(Dof::RootSignature::Bokeh::SI_DepthMap, si_depthMap);

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

bool D3D12Dof::BokehBlur(
	D3D12FrameResource* const pFrameResource
	, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect
	, GpuResource* const pBackBuffer, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer
	, GpuResource* const pBackBufferCopy, D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy
	, D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap, float maxBlurRadius, bool protectDepthEdges) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[mInitData.Device->IsMeshShaderSupported()
		? Dof::PipelineState::MP_BokehBlurNxN : Dof::PipelineState::GP_BokehBlurNxN].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetGraphicsRootSignature(mRootSignatures[Dof::RootSignature::GR_BokehBlurNxN].Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_DEST);

		CmdList->CopyResource(pBackBufferCopy->Resource(), pBackBuffer->Resource());

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		mResources[Dof::Resource::E_CircleOfConfusionMap]->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, nullptr);

		Dof::RootConstant::BokehBlur3x3::Struct rc{};
		rc.gMaxBlurRadius = maxBlurRadius;
		rc.gProtectDepthEdges = protectDepthEdges;

		D3D12Util::SetRoot32BitConstants<Dof::RootConstant::BokehBlur3x3::Struct>(
			Dof::RootSignature::BokehBlurNxN::RC_Consts,
			Dof::RootConstant::BokehBlur3x3::Count,
			&rc,
			0,
			CmdList,
			FALSE);

		CmdList->SetGraphicsRootConstantBufferView(
			Dof::RootSignature::BokehBlurNxN::CB_Pass, pFrameResource->PassCB.CBAddress());
		CmdList->SetGraphicsRootDescriptorTable(Dof::RootSignature::BokehBlurNxN::SI_InputMap, si_backBufferCopy);
		CmdList->SetGraphicsRootDescriptorTable(
			Dof::RootSignature::BokehBlurNxN::SI_CoCMap, mpDescHeap->GetGpuHandle(mhCircleOfConfusionMapSrv));
		CmdList->SetGraphicsRootDescriptorTable(Dof::RootSignature::BokehBlurNxN::SI_DepthMap, si_depthMap);

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

bool D3D12Dof::BuildSizeDependentResources() {
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = mInitData.Width;
	texDesc.Height = mInitData.Height;
	texDesc.Alignment = 0;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	// CircleOfConfusion
	{
		texDesc.Format = Dof::CircleOfConfusionMapFormat;

		const auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		CheckReturn(mResources[Dof::Resource::E_CircleOfConfusionMap]->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"DOF_CircleOfConfusionMap"));
	}

	return true;
}

bool D3D12Dof::BuildSizeIndependentResources() {
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Format = DXGI_FORMAT_UNKNOWN;
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	texDesc.Width = 1 * sizeof(FLOAT);
	texDesc.Height = 1;
	texDesc.Alignment = 0;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	const auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	CheckReturn(mResources[Dof::Resource::E_FocalDistanceBuffer]->Initialize(
		mInitData.Device,
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		L"DOF_FocalDistanceBuffer"));

	return true;
}

bool D3D12Dof::BuildDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	uavDesc.Texture2D.PlaneSlice = 0;

	// CircleOfConfusion
	{
		srvDesc.Format = Dof::CircleOfConfusionMapFormat;
		uavDesc.Format = Dof::CircleOfConfusionMapFormat;

		const auto resource = mResources[Dof::Resource::E_CircleOfConfusionMap]->Resource();
		D3D12Util::CreateShaderResourceView(
			mInitData.Device, resource, &srvDesc, mpDescHeap->GetCpuHandle(mhCircleOfConfusionMapSrv));
		D3D12Util::CreateUnorderedAccessView(
			mInitData.Device, resource, nullptr, &uavDesc, mpDescHeap->GetCpuHandle(mhCircleOfConfusionMapUav));
	}

	return true;
}
