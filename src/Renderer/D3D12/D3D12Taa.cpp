#include "pch.h"
#include "Renderer/D3D12/D3D12Taa.hpp"

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
	const WCHAR* const HLSL_TAA = L"D3D12Taa.hlsl";
}

D3D12Taa::D3D12Taa() {}

D3D12Taa::~D3D12Taa() {}

bool D3D12Taa::Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	mInitData = *reinterpret_cast<InitData*>(pData);

	mHistoryMap = std::make_unique<GpuResource>();

	CheckReturn(BuildResources());

	for (size_t i = 0; i < HaltonSequenceSize; ++i) {
		auto offset = mHaltonSequence[i];
		mFittedToBakcBufferHaltonSequence[i] = Vec2(
			((offset.x - 0.5f) / mInitData.Width) * 2.f,
			((offset.y - 0.5f) / mInitData.Height) * 2.f);
	}

	return true;
}

bool D3D12Taa::CompileShaders() {
	const auto VS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_TAA, L"VS", L"vs_6_5");
	const auto MS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_TAA, L"MS", L"ms_6_5");
	const auto PS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_TAA, L"PS", L"ps_6_5");
	CheckReturn(mInitData.ShaderManager->AddShader(VS, mShaderHashes[Taa::Shader::VS_Taa]));
	CheckReturn(mInitData.ShaderManager->AddShader(MS, mShaderHashes[Taa::Shader::MS_Taa]));
	CheckReturn(mInitData.ShaderManager->AddShader(PS, mShaderHashes[Taa::Shader::PS_Taa]));

	return true;
}

bool D3D12Taa::BuildRootSignatures() {
	decltype(auto) samplers = D3D12Util::GetStaticSamplers();

	CD3DX12_DESCRIPTOR_RANGE texTables[3]{}; UINT index = 0;
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
	texTables[index++].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);

	index = 0;

	CD3DX12_ROOT_PARAMETER slotRootParameter[Taa::RootSignature::Default::Count]{};
	slotRootParameter[Taa::RootSignature::Default::RC_Consts].InitAsConstants(
		Taa::RootConstant::Default::Count, 0);
	slotRootParameter[Taa::RootSignature::Default::SI_BackBuffer].
		InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[Taa::RootSignature::Default::SI_HistoryMap].
		InitAsDescriptorTable(1, &texTables[index++]);
	slotRootParameter[Taa::RootSignature::Default::SI_VelocityMap].
		InitAsDescriptorTable(1, &texTables[index++]);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		D3D12Util::StaticSamplerCount, samplers,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	CheckReturn(D3D12Util::CreateRootSignature(
		mInitData.Device,
		rootSigDesc,
		IID_PPV_ARGS(&mRootSignature),
		L"TAA_GR_Default"));

	return true;
}

bool D3D12Taa::BuildPipelineStates() {
	if (mInitData.Device->IsMeshShaderSupported()) {
		auto psoDesc = D3D12Util::FitToScreenMeshPsoDesc();
		psoDesc.pRootSignature = mRootSignature.Get();
		{
			const auto MS = mInitData.ShaderManager->GetShader(mShaderHashes[Taa::Shader::MS_Taa]);
			NullCheck(MS);
			const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Taa::Shader::PS_Taa]);
			NullCheck(PS);
			psoDesc.MS = { reinterpret_cast<BYTE*>(MS->GetBufferPointer()), MS->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.RTVFormats[0] = HDR_FORMAT;

		CheckReturn(D3D12Util::CreatePipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[Taa::PipelineState::MP_Taa]),
			L"TAA_MP_Default"));
	}
	else {
		auto psoDesc = D3D12Util::FitToScreenPsoDesc();
		psoDesc.pRootSignature = mRootSignature.Get();
		{
			const auto VS = mInitData.ShaderManager->GetShader(mShaderHashes[Taa::Shader::VS_Taa]);
			NullCheck(VS);
			const auto PS = mInitData.ShaderManager->GetShader(mShaderHashes[Taa::Shader::PS_Taa]);
			NullCheck(PS);
			psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.RTVFormats[0] = HDR_FORMAT;

		CheckReturn(D3D12Util::CreateGraphicsPipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[Taa::PipelineState::GP_Taa]),
			L"TAA_GP_Default"));
	}

	return true;
}

