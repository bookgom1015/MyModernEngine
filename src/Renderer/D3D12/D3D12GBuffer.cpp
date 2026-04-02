#include "pch.h"
#include "Renderer/D3D12/D3D12GBuffer.hpp"

#include "Renderer/D3D12/D3D12Device.hpp"
#include "Renderer/D3D12/D3D12Util.hpp"
#include "Renderer/D3D12/D3D12ShaderManager.hpp"
#include "Renderer/D3D12/D3D12FrameResource.hpp"
#include "Renderer/D3D12/D3D12CommandObject.hpp"
#include "Renderer/D3D12/D3D12RenderItem.hpp"
#include "Renderer/D3D12/D3D12MeshData.hpp"
#include "Renderer/D3D12/D3D12MaterialData.h"
#include "Renderer/D3D12/D3D12GpuResource.hpp"

#include "Renderer/D3D12/D3D12Texture.hpp"

namespace {
	const WCHAR* const HLSL_GBuffer = L"D3D12GBuffer.hlsl";
}

D3D12GBuffer::D3D12GBuffer() {}

D3D12GBuffer::~D3D12GBuffer() {}

bool D3D12GBuffer::Initialize(
	D3D12DescriptorHeap* const pDescHeap
	, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	mInitData = *reinterpret_cast<InitData*>(pData);

	for (size_t i = 0; i < GBuffer::Resource::Count; ++i)
		mResources[i] = std::make_unique<GpuResource>();

	CheckReturn(BuildResources());
	
	return true;
}

bool D3D12GBuffer::CompileShaders() {
	const auto VS_Static = D3D12ShaderManager::D3D12ShaderInfo(HLSL_GBuffer, L"VS_Static", L"vs_6_5");
	const auto VS_Skinned = D3D12ShaderManager::D3D12ShaderInfo(HLSL_GBuffer, L"VS_Skinned", L"vs_6_5");
	const auto MS_Static = D3D12ShaderManager::D3D12ShaderInfo(HLSL_GBuffer, L"MS_Static", L"ms_6_5");
	const auto MS_Skinned = D3D12ShaderManager::D3D12ShaderInfo(HLSL_GBuffer, L"MS_Skinned", L"ms_6_5");
	const auto PS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_GBuffer, L"PS", L"ps_6_5");
	CheckReturn(mInitData.ShaderManager->AddShader(
		VS_Static, mShaderHashes[GBuffer::Shader::VS_GBuffer_Static]));
	CheckReturn(mInitData.ShaderManager->AddShader(
		VS_Skinned, mShaderHashes[GBuffer::Shader::VS_GBuffer_Skinned]));
	CheckReturn(mInitData.ShaderManager->AddShader(
		MS_Static, mShaderHashes[GBuffer::Shader::MS_GBuffer_Static]));
	CheckReturn(mInitData.ShaderManager->AddShader(
		MS_Skinned, mShaderHashes[GBuffer::Shader::MS_GBuffer_Skinned]));
	CheckReturn(mInitData.ShaderManager->AddShader(
		PS, mShaderHashes[GBuffer::Shader::PS_GBuffer]));

	return true;
}

