#include "Renderer/D3D12/pch.h"
#include "Renderer/D3D12/D3D12Ssr.hpp"

#include "Renderer/Interface/ShaderArgumentManager.hpp"

#include "Renderer/D3D12/D3D12Device.hpp"
#include "Renderer/D3D12/D3D12Util.hpp"
#include "Renderer/D3D12/D3D12ShaderManager.hpp"
#include "Renderer/D3D12/D3D12FrameResource.hpp"
#include "Renderer/D3D12/D3D12CommandObject.hpp"
#include "Renderer/D3D12/D3D12RenderItem.hpp"
#include "Renderer/D3D12/D3D12GpuResource.hpp"

namespace {
	const WCHAR* const HLSL_SSR = L"D3D12Ssr.hlsl";
}

D3D12Ssr::D3D12Ssr() {}

D3D12Ssr::~D3D12Ssr() {}

bool D3D12Ssr::Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	mInitData = *reinterpret_cast<InitData*>(pData);

	for (size_t i = 0; i < Ssr::Resource::Count; ++i)
		mResources[i] = std::make_unique<GpuResource>();

	CheckReturn(BuildResources());

	return true;
}

bool D3D12Ssr::CompileShaders() { 
	const auto VS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_SSR, L"VS", L"vs_6_5");
	const auto MS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_SSR, L"MS", L"ms_6_5");
	const auto PS_View = D3D12ShaderManager::D3D12ShaderInfo(HLSL_SSR, L"PS_View", L"ps_6_5");
	const auto PS_Screen = D3D12ShaderManager::D3D12ShaderInfo(HLSL_SSR, L"PS_Screen", L"ps_6_5");
	CheckReturn(mInitData.ShaderManager->AddShader(VS, mShaderHashes[Ssr::Shader::VS_Ssr]));
	CheckReturn(mInitData.ShaderManager->AddShader(MS, mShaderHashes[Ssr::Shader::MS_Ssr]));
	CheckReturn(mInitData.ShaderManager->AddShader(PS_View, mShaderHashes[Ssr::Shader::PS_Ssr_View]));
	CheckReturn(mInitData.ShaderManager->AddShader(PS_Screen, mShaderHashes[Ssr::Shader::PS_Ssr_Screen]));

	return true; 
}

bool D3D12Ssr::BuildRootSignatures() { 
	decltype(auto) samplers = D3D12Util::GetStaticSamplers();

	CD3DX12_DESCRIPTOR_RANGE texTables[5]{}; UINT index = 0;
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4, 0);

	index = 0;

	CD3DX12_ROOT_PARAMETER slotRootParameter[Ssr::RootSignature::Default::Count]{};
	slotRootParameter[Ssr::RootSignature::Default::CB_Pass].InitAsConstantBufferView(0);
	slotRootParameter[Ssr::RootSignature::Default::CB_Ssr].InitAsConstantBufferView(1);
	slotRootParameter[Ssr::RootSignature::Default::SI_BackBuffer].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[Ssr::RootSignature::Default::SI_PositionMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[Ssr::RootSignature::Default::SI_NormalMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[Ssr::RootSignature::Default::SI_DepthMap].InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[Ssr::RootSignature::Default::SI_RMSMap].InitAsDescriptorTable(1, &texTables[index++]);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		D3D12Util::StaticSamplerCount, samplers,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	CheckReturn(D3D12Util::CreateRootSignature(
		mInitData.Device,
		rootSigDesc,
		IID_PPV_ARGS(&mRootSignature),
		L"SSR_GR_Default"));

	return true;
}

bool D3D12Ssr::BuildPipelineStates() { 
	if (mInitData.Device->IsMeshShaderSupported()) {
		auto psoDesc = D3D12Util::FitToScreenMeshPsoDesc();
		psoDesc.pRootSignature = mRootSignature.Get();
		{
			const auto MS = mInitData.ShaderManager->GetShader(mShaderHashes[Ssr::Shader::MS_Ssr]);
			NullCheck(MS);
			psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
		}
		psoDesc.RTVFormats[0] = HDR_FORMAT;

		// 뷰 공간 픽셀 셰이더
		{
			const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Ssr::Shader::PS_Ssr_View]);
			NullCheck(PS);
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };

			CheckReturn(D3D12Util::CreatePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[Ssr::PipelineState::MP_Ssr_View]),
				L"SSR_MP_View"));
		}
		// 화면 공간 픽셀 셰이더
		{
			const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Ssr::Shader::PS_Ssr_Screen]);
			NullCheck(PS);
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };

			CheckReturn(D3D12Util::CreatePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[Ssr::PipelineState::MP_Ssr_Screen]),
				L"SSR_MP_Screen"));
		}
	}
	else {
		auto psoDesc = D3D12Util::FitToScreenPsoDesc();
		psoDesc.pRootSignature = mRootSignature.Get();
		{
			const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[Ssr::Shader::VS_Ssr]);
			NullCheck(VS);
			psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
		}
		psoDesc.RTVFormats[0] = HDR_FORMAT;

		// 뷰 공간 픽셀 셰이더
		{
			const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Ssr::Shader::PS_Ssr_View]);
			NullCheck(PS);
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };

			CheckReturn(D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[Ssr::PipelineState::GP_Ssr_View]),
				L"SSR_GP_View"));
		}
		// 화면 공간 픽셀 셰이더
		{
			const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Ssr::Shader::PS_Ssr_Screen]);
			NullCheck(PS);
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };

			CheckReturn(D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[Ssr::PipelineState::GP_Ssr_Screen]),
				L"SSR_GP_Screen"));
		}
	}

	return true;
}

