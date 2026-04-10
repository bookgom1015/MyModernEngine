#include "pch.h"
#include "Renderer/D3D12/D3D12Shadow.hpp"

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
	const WCHAR* const HLSL_DrawDepth = L"D3D12DrawDepth.hlsl";
}

D3D12Shadow::D3D12Shadow() {}

D3D12Shadow::~D3D12Shadow() {}

bool D3D12Shadow::Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	mInitData = *reinterpret_cast<InitData*>(pData);

	mViewport = { 
		0.0f, 0.0f, static_cast<FLOAT>(mInitData.Width), static_cast<FLOAT>(mInitData.Height), 0.0f, 1.0f };
	mScissorRect = { 
		0, 0, static_cast<INT>(mInitData.Width), static_cast<INT>(mInitData.Height) };

	mDepthArrayMap = std::make_unique<GpuResource>();

	CheckReturn(BuildResources());

	return true;
}

bool D3D12Shadow::CompileShaders() {
	const auto VS_Static = D3D12ShaderManager::D3D12ShaderInfo(
		HLSL_DrawDepth, L"VS_Static", L"vs_6_5");
	const auto VS_Skinned = D3D12ShaderManager::D3D12ShaderInfo(
		HLSL_DrawDepth, L"VS_Skinned", L"vs_6_5");
	const auto GS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_DrawDepth, L"GS", L"gs_6_5");
	const auto PS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_DrawDepth, L"PS", L"ps_6_5");
	CheckReturn(mInitData.ShaderManager->AddShader(
		VS_Static, mShaderHashes[Shadow::Shader::VS_DrawDepth_Static]));
	CheckReturn(mInitData.ShaderManager->AddShader(
		VS_Skinned, mShaderHashes[Shadow::Shader::VS_DrawDepth_Skinned]));
	CheckReturn(mInitData.ShaderManager->AddShader(
		GS, mShaderHashes[Shadow::Shader::GS_DrawDepth]));
	CheckReturn(mInitData.ShaderManager->AddShader(
		PS, mShaderHashes[Shadow::Shader::PS_DrawDepth]));

	return true;
}

bool D3D12Shadow::BuildRootSignatures() {
	decltype(auto) samplers = D3D12Util::GetStaticSamplers();

	CD3DX12_ROOT_PARAMETER slotRootParameter[Shadow::RootSignature::DrawDepth::Count]{};
	slotRootParameter[Shadow::RootSignature::DrawDepth::CB_Light].
		InitAsConstantBufferView(0);
	slotRootParameter[Shadow::RootSignature::DrawDepth::CB_Object].
		InitAsConstantBufferView(1);
	slotRootParameter[Shadow::RootSignature::DrawDepth::CB_Material].
		InitAsConstantBufferView(2);
	slotRootParameter[Shadow::RootSignature::DrawDepth::SB_BonePalette].
		InitAsShaderResourceView(3);
	slotRootParameter[Shadow::RootSignature::DrawDepth::RC_Consts].
		InitAsConstants(Shadow::RootConstant::DrawDepth::Count, 3);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		D3D12Util::StaticSamplerCount, samplers,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	CheckReturn(D3D12Util::CreateRootSignature(
		mInitData.Device,
		rootSigDesc,
		IID_PPV_ARGS(&mRootSignature),
		L"Shadow_GR_DrawDepth"));

	return true;
}

bool D3D12Shadow::BuildPipelineStates() {
	auto psoDesc = D3D12Util::DefaultPsoDesc({}, Shadow::DepthMapFormat);
	psoDesc.pRootSignature = mRootSignature.Get();
	{
		const auto GS = mInitData.ShaderManager->GetShader(
			mShaderHashes[Shadow::Shader::GS_DrawDepth]);
		NullCheck(GS);
		const auto PS = mInitData.ShaderManager->GetShader(
			mShaderHashes[Shadow::Shader::PS_DrawDepth]);
		NullCheck(PS);
		psoDesc.GS = { reinterpret_cast<BYTE*>(GS->GetBufferPointer()), GS->GetBufferSize() };
		psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
	}
	psoDesc.NumRenderTargets = 0;
	psoDesc.DSVFormat = Shadow::DepthMapFormat;
	psoDesc.RasterizerState.DepthBias = 10000;
	psoDesc.RasterizerState.SlopeScaledDepthBias = 1.f;
	psoDesc.RasterizerState.DepthBiasClamp = 0.f;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	{
		const auto inputLayout = D3D12Util::StaticVertexInputLayoutDesc();
		psoDesc.InputLayout = inputLayout;

		const auto VS = mInitData.ShaderManager->GetShader(
			mShaderHashes[Shadow::Shader::VS_DrawDepth_Static]);
		NullCheck(VS);
		psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };

		CheckReturn(D3D12Util::CreateGraphicsPipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[Shadow::PipelineState::GP_DrawDepth_Static]),
			L"Shadow_GP_DrawDepth_Static"));
	}
	{
		const auto inputLayout = D3D12Util::SkinnedVertexInputLayoutDesc();
		psoDesc.InputLayout = inputLayout;

		const auto VS = mInitData.ShaderManager->GetShader(
			mShaderHashes[Shadow::Shader::VS_DrawDepth_Skinned]);
		NullCheck(VS);
		psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };

		CheckReturn(D3D12Util::CreateGraphicsPipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[Shadow::PipelineState::GP_DrawDepth_Skinned]),
			L"Shadow_GP_DrawDepth_Skinned"));
	}

	return true;
}

