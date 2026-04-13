#include "pch.h"
#include "Renderer/D3D12/D3D12Svgf.hpp"

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
	const WCHAR* const HLSL_CalcPartialDepthDerivative = L"D3D12CalcPartialDepthDerivative.hlsl";
	const WCHAR* const HLSL_CalcLocalMeanVariance = L"D3D12CalcLocalMeanVariance.hlsl";
	const WCHAR* const HLSL_FillInCheckboard = L"D3D12FillInCheckerboard_CrossBox4TapFilter.hlsl";
	const WCHAR* const HLSL_TemporalSupersamplingReverseReproject = L"D3D12TemporalSupersamplingReverseReproject.hlsl";
	const WCHAR* const HLSL_TemporalSupersamplingBlendWithCurrentFrame = L"D3D12TemporalSupersamplingBlendWithCurrentFrame.hlsl";
	const WCHAR* const HLSL_EdgeStoppingFilterGaussian3x3 = L"D3D12EdgeStoppingFilter_Gaussian3x3.hlsl";
	const WCHAR* const HLSL_DisocclusionBlur3x3 = L"D3D12DisocclusionBlur3x3.hlsl";
}

D3D12Svgf:: D3D12Svgf() {}

D3D12Svgf::~D3D12Svgf() {}

bool D3D12Svgf::Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	mInitData = *reinterpret_cast<InitData*>(pData);

	for (UINT i = 0; i < Svgf::Resource::Count; ++i)
		mResources[i] = std::make_unique<GpuResource>();

	CheckReturn(BuildResources());

	return true;
}

bool D3D12Svgf::CompileShaders() {
	// CalcParticalDepthDerivative
	{
		const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
			HLSL_CalcPartialDepthDerivative, L"CS", L"cs_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[Svgf::Shader::CS_CalcParticalDepthDerivative]));
	}
	// CalcLocalMeanVariance
	{
		const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
			HLSL_CalcLocalMeanVariance, L"CS", L"cs_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[Svgf::Shader::CS_CalcLocalMeanVariance]));
	}
	// FillInCheckerboard
	{
		const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
			HLSL_FillInCheckboard, L"CS", L"cs_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[Svgf::Shader::CS_FillinCheckerboard]));
	}
	// TemporalSupersamplingReverseReproject
	{
		const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
			HLSL_TemporalSupersamplingReverseReproject, L"CS", L"cs_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[Svgf::Shader::CS_TemporalSupersamplingReverseReproject]));
	}
	// TemporalSupersamplingBlendWithCurrentFrame
	{
		const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
			HLSL_TemporalSupersamplingBlendWithCurrentFrame, L"CS", L"cs_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[Svgf::Shader::CS_TemporalSupersamplingBlendWithCurrentFrame]));
	}
	// EdgeStoppingFilterGaussian3x3_Contrast
	{
		const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
			HLSL_EdgeStoppingFilterGaussian3x3, L"CS", L"cs_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[Svgf::Shader::CS_EdgeStoppingFilterGaussian3x3]));
	}
	// DisocclusionBlur3x3
	{
		const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
			HLSL_DisocclusionBlur3x3, L"CS", L"cs_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[Svgf::Shader::CS_DisocclusionBlur3x3]));
	}

	return true;
}

