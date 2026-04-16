#include "pch.h"
#include "Renderer/D3D12/D3D12EyeAdaption.hpp"

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
#include "Renderer/D3D12/D3D12ShaderTable.hpp"


namespace {
	const WCHAR* const HLSL_ClearHistogram = L"D3D12ClearHistogram.hlsl";
	const WCHAR* const HLSL_LuminanceHistogram = L"D3D12LuminanceHistogram.hlsl";
	const WCHAR* const HLSL_PercentileExtract = L"D3D12PercentileExtract.hlsl";
	const WCHAR* const HLSL_TemporalSmoothing = L"D3D12TemporalSmoothing.hlsl";
}

D3D12EyeAdaption::D3D12EyeAdaption() {}

D3D12EyeAdaption::~D3D12EyeAdaption() {}

bool D3D12EyeAdaption::Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	mHistogramBuffer = std::make_unique<GpuResource>();
	mAvgLogLuminance = std::make_unique<GpuResource>();
	mPrevLuminance = std::make_unique<GpuResource>();
	mSmoothedLuminance = std::make_unique<GpuResource>();

	CheckReturn(BuildResources());

	return true;
}

bool D3D12EyeAdaption::CompileShaders() {
	// ClearHistogram
	{
		const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
			HLSL_ClearHistogram, L"CS", L"cs_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[EyeAdaption::Shader::CS_ClearHistogram]));
	}
	// LuminanceHistogram
	{
		const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
			HLSL_LuminanceHistogram, L"CS", L"cs_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[EyeAdaption::Shader::CS_LuminanceHistogram]));
	}
	// PercentileExtract
	{
		const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
			HLSL_PercentileExtract, L"CS", L"cs_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[EyeAdaption::Shader::CS_PercentileExtract]));
	}
	// TemporalSmoothing
	{
		const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
			HLSL_TemporalSmoothing, L"CS", L"cs_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[EyeAdaption::Shader::CS_TemporalSmoothing]));
	}

	return true;
}

bool D3D12EyeAdaption::BuildRootSignatures() {
	decltype(auto) samplers = D3D12Util::GetStaticSamplers();

	// LuminanceHistogram
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[1]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[
			EyeAdaption::RootSignature::LuminanceHistogram::Count]{};
		slotRootParameter[EyeAdaption::RootSignature::LuminanceHistogram::RC_Consts]
			.InitAsConstants(EyeAdaption::RootConstant::LuminanceHistogram::Count, 0);
		slotRootParameter[EyeAdaption::RootSignature::LuminanceHistogram::SI_BackBuffer].
			InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[EyeAdaption::RootSignature::LuminanceHistogram::UO_HistogramBuffer].
			InitAsUnorderedAccessView(0);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[EyeAdaption::RootSignature::GR_LuminanceHistogram]),
			L"EyeAdaption_GR_LuminanceHistogram"));
	}
	// PercentileExtract
	{
		CD3DX12_ROOT_PARAMETER slotRootParameter[EyeAdaption::RootSignature::PercentileExtract::Count]{};
		slotRootParameter[EyeAdaption::RootSignature::PercentileExtract::RC_Consts]
			.InitAsConstants(EyeAdaption::RootConstant::PercentileExtract::Count, 0);
		slotRootParameter[EyeAdaption::RootSignature::PercentileExtract::UI_HistogramBuffer].
			InitAsUnorderedAccessView(0);
		slotRootParameter[EyeAdaption::RootSignature::PercentileExtract::UO_AvgLogLuminance].
			InitAsUnorderedAccessView(1);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[EyeAdaption::RootSignature::GR_PercentileExtract]),
			L"EyeAdaption_GR_PercentileExtract"));
	}
	// TemporalSmoothing
	{
		CD3DX12_ROOT_PARAMETER slotRootParameter[EyeAdaption::RootSignature::TemporalSmoothing::Count]{};
		slotRootParameter[EyeAdaption::RootSignature::TemporalSmoothing::RC_Consts]
			.InitAsConstants(EyeAdaption::RootConstant::TemporalSmoothing::Count, 0);
		slotRootParameter[EyeAdaption::RootSignature::TemporalSmoothing::UI_AvgLogLuminance].
			InitAsUnorderedAccessView(0);
		slotRootParameter[EyeAdaption::RootSignature::TemporalSmoothing::UO_SmoothedLum].
			InitAsUnorderedAccessView(1);
		slotRootParameter[EyeAdaption::RootSignature::TemporalSmoothing::UIO_PrevLum].
			InitAsUnorderedAccessView(2);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[EyeAdaption::RootSignature::GR_TemporalSmoothing]),
			L"EyeAdaption_GR_TemporalSmoothing"));
	}

	return true;
}

