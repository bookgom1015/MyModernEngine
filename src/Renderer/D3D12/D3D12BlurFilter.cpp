#include "pch.h"
#include "Renderer/D3D12/D3D12BlurFilter.hpp"

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
	const WCHAR* const HLSL_GaussianBlurFilter3x3 = L"D3D12GaussianBlurFilter3x3.hlsl";
	const WCHAR* const HLSL_GaussianBlurFilterNxN = L"D3D12GaussianBlurFilterNxN.hlsl";
}

D3D12BlurFilter::D3D12BlurFilter() {}

D3D12BlurFilter::~D3D12BlurFilter() {}

bool D3D12BlurFilter::Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	return true;
}

bool D3D12BlurFilter::CompileShaders() {
	// GaussianBlurFilter3x3
	{
		DxcDefine defines[] = { { L"ValueType", L"float" } };

		const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
			HLSL_GaussianBlurFilter3x3, L"CS", L"cs_6_5", defines, _countof(defines));
		CheckReturn(mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[BlurFilter::Shader::CS_GaussianBlurFilter3x3]));
	}
	// GaussianBlurFilterRGBA3x3
	{
		DxcDefine defines[] = { { L"ValueType", L"float4" } };

		const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
			HLSL_GaussianBlurFilter3x3, L"CS", L"cs_6_5", defines, _countof(defines));
		CheckReturn(mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[BlurFilter::Shader::CS_GaussianBlurFilterRGBA3x3]));
	}
	// GaussianBlurFilterNxN
	{
		// R
		{
			// 3x3
			{
				DxcDefine defines[] = {
					{ L"KERNEL_RADIUS", L"1" },
					{ L"ValueType", L"float" }
				};

				const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
					HLSL_GaussianBlurFilterNxN, L"CS", L"cs_6_5", defines, _countof(defines));
				CheckReturn(mInitData.ShaderManager->AddShader(
					CS, mShaderHashes[BlurFilter::Shader::CS_GaussianBlurFilterNxN3x3]));
			}
			// 5x5
			{
				DxcDefine defines[] = {
					{ L"KERNEL_RADIUS", L"2" },
					{ L"ValueType", L"float" }
				};

				const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
					HLSL_GaussianBlurFilterNxN, L"CS", L"cs_6_5", defines, _countof(defines));
				CheckReturn(mInitData.ShaderManager->AddShader(
					CS, mShaderHashes[BlurFilter::Shader::CS_GaussianBlurFilterNxN5x5]));
			}
			// 7x7
			{
				DxcDefine defines[] = {
					{ L"KERNEL_RADIUS", L"3" },
					{ L"ValueType", L"float" }
				};

				const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
					HLSL_GaussianBlurFilterNxN, L"CS", L"cs_6_5", defines, _countof(defines));
				CheckReturn(mInitData.ShaderManager->AddShader(
					CS, mShaderHashes[BlurFilter::Shader::CS_GaussianBlurFilterNxN7x7]));
			}
			// 9x9
			{
				DxcDefine defines[] = {
					{ L"KERNEL_RADIUS", L"4" },
					{ L"ValueType", L"float" }
				};

				const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
					HLSL_GaussianBlurFilterNxN, L"CS", L"cs_6_5", defines, _countof(defines));
				CheckReturn(mInitData.ShaderManager->AddShader(
					CS, mShaderHashes[BlurFilter::Shader::CS_GaussianBlurFilterNxN9x9]));
			}
		}
		// RGBA
		{
			// 3x3
			{
				DxcDefine defines[] = {
					{ L"KERNEL_RADIUS", L"1" },
					{ L"ValueType", L"float4" }
				};

				const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
					HLSL_GaussianBlurFilterNxN, L"CS", L"cs_6_5", defines, _countof(defines));
				CheckReturn(mInitData.ShaderManager->AddShader(
					CS, mShaderHashes[BlurFilter::Shader::CS_GaussianBlurFilterRGBANxN3x3]));
			}
			// 5x5
			{
				DxcDefine defines[] = {
					{ L"KERNEL_RADIUS", L"2" },
					{ L"ValueType", L"float4" }
				};

				const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
					HLSL_GaussianBlurFilterNxN, L"CS", L"cs_6_5", defines, _countof(defines));
				CheckReturn(mInitData.ShaderManager->AddShader(
					CS, mShaderHashes[BlurFilter::Shader::CS_GaussianBlurFilterRGBANxN5x5]));
			}
			// 7x7
			{
				DxcDefine defines[] = {
					{ L"KERNEL_RADIUS", L"3" },
					{ L"ValueType", L"float4" }
				};

				const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
					HLSL_GaussianBlurFilterNxN, L"CS", L"cs_6_5", defines, _countof(defines));
				CheckReturn(mInitData.ShaderManager->AddShader(
					CS, mShaderHashes[BlurFilter::Shader::CS_GaussianBlurFilterRGBANxN7x7]));
			}
			// 9x9
			{
				DxcDefine defines[] = {
					{ L"KERNEL_RADIUS", L"4" },
					{ L"ValueType", L"float4" }
				};

				const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
					HLSL_GaussianBlurFilterNxN, L"CS", L"cs_6_5", defines, _countof(defines));
				CheckReturn(mInitData.ShaderManager->AddShader(
					CS, mShaderHashes[BlurFilter::Shader::CS_GaussianBlurFilterRGBANxN9x9]));
			}
		}
	}

	return true;
}

