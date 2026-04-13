#include "pch.h"
#include "Renderer/D3D12/D3D12VolumetricLight.hpp"

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
	const WCHAR* const HLSL_CalculateScatteringAndDensity = L"D3D12CalculateScatteringAndDensity.hlsl";
	const WCHAR* const HLSL_AccumulateSacttering = L"D3D12AccumulateSacttering.hlsl";
	const WCHAR* const HLSL_BlendScattering = L"D3D12BlendScattering.hlsl";
	const WCHAR* const HLSL_ApplyFog = L"D3D12ApplyFog.hlsl";
}

D3D12VolumetricLight::D3D12VolumetricLight() {}

D3D12VolumetricLight::~D3D12VolumetricLight() {}

bool D3D12VolumetricLight::Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	mInitData = *reinterpret_cast<InitData*>(pData);

	for (UINT i = 0; i < 2; ++i)
		mFrustumVolumeMaps[i] = std::make_unique<GpuResource>();

	CheckReturn(BuildResources());

	return true;
}

bool D3D12VolumetricLight::CompileShaders() {
	// CalculateScatteringAndDensity
	{
		const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
			HLSL_CalculateScatteringAndDensity, L"CS", L"cs_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[VolumetricLight::Shader::CS_CalculateScatteringAndDensity]));
	}
	// AccumulateScattering
	{
		const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
			HLSL_AccumulateSacttering, L"CS", L"cs_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[VolumetricLight::Shader::CS_AccumulateScattering]));
	}
	// BlendScattering
	{
		const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
			HLSL_BlendScattering, L"CS", L"cs_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[VolumetricLight::Shader::CS_BlendScattering]));
	}
	// ApplyFog
	{
		const auto VS = D3D12ShaderManager::D3D12ShaderInfo(
			HLSL_ApplyFog, L"VS", L"vs_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			VS, mShaderHashes[VolumetricLight::Shader::VS_ApplyFog]));

		// Default
		{
			const auto PS = D3D12ShaderManager::D3D12ShaderInfo(
				HLSL_ApplyFog, L"PS", L"ps_6_5");
			CheckReturn(mInitData.ShaderManager->AddShader(
				PS, mShaderHashes[VolumetricLight::Shader::PS_ApplyFog]));
		}
		// Tricubic Sampling
		{
			DxcDefine defines[] = {
				{ L"TriCubicSampling", L"1" },
			};
			const auto PS = D3D12ShaderManager::D3D12ShaderInfo(
				HLSL_ApplyFog, L"PS", L"ps_6_5", defines, _countof(defines));
			CheckReturn(mInitData.ShaderManager->AddShader(
				PS, mShaderHashes[VolumetricLight::Shader::PS_ApplyFog_Tricubic]));
		}
	}

	return true;
}

bool D3D12VolumetricLight::BuildRootSignatures() {
	decltype(auto) samplers = D3D12Util::GetStaticSamplers();

	// CalculateScatteringAndDensity
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[3]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[VolumetricLight::RootSignature::CalculateScatteringAndDensity::Count]{};
		slotRootParameter[VolumetricLight::RootSignature::CalculateScatteringAndDensity::CB_Pass]
			.InitAsConstantBufferView(0);
		slotRootParameter[VolumetricLight::RootSignature::CalculateScatteringAndDensity::CB_Light]
			.InitAsConstantBufferView(1);
		slotRootParameter[VolumetricLight::RootSignature::CalculateScatteringAndDensity::RC_Consts]
			.InitAsConstants(VolumetricLight::RootConstant::CalculateScatteringAndDensity::Count, 2);
		slotRootParameter[VolumetricLight::RootSignature::CalculateScatteringAndDensity::SI_DepthMaps]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[VolumetricLight::RootSignature::CalculateScatteringAndDensity::UO_FrustumVolumeMap]
			.InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[VolumetricLight::RootSignature::GR_CalculateScatteringAndDensity]),
			L"VolumetricLight_GR_CalculateScatteringAndDensity"));
	}
	// AccumulateScattering
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[1]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[VolumetricLight::RootSignature::AccumulateScattering::Count]{};
		slotRootParameter[VolumetricLight::RootSignature::AccumulateScattering::RC_Consts]
			.InitAsConstants(VolumetricLight::RootConstant::AccumulateScattering::Count, 0);
		slotRootParameter[VolumetricLight::RootSignature::AccumulateScattering::UIO_FrustumVolumeMap]
			.InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[VolumetricLight::RootSignature::GR_AccumulateScattering]),
			L"VolumetricLight_GR_AccumulateScattering"));
	}
	// BlendScattering
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[2]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[VolumetricLight::RootSignature::BlendScattering::Count]{};
		slotRootParameter[VolumetricLight::RootSignature::BlendScattering::SI_PreviousScattering]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[VolumetricLight::RootSignature::BlendScattering::UIO_CurrentScattering]
			.InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[VolumetricLight::RootSignature::GR_BlendScattering]),
			L"VolumetricLight_GR_BlendScattering"));
	}
	// ApplyFog
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[2]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[VolumetricLight::RootSignature::ApplyFog::Count]{};
		slotRootParameter[VolumetricLight::RootSignature::ApplyFog::CB_Pass]
			.InitAsConstantBufferView(0);
		slotRootParameter[VolumetricLight::RootSignature::ApplyFog::RC_Consts]
			.InitAsConstants(VolumetricLight::RootConstant::ApplyFog::Count, 1);
		slotRootParameter[VolumetricLight::RootSignature::ApplyFog::SI_PositionMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[VolumetricLight::RootSignature::ApplyFog::SI_FrustumVolumeMap]
			.InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[VolumetricLight::RootSignature::GR_ApplyFog]),
			L"VolumetricLight_GR_ApplyFog"));
	}

	return true;
}