bool D3D12EyeAdaption::BuildPipelineStates() {
	// ClearHistogram
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[
			EyeAdaption::RootSignature::GR_LuminanceHistogram].Get();
			psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
			{
				const auto CS = mInitData.ShaderManager->GetShader(
					mShaderHashes[EyeAdaption::Shader::CS_ClearHistogram]);
				NullCheck(CS);
				psoDesc.CS = {
					reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
			}

			CheckReturn(D3D12Util::CreateComputePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[EyeAdaption::PipelineState::CP_ClearHistogram]),
				L"EyeAdaption_CP_ClearHistogram"));
	}
	// LuminanceHistogram
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[
			EyeAdaption::RootSignature::GR_LuminanceHistogram].Get();
			psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
			{
				const auto CS = mInitData.ShaderManager->GetShader(
					mShaderHashes[EyeAdaption::Shader::CS_LuminanceHistogram]);
				NullCheck(CS);
				psoDesc.CS = {
					reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
			}

			CheckReturn(D3D12Util::CreateComputePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[EyeAdaption::PipelineState::CP_LuminanceHistogram]),
				L"EyeAdaption_CP_LuminanceHistogram"));
	}
	// PercentileExtract
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[
			EyeAdaption::RootSignature::GR_PercentileExtract].Get();
			psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
			{
				const auto CS = mInitData.ShaderManager->GetShader(
					mShaderHashes[EyeAdaption::Shader::CS_PercentileExtract]);
				NullCheck(CS);
				psoDesc.CS = {
					reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
			}

			CheckReturn(D3D12Util::CreateComputePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[EyeAdaption::PipelineState::CP_PercentileExtract]),
				L"EyeAdaption_CP_PercentileExtract"));
	}
	// TemporalSmoothing
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[
			EyeAdaption::RootSignature::GR_TemporalSmoothing].Get();
			psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
			{
				const auto CS = mInitData.ShaderManager->GetShader(
					mShaderHashes[EyeAdaption::Shader::CS_TemporalSmoothing]);
				NullCheck(CS);
				psoDesc.CS = {
					reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
			}

			CheckReturn(D3D12Util::CreateComputePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[EyeAdaption::PipelineState::CP_TemporalSmoothing]),
				L"EyeAdaption_CP_TemporalSmoothing"));
	}

	return true;
}

bool D3D12EyeAdaption::OnResize(unsigned width, unsigned height) {
	mInitData.Width = width;
	mInitData.Height = height;

	CheckReturn(BuildResources());

	return true;
}

