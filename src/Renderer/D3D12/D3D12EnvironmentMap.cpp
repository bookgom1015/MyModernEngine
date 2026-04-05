#include "pch.h"
#include "Renderer/D3D12/D3D12EnvironmentMap.hpp"

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
	const WCHAR* const HLSL_IntegrateBrdf = L"D3D12IntegrateBrdf.hlsl";
	const WCHAR* const HLSL_DrawSkySphere = L"D3D12DrawSkySphere.hlsl";
}

D3D12EnvironmentMap::D3D12EnvironmentMap() {}

D3D12EnvironmentMap::~D3D12EnvironmentMap() {}

bool D3D12EnvironmentMap::Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	mInitData = *reinterpret_cast<InitData*>(pData);

	auto sizeF = static_cast<FLOAT>(EnvironmentMap::BrdfLutMapSize);
	auto sizeI = static_cast<INT>(EnvironmentMap::BrdfLutMapSize);
	mViewport = { 0.f, 0.f, sizeF , sizeF , 0.f, 1.f };
	mScissorRect = { 0, 0, sizeI , sizeI };

	mBrdfLutMap = std::make_unique<GpuResource>();

	CheckReturn(BuildResources());

	return true;
}

bool D3D12EnvironmentMap::CompileShaders() {
	// IntegrateBrdf
	{
		const auto VS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_IntegrateBrdf, L"VS", L"vs_6_5");
		const auto MS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_IntegrateBrdf, L"MS", L"ms_6_5");
		const auto PS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_IntegrateBrdf, L"PS", L"ps_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			VS, mShaderHashes[EnvironmentMap::Shader::VS_IntegrateBrdf]));
		CheckReturn(mInitData.ShaderManager->AddShader(
			MS, mShaderHashes[EnvironmentMap::Shader::MS_IntegrateBrdf]));
		CheckReturn(mInitData.ShaderManager->AddShader(
			PS, mShaderHashes[EnvironmentMap::Shader::PS_IntegrateBrdf]));
	}
	// DrawSkySphere
	{
		const auto VS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_DrawSkySphere, L"VS", L"vs_6_5");
		const auto MS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_DrawSkySphere, L"MS", L"ms_6_5");
		const auto PS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_DrawSkySphere, L"PS", L"ps_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			VS, mShaderHashes[EnvironmentMap::Shader::VS_DrawSkySphere]));
		CheckReturn(mInitData.ShaderManager->AddShader(
			MS, mShaderHashes[EnvironmentMap::Shader::MS_DrawSkySphere]));
		CheckReturn(mInitData.ShaderManager->AddShader(
			PS, mShaderHashes[EnvironmentMap::Shader::PS_DrawSkySphere]));
	}

	return true;
}

bool D3D12EnvironmentMap::BuildRootSignatures() {
	decltype(auto) samplers = D3D12Util::GetStaticSamplers();

	// IntegrateBrdf
	{
		CD3DX12_ROOT_PARAMETER slotRootParameter[EnvironmentMap::RootSignature::IntegrateBrdf::Count]{};
		slotRootParameter[EnvironmentMap::RootSignature::IntegrateBrdf::CB_Pass].InitAsConstantBufferView(0);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[EnvironmentMap::RootSignature::GR_IntegrateBrdf]),
			L"EnvironmentMap_GR_IntegrateBrdf"));
	}
	// DrawSkySphere 
	{
		CD3DX12_DESCRIPTOR_RANGE texTables[1]{}; UINT index = 0;
		texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1);

		index = 0;

		CD3DX12_ROOT_PARAMETER slotRootParameter[EnvironmentMap::RootSignature::DrawSkySphere::Count]{};
		slotRootParameter[EnvironmentMap::RootSignature::DrawSkySphere::CB_Pass]
			.InitAsConstantBufferView(0);
		slotRootParameter[EnvironmentMap::RootSignature::DrawSkySphere::CB_Object]
			.InitAsConstantBufferView(1);
		slotRootParameter[EnvironmentMap::RootSignature::DrawSkySphere::RC_Consts]
			.InitAsConstants(EnvironmentMap::RootConstant::DrawSkySphere::Count, 2);
		slotRootParameter[EnvironmentMap::RootSignature::DrawSkySphere::SB_VertexBuffer]
			.InitAsShaderResourceView(0);
		slotRootParameter[EnvironmentMap::RootSignature::DrawSkySphere::SB_IndexBuffer]
			.InitAsShaderResourceView(1);

		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
			_countof(slotRootParameter), slotRootParameter,
			D3D12Util::StaticSamplerCount, samplers,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		CheckReturn(D3D12Util::CreateRootSignature(
			mInitData.Device,
			rootSigDesc,
			IID_PPV_ARGS(&mRootSignatures[EnvironmentMap::RootSignature::GR_DrawSkySphere]),
			L"EnvironmentMap_GR_DrawSkySphere"));
	}

	return true;
}

