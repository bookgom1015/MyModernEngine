#include "pch.h"
#include "Renderer/D3D12/D3D12Gizmo.hpp"

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
	const WCHAR* const HLSL_DrawAxisLine = L"D3D12DrawAxisLine.hlsl";
	const WCHAR* const HLSL_DrawAxisCap = L"D3D12DrawAxisCap.hlsl";
}

D3D12Gizmo::D3D12Gizmo() {}

D3D12Gizmo::~D3D12Gizmo() {}

bool D3D12Gizmo::Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	mInitData = *reinterpret_cast<InitData*>(pData);

	float quarterWidth = mInitData.Width * 0.25f;
	float quarterHeight = mInitData.Height * 0.25f;

	float leftTopX = mInitData.Width * 0.75f;

	mDrawAxisLineViewport = { 
		mInitData.Width - quarterWidth,  
		quarterHeight * 0.25f,
		quarterWidth, 
		quarterHeight,
		0.f, 
		1.f };
	mDrawAxisLineScissorRect = { 
		0, 0, static_cast<LONG>(mInitData.Width), static_cast<LONG>(mInitData.Height) };

	return true;
}


bool D3D12Gizmo::CompileShaders() {
	// DrawAxisLine
	{
		const auto VS = D3D12ShaderManager::D3D12ShaderInfo(
			HLSL_DrawAxisLine, L"VS", L"vs_6_5");
		const auto PS = D3D12ShaderManager::D3D12ShaderInfo(
			HLSL_DrawAxisLine, L"PS", L"ps_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			VS, mShaderHashes[Gizmo::Shader::VS_DrawAxisLine]));
		CheckReturn(mInitData.ShaderManager->AddShader(
			PS, mShaderHashes[Gizmo::Shader::PS_DrawAxisLine]));
	}
	// DrawAxisCap
	{
		const auto VS = D3D12ShaderManager::D3D12ShaderInfo(
			HLSL_DrawAxisCap, L"VS", L"vs_6_5");
		const auto PS = D3D12ShaderManager::D3D12ShaderInfo(
			HLSL_DrawAxisCap, L"PS", L"ps_6_5");
		CheckReturn(mInitData.ShaderManager->AddShader(
			VS, mShaderHashes[Gizmo::Shader::VS_DrawAxisCap]));
		CheckReturn(mInitData.ShaderManager->AddShader(
			PS, mShaderHashes[Gizmo::Shader::PS_DrawAxisCap]));
	}

	return true;
}

bool D3D12Gizmo::BuildRootSignatures() {
	decltype(auto) samplers = D3D12Util::GetStaticSamplers();

	CD3DX12_ROOT_PARAMETER slotRootParameter[Gizmo::RootSignature::DrawAxisLine::Count]{};
	slotRootParameter[Gizmo::RootSignature::DrawAxisLine::CB_Gizmo]
		.InitAsConstantBufferView(0);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		D3D12Util::StaticSamplerCount, samplers,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	CheckReturn(D3D12Util::CreateRootSignature(
		mInitData.Device,
		rootSigDesc,
		IID_PPV_ARGS(&mRootSignatures[Gizmo::RootSignature::GR_DrawAxisLine]),
		L"Gizmo_GR_DrawAxisLine"));

	return true;
}

bool D3D12Gizmo::BuildPipelineStates() {
	// DrawAxisLine
	{
		auto psoDesc = D3D12Util::FitToScreenPsoDesc();
		psoDesc.pRootSignature = mRootSignatures[Gizmo::RootSignature::GR_DrawAxisLine].Get();
		{
			const auto VS = mInitData.ShaderManager->GetShader(
				mShaderHashes[Gizmo::Shader::VS_DrawAxisLine]);
			NullCheck(VS);
			const auto PS = mInitData.ShaderManager->GetShader(
				mShaderHashes[Gizmo::Shader::PS_DrawAxisLine]);
			NullCheck(PS);
			psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.RTVFormats[0] = SDR_FORMAT;
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DSVFormat = DepthStencilBuffer::DepthStencilBufferFormat;

		CheckReturn(D3D12Util::CreateGraphicsPipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[Gizmo::PipelineState::GP_DrawAxisLine]),
			L"Gizmo_GP_DrawAxisLine"));
	}
	// DrawAxisCap
	{
		auto psoDesc = D3D12Util::FitToScreenPsoDesc();
		psoDesc.pRootSignature = mRootSignatures[Gizmo::RootSignature::GR_DrawAxisLine].Get();
		{
			const auto VS = mInitData.ShaderManager->GetShader(
				mShaderHashes[Gizmo::Shader::VS_DrawAxisCap]);
			NullCheck(VS);
			const auto PS = mInitData.ShaderManager->GetShader(
				mShaderHashes[Gizmo::Shader::PS_DrawAxisCap]);
			NullCheck(PS);
			psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
			psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
		}
		psoDesc.RTVFormats[0] = SDR_FORMAT;
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DSVFormat = DepthStencilBuffer::DepthStencilBufferFormat;

		CheckReturn(D3D12Util::CreateGraphicsPipelineState(
			mInitData.Device,
			psoDesc,
			IID_PPV_ARGS(&mPipelineStates[Gizmo::PipelineState::GP_DrawAxisCap]),
			L"Gizmo_GP_DrawAxisCap"));
	}

	return true;
}

bool D3D12Gizmo::DrawAxisLine(
	D3D12FrameResource* const pFrameResource
	, GpuResource* const pBackBuffer
	, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer
	, GpuResource* const pDepthBuffer
	,	D3D12_CPU_DESCRIPTOR_HANDLE dio_depthBuffer) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->CommandAllocator(),
		mPipelineStates[Gizmo::PipelineState::GP_DrawAxisLine].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetGraphicsRootSignature(mRootSignatures[Gizmo::RootSignature::GR_DrawAxisLine].Get());

		CmdList->RSSetViewports(1, &mDrawAxisLineViewport);
		CmdList->RSSetScissorRects(1, &mDrawAxisLineScissorRect);

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		pDepthBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, &dio_depthBuffer);

		CmdList->SetGraphicsRootConstantBufferView(
			Gizmo::RootSignature::DrawAxisLine::CB_Gizmo, pFrameResource->GizmoCB.CBAddress());

		CmdList->IASetVertexBuffers(0, 0, nullptr);
		CmdList->IASetIndexBuffer(nullptr);
		CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		CmdList->DrawInstanced(6, 3, 0, 0);
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}

bool D3D12Gizmo::DrawAxisCap(
	D3D12FrameResource* const pFrameResource
	, GpuResource* const pBackBuffer
	, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer
	, GpuResource* const pDepthBuffer
	, D3D12_CPU_DESCRIPTOR_HANDLE dio_depthBuffer) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->CommandAllocator(),
		mPipelineStates[Gizmo::PipelineState::GP_DrawAxisCap].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetGraphicsRootSignature(mRootSignatures[Gizmo::RootSignature::GR_DrawAxisLine].Get());

		CmdList->RSSetViewports(1, &mDrawAxisLineViewport);
		CmdList->RSSetScissorRects(1, &mDrawAxisLineScissorRect);

		pBackBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		pDepthBuffer->Transite(CmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, &dio_depthBuffer);

		CmdList->SetGraphicsRootConstantBufferView(
			Gizmo::RootSignature::DrawAxisLine::CB_Gizmo, pFrameResource->GizmoCB.CBAddress());

		CmdList->IASetVertexBuffers(0, 0, nullptr);
		CmdList->IASetIndexBuffer(nullptr);
		CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		CmdList->DrawInstanced(6, 3, 0, 0);
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}