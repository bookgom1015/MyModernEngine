#include "pch.h"
#include "Renderer/D3D12/D3D12Ssao.hpp"

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
	const WCHAR* const HLSL_SSAO = L"D3D12Ssao.hlsl";
}

D3D12Ssao::D3D12Ssao()
	: mCurrentTemporalCacheFrameIndex{}
	, mCurrentTemporalAOFrameIndex{} {}

D3D12Ssao::~D3D12Ssao() {}

bool D3D12Ssao::Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	for (UINT resource = 0; resource < Ssao::Resource::AO::Count; ++resource)
		mAOResources[resource] = std::make_unique<GpuResource>();

	for (UINT frame = 0; frame < 2; ++frame) {
		for (UINT resource = 0; resource < Ssao::Resource::TemporalCache::Count; ++resource)
			mTemporalCaches[frame][resource] = std::make_unique<GpuResource>();

		mTemporalAOResources[frame] = std::make_unique<GpuResource>();
	}

	CheckReturn(BuildResources());

	return true;
}

bool D3D12Ssao::CompileShaders() {
	const auto CS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_SSAO, L"CS", L"cs_6_5");
	CheckReturn(mInitData.ShaderManager->AddShader(CS, mShaderHashes[Ssao::Shader::CS_SSAO]));

	return true;
}