bool D3D12GBuffer::BuildRootSignatures() {
	decltype(auto) samplers = D3D12Util::GetStaticSamplers();

	CD3DX12_DESCRIPTOR_RANGE texTables[2]{}; UINT index = 0;
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 1);

	index = 0;

	CD3DX12_ROOT_PARAMETER slotRootParameter[GBuffer::RootSignature::Default::Count]{};
	slotRootParameter[GBuffer::RootSignature::Default::CB_Pass]
		.InitAsConstantBufferView(0);
	slotRootParameter[GBuffer::RootSignature::Default::CB_Object]
		.InitAsConstantBufferView(1);
	slotRootParameter[GBuffer::RootSignature::Default::CB_Material]
		.InitAsConstantBufferView(2);
	slotRootParameter[GBuffer::RootSignature::Default::RC_Consts]
		.InitAsConstants(GBuffer::RootConstant::Default::Count, 3);
	slotRootParameter[GBuffer::RootSignature::Default::SB_StaticVertexBuffer]
		.InitAsShaderResourceView(0);
	slotRootParameter[GBuffer::RootSignature::Default::SB_SkinnedVertexBuffer]
		.InitAsShaderResourceView(1);
	slotRootParameter[GBuffer::RootSignature::Default::SB_IndexBuffer]
		.InitAsShaderResourceView(2);
	slotRootParameter[GBuffer::RootSignature::Default::SB_BonePalette]
		.InitAsShaderResourceView(3);
	slotRootParameter[GBuffer::RootSignature::Default::SI_Textures_AlbedoMap]
		.InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[GBuffer::RootSignature::Default::SI_Textures_NormalMap]
		.InitAsDescriptorTable(1, &texTables[index++]);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		D3D12Util::StaticSamplerCount, samplers,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	CheckReturn(D3D12Util::CreateRootSignature(
		mInitData.Device,
		rootSigDesc,
		IID_PPV_ARGS(&mRootSignature),
		L"GBuffer_GR_Default"));

	return true;
}

bool D3D12GBuffer::BuildPipelineStates() {
	if (mInitData.Device->IsMeshShaderSupported()) {
		auto psoDesc = D3D12Util::DefaultMeshPsoDesc(DepthStencilBuffer::DepthStencilBufferFormat);
		psoDesc.pRootSignature = mRootSignature.Get();
		{
			const auto PS = mInitData.ShaderManager->GetShader(
				mShaderHashes[GBuffer::Shader::PS_GBuffer]);
			NullCheck(PS);
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.NumRenderTargets = 7;
		psoDesc.RTVFormats[1] = GBuffer::NormalMapFormat;
		psoDesc.RTVFormats[0] = GBuffer::AlbedoMapFormat;
		psoDesc.RTVFormats[2] = GBuffer::NormalDepthMapFormat;
		psoDesc.RTVFormats[3] = GBuffer::NormalDepthMapFormat;
		psoDesc.RTVFormats[4] = GBuffer::RMSMapFormat;
		psoDesc.RTVFormats[5] = GBuffer::VelocityMapFormat;
		psoDesc.RTVFormats[6] = GBuffer::PositionMapFormat;

		{
			const auto MS = mInitData.ShaderManager->GetShader(
				mShaderHashes[GBuffer::Shader::MS_GBuffer_Static]);
			NullCheck(MS);
			psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
		
			CheckReturn(D3D12Util::CreatePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[GBuffer::PipelineState::MP_GBuffer_Static]),
				L"GBuffer_MP_Static"));
		}
		{
			const auto MS = mInitData.ShaderManager->GetShader(
				mShaderHashes[GBuffer::Shader::MS_GBuffer_Skinned]);
			NullCheck(MS);
			psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
		
			CheckReturn(D3D12Util::CreatePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[GBuffer::PipelineState::MP_GBuffer_Skinned]),
				L"GBuffer_MP_Skinned"));
		}
	}
	else {
		auto psoDesc = D3D12Util::DefaultPsoDesc({}, DepthStencilBuffer::DepthStencilBufferFormat);
		psoDesc.pRootSignature = mRootSignature.Get();
		{
			const auto PS = mInitData.ShaderManager->GetShader(
				mShaderHashes[GBuffer::Shader::PS_GBuffer]);
			NullCheck(PS);
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.NumRenderTargets = 7;
		psoDesc.RTVFormats[0] = GBuffer::AlbedoMapFormat;
		psoDesc.RTVFormats[1] = GBuffer::NormalMapFormat;
		psoDesc.RTVFormats[2] = GBuffer::NormalDepthMapFormat;
		psoDesc.RTVFormats[3] = GBuffer::NormalDepthMapFormat;
		psoDesc.RTVFormats[4] = GBuffer::RMSMapFormat;
		psoDesc.RTVFormats[5] = GBuffer::VelocityMapFormat;
		psoDesc.RTVFormats[6] = GBuffer::PositionMapFormat;

		{
			psoDesc.InputLayout = D3D12Util::StaticVertexInputLayoutDesc();

			const auto VS = mInitData.ShaderManager->GetShader(
				mShaderHashes[GBuffer::Shader::VS_GBuffer_Static]);
			NullCheck(VS);
			psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };

			CheckReturn(D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[GBuffer::PipelineState::GP_GBuffer_Static]),
				L"GBuffer_GP_Static"));
		}
		{
			psoDesc.InputLayout = D3D12Util::SkinnedVertexInputLayoutDesc();;

			const auto VS = mInitData.ShaderManager->GetShader(
				mShaderHashes[GBuffer::Shader::VS_GBuffer_Skinned]);
			NullCheck(VS);
			psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };

			CheckReturn(D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[GBuffer::PipelineState::GP_GBuffer_Skinned]),
				L"GBuffer_GP_Skinned"));
		}
	}

	return true;
}