bool D3D12BlurFilter::BuildRootSignatures() {
	decltype(auto) samplers = D3D12Util::GetStaticSamplers();

	CD3DX12_DESCRIPTOR_RANGE texTables[2]{}; UINT index = 0;
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

	index = 0;

	CD3DX12_ROOT_PARAMETER slotRootParameter[BlurFilter::RootSignature::Default::Count]{};
	slotRootParameter[BlurFilter::RootSignature::Default::RC_Consts]
		.InitAsConstants(BlurFilter::RootConstant::Default::Count, 0);
	slotRootParameter[BlurFilter::RootSignature::Default::SI_InputMap]
		.InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[BlurFilter::RootSignature::Default::UO_OutputMap]
		.InitAsDescriptorTable(1, &texTables[index++]);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		D3D12Util::StaticSamplerCount, samplers,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	CheckReturn(D3D12Util::CreateRootSignature(
		mInitData.Device,
		rootSigDesc,
		IID_PPV_ARGS(&mRootSignature),
		L"BlurFilter_GR_Default"));

	return true;
}

bool D3D12BlurFilter::BuildPipelineStates() {
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	// GaussianBlurFilter3x3
	{
		{
			const auto CS = mInitData.ShaderManager->GetShader(
				mShaderHashes[BlurFilter::Shader::CS_GaussianBlurFilter3x3]);
			NullCheck(CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}

		CheckReturn(D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[BlurFilter::PipelineState::CP_GaussianBlurFilter3x3]),
			L"BlurFilter_CP_GaussianBlurFilter3x3"));
	}
	// GaussianBlurFilterRGBA3x3
	{
		{
			const auto CS = mInitData.ShaderManager->GetShader(
				mShaderHashes[BlurFilter::Shader::CS_GaussianBlurFilterRGBA3x3]);
			NullCheck(CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}

		CheckReturn(D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[BlurFilter::PipelineState::CP_GaussianBlurFilterRGBA3x3]),
			L"BlurFilter_CP_GaussianBlurFilterRGBA3x3"));
	}
	// GaussianBlurFilterNxN
	{
		// R
		{
			// 3x3
			{
				{
					const auto CS = mInitData.ShaderManager->GetShader(
						mShaderHashes[BlurFilter::Shader::CS_GaussianBlurFilterNxN3x3]);
					NullCheck(CS);
					psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
				}

				CheckReturn(D3D12Util::CreateComputePipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[BlurFilter::PipelineState::CP_GaussianBlurFilterNxN3x3]),
					L"BlurFilter_CP_GaussianBlurFilterNxN3x3"));
			}
			// 5x5
			{
				{
					const auto CS = mInitData.ShaderManager->GetShader(
						mShaderHashes[BlurFilter::Shader::CS_GaussianBlurFilterNxN5x5]);
					NullCheck(CS);
					psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
				}

				CheckReturn(D3D12Util::CreateComputePipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[BlurFilter::PipelineState::CP_GaussianBlurFilterNxN5x5]),
					L"BlurFilter_CP_GaussianBlurFilterNxN5x5"));
			}
			// 7x7
			{
				{
					const auto CS = mInitData.ShaderManager->GetShader(
						mShaderHashes[BlurFilter::Shader::CS_GaussianBlurFilterNxN7x7]);
					NullCheck(CS);
					psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
				}

				CheckReturn(D3D12Util::CreateComputePipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[BlurFilter::PipelineState::CP_GaussianBlurFilterNxN7x7]),
					L"BlurFilter_CP_GaussianBlurFilterNxN7x7"));
			}
			// 9x9
			{
				{
					const auto CS = mInitData.ShaderManager->GetShader(
						mShaderHashes[BlurFilter::Shader::CS_GaussianBlurFilterNxN9x9]);
					NullCheck(CS);
					psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
				}

				CheckReturn(D3D12Util::CreateComputePipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[BlurFilter::PipelineState::CP_GaussianBlurFilterNxN9x9]),
					L"BlurFilter_CP_GaussianBlurFilterNxN9x9"));
			}
		}
		// RGBA
		{
			// 3x3
			{
				{
					const auto CS = mInitData.ShaderManager->GetShader(
						mShaderHashes[BlurFilter::Shader::CS_GaussianBlurFilterRGBANxN3x3]);
					NullCheck(CS);
					psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
				}

				CheckReturn(D3D12Util::CreateComputePipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[BlurFilter::PipelineState::CP_GaussianBlurFilterRGBANxN3x3]),
					L"BlurFilter_CP_GaussianBlurFilterRGBANxN3x3"));
			}
			// 5x5
			{
				{
					const auto CS = mInitData.ShaderManager->GetShader(
						mShaderHashes[BlurFilter::Shader::CS_GaussianBlurFilterRGBANxN5x5]);
					NullCheck(CS);
					psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
				}

				CheckReturn(D3D12Util::CreateComputePipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[BlurFilter::PipelineState::CP_GaussianBlurFilterRGBANxN5x5]),
					L"BlurFilter_CP_GaussianBlurFilterRGBANxN5x5"));
			}
			// 7x7
			{
				{
					const auto CS = mInitData.ShaderManager->GetShader(
						mShaderHashes[BlurFilter::Shader::CS_GaussianBlurFilterRGBANxN7x7]);
					NullCheck(CS);
					psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
				}

				CheckReturn(D3D12Util::CreateComputePipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[BlurFilter::PipelineState::CP_GaussianBlurFilterRGBANxN7x7]),
					L"BlurFilter_CP_GaussianBlurFilterRGBANxN7x7"));
			}
			// 9x9
			{
				{
					const auto CS = mInitData.ShaderManager->GetShader(
						mShaderHashes[BlurFilter::Shader::CS_GaussianBlurFilterRGBANxN9x9]);
					NullCheck(CS);
					psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
				}

				CheckReturn(D3D12Util::CreateComputePipelineState(
					mInitData.Device,
					psoDesc,
					IID_PPV_ARGS(&mPipelineStates[BlurFilter::PipelineState::CP_GaussianBlurFilterRGBANxN9x9]),
					L"BlurFilter_CP_GaussianBlurFilterRGBANxN9x9"));
			}
		}
	}

	return true;
}