bool D3D12EyeAdaption::ClearHistogram(D3D12FrameResource* const pFrameResource) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[EyeAdaption::PipelineState::CP_ClearHistogram].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetComputeRootSignature(
			mRootSignatures[EyeAdaption::RootSignature::GR_LuminanceHistogram].Get());

		const auto Histogram = mHistogramBuffer.get();
		Histogram->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Util::UavBarrier(CmdList, Histogram);

		EyeAdaption::RootConstant::LuminanceHistogram::Struct rc{};
		rc.gTexDim = { mInitData.Width, mInitData.Height };
		rc.gMinLogLum = -8.f;
		rc.gMaxLogLum = 4.f;
		rc.gBinCount = 64;

		D3D12Util::SetRoot32BitConstants<
			EyeAdaption::RootConstant::LuminanceHistogram::Struct>(
				EyeAdaption::RootSignature::LuminanceHistogram::RC_Consts,
				EyeAdaption::RootConstant::LuminanceHistogram::Count,
				&rc,
				0,
				CmdList,
				TRUE);

		CmdList->SetComputeRootUnorderedAccessView(
			EyeAdaption::RootSignature::LuminanceHistogram::UO_HistogramBuffer,
			mHistogramBuffer->Resource()->GetGPUVirtualAddress());

		CmdList->Dispatch(MAX_BIN_COUNT, 1, 1);
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12EyeAdaption::BuildLuminanceHistogram(
	D3D12FrameResource* const pFrameResource
	, GpuResource* const pBackBuffer
	, D3D12_GPU_DESCRIPTOR_HANDLE si_backBuffer) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[EyeAdaption::PipelineState::CP_LuminanceHistogram].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetComputeRootSignature(
			mRootSignatures[EyeAdaption::RootSignature::GR_LuminanceHistogram].Get());

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		const auto Histogram = mHistogramBuffer.get();
		Histogram->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Util::UavBarrier(CmdList, Histogram);

		EyeAdaption::RootConstant::LuminanceHistogram::Struct rc{};
		rc.gTexDim = { mInitData.Width >> 1, mInitData.Height >> 1 };
		rc.gMinLogLum = -8.f;
		rc.gMaxLogLum = 4.f;
		rc.gBinCount = 64;

		D3D12Util::SetRoot32BitConstants<
			EyeAdaption::RootConstant::LuminanceHistogram::Struct>(
				EyeAdaption::RootSignature::LuminanceHistogram::RC_Consts,
				EyeAdaption::RootConstant::LuminanceHistogram::Count,
				&rc,
				0,
				CmdList,
				TRUE);

		CmdList->SetComputeRootDescriptorTable(
			EyeAdaption::RootSignature::LuminanceHistogram::SI_BackBuffer, si_backBuffer);
		CmdList->SetComputeRootUnorderedAccessView(
			EyeAdaption::RootSignature::LuminanceHistogram::UO_HistogramBuffer,
			mHistogramBuffer->Resource()->GetGPUVirtualAddress());

		CmdList->Dispatch(
			D3D12Util::CeilDivide(mInitData.Width >> 1, EyeAdaption::ThreadGroup::Default::Width),
			D3D12Util::CeilDivide(mInitData.Height >> 1, EyeAdaption::ThreadGroup::Default::Height),
			EyeAdaption::ThreadGroup::Default::Depth);
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12EyeAdaption::TemporalSmoothing(D3D12FrameResource* const pFrameResource, FLOAT dt) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[EyeAdaption::PipelineState::CP_TemporalSmoothing].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetComputeRootSignature(
			mRootSignatures[EyeAdaption::RootSignature::GR_TemporalSmoothing].Get());

		const auto AvgLogLum = mAvgLogLuminance.get();
		AvgLogLum->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Util::UavBarrier(CmdList, AvgLogLum);

		const auto Smoothed = mSmoothedLuminance.get();
		Smoothed->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Util::UavBarrier(CmdList, Smoothed);

		const auto Prev = mPrevLuminance.get();
		Prev->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Util::UavBarrier(CmdList, Prev);

		EyeAdaption::RootConstant::TemporalSmoothing::Struct rc{};
		rc.gUpSpeed = 1.25f;
		rc.gGlareUpSpeed = 0.8f;
		rc.gDownSpeed = 3.f;
		rc.gDeltaTime = dt;

		D3D12Util::SetRoot32BitConstants<
			EyeAdaption::RootConstant::TemporalSmoothing::Struct>(
				EyeAdaption::RootSignature::TemporalSmoothing::RC_Consts,
				EyeAdaption::RootConstant::TemporalSmoothing::Count,
				&rc,
				0,
				CmdList,
				TRUE);

		CmdList->SetComputeRootUnorderedAccessView(
			EyeAdaption::RootSignature::TemporalSmoothing::UI_AvgLogLuminance,
			mAvgLogLuminance->Resource()->GetGPUVirtualAddress());
		CmdList->SetComputeRootUnorderedAccessView(
			EyeAdaption::RootSignature::TemporalSmoothing::UO_SmoothedLum,
			mSmoothedLuminance->Resource()->GetGPUVirtualAddress());
		CmdList->SetComputeRootUnorderedAccessView(
			EyeAdaption::RootSignature::TemporalSmoothing::UIO_PrevLum,
			mPrevLuminance->Resource()->GetGPUVirtualAddress());

		CmdList->Dispatch(1, 1, 1);
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12EyeAdaption::PercentileExtract(D3D12FrameResource* const pFrameResource) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[EyeAdaption::PipelineState::CP_PercentileExtract].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetComputeRootSignature(
			mRootSignatures[EyeAdaption::RootSignature::GR_PercentileExtract].Get());

		const auto Histogram = mHistogramBuffer.get();
		Histogram->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Util::UavBarrier(CmdList, Histogram);

		EyeAdaption::RootConstant::PercentileExtract::Struct rc{};
		rc.gMinLogLum = -8.f;
		rc.gMaxLogLum = 4.f;
		rc.gLowPercent = 0.1f;
		rc.gHighPercent = 0.99f;
		rc.gBinCount = 64;

		D3D12Util::SetRoot32BitConstants<
			EyeAdaption::RootConstant::PercentileExtract::Struct>(
				EyeAdaption::RootSignature::PercentileExtract::RC_Consts,
				EyeAdaption::RootConstant::PercentileExtract::Count,
				&rc,
				0,
				CmdList,
				TRUE);

		CmdList->SetComputeRootUnorderedAccessView(
			EyeAdaption::RootSignature::PercentileExtract::UI_HistogramBuffer,
			mHistogramBuffer->Resource()->GetGPUVirtualAddress());
		CmdList->SetComputeRootUnorderedAccessView(
			EyeAdaption::RootSignature::PercentileExtract::UO_AvgLogLuminance,
			mAvgLogLuminance->Resource()->GetGPUVirtualAddress());

		CmdList->Dispatch(1, 1, 1);
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12EyeAdaption::BuildResources() {
	D3D12_RESOURCE_DESC texDesc{};
	texDesc.Format = DXGI_FORMAT_UNKNOWN;
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	texDesc.Height = 1;
	texDesc.Alignment = 0;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	const auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	// HistogramBuffer
	{
		texDesc.Width = MAX_BIN_COUNT * sizeof(EyeAdaption::HistogramBin);

		CheckReturn(mHistogramBuffer->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"EyeAdaption_HistogramBuffer"));
	}
	// AvgLogLuminance
	{
		texDesc.Width = sizeof(EyeAdaption::Result);

		CheckReturn(mAvgLogLuminance->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"EyeAdaption_AvgLogLuminance"));
	}
	// Smoothed/Prev-Luminance
	{
		texDesc.Width = sizeof(FLOAT);

		CheckReturn(mPrevLuminance->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"EyeAdaption_PrevLuminance"));

		CheckReturn(mSmoothedLuminance->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"EyeAdaption_SmoothedLuminance"));
	}

	return true;
}