bool D3D12GBuffer::AllocateDescriptors() {
	for (size_t i = 0; i < GBuffer::Descriptor::Rtv::Count; ++i) 
		CheckReturn(mpDescHeap->AllocateRtv(1, mhRtvs[i]));

	for (size_t i = 0; i < GBuffer::Descriptor::Srv::Count; ++i) 
		CheckReturn(mpDescHeap->AllocateCbvSrvUav(1, mhSrvs[i]));

	CheckReturn(BuildDescriptors());

	return true;
}

bool D3D12GBuffer::OnResize(unsigned width, unsigned height) {
	mInitData.Width = width;
	mInitData.Height = height;

	CheckReturn(BuildResources());
	CheckReturn(BuildDescriptors());

	return true;
}

bool D3D12GBuffer::DrawGBuffer(
	D3D12FrameResource* const pFrameResource
	, D3D12_VIEWPORT viewport
	, D3D12_RECT scissorRect
	, GpuResource* const backBuffer
	, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer
	, GpuResource* const depthBuffer
	, D3D12_CPU_DESCRIPTOR_HANDLE do_depthBuffer
	, const std::vector<D3D12RenderItem*>& staticRitems
	, const std::vector<D3D12RenderItem*>& skinnedRitems
	, FLOAT ditheringMaxDist, FLOAT ditheringMinDist) {
	CheckReturn(DrawGBufferForStaticRitems(
		pFrameResource,
		viewport,
		scissorRect,
		backBuffer,
		ro_backBuffer,
		depthBuffer,
		do_depthBuffer,
		staticRitems,
		ditheringMaxDist,
		ditheringMinDist));
	CheckReturn(DrawGBufferForSkinnedRitems(
		pFrameResource,
		viewport,
		scissorRect,
		backBuffer,
		ro_backBuffer,
		depthBuffer,
		do_depthBuffer,
		skinnedRitems,
		ditheringMaxDist,
		ditheringMinDist));

	return true;
}

