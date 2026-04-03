#include "pch.h"
#include "Renderer/D3D12/D3D12Bloom.hpp"

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

#include "Renderer/D3D12/D3D12TextureScaler.hpp"
#include "Renderer/D3D12/D3D12BlurFilter.hpp"

namespace {
	const WCHAR* const HLSL_BlendBloomWithDownSampled = L"D3D12BlendBloomWithDownSampled.hlsl";
	const WCHAR* const HLSL_ApplyBloom = L"D3D12ApplyBloom.hlsl";
}

D3D12Bloom::D3D12Bloom() {}

D3D12Bloom::~D3D12Bloom() {}

bool D3D12Bloom::Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	mInitData = *reinterpret_cast<InitData*>(pData);

	for (size_t i = 0; i < Bloom::Resource::Count; ++i) {
		mHighlightMaps[i] = std::make_unique<GpuResource>();
		mBloomMaps[i] = std::make_unique<GpuResource>();
	}

	CheckReturn(BuildResources());

	return true;
}

bool D3D12Bloom::CompileShaders() {
	// BlendBloomWithDownSampled
	{
		const auto CS = D3D12ShaderManager::D3D12ShaderInfo(
			HLSL_BlendBloomWithDownSampled, L"CS", L"cs_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			CS, mShaderHashes[Bloom::Shader::CS_BlendBloomWithDownSampled]));
	}
	// ApplyBloom
	{
		const auto VS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_ApplyBloom, L"VS", L"vs_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			VS, mShaderHashes[Bloom::Shader::VS_ApplyBloom]));
		const auto MS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_ApplyBloom, L"MS", L"ms_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			MS, mShaderHashes[Bloom::Shader::MS_ApplyBloom]));
		const auto PS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_ApplyBloom, L"PS", L"ps_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			PS, mShaderHashes[Bloom::Shader::PS_ApplyBloom]));
	}

	return true;
}

bool D3D12Bloom::BuildRootSignatures() {
	decltype(auto) samplers = D3D12Util::GetStaticSamplers();

	// BlendBloomWithDownSampled
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[2]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[Bloom::RootSignature::BlendBloomWithDownSampled::Count]{};
		slotRootParameter[Bloom::RootSignature::BlendBloomWithDownSampled::RC_Consts]
			.InitAsConstants(Bloom::RootConstant::BlendBloomWithDownSampled::Count, 0);
		slotRootParameter[Bloom::RootSignature::BlendBloomWithDownSampled::SI_LowerScaleMap]
			.InitAsDescriptorTable(
			1, &texTables[index++]);
		slotRootParameter[Bloom::RootSignature::BlendBloomWithDownSampled::UIO_HigherScaleMap]
			.InitAsDescriptorTable(
			1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[Bloom::RootSignature::GR_BlendBloomWithDownSampled]),
			L"Bloom_GR_BlendBloomWithDownSampled");

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[Bloom::RootSignature::GR_BlendBloomWithDownSampled]),
			L"Bloom_GR_BlendBloomWithDownSampled"));
	}
	// ApplyBloom
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[2]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[Bloom::RootSignature::ApplyBloom::Count]{};
		slotRootParameter[Bloom::RootSignature::ApplyBloom::RC_Consts]
			.InitAsConstants(Bloom::RootConstant::ApplyBloom::Count, 0);
		slotRootParameter[Bloom::RootSignature::ApplyBloom::SI_BackBuffer]
			.InitAsDescriptorTable(1, &texTables[index++]);
		slotRootParameter[Bloom::RootSignature::ApplyBloom::SI_BloomMap]
			.InitAsDescriptorTable(1, &texTables[index++]);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[Bloom::RootSignature::GR_ApplyBloom]),
			L"Bloom_GR_ApplyBloom"));
	}

	return true;
}