bool D3D12VolumetricLight::BuildPipelineStates() {
	// CalculateScatteringAndDensity
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[VolumetricLight::RootSignature::GR_CalculateScatteringAndDensity].Get();
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		{
			const auto CS = mInitData.ShaderManager->GetShader(
				mShaderHashes[VolumetricLight::Shader::CS_CalculateScatteringAndDensity]);
			NullCheck(CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}

		CheckReturn(D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[VolumetricLight::PipelineState::CP_CalculateScatteringAndDensity]),
			L"VolumetricLight_CP_CalculateScatteringAndDensity"));
	}
	// AccumulateScattering
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[VolumetricLight::RootSignature::GR_AccumulateScattering].Get();
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		{
			const auto CS = mInitData.ShaderManager->GetShader(
				mShaderHashes[VolumetricLight::Shader::CS_AccumulateScattering]);
			NullCheck(CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}

		CheckReturn(D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[VolumetricLight::PipelineState::CP_AccumulateScattering]),
			L"VolumetricLight_CP_AccumulateScattering"));
	}
	// BlendScattering
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[VolumetricLight::RootSignature::GR_BlendScattering].Get();
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		{
			const auto CS = mInitData.ShaderManager->GetShader(
				mShaderHashes[VolumetricLight::Shader::CS_BlendScattering]);
			NullCheck(CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}

		CheckReturn(D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[VolumetricLight::PipelineState::CP_BlendScattering]),
			L"VolumetricLight_CP_BlendScattering"));
	}
	// ApplyFog
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = D3D12Util::FitToScreenPsoDesc();
		psoDesc.pRootSignature = mRootSignatures[VolumetricLight::RootSignature::GR_ApplyFog].Get();
		{
			const auto VS = mInitData.ShaderManager->GetShader(
				mShaderHashes[VolumetricLight::Shader::VS_ApplyFog]);
			NullCheck(VS);
			psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
		}
		psoDesc.NumRenderTargets = 2;
		psoDesc.RTVFormats[0] = HDR_FORMAT;

		D3D12_RENDER_TARGET_BLEND_DESC blendDesc;
		blendDesc.BlendEnable = TRUE;
		blendDesc.LogicOpEnable = FALSE;
		blendDesc.SrcBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.DestBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.SrcBlendAlpha = D3D12_BLEND_ZERO;
		blendDesc.DestBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
		blendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		psoDesc.BlendState.RenderTarget[0] = blendDesc;

		// Default
		{
			{
				const auto PS = mInitData.ShaderManager->GetShader(
					mShaderHashes[VolumetricLight::Shader::PS_ApplyFog]);
				NullCheck(PS);
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			CheckReturn(D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[VolumetricLight::PipelineState::GP_ApplyFog]),
				L"VolumetricLight_GP_ApplyFog"));
		}
		// Tricubic Sampling
		{
			{
				const auto PS = mInitData.ShaderManager->GetShader(
					mShaderHashes[VolumetricLight::Shader::PS_ApplyFog_Tricubic]);
				NullCheck(PS);
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			CheckReturn(D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[VolumetricLight::PipelineState::GP_ApplyFog_Tricubic]),
				L"VolumetricLight_GP_ApplyFog_Tricubic"));
		}
	}

	return true;
}

