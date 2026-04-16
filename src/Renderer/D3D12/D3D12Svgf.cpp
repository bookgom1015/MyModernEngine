#include "pch.h"
#include "Renderer/D3D12/D3D12Svgf.hpp"

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

bool D3D12Svgf::CalculateDepthParticalDerivative(
	D3D12FrameResource* const pFrameResource
	, GpuResource* const pDepthMap
	, D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[Svgf::PipelineState::CP_CalcDepthPartialDerivative].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetComputeRootSignature(
			mRootSignatures[Svgf::RootSignature::GR_CalcDepthPartialDerivative].Get());

		const auto DepthPartialDerivative = mResources[Svgf::Resource::E_DepthPartialDerivative].get();

		DepthPartialDerivative->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Util::UavBarrier(CmdList, DepthPartialDerivative);

		Svgf::RootConstant::CalcDepthPartialDerivative::Struct rc{};
		rc.gInvTexDim.x = 1.f / static_cast<FLOAT>(mInitData.Width);
		rc.gInvTexDim.y = 1.f / static_cast<FLOAT>(mInitData.Height);

		D3D12Util::SetRoot32BitConstants<Svgf::RootConstant::CalcDepthPartialDerivative::Struct>(
			Svgf::RootSignature::CalcDepthPartialDerivative::RC_Consts,
			Svgf::RootConstant::CalcDepthPartialDerivative::Count,
			&rc,
			0,
			CmdList,
			TRUE);

		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::CalcDepthPartialDerivative::SI_DepthMap, si_depthMap);

		const auto uav = mpDescHeap->GetGpuHandle(mhDescs[Svgf::Descriptor::EU_DepthPartialDerivative]);
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::CalcDepthPartialDerivative::UO_DepthPartialDerivative,
			uav);

		CmdList->Dispatch(
			D3D12Util::D3D12Util::CeilDivide(mInitData.Width, Svgf::ThreadGroup::Default::Width),
			D3D12Util::D3D12Util::CeilDivide(mInitData.Height, Svgf::ThreadGroup::Default::Height),
			Svgf::ThreadGroup::Default::Depth);
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12Svgf::CalculateLocalMeanVariance(
	D3D12FrameResource* const pFrameResource
	, GpuResource* const pValueMap
	, D3D12_GPU_DESCRIPTOR_HANDLE si_valueMap
	, bool bCheckerboardSamplingEnabled) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[Svgf::PipelineState::CP_CalcLocalMeanVariance].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetComputeRootSignature(
			mRootSignatures[Svgf::RootSignature::GR_CalcLocalMeanVariance].Get());

		const auto RawLocalMeanVariance = mResources[Svgf::Resource::LocalMeanVariance::E_Raw].get();
		RawLocalMeanVariance->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Util::UavBarrier(CmdList, RawLocalMeanVariance);

		const auto uo_localMeanVariance = mpDescHeap->GetGpuHandle(
			mhDescs[Svgf::Descriptor::LocalMeanVariance::EU_Raw]);

		CmdList->SetComputeRootConstantBufferView(
			Svgf::RootSignature::CalcLocalMeanVariance::CB_LocalMeanVariance,
			pFrameResource->CalcLocalMeanVarianceCB.CBAddress());
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::CalcLocalMeanVariance::SI_AOCoefficient, si_valueMap);
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::CalcLocalMeanVariance::UO_LocalMeanVariance, uo_localMeanVariance);

		const INT PixelStepY = bCheckerboardSamplingEnabled ? 2 : 1;
		CmdList->Dispatch(
			D3D12Util::CeilDivide(mInitData.Width, Svgf::ThreadGroup::Default::Width),
			D3D12Util::CeilDivide(mInitData.Height, Svgf::ThreadGroup::Default::Height * PixelStepY),
			Svgf::ThreadGroup::Default::Depth);
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12Svgf::FillInCheckerboard(
	D3D12FrameResource* const pFrameResource
	, bool bCheckerboardSamplingEnabled) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[Svgf::PipelineState::CP_FillInCheckerboard].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetComputeRootSignature(
			mRootSignatures[Svgf::RootSignature::GR_FillInCheckerboard].Get());

		const auto pLocalMeanVarMap = mResources[Svgf::Resource::LocalMeanVariance::E_Raw].get();
		pLocalMeanVarMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Util::UavBarrier(CmdList, pLocalMeanVarMap);

		CmdList->SetComputeRootConstantBufferView(
			Svgf::RootSignature::FillInCheckerboard::CB_LocalMeanVariance,
			pFrameResource->CalcLocalMeanVarianceCB.CBAddress());

		auto uav = mpDescHeap->GetGpuHandle(mhDescs[Svgf::Descriptor::LocalMeanVariance::EU_Raw]);
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::FillInCheckerboard::UIO_LocalMeanVariance, uav);

		const INT PixelStepY = bCheckerboardSamplingEnabled ? 2 : 1;
		CmdList->Dispatch(
			D3D12Util::CeilDivide(mInitData.Width, Svgf::ThreadGroup::Default::Width),
			D3D12Util::CeilDivide(mInitData.Height, Svgf::ThreadGroup::Default::Height * PixelStepY),
			Svgf::ThreadGroup::Default::Depth);
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12Svgf::ReverseReprojectPreviousFrame(
	D3D12FrameResource* const pFrameResource
	, GpuResource* const pNormalDepthMap, D3D12_GPU_DESCRIPTOR_HANDLE si_normalDepthMap
	, GpuResource* const pReprojNormalDepthMap, D3D12_GPU_DESCRIPTOR_HANDLE si_reprojNormalDepthMap
	, GpuResource* const pCachedNormalDepthMap, D3D12_GPU_DESCRIPTOR_HANDLE si_cachedNormalDepthMap
	, GpuResource* const pVelocityMap, D3D12_GPU_DESCRIPTOR_HANDLE si_velocityMap
	, GpuResource* const pCachedValueMap, D3D12_GPU_DESCRIPTOR_HANDLE si_cachedValueMap
	, GpuResource* const pCachedValueSquaredMeanMap, D3D12_GPU_DESCRIPTOR_HANDLE si_cachedValueSquaredMeanMap
	, GpuResource* const pCachedRayHitDistMap, D3D12_GPU_DESCRIPTOR_HANDLE si_cachedRayHitDistMap
	, GpuResource* const pCachedTSPPMap0, D3D12_GPU_DESCRIPTOR_HANDLE si_cachedTSPPMap
	, GpuResource* const pCachedTSPPMap1, D3D12_GPU_DESCRIPTOR_HANDLE uo_cachedTSPPMap) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[Svgf::PipelineState::CP_TemporalSupersamplingReverseReproject].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetComputeRootSignature(
			mRootSignatures[Svgf::RootSignature::GR_TemporalSupersamplingReverseReproject].Get());

		pNormalDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pReprojNormalDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pCachedNormalDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pVelocityMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pCachedValueMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pCachedValueSquaredMeanMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pCachedRayHitDistMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pCachedTSPPMap0->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		pCachedTSPPMap0->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Util::UavBarrier(CmdList, pCachedTSPPMap0);

		const auto pTSPPSquaredMeanRayHitDist = 
			mResources[Svgf::Resource::E_TSPPSquaredMeanRayHitDistance].get();
		pTSPPSquaredMeanRayHitDist->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Util::UavBarrier(CmdList, pTSPPSquaredMeanRayHitDist);

		CmdList->SetComputeRootConstantBufferView(
			Svgf::RootSignature::TemporalSupersamplingReverseReproject::CB_CrossBilateralFilter,
			pFrameResource->CrossBilateralFilterCB.CBAddress());
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::TemporalSupersamplingReverseReproject::SI_NormalDepth,
			si_normalDepthMap);
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::TemporalSupersamplingReverseReproject::SI_ReprojectedNormalDepth,
			si_reprojNormalDepthMap);
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::TemporalSupersamplingReverseReproject::SI_Velocity,
			si_velocityMap);
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::TemporalSupersamplingReverseReproject::SI_DepthPartialDerivative,
			mpDescHeap->GetGpuHandle(mhDescs[Svgf::Descriptor::ES_DepthPartialDerivative]));
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::TemporalSupersamplingReverseReproject::SI_CachedNormalDepth,
			si_cachedNormalDepthMap);
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::TemporalSupersamplingReverseReproject::SI_CachedValue,
			si_cachedValueMap);
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::TemporalSupersamplingReverseReproject::SI_CachedValueSquaredMean,
			si_cachedValueSquaredMeanMap);
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::TemporalSupersamplingReverseReproject::SI_CachedTSPP,
			si_cachedTSPPMap);
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::TemporalSupersamplingReverseReproject::SI_CachedRayHitDistance,
			si_cachedRayHitDistMap);
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::TemporalSupersamplingReverseReproject::UO_CachedTSPP,
			uo_cachedTSPPMap);
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::TemporalSupersamplingReverseReproject::UO_TSPPSquaredMeanRayHitDistacne,
			mpDescHeap->GetGpuHandle(mhDescs[Svgf::Descriptor::EU_TSPPSquaredMeanRayHitDistance]));
		
		Svgf::RootConstant::TemporalSupersamplingReverseReproject::Struct rc{};
		rc.gTexDim = { 
			static_cast<FLOAT>(mInitData.Width), static_cast<FLOAT>(mInitData.Height) };
		rc.gInvTexDim = { 
			1.f / static_cast<FLOAT>(mInitData.Width), 1.f / static_cast<FLOAT>(mInitData.Height) };

		D3D12Util::SetRoot32BitConstants<Svgf::RootConstant::TemporalSupersamplingReverseReproject::Struct>(
			Svgf::RootSignature::TemporalSupersamplingReverseReproject::RC_Consts,
			Svgf::RootConstant::TemporalSupersamplingReverseReproject::Count,
			&rc,
			0,
			CmdList,
			TRUE);

		CmdList->Dispatch(
			D3D12Util::D3D12Util::CeilDivide(mInitData.Width, Svgf::ThreadGroup::Default::Width),
			D3D12Util::D3D12Util::CeilDivide(mInitData.Height, Svgf::ThreadGroup::Default::Height),
			Svgf::ThreadGroup::Default::Depth);
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());
	return true;
}