bool D3D12Taa::AllocateDescriptors() {
	CheckReturn(mpDescHeap->AllocateCbvSrvUav(1, mhHistoryMapSrv));

	CheckReturn(BuildDescriptors());

	return true;
}

bool D3D12Taa::OnResize(unsigned width, unsigned height) {
	mInitData.Width = width;
	mInitData.Height = height;

	CheckReturn(BuildResources());
	CheckReturn(BuildDescriptors());

	for (size_t i = 0; i < HaltonSequenceSize; ++i) {
		auto offset = mHaltonSequence[i];
		mFittedToBakcBufferHaltonSequence[i] = Vec2(
			((offset.x - 0.5f) / mInitData.Width) * 2.f,
			((offset.y - 0.5f) / mInitData.Height) * 2.f);
	}

	return true;
}

bool D3D12Taa::ApplyTAA(
	D3D12FrameResource* const pFrameResource
	, const D3D12_VIEWPORT& viewport
	, const D3D12_RECT& scissorRect
	, GpuResource* const pBackBuffer
	, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer
	, GpuResource* const pBackBufferCopy
	, D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy
	, GpuResource* const pVelocityMap
	, D3D12_GPU_DESCRIPTOR_HANDLE si_velocityMap) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[mInitData.Device->IsMeshShaderSupported() 
		? Taa::PipelineState::MP_Taa : Taa::PipelineState::GP_Taa].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetGraphicsRootSignature(mRootSignature.Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_DEST);

		CmdList->CopyResource(pBackBufferCopy->Resource(), pBackBuffer->Resource());

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		pBackBufferCopy->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		mHistoryMap->Transite(CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, nullptr);

		Taa::RootConstant::Default::Struct rc;
		rc.gModulationFactor = SHADER_ARGUMENT_MANAGER->TAA.ModulationFactor;
		rc.gInvTexDim.x = 1.f / static_cast<FLOAT>(mInitData.Width);
		rc.gInvTexDim.y = 1.f / static_cast<FLOAT>(mInitData.Height);

		D3D12Util::SetRoot32BitConstants<Taa::RootConstant::Default::Struct>(
			Taa::RootSignature::Default::RC_Consts,
			Taa::RootConstant::Default::Count,
			&rc,
			0,
			CmdList,
			FALSE);

		CmdList->SetGraphicsRootDescriptorTable(
			Taa::RootSignature::Default::SI_BackBuffer, si_backBufferCopy);
		CmdList->SetGraphicsRootDescriptorTable(
			Taa::RootSignature::Default::SI_HistoryMap, mpDescHeap->GetGpuHandle(mhHistoryMapSrv));
		CmdList->SetGraphicsRootDescriptorTable(
			Taa::RootSignature::Default::SI_VelocityMap, si_velocityMap);

		if (mInitData.Device->IsMeshShaderSupported()) {
			CmdList->DispatchMesh(1, 1, 1);
		}
		else {
			CmdList->IASetVertexBuffers(0, 0, nullptr);
			CmdList->IASetIndexBuffer(nullptr);
			CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			CmdList->DrawInstanced(6, 1, 0, 0);
		}

		mHistoryMap->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_DEST);
		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);

		CmdList->CopyResource(mHistoryMap->Resource(), pBackBuffer->Resource());
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12Taa::BuildResources() {
	D3D12_RESOURCE_DESC rscDesc = {};
	rscDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	rscDesc.Format = HDR_FORMAT;
	rscDesc.Alignment = 0;
	rscDesc.Width = mInitData.Width;
	rscDesc.Height = mInitData.Height;
	rscDesc.DepthOrArraySize = 1;
	rscDesc.MipLevels = 1;
	rscDesc.SampleDesc.Count = 1;
	rscDesc.SampleDesc.Quality = 0;
	rscDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rscDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	const auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CheckReturn(mHistoryMap->Initialize(
		mInitData.Device,
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&rscDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		L"TAA_HistoryMap"));

	return true;
}

bool D3D12Taa::BuildDescriptors() {
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = HDR_FORMAT;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.f;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12Util::CreateShaderResourceView(
		mInitData.Device, 
		mHistoryMap->Resource(), 
		&srvDesc, 
		mpDescHeap->GetCpuHandle(mhHistoryMapSrv));

	return true;
}