bool D3D12VolumetricLight::AllocateDescriptors() {
	for (UINT i = 0; i < 2; ++i) {
		mpDescHeap->AllocateCbvSrvUav(1, mhFrustumVolumeMapDescs[
			VolumetricLight::Descriptor::FrustumVolumeMap::E_Srv][i]);
		mpDescHeap->AllocateCbvSrvUav(1, mhFrustumVolumeMapDescs[
			VolumetricLight::Descriptor::FrustumVolumeMap::E_Uav][i]);		
	}

	CheckReturn(BuildDescriptors());

	return true;
}

bool D3D12VolumetricLight::BuildFog(
	D3D12FrameResource* const pFrameResource
	, GpuResource* pDepthMap
	, D3D12_GPU_DESCRIPTOR_HANDLE si_depthMaps
	, FLOAT nearZ, FLOAT farZ) {
	mCurrentFrame = ++mFrameCount % 2 != 0;
	mPreviousFrame = (mCurrentFrame + 1) % 2;

	CheckReturn(CalculateScatteringAndDensity(
		pFrameResource, pDepthMap, si_depthMaps, nearZ, farZ));
	CheckReturn(AccumulateScattering(pFrameResource, nearZ, farZ));
	CheckReturn(BlendScattering(pFrameResource));

	return true;
}