bool D3D12Bloom::BuildPipelineStates() {
	// BlendBloomWithDownSampled
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = mRootSignatures[Bloom::RootSignature::GR_BlendBloomWithDownSampled].Get();
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		{
			const auto CS = mInitData.ShaderManager->GetShader(
				mShaderHashes[Bloom::Shader::CS_BlendBloomWithDownSampled]);
			NullCheck(CS);
			psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
		}

		CheckReturn(D3D12Util::CreateComputePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[Bloom::PipelineState::CP_BlendBloomWithDownSampled]),
			L"Bloom_CP_BlendBloomWithDownSampled"));
	}
	// ApplyBloom
	{
		if (mInitData.Device->IsMeshShaderSupported()) {
			auto psoDesc = D3D12Util::FitToScreenMeshPsoDesc();
			psoDesc.pRootSignature = mRootSignatures[Bloom::RootSignature::GR_ApplyBloom].Get();
			{
				const auto MS = mInitData.ShaderManager->GetShader(
					mShaderHashes[Bloom::Shader::MS_ApplyBloom]);
				NullCheck(MS);
				const auto PS = mInitData.ShaderManager->GetShader(
					mShaderHashes[Bloom::Shader::PS_ApplyBloom]);
				NullCheck(PS);
				psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			CheckReturn(D3D12Util::CreatePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[Bloom::PipelineState::MP_ApplyBloom]),
				L"Bloom_MP_ApplyBloom"));
		}
		else {
			auto psoDesc = D3D12Util::FitToScreenPsoDesc();
			psoDesc.pRootSignature = mRootSignatures[Bloom::RootSignature::GR_ApplyBloom].Get();
			{
				const auto VS = mInitData.ShaderManager->GetShader(
					mShaderHashes[Bloom::Shader::VS_ApplyBloom]);
				NullCheck(VS);
				const auto PS = mInitData.ShaderManager->GetShader(
					mShaderHashes[Bloom::Shader::PS_ApplyBloom]);
				NullCheck(PS);
				psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.RTVFormats[0] = HDR_FORMAT;

			CheckReturn(D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[Bloom::PipelineState::GP_ApplyBloom]),
				L"Bloom_GP_ApplyBloom"));
		}
	}

	return true;
}

bool D3D12Bloom::AllocateDescriptors() {
	for (UINT i = 0; i < Bloom::Resource::Count; ++i) {
		CheckReturn(mpDescHeap->AllocateCbvSrvUav(1, mhHighlightMapSrvs[i]));
		CheckReturn(mpDescHeap->AllocateCbvSrvUav(1, mhHighlightMapUavs[i]));

		CheckReturn(mpDescHeap->AllocateCbvSrvUav(1, mhBloomMapSrvs[i]));
		CheckReturn(mpDescHeap->AllocateCbvSrvUav(1, mhBloomMapUavs[i]));
	}

	CheckReturn(BuildDescriptors());

	return true;
}

bool D3D12Bloom::OnResize(unsigned width, unsigned height) {
	mInitData.Width = width;
	mInitData.Height = height;

	CheckReturn(BuildResources());
	CheckReturn(BuildDescriptors());

	return true;
}

bool D3D12Bloom::ApplyBloom(
	D3D12FrameResource* const pFrameResource
	, D3D12_VIEWPORT viewport
	, D3D12_RECT scissorRect
	, GpuResource* const pBackBuffer
	, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer
	, GpuResource* const pBackBufferCopy
	, D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy) {
	// Copy back buffer to temporary buffer
	{
		CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
			pFrameResource->FrameCommandAllocator()));

		const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
		CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_DEST);

		CmdList->CopyResource(pBackBufferCopy->Resource(), pBackBuffer->Resource());

		CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());
	}

	CheckReturn(DownSampleHighlights(pFrameResource, pBackBufferCopy, si_backBufferCopy));
	CheckReturn(UpSampleWithBlur(pFrameResource));

	// Applyt bloom effec to back buffer
	{
		CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
			pFrameResource->FrameCommandAllocator(),
			mPipelineStates[mInitData.Device->IsMeshShaderSupported() ?
			Bloom::PipelineState::MP_ApplyBloom : Bloom::PipelineState::GP_ApplyBloom].Get()));

		const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
		CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

		{
			CmdList->SetGraphicsRootSignature(
				mRootSignatures[Bloom::RootSignature::GR_ApplyBloom].Get());

			CmdList->RSSetViewports(1, &viewport);
			CmdList->RSSetScissorRects(1, &scissorRect);

			pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
			pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_DEST);

			CmdList->CopyResource(pBackBufferCopy->Resource(), pBackBuffer->Resource());

			const auto BloomMap = mBloomMaps[0].get();
			pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
			BloomMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, nullptr);

			CmdList->SetGraphicsRootDescriptorTable(
				Bloom::RootSignature::ApplyBloom::SI_BackBuffer, si_backBufferCopy);
			CmdList->SetGraphicsRootDescriptorTable(
				Bloom::RootSignature::ApplyBloom::SI_BloomMap, mpDescHeap->GetGpuHandle(mhBloomMapSrvs[0]));

			Bloom::RootConstant::ApplyBloom::Struct rc{};
			rc.gSharpness = SHADER_ARGUMENT_MANAGER->Bloom.Sharpness;

			D3D12Util::SetRoot32BitConstants<Bloom::RootConstant::ApplyBloom::Struct>(
				Bloom::RootSignature::ApplyBloom::RC_Consts,
				Bloom::RootConstant::ApplyBloom::Count,
				&rc,
				0,
				CmdList,
				FALSE);

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

	return true;
}

