#include "pch.h"
#include "Renderer/D3D12/D3D12Rtao.hpp"

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
	const WCHAR* const HLSL_RTAO = L"D3D12Rtao.hlsl";

	const WCHAR* const RTAO_RayGenName = L"RTAO_RayGen";
	const WCHAR* const RTAO_RayGenRaySortedName = L"RTAO_RayGen_RaySorted";
	const WCHAR* const RTAO_ClosestHitName = L"RTAO_ClosestHit";
	const WCHAR* const RTAO_MissName = L"RTAO_Miss";
	const WCHAR* const RTAO_HitGroupName = L"RTAO_HitGroup";
}

using namespace DirectX;

D3D12Rtao::D3D12Rtao() {}

D3D12Rtao::~D3D12Rtao() {}

bool D3D12Rtao::Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	const auto initData = reinterpret_cast<InitData*>(pData);
	mInitData = *initData;

	for (UINT resource = 0; resource < Rtao::Resource::AO::Count; ++resource)
		mAOResources[resource] = std::make_unique<GpuResource>();

	for (UINT frame = 0; frame < 2; ++frame) {
		for (UINT resource = 0; resource < Rtao::Resource::TemporalCache::Count; ++resource) 
			mTemporalCaches[frame][resource] = std::make_unique<GpuResource>();

		mTemporalAOResources[frame] = std::make_unique<GpuResource>();
	}

	CheckReturn(BuildResources());

	return true;
}

bool D3D12Rtao::CompileShaders() {
	const auto Lib = D3D12ShaderManager::D3D12ShaderInfo(HLSL_RTAO, L"", L"lib_6_5");
	CheckReturn(mInitData.ShaderManager->AddShader(Lib, mShaderHashes[Rtao::Shader::Lib_RTAO]));

	return true;
}

