#include "pch.h"
#include "Renderer/D3D12/D3D12TextureScaler.hpp"

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
	const WCHAR* const HLSL_DownSample2Nx2N = L"D3D12DownSample2Nx2N.hlsl";
}

D3D12TextureScaler::D3D12TextureScaler() {}

D3D12TextureScaler::~D3D12TextureScaler() {}

bool D3D12TextureScaler::Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	return true;
}

bool D3D12TextureScaler::CompileShaders() {
	// DownSample2x2
	{
		DxcDefine defines[] = { { L"KERNEL_RADIUS", L"1" } };

		const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
			HLSL_DownSample2Nx2N, L"CS", L"cs_6_5", defines, _countof(defines));
		CheckReturn(mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[TextureScaler::Shader::CS_DownSample2x2]));
	}
	// DownSample4x4
	{
		DxcDefine defines[] = { { L"KERNEL_RADIUS", L"2" } };

		const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
			HLSL_DownSample2Nx2N, L"CS", L"cs_6_5", defines, _countof(defines));
		CheckReturn(mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[TextureScaler::Shader::CS_DownSample4x4]));
	}
	// DownSample6x6
	{
		DxcDefine defines[] = { { L"KERNEL_RADIUS", L"3" } };

		const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
			HLSL_DownSample2Nx2N, L"CS", L"cs_6_5", defines, _countof(defines));
		CheckReturn(mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[TextureScaler::Shader::CS_DownSample6x6]));
	}

	return true;
}

bool D3D12TextureScaler::BuildRootSignatures() {
	decltype(auto) samplers = D3D12Util::GetStaticSamplers();

	// DownSample2Nx2N
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[2]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[TextureScaler::RootSignature::DownSample2Nx2N::Count]{};
		slotRootParameter[TextureScaler::RootSignature::DownSample2Nx2N::RC_Consts].InitAsConstants(
			TextureScaler::RootConstant::DownSample2Nx2N::Count, 0);
		slotRootParameter[TextureScaler::RootSignature::DownSample2Nx2N::SI_InputMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[TextureScaler::RootSignature::DownSample2Nx2N::UO_OutputMap]
			.InitAsDescriptorTable(1, &texTables[index++]);
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[TextureScaler::RootSignature::GR_DownSample2Nx2N]),
			L"TextureScaler_GR_DownSample2Nx2N"));
	}

	return true;
}

bool D3D12TextureScaler::BuildPipelineStates() {
	// DownSample2Nx2N
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[TextureScaler::RootSignature::GR_DownSample2Nx2N].Get();
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		// 2x2
		{
			{
				const auto CS = mInitData.ShaderManager->GetShader(
					mShaderHashes[TextureScaler::Shader::CS_DownSample2x2]);
				NullCheck(CS);
				psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
			}

			CheckReturn(D3D12Util::CreateComputePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[TextureScaler::PipelineState::CP_DownSample2x2]),
				L"TextureScaler_CP_DownSample2x2"));
		}
		// 4x4
		{
			{
				const auto CS = mInitData.ShaderManager->GetShader(
					mShaderHashes[TextureScaler::Shader::CS_DownSample4x4]);
				NullCheck(CS);
				psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
			}

			CheckReturn(D3D12Util::CreateComputePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[TextureScaler::PipelineState::CP_DownSample4x4]),
				L"TextureScaler_CP_DownSample4x4"));
		}
		// 6x6
		{
			{
				const auto CS = mInitData.ShaderManager->GetShader(
					mShaderHashes[TextureScaler::Shader::CS_DownSample6x6]);
				NullCheck(CS);
				psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
			}

			CheckReturn(D3D12Util::CreateComputePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[TextureScaler::PipelineState::CP_DownSample6x6]),
				L"TextureScaler_CP_DownSample6x6"));
		}
	}

	return true;
}

bool D3D12TextureScaler::DownSample2Nx2N(
	D3D12FrameResource* const pFrameResource
	, GpuResource* const pInputMap
	, D3D12_GPU_DESCRIPTOR_HANDLE si_inputMap
	, GpuResource* const pOutputMap
	, D3D12_GPU_DESCRIPTOR_HANDLE uo_outputMap
	, UINT srcTexDimX, UINT srcTexDimY, UINT dstTexDimX, UINT dstTexDimY
	, TextureScaler::KernelRadius::Type kernelRadius) {
	TextureScaler::PipelineState::Type type;
	if (kernelRadius == 1) type = TextureScaler::PipelineState::CP_DownSample2x2;
	else if (kernelRadius == 2) type = TextureScaler::PipelineState::CP_DownSample4x4;
	else if (kernelRadius == 3) type = TextureScaler::PipelineState::CP_DownSample6x6;
	else ReturnFalse("Invalid kernel radius type");

	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[type].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetComputeRootSignature(mRootSignatures[
			TextureScaler::RootSignature::GR_DownSample2Nx2N].Get());

		TextureScaler::RootConstant::DownSample2Nx2N::Struct rc;
		rc.gSrcTexDim = { srcTexDimX, srcTexDimY };
		rc.gDstTexDim = { dstTexDimX, dstTexDimY };

		D3D12Util::SetRoot32BitConstants<TextureScaler::RootConstant::DownSample2Nx2N::Struct>(
			TextureScaler::RootSignature::DownSample2Nx2N::RC_Consts,
			TextureScaler::RootConstant::DownSample2Nx2N::Count,
			&rc,
			0,
			CmdList,
			TRUE);

		pInputMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);;

		pOutputMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Util::UavBarrier(CmdList, pOutputMap);

		CmdList->SetComputeRootDescriptorTable(
			TextureScaler::RootSignature::DownSample2Nx2N::SI_InputMap, si_inputMap);
		CmdList->SetComputeRootDescriptorTable(
			TextureScaler::RootSignature::DownSample2Nx2N::UO_OutputMap, uo_outputMap);

		CmdList->Dispatch(
			D3D12Util::CeilDivide(
				dstTexDimX, TextureScaler::ThreadGroup::DownSample2Nx2N::Width),
			D3D12Util::CeilDivide(
				dstTexDimY, TextureScaler::ThreadGroup::DownSample2Nx2N::Height),
			TextureScaler::ThreadGroup::DownSample2Nx2N::Depth);
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}