bool D3D12Shadow::AllocateDescriptors() {
	CheckReturn(mpDescHeap->AllocateDsv(1, mhDsv));
	for (size_t i = 0; i < MAX_LIGHT_TEX_COUNT; ++i)
		CheckReturn(mpDescHeap->AllocateDsv(1, mhDsvs[i]));

	CheckReturn(mpDescHeap->AllocateCbvSrvUav(1, mhSrv));
	for (size_t i = 0; i < MAX_LIGHT_TEX_COUNT; ++i)
		CheckReturn(mpDescHeap->AllocateCbvSrvUav(1, mhSrvs[i]));

	CheckReturn(BuildDescriptors());

	return true;
}

bool D3D12Shadow::Run(
	D3D12FrameResource* const pFrameResource
	, const std::vector<D3D12RenderItem*>& staticRitems	
	, const std::vector<D3D12RenderItem*>& skinnedRitems
	, const std::vector<const LightData*>& lights) {
	UINT index = 0;

	for (const auto light : lights) {
		CheckReturn(DrawDepthStatic(pFrameResource, staticRitems, light, index));
		CheckReturn(DrawDepthSkinned(pFrameResource, skinnedRitems, light, index));

		++index;
	}

	return true;
}

bool D3D12Shadow::DrawDepthStatic(
	D3D12FrameResource* const pFrameResource
	, const std::vector<D3D12RenderItem*>& ritems
	, const LightData* light
	, UINT lightIndex) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[Shadow::PipelineState::GP_DrawDepth_Static].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetGraphicsRootSignature(mRootSignature.Get());

		CmdList->RSSetViewports(1, &mViewport);
		CmdList->RSSetScissorRects(1, &mScissorRect);

		mDepthArrayMap->Transite(CmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		const auto dsv = mpDescHeap->GetCpuHandle(mhDsv);

		if (lightIndex == 0) CmdList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		CmdList->OMSetRenderTargets(0, nullptr, FALSE, &dsv);

		CmdList->SetGraphicsRootConstantBufferView(
			Shadow::RootSignature::DrawDepth::CB_Light, pFrameResource->LightCB.CBAddress());

		Shadow::RootConstant::DrawDepth::Struct rc;
		rc.gLightIndex = lightIndex;
		rc.gBaseIndex = light->BaseIndex;
		rc.gIndexStride = light->IndexStride;

		D3D12Util::SetRoot32BitConstants<Shadow::RootConstant::DrawDepth::Struct>(
			Shadow::RootSignature::DrawDepth::RC_Consts,
			Shadow::RootConstant::DrawDepth::Count,
			&rc,
			0,
			CmdList,
			FALSE);

		CheckReturn(DrawRenderItems(pFrameResource, CmdList, ritems));
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12Shadow::DrawDepthSkinned(
	D3D12FrameResource* const pFrameResource
	, const std::vector<D3D12RenderItem*>& ritems
	, const LightData* light
	, UINT lightIndex) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[Shadow::PipelineState::GP_DrawDepth_Skinned].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetGraphicsRootSignature(mRootSignature.Get());

		CmdList->RSSetViewports(1, &mViewport);
		CmdList->RSSetScissorRects(1, &mScissorRect);

		mDepthArrayMap->Transite(CmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		const auto dsv = mpDescHeap->GetCpuHandle(mhDsv);

		if (lightIndex == 0) CmdList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		CmdList->OMSetRenderTargets(0, nullptr, FALSE, &dsv);

		CmdList->SetGraphicsRootConstantBufferView(
			Shadow::RootSignature::DrawDepth::CB_Light, pFrameResource->LightCB.CBAddress());

		Shadow::RootConstant::DrawDepth::Struct rc;
		rc.gLightIndex = lightIndex;
		rc.gBaseIndex = light->BaseIndex;
		rc.gIndexStride = light->IndexStride;

		D3D12Util::SetRoot32BitConstants<Shadow::RootConstant::DrawDepth::Struct>(
			Shadow::RootSignature::DrawDepth::RC_Consts,
			Shadow::RootConstant::DrawDepth::Count,
			&rc,
			0,
			CmdList,
			FALSE);

		CmdList->SetGraphicsRootShaderResourceView(
			Shadow::RootSignature::DrawDepth::SB_BonePalette,
			pFrameResource->BoneSB[D3D12FrameResource::CurrentBonePaletteIndex].Resource()->GetGPUVirtualAddress());

		CheckReturn(DrawRenderItems(pFrameResource, CmdList, ritems));
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12Shadow::DrawRenderItems(
	D3D12FrameResource* const pFrameResource
	, ID3D12GraphicsCommandList6* const pCmdList
	, const std::vector<D3D12RenderItem*>& ritems) {
	for (size_t i = 0, end = ritems.size(); i < end; ++i) {
		const auto ri = ritems[i];

		pCmdList->SetGraphicsRootConstantBufferView(
			Shadow::RootSignature::DrawDepth::CB_Object,
			pFrameResource->ObjectCB.CBAddress(ri->ObjectCBIndex));

		pCmdList->SetGraphicsRootConstantBufferView(
			Shadow::RootSignature::DrawDepth::CB_Material,
			pFrameResource->MaterialCB.CBAddress(ri->MaterialCBIndex));

		auto vb = ri->MeshData->VertexBufferView();
		auto iv = ri->MeshData->IndexBufferView();

		pCmdList->IASetVertexBuffers(0, 1, &vb);
		pCmdList->IASetIndexBuffer(&iv);
		pCmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		pCmdList->DrawIndexedInstanced(
			ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}

	return true;
}

bool D3D12Shadow::BuildResources() {
	D3D12_RESOURCE_DESC rscDesc{};
	ZeroMemory(&rscDesc, sizeof(D3D12_RESOURCE_DESC));
	rscDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rscDesc.Alignment = 0;
	rscDesc.Format = Shadow::DepthMapFormat;
	rscDesc.Width = mInitData.Width;
	rscDesc.Height = mInitData.Height;
	rscDesc.DepthOrArraySize = MAX_LIGHT_TEX_COUNT;
	rscDesc.MipLevels = 1;
	rscDesc.SampleDesc.Count = 1;
	rscDesc.SampleDesc.Quality = 0;
	rscDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rscDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear{};
	optClear.Format = Shadow::DepthMapFormat;
	optClear.DepthStencil.Depth = 1.f;
	optClear.DepthStencil.Stencil = 0;

	const auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	CheckReturn(mDepthArrayMap->Initialize(
		mInitData.Device,
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&rscDesc,
		D3D12_RESOURCE_STATE_DEPTH_READ,
		&optClear,
		L"Shadow_DepthArray"));

	return true;
}

bool D3D12Shadow::BuildDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.Texture2DArray.MipLevels = 1;
	srvDesc.Texture2DArray.MostDetailedMip = 0;
	srvDesc.Texture2DArray.PlaneSlice = 0;
	srvDesc.Texture2DArray.ResourceMinLODClamp = 0.f;

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
	dsvDesc.Format = Shadow::DepthMapFormat;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.Texture2DArray.MipSlice = 0;

	const auto resource = mDepthArrayMap->Resource();

	{
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		srvDesc.Texture2DArray.ArraySize = MAX_LIGHT_TEX_COUNT;

		D3D12Util::CreateShaderResourceView(
			mInitData.Device, resource, &srvDesc
			, mpDescHeap->GetCpuHandle(mhSrv));

		dsvDesc.Texture2DArray.FirstArraySlice = 0;
		dsvDesc.Texture2DArray.ArraySize = MAX_LIGHT_TEX_COUNT;

		D3D12Util::CreateDepthStencilView(
			mInitData.Device, resource, &dsvDesc
			, mpDescHeap->GetCpuHandle(mhDsv));
	}

	for (UINT i = 0; i < MAX_LIGHT_TEX_COUNT; ++i) {
		srvDesc.Texture2DArray.FirstArraySlice = i;
		srvDesc.Texture2DArray.ArraySize = 1;

		dsvDesc.Texture2DArray.FirstArraySlice = i;
		dsvDesc.Texture2DArray.ArraySize = 1;

		D3D12Util::CreateShaderResourceView(
			mInitData.Device, resource, &srvDesc
			, mpDescHeap->GetCpuHandle(mhSrvs[i]));

		D3D12Util::CreateDepthStencilView(
			mInitData.Device, resource, &dsvDesc
			, mpDescHeap->GetCpuHandle(mhDsvs[i]));
	}

	return true;
}