bool D3D12Svgf::BuildRootSignatures() {
	decltype(auto) samplers = D3D12Util::GetStaticSamplers();

	// TemporalSupersamplingReverseReproject
	{
		using namespace Svgf::RootSignature::TemporalSupersamplingReverseReproject;

		CD3DX12_DESCRIPTOR_RANGE texTables[11]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 7);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 8);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[Count]{};
		slotRootParameter[CB_CrossBilateralFilter].InitAsConstantBufferView(0);
		slotRootParameter[RC_Consts].InitAsConstants(
			Svgf::RootConstant::TemporalSupersamplingReverseReproject::Count, 1);
		slotRootParameter[SI_NormalDepth].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[SI_ReprojectedNormalDepth].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[SI_Velocity].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[SI_DepthPartialDerivative].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[SI_CachedNormalDepth].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[SI_CachedValue].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[SI_CachedValueSquaredMean].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[SI_CachedTSPP].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[SI_CachedRayHitDistance].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[UO_CachedTSPP].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[UO_TSPPSquaredMeanRayHitDistacne].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_NONE);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSignatureDesc,
			IID_PPV_ARGS(&mRootSignatures[Svgf::RootSignature::GR_TemporalSupersamplingReverseReproject]),
			L"SVGF_GR_TemporalSupersamplingReverseReproject"));
	}
	// TemporalSupersamplingBlendWithCurrentFrame
	{
		using namespace Svgf::RootSignature::TemporalSupersamplingBlendWithCurrentFrame;

		CD3DX12_DESCRIPTOR_RANGE texTables[10]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 3, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 4, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 5, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[Count]{};
		slotRootParameter[CB_TSPPBlendWithCurrentFrame].InitAsConstantBufferView(0);
		slotRootParameter[SI_AOCoefficient].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[SI_LocalMeanVaraince].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[SI_RayHitDistance].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[SI_TSPPSquaredMeanRayHitDistance].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[UO_TemporalAOCoefficient].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[UO_TSPP].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[UO_AOCoefficientSquaredMean].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[UO_RayHitDistance].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[UO_VarianceMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[UO_BlurStrength].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_NONE);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSignatureDesc,
			IID_PPV_ARGS(&mRootSignatures[Svgf::RootSignature::GR_TemporalSupersamplingBlendWithCurrentFrame]),
			L"SVGF_GR_TemporalSupersamplingBlendWithCurrentFrame"));
	}
	// CalculateDepthPartialDerivative
	{
		using namespace Svgf::RootSignature::CalcDepthPartialDerivative;

		CD3DX12_DESCRIPTOR_RANGE texTables[2]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[Count]{};
		slotRootParameter[RC_Consts].InitAsConstants(
			Svgf::RootConstant::CalcDepthPartialDerivative::Count, 0, 0);
		slotRootParameter[SI_DepthMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[UO_DepthPartialDerivative].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_NONE);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSignatureDesc,
			IID_PPV_ARGS(&mRootSignatures[Svgf::RootSignature::GR_CalcDepthPartialDerivative]),
			L"SVGF_GR_CalcDepthPartialDerivative"));
	}
	// CalculateMeanVariance
	{
		using namespace Svgf::RootSignature::CalcLocalMeanVariance;

		CD3DX12_DESCRIPTOR_RANGE texTables[2]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[Count]{};
		slotRootParameter[CB_LocalMeanVariance].InitAsConstantBufferView(0, 0);
		slotRootParameter[SI_AOCoefficient].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[UO_LocalMeanVariance].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_NONE);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSignatureDesc,
			IID_PPV_ARGS(&mRootSignatures[Svgf::RootSignature::GR_CalcLocalMeanVariance]),
			L"SVGF_GR_CalcLocalMeanVariance"));
	}
	// FillInCheckerboard
	{
		using namespace Svgf::RootSignature::FillInCheckerboard;

		CD3DX12_DESCRIPTOR_RANGE texTables[1]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[Count]{};
		slotRootParameter[CB_LocalMeanVariance].InitAsConstantBufferView(0, 0);
		slotRootParameter[UIO_LocalMeanVariance].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_NONE);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSignatureDesc,
			IID_PPV_ARGS(&mRootSignatures[Svgf::RootSignature::GR_FillInCheckerboard]),
			L"SVGF_GR_FillInCheckerboard"));
	}
	// Atrous Wavelet transform filter
	{
		using namespace Svgf::RootSignature::AtrousWaveletTransformFilter;

		CD3DX12_DESCRIPTOR_RANGE texTables[7]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[Count]{};
		slotRootParameter[CB_AtrousFilter].InitAsConstantBufferView(0, 0);
		slotRootParameter[RC_Consts].InitAsConstants(
			Svgf::RootConstant::AtrousWaveletTransformFilter::Count, 1);
		slotRootParameter[SI_TemporalValue].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[SI_NormalDepth].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[SI_Variance].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[SI_HitDistance].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[SI_DepthPartialDerivative].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[SI_TSPP].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[UO_TemporalValue].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
			_countof(slotRootParameter), slotRootParameter,
			0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_NONE);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSignatureDesc,
			IID_PPV_ARGS(&mRootSignatures[Svgf::RootSignature::GR_AtrousWaveletTransformFilter]),
			L"SVGF_GR_AtrousWaveletTransformFilter"));
	}
	// Disocclusion blur
	{
		using namespace Svgf::RootSignature::DisocclusionBlur;

		CD3DX12_DESCRIPTOR_RANGE texTables[4]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[Count]{};
		slotRootParameter[RC_Consts].InitAsConstants(
			Svgf::RootConstant::DisocclusionBlur::Count, 0);
		slotRootParameter[SI_DepthMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[SI_BlurStrength].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[SI_RoughnessMetalnessMap].InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[UIO_AOCoefficient].InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
			_countof(slotRootParameter), slotRootParameter,
			0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_NONE);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,	
			rootSignatureDesc,
			IID_PPV_ARGS(&mRootSignatures[Svgf::RootSignature::GR_DisocclusionBlur]),
			L"SVGF_GR_DisocclusionBlur"));
	}

	return true;
}