bool D3D12EnvironmentMap::BuildPipelineStates() {
	// IntegrateBrdf
	{
		if (mInitData.Device->IsMeshShaderSupported()) {
			auto psoDesc = D3D12Util::FitToScreenMeshPsoDesc();

			psoDesc.pRootSignature = mRootSignatures[EnvironmentMap::RootSignature::GR_IntegrateBrdf].Get();
			{
				const auto MS = mInitData.ShaderManager->GetShader(
					mShaderHashes[EnvironmentMap::Shader::MS_IntegrateBrdf]);
				NullCheck(MS);
				const auto PS = mInitData.ShaderManager->GetShader(
					mShaderHashes[EnvironmentMap::Shader::PS_IntegrateBrdf]);
				NullCheck(PS);
				psoDesc.MS = {
					reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
				psoDesc.PS = {
					reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = EnvironmentMap::BrdfLutMapFormat;
			psoDesc.DepthStencilState.DepthEnable = FALSE;

			CheckReturn(D3D12Util::CreatePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[EnvironmentMap::PipelineState::MP_IntegrateBrdf]),
				L"EnvironmentMap_MP_IntegrateBrdf"));
		}
		else {
			auto psoDesc = D3D12Util::FitToScreenPsoDesc();

			psoDesc.pRootSignature = mRootSignatures[EnvironmentMap::RootSignature::GR_IntegrateBrdf].Get();
			{
				const auto VS = mInitData.ShaderManager->GetShader(
					mShaderHashes[EnvironmentMap::Shader::VS_IntegrateBrdf]);
				NullCheck(VS);
				const auto PS = mInitData.ShaderManager->GetShader(
					mShaderHashes[EnvironmentMap::Shader::PS_IntegrateBrdf]);
				NullCheck(PS);
				psoDesc.VS = {
					reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
				psoDesc.PS = {
					reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = EnvironmentMap::BrdfLutMapFormat;
			psoDesc.DepthStencilState.DepthEnable = FALSE;

			CheckReturn(D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[EnvironmentMap::PipelineState::GP_IntegrateBrdf]),
				L"EnvironmentMap_GP_IntegrateBrdf"));
		}
	}
	// DrawSkySphere
	{
		if (mInitData.Device->IsMeshShaderSupported()) {
			auto psoDesc = D3D12Util::DefaultMeshPsoDesc(DepthStencilBuffer::DepthStencilBufferFormat);

			psoDesc.pRootSignature = mRootSignatures[EnvironmentMap::RootSignature::GR_DrawSkySphere].Get();
			{
				const auto MS = mInitData.ShaderManager->GetShader(
					mShaderHashes[EnvironmentMap::Shader::MS_DrawSkySphere]);
				NullCheck(MS);
				const auto PS = mInitData.ShaderManager->GetShader(
					mShaderHashes[EnvironmentMap::Shader::PS_DrawSkySphere]);
				NullCheck(PS);
				psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;
			psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
			psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

			CheckReturn(D3D12Util::CreatePipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[EnvironmentMap::PipelineState::MP_DrawSkySphere]),
				L"EnvironmentMap_MP_DrawSkySphere"));
		}
		else {
			const auto inputLayout = D3D12Util::StaticVertexInputLayoutDesc();
			auto psoDesc = D3D12Util::DefaultPsoDesc(inputLayout, DepthStencilBuffer::DepthStencilBufferFormat);

			psoDesc.pRootSignature = mRootSignatures[EnvironmentMap::RootSignature::GR_DrawSkySphere].Get();
			{
				const auto VS = mInitData.ShaderManager->GetShader(
					mShaderHashes[EnvironmentMap::Shader::VS_DrawSkySphere]);
				NullCheck(VS);
				const auto PS = mInitData.ShaderManager->GetShader(
					mShaderHashes[EnvironmentMap::Shader::PS_DrawSkySphere]);
				NullCheck(PS);
				psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
				psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
			}
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = HDR_FORMAT;
			psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
			psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

			CheckReturn(D3D12Util::CreateGraphicsPipelineState(
				mInitData.Device,
				psoDesc,
				IID_PPV_ARGS(&mPipelineStates[EnvironmentMap::PipelineState::GP_DrawSkySphere]),
				L"EnvironmentMap_GP_DrawSkySphere"));
		}
	}

	return true;
}

bool D3D12EnvironmentMap::AllocateDescriptors() {
	CheckReturn(mpDescHeap->AllocateCbvSrvUav(1, mhBrdfLutMapSrv));
	CheckReturn(mpDescHeap->AllocateRtv(1, mhBrdfLutMapRtv));

	CheckReturn(BuildDescriptors());

	return true;
}

bool D3D12EnvironmentMap::DrawBrdfLutMap(D3D12FrameResource* const pFrameResource) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[
			mInitData.Device->IsMeshShaderSupported()
				? EnvironmentMap::PipelineState::MP_IntegrateBrdf
				: EnvironmentMap::PipelineState::GP_IntegrateBrdf].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	CmdList->SetGraphicsRootSignature(mRootSignatures[EnvironmentMap::RootSignature::GR_IntegrateBrdf].Get());

	CmdList->RSSetViewports(1, &mViewport);
	CmdList->RSSetScissorRects(1, &mScissorRect);

	mBrdfLutMap->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);

	auto rtv = mpDescHeap->GetCpuHandle(mhBrdfLutMapRtv);
	CmdList->OMSetRenderTargets(1, &rtv, TRUE, nullptr);

	CmdList->SetGraphicsRootConstantBufferView(
		EnvironmentMap::RootSignature::IntegrateBrdf::CB_Pass, pFrameResource->PassCB.CBAddress());

	if (mInitData.Device->IsMeshShaderSupported()) {
		CmdList->DispatchMesh(1, 1, 1);
	}
	else {
		CmdList->IASetVertexBuffers(0, 0, nullptr);
		CmdList->IASetIndexBuffer(nullptr);
		CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		CmdList->DrawInstanced(6, 1, 0, 0);
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12EnvironmentMap::DrawSkySphere(
	D3D12FrameResource* const pFrameResource
	, D3D12_VIEWPORT viewport
	, D3D12_RECT scissorRect
	, GpuResource* const backBuffer
	, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer
	, GpuResource* const depthBuffer
	, D3D12_CPU_DESCRIPTOR_HANDLE dio_depthStencil
	, const std::vector<D3D12RenderItem*>& ritems) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[mInitData.Device->IsMeshShaderSupported() ?
		EnvironmentMap::PipelineState::MP_DrawSkySphere: EnvironmentMap::PipelineState::GP_DrawSkySphere].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetGraphicsRootSignature(mRootSignatures[EnvironmentMap::RootSignature::GR_DrawSkySphere].Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		backBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		depthBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, &dio_depthStencil);

		CmdList->SetGraphicsRootConstantBufferView(
			EnvironmentMap::RootSignature::DrawSkySphere::CB_Pass,
			pFrameResource->PassCB.CBAddress());

		CheckReturn(DrawRenderItems(pFrameResource, CmdList, ritems));
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12EnvironmentMap::DrawRenderItems(
	D3D12FrameResource* const pFrameResource
	, ID3D12GraphicsCommandList6* const pCmdList
	, const std::vector<D3D12RenderItem*>& ritems) {
	for (size_t i = 0, end = ritems.size(); i < end; ++i) {
		const auto ri = ritems[i];

		pCmdList->SetGraphicsRootConstantBufferView(
			EnvironmentMap::RootSignature::DrawSkySphere::CB_Object,
			pFrameResource->ObjectCB.CBAddress(ri->ObjectCBIndex));

		EnvironmentMap::RootConstant::DrawSkySphere::Struct rc;
		rc.gIndexCount = ri->IndexCount;

		D3D12Util::SetRoot32BitConstants<EnvironmentMap::RootConstant::DrawSkySphere::Struct>(
			EnvironmentMap::RootSignature::DrawSkySphere::RC_Consts,
			EnvironmentMap::RootConstant::DrawSkySphere::Count,
			&rc,
			0,
			pCmdList,
			FALSE);	

		if (mInitData.Device->IsMeshShaderSupported()) {
			pCmdList->SetGraphicsRootShaderResourceView(
				EnvironmentMap::RootSignature::DrawSkySphere::SB_VertexBuffer
				, ri->MeshData->VertexBufferGPU->GetGPUVirtualAddress());
			pCmdList->SetGraphicsRootShaderResourceView(
				EnvironmentMap::RootSignature::DrawSkySphere::SB_IndexBuffer
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

bool D3D12EnvironmentMap::BuildResources() {
	D3D12_RESOURCE_DESC rscDesc;
	ZeroMemory(&rscDesc, sizeof(D3D12_RESOURCE_DESC));
	rscDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rscDesc.Alignment = 0;
	rscDesc.Format = EnvironmentMap::BrdfLutMapFormat;
	rscDesc.Width = EnvironmentMap::BrdfLutMapSize;
	rscDesc.Height = EnvironmentMap::BrdfLutMapSize;
	rscDesc.MipLevels = 1;
	rscDesc.DepthOrArraySize = 1;
	rscDesc.SampleDesc.Count = 1;
	rscDesc.SampleDesc.Quality = 0;
	rscDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	const auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CheckReturn(mBrdfLutMap->Initialize(
		mInitData.Device,
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&rscDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		L"EnvironmentMap_BrdfLutMap"));

	return true;
}

bool D3D12EnvironmentMap::BuildDescriptors() {
	// Srv
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Format = EnvironmentMap::BrdfLutMapFormat;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.f;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.MipLevels = 1;

		D3D12Util::CreateShaderResourceView(
			mInitData.Device,
			mBrdfLutMap->Resource(),
			&srvDesc,
			mpDescHeap->GetCpuHandle(mhBrdfLutMapSrv));
	}
	// Rtv
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Format = EnvironmentMap::BrdfLutMapFormat;
		rtvDesc.Texture2D.PlaneSlice = 0;
		rtvDesc.Texture2D.MipSlice = 0;

		D3D12Util::CreateRenderTargetView(
			mInitData.Device,
			mBrdfLutMap->Resource(),
			&rtvDesc,
			mpDescHeap->GetCpuHandle(mhBrdfLutMapRtv));
	}

	return true;
}