bool D3D12VolumetricLight::ApplyFog(
	D3D12FrameResource* const pFrameResource
	, GpuResource* pBackBuffer
	, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer
	, GpuResource* pPositionMap
	, D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap
	, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect
	, FLOAT nearZ, FLOAT farZ) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[SHADER_ARGUMENT_MANAGER->VolumetricLight.TricubicSamplingEnabled ?
		VolumetricLight::PipelineState::GP_ApplyFog_Tricubic : VolumetricLight::PipelineState::GP_ApplyFog].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetGraphicsRootSignature(mRootSignatures[VolumetricLight::RootSignature::GR_ApplyFog].Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		pPositionMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, nullptr);

		CmdList->SetGraphicsRootConstantBufferView(
			VolumetricLight::RootSignature::ApplyFog::CB_Pass, pFrameResource->PassCB.CBAddress());
		CmdList->SetGraphicsRootDescriptorTable(
			VolumetricLight::RootSignature::ApplyFog::SI_PositionMap, si_positionMap);

		auto srv = mpDescHeap->GetGpuHandle(mhFrustumVolumeMapDescs[
			VolumetricLight::Descriptor::FrustumVolumeMap::E_Srv][mCurrentFrame]);
		CmdList->SetGraphicsRootDescriptorTable(
			VolumetricLight::RootSignature::ApplyFog::SI_FrustumVolumeMap, srv);

		VolumetricLight::RootConstant::ApplyFog::Struct rc;
		rc.gNearZ = nearZ;
		rc.gFarZ = farZ;
		rc.gDepthExponent = SHADER_ARGUMENT_MANAGER->VolumetricLight.DepthExponent;

		D3D12Util::SetRoot32BitConstants<VolumetricLight::RootConstant::ApplyFog::Struct>(
			VolumetricLight::RootSignature::ApplyFog::RC_Consts,
			VolumetricLight::RootConstant::ApplyFog::Count,
			&rc,
			0,
			CmdList,
			FALSE);

		CmdList->IASetVertexBuffers(0, 0, nullptr);
		CmdList->IASetIndexBuffer(nullptr);
		CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		CmdList->DrawInstanced(6, 1, 0, 0);
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12VolumetricLight::CalculateScatteringAndDensity(
	D3D12FrameResource* const pFrameResource
	, GpuResource* pDepthMap
	, D3D12_GPU_DESCRIPTOR_HANDLE si_depthMaps
	, FLOAT nearZ, FLOAT farZ) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[VolumetricLight::PipelineState::CP_CalculateScatteringAndDensity].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetComputeRootSignature(
			mRootSignatures[VolumetricLight::RootSignature::GR_CalculateScatteringAndDensity].Get());

		pDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		mFrustumVolumeMaps[mPreviousFrame]->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		mFrustumVolumeMaps[mCurrentFrame]->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Util::UavBarrier(CmdList, mFrustumVolumeMaps[mCurrentFrame].get());

		CmdList->SetComputeRootConstantBufferView(
			VolumetricLight::RootSignature::CalculateScatteringAndDensity::CB_Pass,
			pFrameResource->PassCB.CBAddress());
		CmdList->SetComputeRootConstantBufferView(
			VolumetricLight::RootSignature::CalculateScatteringAndDensity::CB_Light,
			pFrameResource->LightCB.CBAddress());
		CmdList->SetComputeRootDescriptorTable(
			VolumetricLight::RootSignature::CalculateScatteringAndDensity::SI_DepthMaps, si_depthMaps);

		auto uav = mpDescHeap->GetGpuHandle(
			mhFrustumVolumeMapDescs[VolumetricLight::Descriptor::E_Uav][mCurrentFrame]);
		CmdList->SetComputeRootDescriptorTable(
			VolumetricLight::RootSignature::CalculateScatteringAndDensity::UO_FrustumVolumeMap, uav);

		VolumetricLight::RootConstant::CalculateScatteringAndDensity::Struct rc;
		rc.gNearZ = nearZ;
		rc.gFarZ = farZ;
		rc.gDepthExponent = SHADER_ARGUMENT_MANAGER->VolumetricLight.DepthExponent;
		rc.gAnisotropicCoefficient = SHADER_ARGUMENT_MANAGER->VolumetricLight.AnisotropicCoefficient;
		rc.gUniformDensity = SHADER_ARGUMENT_MANAGER->VolumetricLight.UniformDensity;
		rc.gFrameCount = mFrameCount;

		D3D12Util::SetRoot32BitConstants<VolumetricLight::RootConstant::CalculateScatteringAndDensity::Struct>(
			VolumetricLight::RootSignature::CalculateScatteringAndDensity::RC_Consts,
			VolumetricLight::RootConstant::CalculateScatteringAndDensity::Count,
			&rc,
			0,
			CmdList,
			TRUE);

		CmdList->Dispatch(
			D3D12Util::D3D12Util::CeilDivide(
				mInitData.Width,
				VolumetricLight::ThreadGroup::CalculateScatteringAndDensity::Width),
			D3D12Util::D3D12Util::CeilDivide(
				mInitData.Height,
				VolumetricLight::ThreadGroup::CalculateScatteringAndDensity::Height),
			D3D12Util::D3D12Util::CeilDivide(
				mInitData.Depth,
				VolumetricLight::ThreadGroup::CalculateScatteringAndDensity::Depth));
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}
bool D3D12VolumetricLight::AccumulateScattering(
	D3D12FrameResource* const pFrameResource
	, FLOAT nearZ, FLOAT farZ) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[VolumetricLight::PipelineState::CP_AccumulateScattering].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetComputeRootSignature(
			mRootSignatures[VolumetricLight::RootSignature::GR_AccumulateScattering].Get());

		mFrustumVolumeMaps[mCurrentFrame]->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Util::UavBarrier(CmdList, mFrustumVolumeMaps[mCurrentFrame].get());

		VolumetricLight::RootConstant::AccumulateScattering::Struct rc;
		rc.gNearZ = nearZ;
		rc.gFarZ = farZ;
		rc.gDepthExponent = SHADER_ARGUMENT_MANAGER->VolumetricLight.DepthExponent;
		rc.gDensityScale = SHADER_ARGUMENT_MANAGER->VolumetricLight.DensityScale;

		D3D12Util::SetRoot32BitConstants<VolumetricLight::RootConstant::AccumulateScattering::Struct>(
			VolumetricLight::RootSignature::AccumulateScattering::RC_Consts,
			VolumetricLight::RootConstant::AccumulateScattering::Count,
			&rc,
			0,
			CmdList,
			TRUE);

		auto uav = mpDescHeap->GetGpuHandle(
			mhFrustumVolumeMapDescs[VolumetricLight::Descriptor::E_Uav][mCurrentFrame]);
		CmdList->SetComputeRootDescriptorTable(
			VolumetricLight::RootSignature::AccumulateScattering::UIO_FrustumVolumeMap, uav);

		CmdList->Dispatch(
			D3D12Util::CeilDivide(
				static_cast<UINT>(mInitData.Width),
				VolumetricLight::ThreadGroup::AccumulateScattering::Width),
			D3D12Util::CeilDivide(
				static_cast<UINT>(mInitData.Height),
				VolumetricLight::ThreadGroup::AccumulateScattering::Height),
			VolumetricLight::ThreadGroup::AccumulateScattering::Depth);
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12VolumetricLight::BlendScattering(D3D12FrameResource* const pFrameResource) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[VolumetricLight::PipelineState::CP_BlendScattering].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetComputeRootSignature(
			mRootSignatures[VolumetricLight::RootSignature::GR_BlendScattering].Get());

		mFrustumVolumeMaps[mPreviousFrame]->Transite(
			CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		mFrustumVolumeMaps[mCurrentFrame]->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Util::UavBarrier(CmdList, mFrustumVolumeMaps[mCurrentFrame].get());

		auto srv = mpDescHeap->GetGpuHandle(mhFrustumVolumeMapDescs[
			VolumetricLight::Descriptor::E_Srv][mPreviousFrame]);
		auto uav = mpDescHeap->GetGpuHandle(mhFrustumVolumeMapDescs[
			VolumetricLight::Descriptor::E_Uav][mCurrentFrame]);

		CmdList->SetComputeRootDescriptorTable(
			VolumetricLight::RootSignature::BlendScattering::SI_PreviousScattering, srv);
		CmdList->SetComputeRootDescriptorTable(
			VolumetricLight::RootSignature::BlendScattering::UIO_CurrentScattering, uav);

		CmdList->Dispatch(
			D3D12Util::D3D12Util::CeilDivide(
				mInitData.Width,
				VolumetricLight::ThreadGroup::BlendScattering::Width),
			D3D12Util::D3D12Util::CeilDivide(
				mInitData.Height,
				VolumetricLight::ThreadGroup::BlendScattering::Height),
			D3D12Util::D3D12Util::CeilDivide(
				mInitData.Depth,
				VolumetricLight::ThreadGroup::BlendScattering::Depth));
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12VolumetricLight::BuildResources() {
	D3D12_RESOURCE_DESC texDesc{};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	texDesc.Format = VolumetricLight::FrustumVolumeMapFormat;
	texDesc.Width = mInitData.Width;
	texDesc.Height = mInitData.Height;
	texDesc.DepthOrArraySize = mInitData.Depth;
	texDesc.Alignment = 0;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	const auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	for (size_t i = 0; i < 2; ++i) {
		auto name = std::format(L"VolumetricLight_FrustumVolumeMap_{}", i);

		CheckReturn(mFrustumVolumeMaps[i]->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			name.c_str()));
	}

	return true;
}

bool D3D12VolumetricLight::BuildDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	srvDesc.Format = VolumetricLight::FrustumVolumeMapFormat;
	srvDesc.Texture3D.MostDetailedMip = 0;
	srvDesc.Texture3D.MipLevels = 1;
	srvDesc.Texture3D.ResourceMinLODClamp = 0.f;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
	uavDesc.Format = VolumetricLight::FrustumVolumeMapFormat;
	uavDesc.Texture3D.MipSlice = 0;
	uavDesc.Texture3D.WSize = mInitData.Depth;
	uavDesc.Texture3D.FirstWSlice = 0;

	for (UINT i = 0; i < 2; ++i) {
		const auto& resource = mFrustumVolumeMaps[i]->Resource();

		auto srv = mpDescHeap->GetCpuHandle(mhFrustumVolumeMapDescs[
			VolumetricLight::Descriptor::FrustumVolumeMap::E_Srv][i]);
		auto uav = mpDescHeap->GetCpuHandle(mhFrustumVolumeMapDescs[
			VolumetricLight::Descriptor::FrustumVolumeMap::E_Uav][i]);

		mInitData.Device->CreateShaderResourceView(resource, &srvDesc, srv);
		mInitData.Device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, uav);
	}

	return true;
}