bool D3D12Ssr::AllocateDescriptors() { 
	for (size_t i = 0; i < Ssr::Descriptor::Rtv::Count; ++i)
		CheckReturn(mpDescHeap->AllocateRtv(1, mhRtvs[i]));

	for (size_t i = 0; i < Ssr::Descriptor::Uav::Count; ++i)
		CheckReturn(mpDescHeap->AllocateCbvSrvUav(1, mhUavs[i]));

	for (size_t i = 0; i < Ssr::Descriptor::Srv::Count; ++i)
		CheckReturn(mpDescHeap->AllocateCbvSrvUav(1, mhSrvs[i]));

	CheckReturn(BuildDescriptors());

	return true;
}

bool D3D12Ssr::OnResize(unsigned width, unsigned height) { 
	mInitData.Width = width;
	mInitData.Height = height;

	CheckReturn(BuildResources());
	CheckReturn(BuildDescriptors());

	return true;
}

bool D3D12Ssr::Run(
	D3D12FrameResource* const pFrameResource
	, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect
	, GpuResource* const pBackBuffer, D3D12_GPU_DESCRIPTOR_HANDLE si_backBuffer
	, GpuResource* const pDepthMap, D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap
	, GpuResource* const pNormalMap, D3D12_GPU_DESCRIPTOR_HANDLE si_normalMap
	, GpuResource* const pRMSMap, D3D12_GPU_DESCRIPTOR_HANDLE si_rmsMap
	, GpuResource* const pPositionMap, D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap
	, Ssr::Space::Type space) {

	Ssr::PipelineState::Type StateType;
	if (mInitData.Device->IsMeshShaderSupported()) {
		if (space == Ssr::Space::E_View)
			StateType = Ssr::PipelineState::MP_Ssr_View;
		else
			StateType = Ssr::PipelineState::MP_Ssr_Screen;
	}
	else {
		if (space == Ssr::Space::E_View)
			StateType = Ssr::PipelineState::GP_Ssr_View;
		else
			StateType = Ssr::PipelineState::GP_Ssr_Screen;
	}

	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[StateType].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetPipelineState(mPipelineStates[StateType].Get());
		CmdList->SetGraphicsRootSignature(mRootSignature.Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		const auto ssrMap = mResources[Ssr::Resource::E_ReflectionMap0].get();

		ssrMap->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pNormalMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pDepthMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pRMSMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		pPositionMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		const auto rtv = mpDescHeap->GetCpuHandle(mhRtvs[Ssr::Descriptor::Rtv::E_ReflectionMap0]);
		CmdList->ClearRenderTargetView(rtv, Ssr::ReflectionMapClearValues, 0, nullptr);
		CmdList->OMSetRenderTargets(1, &rtv, TRUE, nullptr);

		CmdList->SetGraphicsRootConstantBufferView(
			Ssr::RootSignature::Default::CB_Pass, 
			pFrameResource->PassCB.CBAddress());

		CmdList->SetGraphicsRootConstantBufferView(
			Ssr::RootSignature::Default::CB_Ssr,
			pFrameResource->ScreenSpaceReflectionCB.CBAddress());

		CmdList->SetGraphicsRootDescriptorTable(Ssr::RootSignature::Default::SI_BackBuffer, si_backBuffer);
		CmdList->SetGraphicsRootDescriptorTable(Ssr::RootSignature::Default::SI_PositionMap, si_positionMap);
		CmdList->SetGraphicsRootDescriptorTable(Ssr::RootSignature::Default::SI_NormalMap, si_normalMap);
		CmdList->SetGraphicsRootDescriptorTable(Ssr::RootSignature::Default::SI_DepthMap, si_depthMap);
		CmdList->SetGraphicsRootDescriptorTable(Ssr::RootSignature::Default::SI_RMSMap, si_rmsMap);

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

bool D3D12Ssr::BuildResources() { 
	D3D12_RESOURCE_DESC rscDesc{};
	rscDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rscDesc.Alignment = 0;
	rscDesc.Width = mInitData.Width;
	rscDesc.Height = mInitData.Height;
	rscDesc.DepthOrArraySize = 1;
	rscDesc.MipLevels = 1;
	rscDesc.SampleDesc.Count = 1;
	rscDesc.SampleDesc.Quality = 0;
	rscDesc.Format = Ssr::ReflectionMapFormat;
	rscDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	const auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	
	const CD3DX12_CLEAR_VALUE ReflectionMapOptClear(
		Ssr::ReflectionMapFormat,
		Ssr::ReflectionMapClearValues);

	for (size_t i = 0; i < Ssr::Resource::Count; ++i) {
		CheckReturn(mResources[i]->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&ReflectionMapOptClear,
			std::format(L"SSR_ReflectionMap_{}", i).c_str()));
	}

	return true;
}

bool D3D12Ssr::BuildDescriptors() { 
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.f;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Format = Ssr::ReflectionMapFormat;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	uavDesc.Texture2D.PlaneSlice = 0;
	uavDesc.Format = Ssr::ReflectionMapFormat;

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;
	rtvDesc.Format = Ssr::ReflectionMapFormat;

	for (size_t i = 0; i < Ssr::Resource::Count; ++i) {
		mInitData.Device->CreateShaderResourceView(
			mResources[i]->Resource(),
			&srvDesc,
			mpDescHeap->GetCpuHandle(mhSrvs[i]));

		mInitData.Device->CreateUnorderedAccessView(
			mResources[i]->Resource(),
			nullptr,
			&uavDesc,
			mpDescHeap->GetCpuHandle(mhUavs[i]));

		mInitData.Device->CreateRenderTargetView(
			mResources[i]->Resource(),
			&rtvDesc,
			mpDescHeap->GetCpuHandle(mhRtvs[i]));
	}

	return true;
}