bool D3D12Svgf::BuildPipelineStates() {
	// CalcDepthPartialDerivative
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[Svgf::RootSignature::GR_CalcDepthPartialDerivative].Get();
		{
			const auto CS = mInitData.ShaderManager->GetShader(
				mShaderHashes[Svgf::Shader::CS_CalcParticalDepthDerivative]);
			NullCheck(CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		CheckReturn(D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[Svgf::PipelineState::CP_CalcDepthPartialDerivative]),
			L"SVGF_CP_CalcDepthPartialDerivative"));
	}
	// CalcLocalMeanVariance
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[Svgf::RootSignature::GR_CalcLocalMeanVariance].Get();
		{
			const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Svgf::Shader::CS_CalcLocalMeanVariance]);
			NullCheck(CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		CheckReturn(D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[Svgf::PipelineState::CP_CalcLocalMeanVariance]),
			L"SVGF_CP_CalcLocalMeanVariance"));
	}
	// FillinCheckerboard
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[Svgf::RootSignature::GR_FillInCheckerboard].Get();
		{
			const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Svgf::Shader::CS_FillinCheckerboard]);
			NullCheck(CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		CheckReturn(D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[Svgf::PipelineState::CP_FillInCheckerboard]),
			L"SVGF_CP_FillInCheckerboard"));
	}
	// TemporalSupersamplingReverseReproject
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[Svgf::RootSignature::GR_TemporalSupersamplingReverseReproject].Get();
		{
			const auto CS = mInitData.ShaderManager->GetShader(
				mShaderHashes[Svgf::Shader::CS_TemporalSupersamplingReverseReproject]);
			NullCheck(CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		CheckReturn(D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[Svgf::PipelineState::CP_TemporalSupersamplingReverseReproject]),
			L"SVGF_CP_TemporalSupersamplingReverseReproject"));
	}
	// TemporalSupersamplingBlendWithCurrentFrame
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[Svgf::RootSignature::GR_TemporalSupersamplingBlendWithCurrentFrame].Get();
		{
			const auto CS = mInitData.ShaderManager->GetShader(
				mShaderHashes[Svgf::Shader::CS_TemporalSupersamplingBlendWithCurrentFrame]);
			NullCheck(CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		CheckReturn(D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[Svgf::PipelineState::CP_TemporalSupersamplingBlendWithCurrentFrame]),
			L"SVGF_CP_TemporalSupersamplingBlendWithCurrentFrame"));
	}
	// E_EdgeStoppingFilterGaussian3x3
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[Svgf::RootSignature::GR_AtrousWaveletTransformFilter].Get();
		{
			const auto CS = mInitData.ShaderManager->GetShader(
				mShaderHashes[Svgf::Shader::CS_EdgeStoppingFilterGaussian3x3]);
			NullCheck(CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		CheckReturn(D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[Svgf::PipelineState::CP_EdgeStoppingFilterGaussian3x3]),
			L"SVGF_CP_EdgeStoppingFilterGaussian3x3"));
	}
	// DisocclusionBlur3x3_Contrast
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[Svgf::RootSignature::GR_DisocclusionBlur].Get();
		{
			const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Svgf::Shader::CS_DisocclusionBlur3x3]);
			NullCheck(CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		CheckReturn(D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[Svgf::PipelineState::CP_DisocclusionBlur]),
			L"SVGF_CP_DisocclusionBlur3x3"));
	}

	return true;
}