bool D3D12Ssao::BuildRootSignatures() {
	decltype(auto) samplers = D3D12Util::GetStaticSamplers();

	CD3DX12_DESCRIPTOR_RANGE texTables[6]{}; UINT index = 0;
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2, 0);

	index = 0;

	CD3DX12_ROOT_PARAMETER slotRootParameter[Ssao::RootSignature::Default::Count]{};
	slotRootParameter[Ssao::RootSignature::Default::CB_AO].InitAsConstantBufferView(0);
	slotRootParameter[Ssao::RootSignature::Default::RC_Consts].InitAsConstants(
		Ssao::RootConstant::Default::Count, 1);
	slotRootParameter[Ssao::RootSignature::Default::SI_NormalDepthMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[Ssao::RootSignature::Default::SI_PositionMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[Ssao::RootSignature::Default::SI_RandomVectorMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[Ssao::RootSignature::Default::UO_AOCoefficientMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[Ssao::RootSignature::Default::UO_RayHitDistMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[Ssao::RootSignature::Default::UO_DebugMap].InitAsDescriptorTable(1, &texTables[index++]);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		D3D12Util::StaticSamplerCount, samplers,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	CheckReturn(D3D12Util::CreateRootSignature(
		mInitData.Device,
		rootSigDesc,
		IID_PPV_ARGS(&mRootSignature),
		L"SSAO_GR_DrawAO"));

	return true;
}

bool D3D12Ssao::BuildPipelineStates() {
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = mRootSignature.Get();
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	{
		const auto CS = mInitData.ShaderManager->GetShader(mShaderHashes[Ssao::Shader::CS_SSAO]);
		NullCheck(CS);
		psoDesc.CS = { reinterpret_cast<BYTE*>(CS->GetBufferPointer()), CS->GetBufferSize() };
	}

	CheckReturn(D3D12Util::CreateComputePipelineState(
		mInitData.Device,
		psoDesc,
		IID_PPV_ARGS(&mPipelineState),
		L"SSAO_CP_DrawAO"));

	return true;
}

bool D3D12Ssao::AllocateDescriptors() {
	// AO
	{
		// AOCoefficient
		mpDescHeap->AllocateCbvSrvUav(1, mhAOResourceDescs[Ssao::Descriptor::AO::ES_AOCoefficient]);
		mpDescHeap->AllocateCbvSrvUav(1, mhAOResourceDescs[Ssao::Descriptor::AO::EU_AOCoefficient]);
		// RayHitDistance
		mpDescHeap->AllocateCbvSrvUav(1, mhAOResourceDescs[Ssao::Descriptor::AO::ES_RayHitDistance]);
		mpDescHeap->AllocateCbvSrvUav(1, mhAOResourceDescs[Ssao::Descriptor::AO::EU_RayHitDistance]);
	}
	// TemporalCache
	for (UINT frame = 0; frame < 2; ++frame) {
		// TSPP
		mpDescHeap->AllocateCbvSrvUav(1, mhTemporalCacheDescs[frame][Ssao::Descriptor::TemporalCache::ES_TSPP]);
		mpDescHeap->AllocateCbvSrvUav(1, mhTemporalCacheDescs[frame][Ssao::Descriptor::TemporalCache::EU_TSPP]);

		// RayHitDistance
		mpDescHeap->AllocateCbvSrvUav(1, mhTemporalCacheDescs[frame][Ssao::Descriptor::TemporalCache::ES_RayHitDistance]);
		mpDescHeap->AllocateCbvSrvUav(1, mhTemporalCacheDescs[frame][Ssao::Descriptor::TemporalCache::EU_RayHitDistance]);

		// AOCoefficientSquaredMean
		mpDescHeap->AllocateCbvSrvUav(1, mhTemporalCacheDescs[frame][Ssao::Descriptor::TemporalCache::ES_AOCoefficientSquaredMean]);
		mpDescHeap->AllocateCbvSrvUav(1, mhTemporalCacheDescs[frame][Ssao::Descriptor::TemporalCache::EU_AOCoefficientSquaredMean]);

		// TemporalAOResource
		mpDescHeap->AllocateCbvSrvUav(1, mhTemporalAOResourceDescs[frame][Ssao::Descriptor::TemporalAO::E_Srv]);
		mpDescHeap->AllocateCbvSrvUav(1, mhTemporalAOResourceDescs[frame][Ssao::Descriptor::TemporalAO::E_Uav]);
	}

	CheckReturn(BuildDescriptors());

	return true;
}

bool D3D12Ssao::OnResize(unsigned width, unsigned height) {
	mInitData.Width = width;
	mInitData.Height = height;

	CheckReturn(BuildResources());
	CheckReturn(BuildDescriptors());

	return true;
}

bool D3D12Ssao::DrawAO(
	D3D12FrameResource* const pFrameResource
	, GpuResource* const pCurrNormalDepthMap
	, D3D12_GPU_DESCRIPTOR_HANDLE si_currNormalDepthMap
	, GpuResource* const pPositionMap
	, D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineState.Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetComputeRootSignature(mRootSignature.Get());

		CmdList->SetComputeRootConstantBufferView(
			Ssao::RootSignature::Default::CB_AO,
			pFrameResource->AmbientOcclusionCB.CBAddress());

		Ssao::RootConstant::Default::Struct rc{};
		rc.gInvTexDim.x = 1.f / static_cast<FLOAT>(mInitData.Width);
		rc.gInvTexDim.y = 1.f / static_cast<FLOAT>(mInitData.Height);

		D3D12Util::SetRoot32BitConstants<Ssao::RootConstant::Default::Struct>(
			Ssao::RootSignature::Default::RC_Consts,
			Ssao::RootConstant::Default::Count,
			&rc,
			0,
			CmdList,
			TRUE);

		const auto AOCoefficient = mAOResources[Ssao::Resource::AO::E_AOCoefficient].get();
		AOCoefficient->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Util::UavBarrier(CmdList, AOCoefficient);

		const auto RayHitDistance = mAOResources[Ssao::Resource::AO::E_RayHitDistance].get();
		RayHitDistance->Transite(CmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3D12Util::UavBarrier(CmdList, RayHitDistance);

		pCurrNormalDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pPositionMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->SetComputeRootDescriptorTable(
			Ssao::RootSignature::Default::SI_NormalDepthMap, si_currNormalDepthMap);
		CmdList->SetComputeRootDescriptorTable(
			Ssao::RootSignature::Default::SI_PositionMap, si_positionMap);

		auto aoUav = mpDescHeap->GetGpuHandle(mhAOResourceDescs[Ssao::Descriptor::AO::EU_AOCoefficient]);
		CmdList->SetComputeRootDescriptorTable(
			Ssao::RootSignature::Default::UO_AOCoefficientMap, aoUav);

		auto rayHitDistUav = mpDescHeap->GetGpuHandle(mhAOResourceDescs[Ssao::Descriptor::AO::EU_RayHitDistance]);
		CmdList->SetComputeRootDescriptorTable(
			Ssao::RootSignature::Default::UO_RayHitDistMap, rayHitDistUav);

		CmdList->Dispatch(
			D3D12Util::CeilDivide(mInitData.Width, Ssao::ThreadGroup::Default::Width),
			D3D12Util::CeilDivide(mInitData.Height, Ssao::ThreadGroup::Default::Height),
			Ssao::ThreadGroup::Default::Depth);
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}

UINT D3D12Ssao::MoveToNextTemporalCacheFrame() {
	mCurrentTemporalCacheFrameIndex = (mCurrentTemporalCacheFrameIndex + 1) % 2;
	return mCurrentTemporalCacheFrameIndex;
}

UINT D3D12Ssao::MoveToNextTemporalAOFrame() {
	mCurrentTemporalAOFrameIndex = (mCurrentTemporalAOFrameIndex + 1) % 2;
	return mCurrentTemporalAOFrameIndex;
}

bool D3D12Ssao::BuildResources() {
	D3D12_RESOURCE_DESC texDesc{};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Format = Ssao::AOCoefficientMapFormat;
	texDesc.Width = mInitData.Width;
	texDesc.Height = mInitData.Height;
	texDesc.Alignment = 0;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	const auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	// AO
	{
		// AOCoefficientMap
		{
			texDesc.Format = Ssao::AOCoefficientMapFormat;

			CheckReturn(mAOResources[Ssao::Resource::AO::E_AOCoefficient]->Initialize(
				mInitData.Device,
				&prop,
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				L"SSAO_AOCoefficientMap"));
		}
		// RayHitDistanceMap
		{
			texDesc.Format = Svgf::RayHitDistanceMapFormat;

			CheckReturn(mAOResources[Ssao::Resource::AO::E_RayHitDistance]->Initialize(
				mInitData.Device,
				&prop,
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				L"SSAO_RayHitDistanceMap"));
		}
	}
	for (UINT frame = 0; frame < 2; ++frame) {
		// TemporalCache
		{
			// TSPPMap
			{
				texDesc.Format = Svgf::TSPPMapFormat;

				auto name = std::format(L"SSAO_TSPPMap_{}", frame);

				CheckReturn(mTemporalCaches[frame][Ssao::Resource::TemporalCache::E_TSPP]->Initialize(
					mInitData.Device,
					&prop,
					D3D12_HEAP_FLAG_NONE,
					&texDesc,
					D3D12_RESOURCE_STATE_COMMON,
					nullptr,
					name.c_str()));
			}
			// TemporalRayHitDistanceMap
			{
				texDesc.Format = Svgf::RayHitDistanceMapFormat;

				auto name = std::format(L"SSAO_TemporalRayHitDistanceMap_{}", frame);

				CheckReturn(mTemporalCaches[frame][Ssao::Resource::TemporalCache::E_RayHitDistance]->Initialize(
					mInitData.Device,
					&prop,
					D3D12_HEAP_FLAG_NONE,
					&texDesc,
					D3D12_RESOURCE_STATE_COMMON,
					nullptr,
					name.c_str()));
			}
			// TemporalAOCoefficientSquaredMeanMap
			{
				texDesc.Format = Ssao::AOCoefficientSquaredMeanMapFormat;

				auto name = std::format(L"SSAO_TemporalAOCoefficientSquaredMeanMap_{}", frame);

				CheckReturn(mTemporalCaches[frame][Ssao::Resource::TemporalCache::E_AOCoefficientSquaredMean]->Initialize(
					mInitData.Device,
					&prop,
					D3D12_HEAP_FLAG_NONE,
					&texDesc,
					D3D12_RESOURCE_STATE_COMMON,
					nullptr,
					name.c_str()));
			}
		}
		// TemporalAOCoefficientMap
		{
			texDesc.Format = Ssao::AOCoefficientMapFormat;

			auto name = std::format(L"SSAO_TemporalAOCoefficientMap_{}", frame);

			CheckReturn(mTemporalAOResources[frame]->Initialize(
				mInitData.Device,
				&prop,
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				name.c_str()));
		}
	}

	return true;
}

bool D3D12Ssao::BuildDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	uavDesc.Texture2D.PlaneSlice = 0;

	// AO
	{
		// AOCoefficientMap
		{
			srvDesc.Format = Ssao::AOCoefficientMapFormat;
			uavDesc.Format = Ssao::AOCoefficientMapFormat;

			const auto resource = mAOResources[Ssao::Resource::AO::E_AOCoefficient]->Resource();

			const auto srv = mpDescHeap->GetCpuHandle(mhAOResourceDescs[Ssao::Descriptor::AO::ES_AOCoefficient]);
			const auto uav = mpDescHeap->GetCpuHandle(mhAOResourceDescs[Ssao::Descriptor::AO::EU_AOCoefficient]);

			D3D12Util::CreateShaderResourceView(mInitData.Device, resource, &srvDesc, srv);
			D3D12Util::CreateUnorderedAccessView(mInitData.Device, resource, nullptr, &uavDesc, uav);
		}
		// RayHitDistanceMap
		{
			srvDesc.Format = Svgf::RayHitDistanceMapFormat;
			uavDesc.Format = Svgf::RayHitDistanceMapFormat;

			const auto resource = mAOResources[Ssao::Resource::AO::E_RayHitDistance]->Resource();

			const auto srv = mpDescHeap->GetCpuHandle(mhAOResourceDescs[Ssao::Descriptor::AO::ES_RayHitDistance]);
			const auto uav = mpDescHeap->GetCpuHandle(mhAOResourceDescs[Ssao::Descriptor::AO::EU_RayHitDistance]);

			D3D12Util::CreateShaderResourceView(mInitData.Device, resource, &srvDesc, srv);
			D3D12Util::CreateUnorderedAccessView(mInitData.Device, resource, nullptr, &uavDesc, uav);
		}
	}
	for (UINT frame = 0; frame < 2; ++frame) {
		// TemporalCache
		{
			// TSSPMap
			{
				srvDesc.Format = Svgf::TSPPMapFormat;
				uavDesc.Format = Svgf::TSPPMapFormat;

				const auto resource = mTemporalCaches[frame][Ssao::Resource::TemporalCache::E_TSPP]->Resource();

				const auto srv = mpDescHeap->GetCpuHandle(mhTemporalCacheDescs[frame][Ssao::Descriptor::TemporalCache::ES_TSPP]);
				const auto uav = mpDescHeap->GetCpuHandle(mhTemporalCacheDescs[frame][Ssao::Descriptor::TemporalCache::EU_TSPP]);

				D3D12Util::CreateShaderResourceView(mInitData.Device, resource, &srvDesc, srv);
				D3D12Util::CreateUnorderedAccessView(mInitData.Device, resource, nullptr, &uavDesc, uav);
			}
			// RayHitDistanceMap
			{
				srvDesc.Format = Svgf::RayHitDistanceMapFormat;
				uavDesc.Format = Svgf::RayHitDistanceMapFormat;

				const auto resource = mTemporalCaches[frame][Ssao::Resource::TemporalCache::E_RayHitDistance]->Resource();

				const auto srv = mpDescHeap->GetCpuHandle(mhTemporalCacheDescs[frame][Ssao::Descriptor::TemporalCache::ES_RayHitDistance]);
				const auto uav = mpDescHeap->GetCpuHandle(mhTemporalCacheDescs[frame][Ssao::Descriptor::TemporalCache::EU_RayHitDistance]);

				D3D12Util::CreateShaderResourceView(mInitData.Device, resource, &srvDesc, srv);
				D3D12Util::CreateUnorderedAccessView(mInitData.Device, resource, nullptr, &uavDesc, uav);
			}
			// AOCoefficientSquaredMeanMap
			{
				srvDesc.Format = Ssao::AOCoefficientSquaredMeanMapFormat;
				uavDesc.Format = Ssao::AOCoefficientSquaredMeanMapFormat;

				const auto resource = mTemporalCaches[frame][Ssao::Resource::TemporalCache::E_AOCoefficientSquaredMean]->Resource();

				const auto srv = mpDescHeap->GetCpuHandle(mhTemporalCacheDescs[frame][Ssao::Descriptor::TemporalCache::ES_AOCoefficientSquaredMean]);
				const auto uav = mpDescHeap->GetCpuHandle(mhTemporalCacheDescs[frame][Ssao::Descriptor::TemporalCache::EU_AOCoefficientSquaredMean]);

				D3D12Util::CreateShaderResourceView(mInitData.Device, resource, &srvDesc, srv);
				D3D12Util::CreateUnorderedAccessView(mInitData.Device, resource, nullptr, &uavDesc, uav);
			}
		}
		// TemporalAOCoefficientMap
		{
			srvDesc.Format = Ssao::AOCoefficientMapFormat;
			uavDesc.Format = Ssao::AOCoefficientMapFormat;

			const auto resource = mTemporalAOResources[frame]->Resource();

			const auto srv = mpDescHeap->GetCpuHandle(mhTemporalAOResourceDescs[frame][Ssao::Descriptor::TemporalAO::E_Srv]);
			const auto uav = mpDescHeap->GetCpuHandle(mhTemporalAOResourceDescs[frame][Ssao::Descriptor::TemporalAO::E_Uav]);

			D3D12Util::CreateShaderResourceView(mInitData.Device, resource, &srvDesc, srv);
			D3D12Util::CreateUnorderedAccessView(mInitData.Device, resource, nullptr, &uavDesc, uav);
		}
	}

	return true;
}