bool D3D12Rtao::BuildRootSignatures() {
	decltype(auto) samplers = D3D12Util::GetStaticSamplers();

	CD3DX12_DESCRIPTOR_RANGE texTables[7]{}; UINT index = 0;
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2);

	index = 0;

	CD3DX12_ROOT_PARAMETER slotRootParameter[Rtao::RootSignature::Count]{};
	slotRootParameter[Rtao::RootSignature::SB_AccelerationStructure].InitAsShaderResourceView(0);
	slotRootParameter[Rtao::RootSignature::CB_AO].InitAsConstantBufferView(0);
	slotRootParameter[Rtao::RootSignature::SI_PositionMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[Rtao::RootSignature::SI_NormalDepthMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[Rtao::RootSignature::SI_RayDirectionOriginDepthMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[Rtao::RootSignature::SI_RayIndexOffsetMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[Rtao::RootSignature::UO_AOCoefficientMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[Rtao::RootSignature::UO_RayHitDistanceMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[Rtao::RootSignature::UO_DebugMap].InitAsDescriptorTable(1, &texTables[index++]);
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
		_countof(slotRootParameter), slotRootParameter,
		D3D12Util::StaticSamplerCount, samplers,
		D3D12_ROOT_SIGNATURE_FLAG_NONE
	);

	CheckReturn(D3D12Util::CreateRootSignature(
		mInitData.Device,
		rootSignatureDesc,
		IID_PPV_ARGS(&mRootSignature),
		L"RTAO_GR_Default"));

	return true;
}

bool D3D12Rtao::BuildPipelineStates() {
	CD3DX12_STATE_OBJECT_DESC rtaoStateObject = { D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

	// RTAO-Library
	const auto rtaoLib = rtaoStateObject.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	const auto rtaoShader = mInitData.ShaderManager->GetShader(mShaderHashes[Rtao::Shader::Lib_RTAO]);
	const D3D12_SHADER_BYTECODE rtaoLibDxil = CD3DX12_SHADER_BYTECODE(
		rtaoShader->GetBufferPointer(), rtaoShader->GetBufferSize());
	rtaoLib->SetDXILLibrary(&rtaoLibDxil);
	LPCWSTR rtaoExports[] = { 
		RTAO_RayGenName, RTAO_RayGenRaySortedName, RTAO_ClosestHitName, RTAO_MissName };
	rtaoLib->DefineExports(rtaoExports);

	// RTAO-HitGroup
	const auto rtaoHitGroup = rtaoStateObject.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
	rtaoHitGroup->SetClosestHitShaderImport(RTAO_ClosestHitName);
	rtaoHitGroup->SetHitGroupExport(RTAO_HitGroupName);
	rtaoHitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

	// ShaderConfig
	const auto shaderConfig = rtaoStateObject.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
	const UINT payloadSize = 4; // tHit(FLOAT)
	const UINT attribSize = sizeof(XMFLOAT2);
	shaderConfig->Config(payloadSize, attribSize);

	// Global-RootSignature
	const auto glbalRootSig = rtaoStateObject.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
	glbalRootSig->SetRootSignature(mRootSignature.Get());

	// Pipeline-Configuration
	const auto pipelineConfig = rtaoStateObject.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
	const UINT maxRecursionDepth = 1;
	pipelineConfig->Config(maxRecursionDepth);

	CheckReturn(D3D12Util::CreateStateObject(
		mInitData.Device, rtaoStateObject, IID_PPV_ARGS(&mStateObject)));
	CheckHResult(mStateObject->QueryInterface(IID_PPV_ARGS(&mStateObjectProp)));

	return true;
}

bool D3D12Rtao::AllocateDescriptors() {
	// AO
	{
		// AOCoefficient
		mpDescHeap->AllocateCbvSrvUav(1, mhAOResourceDescs[Rtao::Descriptor::AO::ES_AOCoefficient]);
		mpDescHeap->AllocateCbvSrvUav(1, mhAOResourceDescs[Rtao::Descriptor::AO::EU_AOCoefficient]);
		// RayHitDistance
		mpDescHeap->AllocateCbvSrvUav(1, mhAOResourceDescs[Rtao::Descriptor::AO::ES_RayHitDistance]);
		mpDescHeap->AllocateCbvSrvUav(1, mhAOResourceDescs[Rtao::Descriptor::AO::EU_RayHitDistance]);
	}
	// TemporalCache
	for (UINT frame = 0; frame < 2; ++frame) {
		// TSPP
		mpDescHeap->AllocateCbvSrvUav(1, mhTemporalCacheDescs[frame][Rtao::Descriptor::TemporalCache::ES_TSPP]);
		mpDescHeap->AllocateCbvSrvUav(1, mhTemporalCacheDescs[frame][Rtao::Descriptor::TemporalCache::EU_TSPP]);
		// RayHitDistance
		mpDescHeap->AllocateCbvSrvUav(1, mhTemporalCacheDescs[frame][Rtao::Descriptor::TemporalCache::ES_RayHitDistance]);
		mpDescHeap->AllocateCbvSrvUav(1, mhTemporalCacheDescs[frame][Rtao::Descriptor::TemporalCache::EU_RayHitDistance]);
		// AOCoefficientSquaredMean
		mpDescHeap->AllocateCbvSrvUav(1, mhTemporalCacheDescs[frame][Rtao::Descriptor::TemporalCache::ES_AOCoefficientSquaredMean]);
		mpDescHeap->AllocateCbvSrvUav(1, mhTemporalCacheDescs[frame][Rtao::Descriptor::TemporalCache::EU_AOCoefficientSquaredMean]);
		// TemporalAOResource
		mpDescHeap->AllocateCbvSrvUav(1, mhTemporalAOResourceDescs[frame][Rtao::Descriptor::TemporalAO::E_Srv]);
		mpDescHeap->AllocateCbvSrvUav(1, mhTemporalAOResourceDescs[frame][Rtao::Descriptor::TemporalAO::E_Uav]);
	}

	CheckReturn(BuildDescriptors());

	return true;
}

bool D3D12Rtao::OnResize(unsigned width, unsigned height) {
	mInitData.Width = width;
	mInitData.Height = height;

	CheckReturn(BuildResources());
	CheckReturn(BuildDescriptors());

	return true;
}

bool D3D12Rtao::BuildShaderTables(UINT numRitems) {
#ifdef _DEBUG
	// A shader name look-up table for shader table debug print out.
	std::unordered_map<void*, std::wstring> shaderIdToStringMap;
#endif

	const UINT ShaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

	{
		// RayGenShaderTable
		{
			void* const rayGenShaderIdentifier = mStateObjectProp->GetShaderIdentifier(RTAO_RayGenName);

			ShaderTable rayGenShaderTable(mInitData.Device, 2, ShaderIdentifierSize);
			CheckReturn(rayGenShaderTable.Initialze());
			rayGenShaderTable.push_back(ShaderRecord(rayGenShaderIdentifier, ShaderIdentifierSize));

#ifdef _DEBUG
			shaderIdToStringMap[rayGenShaderIdentifier] = RTAO_RayGenName;

			WLogln(L"RTAO - Ray Gen");
			rayGenShaderTable.DebugPrint(shaderIdToStringMap);
			WLogln(L"");
#endif

			mShaderTables[Rtao::ShaderTable::E_RayGenShader] = rayGenShaderTable.GetResource();
		}
		// SortedRayGenShaderTable
		{
			void* const rayGenRaySortedShaderIdentifier = mStateObjectProp->GetShaderIdentifier(RTAO_RayGenRaySortedName);

			ShaderTable rayGenShaderTable(mInitData.Device, 2, ShaderIdentifierSize);
			CheckReturn(rayGenShaderTable.Initialze());
			rayGenShaderTable.push_back(ShaderRecord(rayGenRaySortedShaderIdentifier, ShaderIdentifierSize));

#ifdef _DEBUG
			shaderIdToStringMap[rayGenRaySortedShaderIdentifier] = RTAO_RayGenRaySortedName;

			WLogln(L"RTAO - Sorted Ray Gen");
			rayGenShaderTable.DebugPrint(shaderIdToStringMap);
			WLogln(L"");
#endif

			mShaderTables[Rtao::ShaderTable::E_SortedRayGenShader] = rayGenShaderTable.GetResource();
		}
		// MissShaderTable
		{
			void* const missShaderIdentifier = mStateObjectProp->GetShaderIdentifier(RTAO_MissName);

			ShaderTable missShaderTable(mInitData.Device, 1, ShaderIdentifierSize);
			CheckReturn(missShaderTable.Initialze());
			missShaderTable.push_back(ShaderRecord(missShaderIdentifier, ShaderIdentifierSize));

#ifdef _DEBUG
			shaderIdToStringMap[missShaderIdentifier] = RTAO_MissName;

			WLogln(L"RTAO - Miss");
			missShaderTable.DebugPrint(shaderIdToStringMap);
			WLogln(L"");
#endif

			mShaderTables[Rtao::ShaderTable::E_MissShader] = missShaderTable.GetResource();
		}
		// HitGroupShaderTable
		{
			void* const hitGroupShaderIdentifier = mStateObjectProp->GetShaderIdentifier(RTAO_HitGroupName);

			ShaderTable hitGroupTable(mInitData.Device, numRitems, ShaderIdentifierSize);
			CheckReturn(hitGroupTable.Initialze());

			for (UINT i = 0; i < numRitems; ++i)
				hitGroupTable.push_back(ShaderRecord(hitGroupShaderIdentifier, ShaderIdentifierSize));
#ifdef _DEBUG
			shaderIdToStringMap[hitGroupShaderIdentifier] = RTAO_HitGroupName;

			WLogln(L"RTAO - Hit Group");
			hitGroupTable.DebugPrint(shaderIdToStringMap);
			WLogln(L"");
#endif

			mShaderTables[Rtao::ShaderTable::E_HitGroupShader] = hitGroupTable.GetResource();
			mHitGroupShaderTableStrideInBytes = hitGroupTable.GetShaderRecordSize();
		}
	}

	return true;
}

bool D3D12Rtao::DrawAO(
	D3D12FrameResource* const pFrameResource
	, D3D12_GPU_VIRTUAL_ADDRESS accelStruct
	, GpuResource* const pPositionMap
	, D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap
	, GpuResource* const pNormalDepthMap
	, D3D12_GPU_DESCRIPTOR_HANDLE si_normalDepthMap
	, GpuResource* const pRayDirectionOriginDepthMap
	, D3D12_GPU_DESCRIPTOR_HANDLE si_rayDirectionOriginDepthMap
	, GpuResource* const pRayInexOffsetMap
	, D3D12_GPU_DESCRIPTOR_HANDLE si_rayIndexOffsetMap
	, BOOL bRaySortingEnabled, BOOL bCheckboardRayGeneration) {
	return true;
}

UINT D3D12Rtao::MoveToNextTemporalCacheFrame() {
	mCurrentTemporalCacheFrameIndex = (mCurrentTemporalCacheFrameIndex + 1) % 2;
	return mCurrentTemporalCacheFrameIndex;
}

UINT D3D12Rtao::MoveToNextTemporalAOFrame() {
	mCurrentTemporalAOFrameIndex = (mCurrentTemporalAOFrameIndex + 1) % 2;
	return mCurrentTemporalAOFrameIndex;
}

bool D3D12Rtao::BuildResources() {
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

	// AO
	{
		// AOCoefficientMap
		{
			texDesc.Format = Rtao::AOCoefficientMapFormat;

			CheckReturn(mAOResources[Rtao::Resource::AO::E_AOCoefficient]->Initialize(
				mInitData.Device,
				&prop,
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				L"RTAO_AOCoefficientMap"));
		}
		// RayHitDistanceMap
		{
			texDesc.Format = Rtao::RayHitDistanceMapFormat;

			CheckReturn(mAOResources[Rtao::Resource::AO::E_RayHitDistance]->Initialize(
				mInitData.Device,
				&prop,
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				L"RTAO_RayHitDistanceMap"));
		}
	}
	for (UINT frame = 0; frame < 2; ++frame) {
		// TemporalCache
		{
			// TSPPMap
			{
				texDesc.Format = Rtao::TSPPMapFormat;

				auto name = std::format(L"RTAO_TSPPMap_{}", frame);

				CheckReturn(mTemporalCaches[frame][Rtao::Resource::TemporalCache::E_TSPP]->Initialize(
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
				texDesc.Format = Rtao::RayHitDistanceMapFormat;

				auto name = std::format(L"RTAO_TemporalRayHitDistanceMap_{}", frame);

				CheckReturn(mTemporalCaches[frame][Rtao::Resource::TemporalCache::E_RayHitDistance]->Initialize(
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
				texDesc.Format = Rtao::AOCoefficientSquaredMeanMapFormat;

				auto name = std::format(L"RTAO_TemporalAOCoefficientSquaredMeanMap_{}", frame);

				CheckReturn(mTemporalCaches[frame][Rtao::Resource::TemporalCache::E_AOCoefficientSquaredMean]->Initialize(
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
			texDesc.Format = Rtao::AOCoefficientMapFormat;

			auto name = std::format(L"RTAO_TemporalAOCoefficientMap_{}", frame);

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

bool D3D12Rtao::BuildDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.f;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	uavDesc.Texture2D.PlaneSlice = 0;

	// AO
	{
		// AOCoefficientMap
		{
			srvDesc.Format = Rtao::AOCoefficientMapFormat;
			uavDesc.Format = Rtao::AOCoefficientMapFormat;

			const auto resource = mAOResources[Rtao::Resource::AO::E_AOCoefficient]->Resource();

			const auto srv = mpDescHeap->GetCpuHandle(mhAOResourceDescs[Rtao::Descriptor::AO::ES_AOCoefficient]);
			const auto uav = mpDescHeap->GetCpuHandle(mhAOResourceDescs[Rtao::Descriptor::AO::EU_AOCoefficient]);

			D3D12Util::CreateShaderResourceView(
				mInitData.Device, resource, &srvDesc, srv);
			D3D12Util::CreateUnorderedAccessView(
				mInitData.Device, resource, nullptr, &uavDesc, uav);
		}
		// RayHitDistanceMap
		{
			srvDesc.Format = Rtao::RayHitDistanceMapFormat;
			uavDesc.Format = Rtao::RayHitDistanceMapFormat;

			const auto resource = mAOResources[Rtao::Resource::AO::E_RayHitDistance]->Resource();

			const auto srv = mpDescHeap->GetCpuHandle(mhAOResourceDescs[Rtao::Descriptor::AO::ES_RayHitDistance]);
			const auto uav = mpDescHeap->GetCpuHandle(mhAOResourceDescs[Rtao::Descriptor::AO::EU_RayHitDistance]);

			D3D12Util::CreateShaderResourceView(mInitData.Device, resource, &srvDesc, srv);
			D3D12Util::CreateUnorderedAccessView(mInitData.Device, resource, nullptr, &uavDesc, uav);
		}
	}
	for (UINT frame = 0; frame < 2; ++frame) {
		// TemporalCache
		{
			// TSSPMap
			{
				srvDesc.Format = Rtao::TSPPMapFormat;
				uavDesc.Format = Rtao::TSPPMapFormat;

				const auto resource = mTemporalCaches[frame][Rtao::Resource::TemporalCache::E_TSPP]->Resource();

				const auto srv = mpDescHeap->GetCpuHandle(mhTemporalCacheDescs[frame][Rtao::Descriptor::TemporalCache::ES_TSPP]);
				const auto uav = mpDescHeap->GetCpuHandle(mhTemporalCacheDescs[frame][Rtao::Descriptor::TemporalCache::EU_TSPP]);

				D3D12Util::CreateShaderResourceView(mInitData.Device, resource, &srvDesc, srv);
				D3D12Util::CreateUnorderedAccessView(mInitData.Device, resource, nullptr, &uavDesc, uav);
			}
			// RayHitDistanceMap
			{
				srvDesc.Format = Rtao::RayHitDistanceMapFormat;
				uavDesc.Format = Rtao::RayHitDistanceMapFormat;

				const auto resource = mTemporalCaches[frame][Rtao::Resource::TemporalCache::E_RayHitDistance]->Resource();

				const auto srv = mpDescHeap->GetCpuHandle(mhTemporalCacheDescs[frame][Rtao::Descriptor::TemporalCache::ES_RayHitDistance]);
				const auto uav = mpDescHeap->GetCpuHandle(mhTemporalCacheDescs[frame][Rtao::Descriptor::TemporalCache::EU_RayHitDistance]);

				D3D12Util::CreateShaderResourceView(mInitData.Device, resource, &srvDesc, srv);
				D3D12Util::CreateUnorderedAccessView(mInitData.Device, resource, nullptr, &uavDesc, uav);
			}
			// AOCoefficientSquaredMeanMap
			{
				srvDesc.Format = Rtao::AOCoefficientSquaredMeanMapFormat;
				uavDesc.Format = Rtao::AOCoefficientSquaredMeanMapFormat;

				const auto resource = mTemporalCaches[frame][Rtao::Resource::TemporalCache::E_AOCoefficientSquaredMean]->Resource();

				const auto srv = mpDescHeap->GetCpuHandle(mhTemporalCacheDescs[frame][Rtao::Descriptor::TemporalCache::ES_AOCoefficientSquaredMean]);
				const auto uav = mpDescHeap->GetCpuHandle(mhTemporalCacheDescs[frame][Rtao::Descriptor::TemporalCache::EU_AOCoefficientSquaredMean]);

				D3D12Util::CreateShaderResourceView(mInitData.Device, resource, &srvDesc, srv);
				D3D12Util::CreateUnorderedAccessView(mInitData.Device, resource, nullptr, &uavDesc, uav);
			}
		}
		// TemporalAOCoefficientMap
		{
			srvDesc.Format = Rtao::AOCoefficientMapFormat;
			uavDesc.Format = Rtao::AOCoefficientMapFormat;

			const auto resource = mTemporalAOResources[frame]->Resource();

			const auto srv = mpDescHeap->GetCpuHandle(mhTemporalAOResourceDescs[frame][Rtao::Descriptor::TemporalAO::E_Srv]);
			const auto uav = mpDescHeap->GetCpuHandle(mhTemporalAOResourceDescs[frame][Rtao::Descriptor::TemporalAO::E_Uav]);

			D3D12Util::CreateShaderResourceView(mInitData.Device, resource, &srvDesc, srv);
			D3D12Util::CreateUnorderedAccessView(mInitData.Device, resource, nullptr, &uavDesc, uav);
		}
	}

	return true;
}