bool D3D12GBuffer::DrawGBufferForSkinnedRitems(
	D3D12FrameResource* const pFrameResource
	, D3D12_VIEWPORT viewport
	, D3D12_RECT scissorRect
	, GpuResource* const backBuffer
	, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer
	, GpuResource* const depthBuffer
	, D3D12_CPU_DESCRIPTOR_HANDLE do_depthBuffer
	, const std::vector<D3D12RenderItem*>& ritems
	, FLOAT ditheringMaxDist, FLOAT ditheringMinDist) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[
			mInitData.Device->IsMeshShaderSupported()
				? GBuffer::PipelineState::MP_GBuffer_Skinned
				: GBuffer::PipelineState::GP_GBuffer_Skinned].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetGraphicsRootSignature(mRootSignature.Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		mResources[GBuffer::Resource::E_Albedo]->Transite(
			CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mResources[GBuffer::Resource::E_Normal]->Transite(
			CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mResources[GBuffer::Resource::E_NormalDepth]->Transite(
			CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mResources[GBuffer::Resource::E_ReprojNormalDepth]->Transite(
			CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mResources[GBuffer::Resource::E_RMS]->Transite(
			CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mResources[GBuffer::Resource::E_Velocity]->Transite(
			CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mResources[GBuffer::Resource::E_Position]->Transite(
			CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		depthBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		auto albedoRtv = mpDescHeap->GetCpuHandle(mhRtvs[GBuffer::Descriptor::Rtv::E_Albedo]);
		auto normalRtv = mpDescHeap->GetCpuHandle(mhRtvs[GBuffer::Descriptor::Rtv::E_Normal]);
		auto normalDepthRtv = mpDescHeap->GetCpuHandle(mhRtvs[GBuffer::Descriptor::Rtv::E_NormalDepth]);
		auto reprojNormalDepthRtv = mpDescHeap->GetCpuHandle(mhRtvs[GBuffer::Descriptor::Rtv::E_ReprojNormalDepth]);
		auto rmsRtv = mpDescHeap->GetCpuHandle(mhRtvs[GBuffer::Descriptor::Rtv::E_RMS]);
		auto velocityRtv = mpDescHeap->GetCpuHandle(mhRtvs[GBuffer::Descriptor::Rtv::E_Velocity]);
		auto positionRtv = mpDescHeap->GetCpuHandle(mhRtvs[GBuffer::Descriptor::Rtv::E_Position]);

		std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 7> renderTargets = {
			albedoRtv,
			normalRtv,
			normalDepthRtv,
			reprojNormalDepthRtv,
			rmsRtv,
			velocityRtv,
			positionRtv
		};

		CmdList->OMSetRenderTargets(static_cast<UINT>(
			renderTargets.size()), renderTargets.data(), TRUE, &do_depthBuffer);

		CmdList->SetGraphicsRootConstantBufferView(
			GBuffer::RootSignature::Default::CB_Pass,
			pFrameResource->PassCB.CBAddress());

		CmdList->SetGraphicsRootShaderResourceView(
			GBuffer::RootSignature::Default::SB_BonePalette,
			pFrameResource->BoneSB.Resource()->GetGPUVirtualAddress());

		CheckReturn(DrawRenderItems(
			pFrameResource, CmdList, ritems, ditheringMaxDist, ditheringMinDist, true));
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12GBuffer::DrawGBufferForStaticRitems(
	D3D12FrameResource* const pFrameResource
	, D3D12_VIEWPORT viewport
	, D3D12_RECT scissorRect
	, GpuResource* const backBuffer
	, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer
	, GpuResource* const depthBuffer
	, D3D12_CPU_DESCRIPTOR_HANDLE do_depthBuffer
	, const std::vector<D3D12RenderItem*>& ritems
	, FLOAT ditheringMaxDist, FLOAT ditheringMinDist) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[
			mInitData.Device->IsMeshShaderSupported()
				? GBuffer::PipelineState::MP_GBuffer_Static
				: GBuffer::PipelineState::GP_GBuffer_Static].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetGraphicsRootSignature(mRootSignature.Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		mResources[GBuffer::Resource::E_Albedo]->Transite(
			CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mResources[GBuffer::Resource::E_Normal]->Transite(
			CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mResources[GBuffer::Resource::E_NormalDepth]->Transite(
			CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mResources[GBuffer::Resource::E_ReprojNormalDepth]->Transite(
			CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mResources[GBuffer::Resource::E_RMS]->Transite(
			CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mResources[GBuffer::Resource::E_Velocity]->Transite(
			CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mResources[GBuffer::Resource::E_Position]->Transite(
			CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		depthBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		auto albedoRtv = mpDescHeap->GetCpuHandle(mhRtvs[GBuffer::Descriptor::Rtv::E_Albedo]);
		auto normalRtv = mpDescHeap->GetCpuHandle(mhRtvs[GBuffer::Descriptor::Rtv::E_Normal]);
		auto normalDepthRtv = mpDescHeap->GetCpuHandle(mhRtvs[GBuffer::Descriptor::Rtv::E_NormalDepth]);
		auto reprojNormalDepthRtv = mpDescHeap->GetCpuHandle(mhRtvs[GBuffer::Descriptor::Rtv::E_ReprojNormalDepth]);
		auto rmsRtv = mpDescHeap->GetCpuHandle(mhRtvs[GBuffer::Descriptor::Rtv::E_RMS]);
		auto velocityRtv = mpDescHeap->GetCpuHandle(mhRtvs[GBuffer::Descriptor::Rtv::E_Velocity]);
		auto positionRtv = mpDescHeap->GetCpuHandle(mhRtvs[GBuffer::Descriptor::Rtv::E_Position]);

		CmdList->ClearRenderTargetView(
			albedoRtv, GBuffer::AlbedoMapClearValues, 0, nullptr);
		CmdList->ClearRenderTargetView(
			normalRtv, GBuffer::NormalMapClearValues, 0, nullptr);
		CmdList->ClearRenderTargetView(
			normalDepthRtv, GBuffer::NormalDepthMapClearValues, 0, nullptr);
		CmdList->ClearRenderTargetView(
			reprojNormalDepthRtv, GBuffer::NormalDepthMapClearValues, 0, nullptr);
		CmdList->ClearRenderTargetView(
			rmsRtv, GBuffer::RMSMapClearValues, 0, nullptr);
		CmdList->ClearRenderTargetView(
			velocityRtv, GBuffer::VelocityMapClearValues, 0, nullptr);
		CmdList->ClearRenderTargetView(
			positionRtv, GBuffer::PositionMapClearValues, 0, nullptr);
		CmdList->ClearDepthStencilView(
			do_depthBuffer,
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
			DepthStencilBuffer::InvalidDepthValue,
			DepthStencilBuffer::InvalidStencilValue,
			0, nullptr);

		std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 7> renderTargets = {
			albedoRtv,
			normalRtv,
			normalDepthRtv,
			reprojNormalDepthRtv,
			rmsRtv,
			velocityRtv,
			positionRtv
		};

		CmdList->OMSetRenderTargets(
			static_cast<UINT>(renderTargets.size()), renderTargets.data(), TRUE, &do_depthBuffer);

		CmdList->SetGraphicsRootConstantBufferView(
			GBuffer::RootSignature::Default::CB_Pass,
			pFrameResource->PassCB.CBAddress());

		CheckReturn(DrawRenderItems(
			pFrameResource, CmdList, ritems, ditheringMaxDist, ditheringMinDist, false));
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12GBuffer::DrawRenderItems(
	D3D12FrameResource* const pFrameResource
	, ID3D12GraphicsCommandList6* const pCmdList
	, const std::vector<D3D12RenderItem*>& ritems
	, FLOAT ditheringMaxDist, FLOAT ditheringMinDist
	, bool isSkinned) {
	for (size_t i = 0, end = ritems.size(); i < end; ++i) {
		const auto ri = ritems[i];

		pCmdList->SetGraphicsRootConstantBufferView(
			GBuffer::RootSignature::Default::CB_Object,
			pFrameResource->ObjectCB.CBAddress(ri->ObjectCBIndex));

		pCmdList->SetGraphicsRootConstantBufferView(
			GBuffer::RootSignature::Default::CB_Material,
			pFrameResource->MaterialCB.CBAddress(ri->MaterialCBIndex));
		
		GBuffer::RootConstant::Default::Struct rc;
		rc.gTexDim = { mInitData.Width, mInitData.Height };
		rc.gIndexCount = ri->IndexCount;
		rc.gStartIndex = ri->StartIndexLocation;
		rc.gBaseVertex = ri->BaseVertexLocation;
		rc.gDitheringMaxDist = ditheringMaxDist;
		rc.gDitheringMinDist = ditheringMinDist;
		rc.gHasAlbedoMap = ri->AlbedoMap != nullptr;
		rc.gHasNormalMap = ri->NormalMap != nullptr;

		D3D12Util::SetRoot32BitConstants<GBuffer::RootConstant::Default::Struct>(
			GBuffer::RootSignature::Default::RC_Consts,
			GBuffer::RootConstant::Default::Count,
			&rc,
			0,
			pCmdList,
			FALSE);
		
		if (ri->AlbedoMap) {
			const auto albedoMapSrv = mpDescHeap->GetGpuHandle(ri->AlbedoMap->Allocation);
			pCmdList->SetGraphicsRootDescriptorTable(
				GBuffer::RootSignature::Default::SI_Textures_AlbedoMap,
				albedoMapSrv);
		}
		if (ri->NormalMap) {
			const auto normalMapSrv = mpDescHeap->GetGpuHandle(ri->NormalMap->Allocation);
			pCmdList->SetGraphicsRootDescriptorTable(
				GBuffer::RootSignature::Default::SI_Textures_NormalMap,
				normalMapSrv);
		}

		if (mInitData.Device->IsMeshShaderSupported()) {
			if (isSkinned) {
				pCmdList->SetGraphicsRootShaderResourceView(
					GBuffer::RootSignature::Default::SB_SkinnedVertexBuffer
					, ri->MeshData->VertexBufferGPU->GetGPUVirtualAddress());
			}
			else {
				pCmdList->SetGraphicsRootShaderResourceView(
					GBuffer::RootSignature::Default::SB_StaticVertexBuffer
					, ri->MeshData->VertexBufferGPU->GetGPUVirtualAddress());
			}
			pCmdList->SetGraphicsRootShaderResourceView(
				GBuffer::RootSignature::Default::SB_IndexBuffer
				, ri->MeshData->IndexBufferGPU->GetGPUVirtualAddress());

			const UINT PrimCount = ri->IndexCount / 3;

			pCmdList->DispatchMesh(
				D3D12Util::CeilDivide(PrimCount, GBuffer::ThreadGroup::MeshShader::ThreadsPerGroup),
				1,
				1);
		}
		else {
			auto vb = ri->MeshData->VertexBufferView();
			auto iv = ri->MeshData->IndexBufferView();

			pCmdList->IASetVertexBuffers(0, 1, &vb);
			pCmdList->IASetIndexBuffer(&iv);
			pCmdList->IASetPrimitiveTopology(ri->PrimitiveType);

			pCmdList->DrawIndexedInstanced(
				ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
		}
	}

	return true;
}

bool D3D12GBuffer::BuildResources() {
	D3D12_RESOURCE_DESC rscDesc{};
	rscDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rscDesc.Alignment = 0;
	rscDesc.Width = mInitData.Width;
	rscDesc.Height = mInitData.Height;
	rscDesc.DepthOrArraySize = 1;
	rscDesc.MipLevels = 1;
	rscDesc.SampleDesc.Count = 1;
	rscDesc.SampleDesc.Quality = 0;
	rscDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	const auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	// AlbedoMap
	{
		rscDesc.Format = GBuffer::AlbedoMapFormat;
		rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		const CD3DX12_CLEAR_VALUE AlbedoMapOptClear(
			GBuffer::AlbedoMapFormat,
			GBuffer::AlbedoMapClearValues);

		CheckReturn(mResources[GBuffer::Resource::E_Albedo]->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&AlbedoMapOptClear,
			L"GBuffer_AlbedoMap"));
	}
	// NormalMap
	{
		rscDesc.Format = GBuffer::NormalMapFormat;
		rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		const CD3DX12_CLEAR_VALUE NormalMapOptClear(
			GBuffer::NormalMapFormat,
			GBuffer::NormalMapClearValues);

		CheckReturn(mResources[GBuffer::Resource::E_Normal]->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&NormalMapOptClear,
			L"GBuffer_NormalMap"));
	}
	// NormalDepthMap
	{
		rscDesc.Format = GBuffer::NormalDepthMapFormat;
		rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		const CD3DX12_CLEAR_VALUE NormalDepthMapOptClear(
			GBuffer::NormalDepthMapFormat,
			GBuffer::NormalDepthMapClearValues);

		CheckReturn(mResources[GBuffer::Resource::E_NormalDepth]->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&NormalDepthMapOptClear,
			L"GBuffer_NormalDepthMap"));
	}
	// ReprojectedNormalDepthMap
	{
		rscDesc.Format = GBuffer::NormalDepthMapFormat;
		rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		const CD3DX12_CLEAR_VALUE NormalDepthMapOptClear(
			GBuffer::NormalDepthMapFormat,
			GBuffer::NormalDepthMapClearValues);

		CheckReturn(mResources[GBuffer::Resource::E_ReprojNormalDepth]->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&NormalDepthMapOptClear,
			L"GBuffer_ReprojectedNormalDepthMap"));
	}
	// CachedNormalDepthMap
	{
		rscDesc.Format = GBuffer::NormalDepthMapFormat;
		rscDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		CheckReturn(mResources[GBuffer::Resource::E_CachedNormalDepth]->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			L"GBuffer_CachedNormalDepthMap"));
	}	
	// RoughnessMetallicMap
	{
		rscDesc.Format = GBuffer::RMSMapFormat;
		rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		const CD3DX12_CLEAR_VALUE RMSMapOptClear(
			GBuffer::RMSMapFormat,
			GBuffer::RMSMapClearValues);

		CheckReturn(mResources[GBuffer::Resource::E_RMS]->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&RMSMapOptClear,
			L"GBuffer_RoughnessMetallicSpecularMap"));
	}
	// VelocityMap
	{
		rscDesc.Format = GBuffer::VelocityMapFormat;
		rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		const CD3DX12_CLEAR_VALUE VelocityMapOptClear(
			GBuffer::VelocityMapFormat,
			GBuffer::VelocityMapClearValues);

		CheckReturn(mResources[GBuffer::Resource::E_Velocity]->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&VelocityMapOptClear,
			L"GBuffer_VelocityMap"));
	}
	// PositionMap
	{
		rscDesc.Format = GBuffer::PositionMapFormat;
		rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		const CD3DX12_CLEAR_VALUE PositionMapOptClear(
			GBuffer::PositionMapFormat,
			GBuffer::PositionMapClearValues);

		CheckReturn(mResources[GBuffer::Resource::E_Position]->Initialize(
			mInitData.Device,
			&prop,
			D3D12_HEAP_FLAG_NONE,
			&rscDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&PositionMapOptClear,
			L"GBuffer_PositionMap"));
	}

	return true;
}

bool D3D12GBuffer::BuildDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.f;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;

	// AlbedoMap
	{
		srvDesc.Format = GBuffer::AlbedoMapFormat;
		rtvDesc.Format = GBuffer::AlbedoMapFormat;

		const auto AlbedoMap = mResources[GBuffer::Resource::E_Albedo]->Resource();
		D3D12Util::CreateShaderResourceView(
			mInitData.Device, AlbedoMap, &srvDesc
			, mpDescHeap->GetCpuHandle(mhSrvs[GBuffer::Descriptor::Srv::E_Albedo]));
		D3D12Util::CreateRenderTargetView(
			mInitData.Device, AlbedoMap, &rtvDesc
			, mpDescHeap->GetCpuHandle(mhRtvs[GBuffer::Descriptor::Rtv::E_Albedo]));
	}
	// NormalMap
	{
		srvDesc.Format = GBuffer::NormalMapFormat;
		rtvDesc.Format = GBuffer::NormalMapFormat;

		const auto NormalMap = mResources[GBuffer::Resource::E_Normal]->Resource();
		D3D12Util::CreateShaderResourceView(
			mInitData.Device, NormalMap, &srvDesc
			, mpDescHeap->GetCpuHandle(mhSrvs[GBuffer::Descriptor::Srv::E_Normal]));
		D3D12Util::CreateRenderTargetView(
			mInitData.Device, NormalMap, &
			rtvDesc, mpDescHeap->GetCpuHandle(mhRtvs[GBuffer::Descriptor::Rtv::E_Normal]));
	}
	// NormalDepthMap
	{
		srvDesc.Format = GBuffer::NormalDepthMapFormat;
		rtvDesc.Format = GBuffer::NormalDepthMapFormat;

		const auto NormalDepthMap = mResources[GBuffer::Resource::E_NormalDepth]->Resource();
		D3D12Util::CreateShaderResourceView(
			mInitData.Device, NormalDepthMap, &srvDesc
			, mpDescHeap->GetCpuHandle(mhSrvs[GBuffer::Descriptor::Srv::E_NormalDepth]));
		D3D12Util::CreateRenderTargetView(
			mInitData.Device, NormalDepthMap, &rtvDesc
			, mpDescHeap->GetCpuHandle(mhRtvs[GBuffer::Descriptor::Rtv::E_NormalDepth]));
	}
	// ReprojectedNormalDepthMap
	{
		srvDesc.Format = GBuffer::NormalDepthMapFormat;
		rtvDesc.Format = GBuffer::NormalDepthMapFormat;

		const auto PrevNormalDepthMap = mResources[GBuffer::Resource::E_ReprojNormalDepth]->Resource();
		D3D12Util::CreateShaderResourceView(
			mInitData.Device, PrevNormalDepthMap, &srvDesc
			, mpDescHeap->GetCpuHandle(mhSrvs[GBuffer::Descriptor::Srv::E_ReprojNormalDepth]));
		D3D12Util::CreateRenderTargetView(
			mInitData.Device, PrevNormalDepthMap, &rtvDesc
			, mpDescHeap->GetCpuHandle(mhRtvs[GBuffer::Descriptor::Rtv::E_ReprojNormalDepth]));
	}
	// CachedNormalDepthMap
	{
		srvDesc.Format = GBuffer::NormalDepthMapFormat;
		rtvDesc.Format = GBuffer::NormalDepthMapFormat;

		const auto CachedNormalDepthMap = mResources[GBuffer::Resource::E_CachedNormalDepth]->Resource();
		D3D12Util::CreateShaderResourceView(
			mInitData.Device, CachedNormalDepthMap, &srvDesc
			, mpDescHeap->GetCpuHandle(mhSrvs[GBuffer::Descriptor::Srv::E_CachedNormalDepth]));
	}
	// RMSMap
	{
		srvDesc.Format = GBuffer::RMSMapFormat;
		rtvDesc.Format = GBuffer::RMSMapFormat;

		const auto RMSMap = mResources[GBuffer::Resource::E_RMS]->Resource();
		D3D12Util::CreateShaderResourceView(
			mInitData.Device, RMSMap, &srvDesc
			, mpDescHeap->GetCpuHandle(mhSrvs[GBuffer::Descriptor::Srv::E_RMS]));
		D3D12Util::CreateRenderTargetView(
			mInitData.Device, RMSMap, &rtvDesc
			, mpDescHeap->GetCpuHandle(mhRtvs[GBuffer::Descriptor::Rtv::E_RMS]));
	}
	// PositionMap
	{
		srvDesc.Format = GBuffer::PositionMapFormat;
		rtvDesc.Format = GBuffer::PositionMapFormat;

		const auto PositionMap = mResources[GBuffer::Resource::E_Position]->Resource();
		D3D12Util::CreateShaderResourceView(
			mInitData.Device, PositionMap, &srvDesc
			, mpDescHeap->GetCpuHandle(mhSrvs[GBuffer::Descriptor::Srv::E_Position]));
		D3D12Util::CreateRenderTargetView(
			mInitData.Device, PositionMap, &rtvDesc
			, mpDescHeap->GetCpuHandle(mhRtvs[GBuffer::Descriptor::Rtv::E_Position]));
	}
	// VelocityMap
	{
		srvDesc.Format = GBuffer::VelocityMapFormat;
		rtvDesc.Format = GBuffer::VelocityMapFormat;

		const auto VelocityMap = mResources[GBuffer::Resource::E_Velocity]->Resource();
		D3D12Util::CreateShaderResourceView(
			mInitData.Device, VelocityMap, &srvDesc
			, mpDescHeap->GetCpuHandle(mhSrvs[GBuffer::Descriptor::Srv::E_Velocity]));
		D3D12Util::CreateRenderTargetView(
			mInitData.Device, VelocityMap, &rtvDesc
			, mpDescHeap->GetCpuHandle(mhRtvs[GBuffer::Descriptor::Rtv::E_Velocity]));
	}

	return true;
}