bool D3D12Bloom::BuildResources() {
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	{
		texDesc.Format = Bloom::HighlightMapFormat;

		UINT texW = mInitData.Width >> 1;
		UINT texH = mInitData.Height >> 1;

		for (UINT i = 0; i < Bloom::Resource::Count; ++i) {
			texDesc.Width = texW;
			texDesc.Height = texH;

			const auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			// HighlightMap
			{
				CheckReturn(mHighlightMaps[i]->Initialize(
					mInitData.Device,
					&prop,
					D3D12_HEAP_FLAG_NONE,
					&texDesc,
					D3D12_RESOURCE_STATE_COMMON,
					nullptr,
					std::format(L"Bloom_HighlightMap_{}", i).c_str()));
			}
			// BloomMap
			{
				CheckReturn(mBloomMaps[i]->Initialize(
					mInitData.Device,
					&prop,
					D3D12_HEAP_FLAG_NONE,
					&texDesc,
					D3D12_RESOURCE_STATE_COMMON,
					nullptr,
					std::format(L"Bloom_BloomMap_{}", i).c_str()));
			}

			texW = texW >> 1;
			texH = texH >> 1;
		}
	}

	return true;
}

bool D3D12Bloom::BuildDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.f;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	uavDesc.Texture2D.PlaneSlice = 0;

	{
		srvDesc.Format = Bloom::HighlightMapFormat;
		uavDesc.Format = Bloom::HighlightMapFormat;

		for (UINT i = 0; i < Bloom::Resource::Count; ++i) {
			// HighlightMap
			{
				const auto resource = mHighlightMaps[i]->Resource();

				D3D12Util::CreateShaderResourceView(
					mInitData.Device, resource, &srvDesc, mpDescHeap->GetCpuHandle(mhHighlightMapSrvs[i]));
				D3D12Util::CreateUnorderedAccessView(
					mInitData.Device, resource, nullptr, &uavDesc, mpDescHeap->GetCpuHandle(mhHighlightMapUavs[i]));
			}
			// BloomMap
			{
				const auto resource = mBloomMaps[i]->Resource();

				D3D12Util::CreateShaderResourceView(
					mInitData.Device, resource, &srvDesc, mpDescHeap->GetCpuHandle(mhBloomMapSrvs[i]));
				D3D12Util::CreateUnorderedAccessView(
					mInitData.Device, resource, nullptr, &uavDesc, mpDescHeap->GetCpuHandle(mhBloomMapUavs[i]));
			}
		}
	}

	return true;
}

bool D3D12Bloom::DownSampleHighlights(
	D3D12FrameResource* const pFrameResource
	, GpuResource* const pBackBuffer
	, D3D12_GPU_DESCRIPTOR_HANDLE si_backBuffer) {
	const auto textureScaler = RENDER_PASS_MANAGER->Get<D3D12TextureScaler>();

	Uint2 srcTexDim = { mInitData.Width, mInitData.Height };
	Uint2 dstTexDim = { mInitData.Width >> 1, mInitData.Height >> 1 };

	for (UINT i = 0; i < Bloom::Resource::Count; ++i) {
		GpuResource* srcMap = i == 0
			? pBackBuffer : mHighlightMaps[i - 1].get();
		D3D12_GPU_DESCRIPTOR_HANDLE srcHandle = i == 0
			? si_backBuffer : mpDescHeap->GetGpuHandle(mhHighlightMapSrvs[i - 1]);

		GpuResource* dstMap = mHighlightMaps[i].get();
		D3D12_GPU_DESCRIPTOR_HANDLE dstHandle = mpDescHeap->GetGpuHandle(mhHighlightMapUavs[i]);

		CheckReturn(textureScaler->DownSample2Nx2N(
			pFrameResource,
			srcMap,
			srcHandle,
			dstMap,
			dstHandle,
			srcTexDim.x,
			srcTexDim.y,
			dstTexDim.x,
			dstTexDim.y,
			TextureScaler::KernelRadius::E_DownSample6x6));

		srcTexDim.x = srcTexDim.x >> 1;
		srcTexDim.y = srcTexDim.y >> 1;
		dstTexDim.x = dstTexDim.x >> 1;
		dstTexDim.y = dstTexDim.y >> 1;
	}

	return true;
}