bool D3D12Svgf::AllocateDescriptors() {
	for (UINT i = 0; i < Svgf::Descriptor::Count; ++i) 
		mpDescHeap->AllocateCbvSrvUav(1, mhDescs[i]);

	CheckReturn(BuildDescriptors());

	return true;
}

bool D3D12Svgf::OnResize(unsigned width, unsigned height) {
	mInitData.Width = width;
	mInitData.Height = height;

	CheckReturn(BuildResources());
	CheckReturn(BuildDescriptors());

	return true;
}

bool D3D12Svgf::BuildResources() {
	D3D12_RESOURCE_DESC texDesc{};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mInitData.Width;
	texDesc.Height = mInitData.Height;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	const auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	// LocalMeanVarianceMap
	{
		texDesc.Format = Svgf::LocalMeanVarianceMapFormat;

		CheckReturn(mResources[Svgf::Resource::LocalMeanVariance::E_Raw]->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"SVGF_LocalMeanVarianceMap_Raw"));
		CheckReturn(mResources[Svgf::Resource::LocalMeanVariance::E_Smoothed]->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"SVGF_LocalMeanVarianceMap_Smoothed"));
	}
	// VarianceMap
	{
		texDesc.Format = Svgf::VarianceMapFormat;

		CheckReturn(mResources[Svgf::Resource::Variance::E_Raw]->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"SVGF_VarianceMap_Raw"));
		CheckReturn(mResources[Svgf::Resource::Variance::E_Smoothed]->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"SVGF_VarianceMap_Smoothed"));
	}
	// ValueMap
	{
		texDesc.Format = Svgf::ValueMapFormat;

		CheckReturn(mResources[Svgf::Resource::E_CachedValue]->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"SVGF_CachedValueMap"));
	}
	// CachedValueSquaredMeanMap
	{
		texDesc.Format = Svgf::ValueSquaredMeanMapFormat;

		CheckReturn(mResources[Svgf::Resource::E_CachedSquaredMean]->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"SVGF_CachedValueSquaredMeanMap"));
	}
	// DepthPartialDerivativeMap
	{
		texDesc.Format = Svgf::DepthPartialDerivativeMapFormat;

		CheckReturn(mResources[Svgf::Resource::E_DepthPartialDerivative]->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"SVGF_DepthPartialDerivativeMap"));
	}
	// DisocclusionBlurStrengthMap
	{
		texDesc.Format = Svgf::DisocclusionBlurStrengthMapFormat;

		CheckReturn(mResources[Svgf::Resource::E_DisocclusionBlurStrength]->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"SVGF_DisocclusionBlurStrengthMap"));
	}
	// TSPPSquaredMeanRayHitDistanceMap
	{
		texDesc.Format = Svgf::TSPPSquaredMeanRayHitDistanceMapFormat;

		CheckReturn(mResources[Svgf::Resource::E_TSPPSquaredMeanRayHitDistance]->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"SVGF_TSPPSquaredMeanRayHitDistanceMap"));
	}

	return true;
}