bool D3D12Svgf::BlendWithCurrentFrame(
	D3D12FrameResource* const pFrameResource
	, GpuResource* const pValueMap, D3D12_GPU_DESCRIPTOR_HANDLE si_valueMap
	, GpuResource* const pRayHitDistanceMap, D3D12_GPU_DESCRIPTOR_HANDLE si_rayHitDistanceMap
	, GpuResource* const pTemporalCacheValueMap, D3D12_GPU_DESCRIPTOR_HANDLE uo_temporalCacheValueMap
	, GpuResource* const pTemporalCacheValueSquaredMeanMap, D3D12_GPU_DESCRIPTOR_HANDLE uo_temporalCacheValueSquaredMeanMap
	, GpuResource* const pTemporalCacheRayHitDistanceMap, D3D12_GPU_DESCRIPTOR_HANDLE uo_temporalCacheRayHitDistanceMap
	, GpuResource* const pTemporalTSPPMap, D3D12_GPU_DESCRIPTOR_HANDLE uo_temporalCacheTSPPMap) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[Svgf::PipelineState::CP_TemporalSupersamplingBlendWithCurrentFrame].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetComputeRootSignature(
			mRootSignatures[Svgf::RootSignature::GR_TemporalSupersamplingBlendWithCurrentFrame].Get());

		pValueMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pRayHitDistanceMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		pTemporalCacheValueMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Util::UavBarrier(CmdList, pTemporalCacheValueMap);

		pTemporalCacheValueSquaredMeanMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Util::UavBarrier(CmdList, pTemporalCacheValueSquaredMeanMap);

		pTemporalCacheRayHitDistanceMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Util::UavBarrier(CmdList, pTemporalCacheRayHitDistanceMap);

		pTemporalTSPPMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Util::UavBarrier(CmdList, pTemporalTSPPMap);

		CmdList->SetComputeRootConstantBufferView(
			Svgf::RootSignature::TemporalSupersamplingBlendWithCurrentFrame::CB_TSPPBlendWithCurrentFrame,
			pFrameResource->BlendWithCurrentFrameCB.CBAddress());
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::TemporalSupersamplingBlendWithCurrentFrame::SI_AOCoefficient,
			si_valueMap);
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::TemporalSupersamplingBlendWithCurrentFrame::SI_LocalMeanVaraince,
			mpDescHeap->GetGpuHandle(mhDescs[Svgf::Descriptor::LocalMeanVariance::ES_Raw]));
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::TemporalSupersamplingBlendWithCurrentFrame::SI_RayHitDistance,
			si_rayHitDistanceMap);
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::TemporalSupersamplingBlendWithCurrentFrame::SI_TSPPSquaredMeanRayHitDistance,
			mpDescHeap->GetGpuHandle(mhDescs[Svgf::Descriptor::ES_TSPPSquaredMeanRayHitDistance]));
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::TemporalSupersamplingBlendWithCurrentFrame::UO_TemporalAOCoefficient,
			uo_temporalCacheValueMap);
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::TemporalSupersamplingBlendWithCurrentFrame::UO_TSPP,
			uo_temporalCacheTSPPMap);
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::TemporalSupersamplingBlendWithCurrentFrame::UO_AOCoefficientSquaredMean,
			uo_temporalCacheValueSquaredMeanMap);
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::TemporalSupersamplingBlendWithCurrentFrame::UO_RayHitDistance,
			uo_temporalCacheRayHitDistanceMap);
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::TemporalSupersamplingBlendWithCurrentFrame::UO_VarianceMap,
			mpDescHeap->GetGpuHandle(mhDescs[Svgf::Descriptor::Variance::EU_Raw]));
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::TemporalSupersamplingBlendWithCurrentFrame::UO_BlurStrength,
			mpDescHeap->GetGpuHandle(mhDescs[Svgf::Descriptor::EU_DisocclusionBlurStrength]));

		CmdList->Dispatch(
			D3D12Util::D3D12Util::CeilDivide(mInitData.Width, Svgf::ThreadGroup::Default::Width),
			D3D12Util::D3D12Util::CeilDivide(mInitData.Height, Svgf::ThreadGroup::Default::Height),
			Svgf::ThreadGroup::Default::Depth);
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12Svgf::ApplyAtrousWaveletTransformFilter(
	D3D12FrameResource* const pFrameResource
	, GpuResource* const pNormalDepthMap, D3D12_GPU_DESCRIPTOR_HANDLE si_normalDepthMap
	, GpuResource* const pTemporalCacheHitDistanceMap, D3D12_GPU_DESCRIPTOR_HANDLE si_temporalCachehitDistanceMap
	, GpuResource* const pTemporalCacheTSPPMap, D3D12_GPU_DESCRIPTOR_HANDLE si_temporalCacheTSPPMap
	, GpuResource* const pTemporalValueMap_Input, D3D12_GPU_DESCRIPTOR_HANDLE si_temporalValueMap
	, GpuResource* const pTemporalValueMap_Output, D3D12_GPU_DESCRIPTOR_HANDLE uo_TemporalValueMap
	, FLOAT rayHitDistToKernelWidthScale
	, FLOAT rayHitDistToKernelSizeScaleExp) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[Svgf::PipelineState::CP_EdgeStoppingFilterGaussian3x3].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetComputeRootSignature(
			mRootSignatures[Svgf::RootSignature::GR_AtrousWaveletTransformFilter].Get());

		pNormalDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pTemporalCacheHitDistanceMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pTemporalCacheTSPPMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		pTemporalValueMap_Input->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		pTemporalValueMap_Output->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Util::UavBarrier(CmdList, pTemporalValueMap_Output);

		Svgf::RootConstant::AtrousWaveletTransformFilter::Struct rc{};		
		rc.gRayHitDistanceToKernelWidthScale = rayHitDistToKernelWidthScale;
		rc.gRayHitDistanceToKernelSizeScaleExponent = rayHitDistToKernelSizeScaleExp;

		D3D12Util::SetRoot32BitConstants<Svgf::RootConstant::AtrousWaveletTransformFilter::Struct>(
			Svgf::RootSignature::AtrousWaveletTransformFilter::RC_Consts,
			Svgf::RootConstant::AtrousWaveletTransformFilter::Count,
			&rc,
			0,
			CmdList,
			TRUE);

		CmdList->SetComputeRootConstantBufferView(
			Svgf::RootSignature::AtrousWaveletTransformFilter::CB_AtrousFilter,
			pFrameResource->AtrousWaveletTransformFilterCB.CBAddress());
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::AtrousWaveletTransformFilter::SI_TemporalValue,
			si_temporalValueMap);
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::AtrousWaveletTransformFilter::SI_NormalDepth,
			si_normalDepthMap);
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::AtrousWaveletTransformFilter::SI_Variance,
			mpDescHeap->GetGpuHandle(mhDescs[Svgf::Descriptor::Variance::ES_Raw]));
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::AtrousWaveletTransformFilter::SI_HitDistance,
			si_temporalCachehitDistanceMap);
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::AtrousWaveletTransformFilter::SI_DepthPartialDerivative,
			mpDescHeap->GetGpuHandle(mhDescs[Svgf::Descriptor::ES_DepthPartialDerivative]));
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::AtrousWaveletTransformFilter::UO_TemporalValue,
			uo_TemporalValueMap);

		CmdList->Dispatch(
			D3D12Util::D3D12Util::CeilDivide(mInitData.Width, Svgf::ThreadGroup::Atrous::Width),
			D3D12Util::D3D12Util::CeilDivide(mInitData.Height, Svgf::ThreadGroup::Atrous::Height),
			Svgf::ThreadGroup::Atrous::Depth);
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12Svgf::BlurDisocclusion(
	D3D12FrameResource* const pFrameResource
	, GpuResource* const pDepthMap, D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap
	, GpuResource* const pRoughnessMetalnessMap, D3D12_GPU_DESCRIPTOR_HANDLE si_roughnessMetalnessMap
	, GpuResource* const pTemporalValueMap, D3D12_GPU_DESCRIPTOR_HANDLE uio_temporalValueMap
	, UINT numLowTSPPBlurPasses) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[Svgf::PipelineState::CP_DisocclusionBlur].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetComputeRootSignature(
			mRootSignatures[Svgf::RootSignature::GR_DisocclusionBlur].Get());

		pDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pRoughnessMetalnessMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		pTemporalValueMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Util::UavBarrier(CmdList, pTemporalValueMap);

		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::DisocclusionBlur::SI_DepthMap, 
			si_depthMap);
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::DisocclusionBlur::SI_RoughnessMetalnessMap, 
			si_roughnessMetalnessMap);
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::DisocclusionBlur::SI_BlurStrength, 
			mpDescHeap->GetGpuHandle(mhDescs[Svgf::Descriptor::ES_DisocclusionBlurStrength]));
		CmdList->SetComputeRootDescriptorTable(
			Svgf::RootSignature::DisocclusionBlur::UIO_AOCoefficient, 
			uio_temporalValueMap);

		Svgf::RootConstant::DisocclusionBlur::Struct rc{};
		rc.gTextureDim.x = mInitData.Width;
		rc.gTextureDim.y = mInitData.Height;
		rc.gMaxStep = numLowTSPPBlurPasses;

		const UINT ThreadGroupX = Svgf::ThreadGroup::Default::Width;
		const UINT ThreadGroupY = Svgf::ThreadGroup::Default::Height;

		UINT filterStep = 1;
		for (UINT i = 0; i < numLowTSPPBlurPasses; ++i) {
			rc.gStep = filterStep;

			D3D12Util::SetRoot32BitConstants<Svgf::RootConstant::DisocclusionBlur::Struct>(
					Svgf::RootSignature::DisocclusionBlur::RC_Consts,
					Svgf::RootConstant::DisocclusionBlur::Count,
					&rc,
					0,
					CmdList,
					TRUE);

			// Account for interleaved Group execution
			const UINT WidthCS = filterStep * ThreadGroupX *
				D3D12Util::CeilDivide(mInitData.Width, filterStep * ThreadGroupX);
			const UINT HeightCS = filterStep * ThreadGroupY *
				D3D12Util::CeilDivide(mInitData.Height, filterStep * ThreadGroupY);

			CmdList->Dispatch(
				D3D12Util::CeilDivide(WidthCS, ThreadGroupX),
				D3D12Util::CeilDivide(HeightCS, ThreadGroupY),
				Svgf::ThreadGroup::Default::Depth);

			filterStep = filterStep << 1;
		}
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

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