bool D3D12Bloom::UpSampleWithBlur(D3D12FrameResource* const pFrameResource) {
	Uint2 texDim = { mInitData.Width >> 4, mInitData.Height >> 4 };

	auto blurFilter = RENDER_PASS_MANAGER->Get<D3D12BlurFilter>();

	CheckReturn(blurFilter->GaussianBlur(
		pFrameResource,
		BlurFilter::PipelineState::CP_GaussianBlurFilterRGBANxN9x9,
		mHighlightMaps[Bloom::Resource::E_256thRes].get(),
		mpDescHeap->GetGpuHandle(mhHighlightMapSrvs[Bloom::Resource::E_256thRes]),
		mBloomMaps[Bloom::Resource::E_256thRes].get(),
		mpDescHeap->GetGpuHandle(mhBloomMapUavs[Bloom::Resource::E_256thRes]),
		texDim.x, texDim.y));

	auto highSampIndex = Bloom::Resource::E_64thRes;
	auto lowSampIndex = Bloom::Resource::E_256thRes;

	texDim.x = texDim.x << 1;
	texDim.y = texDim.y << 1;

	for (UINT i = 0, end = Bloom::Resource::Count - 1; i < end; ++i) {
		// Upsample
		{
			CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
				pFrameResource->FrameCommandAllocator(),
				mPipelineStates[Bloom::PipelineState::CP_BlendBloomWithDownSampled].Get()));

			const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
			CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

			{
				CmdList->SetComputeRootSignature(
					mRootSignatures[Bloom::RootSignature::GR_BlendBloomWithDownSampled].Get());

				const auto HighSampMap = mHighlightMaps[highSampIndex].get();
				const auto LowSampMap = mBloomMaps[lowSampIndex].get();

				LowSampMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

				HighSampMap->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				D3D12Util::UavBarrier(CmdList, HighSampMap);

				Bloom::RootConstant::BlendBloomWithDownSampled::Struct rc;
				rc.gInvTexDim = { 
					static_cast<FLOAT>(1.f / texDim.x), static_cast<FLOAT>(1.f / texDim.y) };

				D3D12Util::SetRoot32BitConstants<Bloom::RootConstant::BlendBloomWithDownSampled::Struct>(
					Bloom::RootSignature::BlendBloomWithDownSampled::RC_Consts,
					Bloom::RootConstant::BlendBloomWithDownSampled::Count,
					&rc,
					0,
					CmdList,
					TRUE);

				CmdList->SetComputeRootDescriptorTable(
					Bloom::RootSignature::BlendBloomWithDownSampled::SI_LowerScaleMap, 
					mpDescHeap->GetGpuHandle(mhBloomMapSrvs[lowSampIndex]));
				CmdList->SetComputeRootDescriptorTable(
					Bloom::RootSignature::BlendBloomWithDownSampled::UIO_HigherScaleMap, 
					mpDescHeap->GetGpuHandle(mhHighlightMapUavs[highSampIndex]));

				CmdList->Dispatch(
					D3D12Util::CeilDivide(texDim.x, Bloom::ThreadGroup::Default::Width),
					D3D12Util::CeilDivide(texDim.y, Bloom::ThreadGroup::Default::Height),
					Bloom::ThreadGroup::Default::Depth);
			}

			CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());
		}
		// Blur
		{
			CheckReturn(blurFilter->GaussianBlur(
				pFrameResource,
				BlurFilter::PipelineState::CP_GaussianBlurFilterRGBANxN9x9,
				mHighlightMaps[highSampIndex].get(),
				mpDescHeap->GetGpuHandle(mhHighlightMapSrvs[highSampIndex]),
				mBloomMaps[highSampIndex].get(),
				mpDescHeap->GetGpuHandle(mhBloomMapUavs[highSampIndex]),
				texDim.x, texDim.y));

			highSampIndex = static_cast<Bloom::Resource::Type>(
				std::max<UINT>(static_cast<UINT>(highSampIndex) - 1, 0));
			lowSampIndex = static_cast<Bloom::Resource::Type>(
				std::max<UINT>(static_cast<UINT>(lowSampIndex) - 1, 0));

			texDim.x = texDim.x << 1;
			texDim.y = texDim.y << 1;
		}
	}

	return true;
}