bool D3D12Svgf::BuildDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	// LocalMeanVarianceMap
	{
		srvDesc.Format = Svgf::LocalMeanVarianceMapFormat;
		uavDesc.Format = Svgf::LocalMeanVarianceMapFormat;

		// Raw
		{
			const auto resource = mResources[Svgf::Resource::LocalMeanVariance::E_Raw]->Resource();

			auto srv = mpDescHeap->GetCpuHandle(mhDescs[Svgf::Descriptor::LocalMeanVariance::ES_Raw]);
			auto uav = mpDescHeap->GetCpuHandle(mhDescs[Svgf::Descriptor::LocalMeanVariance::EU_Raw]);

			mInitData.Device->CreateShaderResourceView(resource, &srvDesc, srv);
			mInitData.Device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, uav);
		}
		// Smoothed
		{
			const auto resource = mResources[Svgf::Resource::LocalMeanVariance::E_Smoothed]->Resource();

			auto srv = mpDescHeap->GetCpuHandle(mhDescs[Svgf::Descriptor::LocalMeanVariance::ES_Smoothed]);
			auto uav = mpDescHeap->GetCpuHandle(mhDescs[Svgf::Descriptor::LocalMeanVariance::EU_Smoothed]);

			mInitData.Device->CreateShaderResourceView(resource, &srvDesc, srv);
			mInitData.Device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, uav);
		}
	}
	// VarianceMap
	{
		srvDesc.Format = Svgf::VarianceMapFormat;
		uavDesc.Format = Svgf::VarianceMapFormat;

		// Raw
		{
			const auto resource = mResources[Svgf::Resource::Variance::E_Raw]->Resource();

			auto srv = mpDescHeap->GetCpuHandle(mhDescs[Svgf::Descriptor::Variance::ES_Raw]);
			auto uav = mpDescHeap->GetCpuHandle(mhDescs[Svgf::Descriptor::Variance::EU_Raw]);

			mInitData.Device->CreateShaderResourceView(resource, &srvDesc, srv);
			mInitData.Device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, uav);
		}
		// Smoothed
		{
			const auto resource = mResources[Svgf::Resource::Variance::E_Smoothed]->Resource();

			auto srv = mpDescHeap->GetCpuHandle(mhDescs[Svgf::Descriptor::Variance::ES_Smoothed]);
			auto uav = mpDescHeap->GetCpuHandle(mhDescs[Svgf::Descriptor::Variance::EU_Smoothed]);

			mInitData.Device->CreateShaderResourceView(resource, &srvDesc, srv);
			mInitData.Device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, uav);
		}
	}
	// DepthPartialDerivativeMap
	{
		srvDesc.Format = Svgf::DepthPartialDerivativeMapFormat;
		uavDesc.Format = Svgf::DepthPartialDerivativeMapFormat;

		const auto resource = mResources[Svgf::Resource::E_DepthPartialDerivative]->Resource();

		auto srv = mpDescHeap->GetCpuHandle(mhDescs[Svgf::Descriptor::ES_DepthPartialDerivative]);
		auto uav = mpDescHeap->GetCpuHandle(mhDescs[Svgf::Descriptor::EU_DepthPartialDerivative]);

		mInitData.Device->CreateShaderResourceView(resource, &srvDesc, srv);
		mInitData.Device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, uav);
	}
	// DisocclusionBlurStrengthMap
	{
		srvDesc.Format = Svgf::DisocclusionBlurStrengthMapFormat;
		uavDesc.Format = Svgf::DisocclusionBlurStrengthMapFormat;

		const auto resource = mResources[Svgf::Resource::E_DisocclusionBlurStrength]->Resource();

		auto srv = mpDescHeap->GetCpuHandle(mhDescs[Svgf::Descriptor::ES_DisocclusionBlurStrength]);
		auto uav = mpDescHeap->GetCpuHandle(mhDescs[Svgf::Descriptor::EU_DisocclusionBlurStrength]);

		mInitData.Device->CreateShaderResourceView(resource, &srvDesc, srv);
		mInitData.Device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, uav);
	}
	// TSPPSquaredMeanRayHitDistanceMap
	{
		srvDesc.Format = Svgf::TSPPSquaredMeanRayHitDistanceMapFormat;
		uavDesc.Format = Svgf::TSPPSquaredMeanRayHitDistanceMapFormat;

		const auto resource = mResources[Svgf::Resource::E_TSPPSquaredMeanRayHitDistance]->Resource();

		auto srv = mpDescHeap->GetCpuHandle(mhDescs[Svgf::Descriptor::ES_TSPPSquaredMeanRayHitDistance]);
		auto uav = mpDescHeap->GetCpuHandle(mhDescs[Svgf::Descriptor::EU_TSPPSquaredMeanRayHitDistance]);

		mInitData.Device->CreateShaderResourceView(resource, &srvDesc, srv);
		mInitData.Device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, uav);
	}

	return true;
}