bool D3D12BlurFilter::GaussianBlur(
	D3D12FrameResource* const pFrameResource
	, BlurFilter::PipelineState::Type type
	, GpuResource* const pInputMap
	, D3D12_GPU_DESCRIPTOR_HANDLE si_inputMap
	, GpuResource* const pOutputMap
	, D3D12_GPU_DESCRIPTOR_HANDLE uo_outputMap
	, UINT texWidth, UINT texHeight) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[type].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetComputeRootSignature(mRootSignature.Get());

		BlurFilter::RootConstant::Default::Struct rc;
		rc.gTexDim.x = static_cast<FLOAT>(texWidth);
		rc.gTexDim.y = static_cast<FLOAT>(texHeight);
		rc.gInvTexDim.x = 1.f / static_cast<FLOAT>(texWidth);
		rc.gInvTexDim.y = 1.f / static_cast<FLOAT>(texHeight);

		D3D12Util::SetRoot32BitConstants<BlurFilter::RootConstant::Default::Struct>(
			BlurFilter::RootSignature::Default::RC_Consts,
			BlurFilter::RootConstant::Default::Count,
			&rc,
			0,
			CmdList,
			TRUE);

		pInputMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		pOutputMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Util::UavBarrier(CmdList, pOutputMap);

		CmdList->SetComputeRootDescriptorTable(BlurFilter::RootSignature::Default::SI_InputMap, si_inputMap);
		CmdList->SetComputeRootDescriptorTable(BlurFilter::RootSignature::Default::UO_OutputMap, uo_outputMap);

		CmdList->Dispatch(
			D3D12Util::CeilDivide(texWidth, BlurFilter::ThreadGroup::Default::Width),
			D3D12Util::CeilDivide(texHeight, BlurFilter::ThreadGroup::Default::Height),
			BlurFilter::ThreadGroup::Default::Depth);
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}