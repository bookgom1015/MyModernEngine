#include "pch.h"
#include "Renderer/D3D12/D3D12Debug.hpp"

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
	const WCHAR* const HLSL_DrawDebugLine = L"D3D12DrawDebugLine.hlsl";
}

D3D12Debug::D3D12Debug() {}

D3D12Debug::~D3D12Debug() {}

bool D3D12Debug::Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	mInitData = *reinterpret_cast<InitData*>(pData);

	return true;
}

bool D3D12Debug::CompileShaders() {
	const auto VS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_DrawDebugLine, L"VS", L"vs_6_5");
	const auto PS = D3D12ShaderManager::D3D12ShaderInfo(HLSL_DrawDebugLine, L"PS", L"ps_6_5");
	CheckReturn(mInitData.ShaderManager->AddShader(
		VS, mShaderHashes[Debug::Shader::VS_DrawDebugLine]));
	CheckReturn(mInitData.ShaderManager->AddShader(
		PS, mShaderHashes[Debug::Shader::PS_DrawDebugLine]));

	return true;
}

bool D3D12Debug::BuildRootSignatures() {
	decltype(auto) samplers = D3D12Util::GetStaticSamplers();

	CD3DX12_ROOT_PARAMETER slotRootParameter[Debug::RootSignature::Count]{};
	slotRootParameter[Debug::RootSignature::DrawDebugLine::CB_Pass]
		.InitAsConstantBufferView(0);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		D3D12Util::StaticSamplerCount, samplers,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	CheckReturn(D3D12Util::CreateRootSignature(
		mInitData.Device,
		rootSigDesc,
		IID_PPV_ARGS(&mRootSignatures[Debug::RootSignature::GR_DrawDebugLine]),
		L"Debug_GR_DrawDebugLine"));

	return true;
}

bool D3D12Debug::BuildPipelineStates() {
	D3D12_INPUT_ELEMENT_DESC debugLineInputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,	 0, 0,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",	  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	const D3D12_INPUT_LAYOUT_DESC debugLineInputLayoutDesc = { 
		debugLineInputLayout, static_cast<UINT>(_countof(debugLineInputLayout)) };

	auto psoDesc = D3D12Util::DefaultPsoDesc(
		debugLineInputLayoutDesc, DepthStencilBuffer::DepthStencilBufferFormat);
	psoDesc.pRootSignature = mRootSignatures[Debug::RootSignature::GR_DrawDebugLine].Get();
	{
		const auto VS = mInitData.ShaderManager->GetShader(
			mShaderHashes[Debug::Shader::VS_DrawDebugLine]);
		NullCheck(VS);
		const auto PS = mInitData.ShaderManager->GetShader(
			mShaderHashes[Debug::Shader::PS_DrawDebugLine]);
		NullCheck(PS);

		psoDesc.VS = { reinterpret_cast<BYTE*>(VS->GetBufferPointer()), VS->GetBufferSize() };
		psoDesc.PS = { reinterpret_cast<BYTE*>(PS->GetBufferPointer()), PS->GetBufferSize() };
	}
	psoDesc.NumRenderTargets = 1;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	psoDesc.RTVFormats[0] = HDR_FORMAT;
	// 디버그 라인은 깊이 테스트는 하되 깊이 버퍼는 쓰지 않음.
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; 

	CheckReturn(D3D12Util::CreateGraphicsPipelineState(
		mInitData.Device,
		psoDesc,
		IID_PPV_ARGS(&mPipelineStates[Debug::PipelineState::GP_DrawDebugLine]),
		L"Debug_GP_DrawDebugLine"));

	return true;
}

void D3D12Debug::AddDebugLine(const Vec3& start, const Vec3& end, const Vec4& color) {
	mDebugLineVertices.push_back({ start, color });
	mDebugLineVertices.push_back({ end, color });
}

void D3D12Debug::AddWireBox(const Mat4& world, const Vec3& extents, const Vec4& color) {
	Vec3 corners[8] = {
		Vec3(-extents.x, -extents.y, -extents.z),
		Vec3( extents.x, -extents.y, -extents.z),
		Vec3( extents.x,  extents.y, -extents.z),
		Vec3(-extents.x,  extents.y, -extents.z),

		Vec3(-extents.x, -extents.y,  extents.z),
		Vec3( extents.x, -extents.y,  extents.z),
		Vec3( extents.x,  extents.y,  extents.z),
		Vec3(-extents.x,  extents.y,  extents.z),
	};

	for (int i = 0; i < 8; ++i)
		corners[i] = Vec3::Transform(corners[i], world);

	// 아래 면
	AddDebugLine(corners[0], corners[1], color);
	AddDebugLine(corners[1], corners[2], color);
	AddDebugLine(corners[2], corners[3], color);
	AddDebugLine(corners[3], corners[0], color);

	// 위 면
	AddDebugLine(corners[4], corners[5], color);
	AddDebugLine(corners[5], corners[6], color);
	AddDebugLine(corners[6], corners[7], color);
	AddDebugLine(corners[7], corners[4], color);

	// 세로 연결
	AddDebugLine(corners[0], corners[4], color);
	AddDebugLine(corners[1], corners[5], color);
	AddDebugLine(corners[2], corners[6], color);
	AddDebugLine(corners[3], corners[7], color);
}

void D3D12Debug::AddWireSphere(const Mat4& world, float radius, const Vec4& color, UINT segments) {
	const float step = TwoPI / static_cast<float>(segments);

	// XY plane
	for (UINT i = 0; i < segments; ++i) {
		float a0 = step * i;
		float a1 = step * (i + 1);

		Vec3 p0(radius * cosf(a0), radius * sinf(a0), 0.f);
		Vec3 p1(radius * cosf(a1), radius * sinf(a1), 0.f);

		p0 = Vec3::Transform(p0, world);
		p1 = Vec3::Transform(p1, world);

		AddDebugLine(p0, p1, color);
	}

	// XZ plane
	for (UINT i = 0; i < segments; ++i) {
		float a0 = step * i;
		float a1 = step * (i + 1);

		Vec3 p0(radius * cosf(a0), 0.f, radius * sinf(a0));
		Vec3 p1(radius * cosf(a1), 0.f, radius * sinf(a1));

		p0 = Vec3::Transform(p0, world);
		p1 = Vec3::Transform(p1, world);

		AddDebugLine(p0, p1, color);
	}

	// YZ plane
	for (UINT i = 0; i < segments; ++i) {
		float a0 = step * i;
		float a1 = step * (i + 1);

		Vec3 p0(0.f, radius * cosf(a0), radius * sinf(a0));
		Vec3 p1(0.f, radius * cosf(a1), radius * sinf(a1));

		p0 = Vec3::Transform(p0, world);
		p1 = Vec3::Transform(p1, world);

		AddDebugLine(p0, p1, color);
	}
}

void D3D12Debug::AddDebugCross(const Mat4& world, float size, const Vec4& color) {
	Vec3 origin = Vec3::Transform(Vec3(0.f), world);

	Vec3 x0 = Vec3::Transform(Vec3(-size, 0.f, 0.f), world);
	Vec3 x1 = Vec3::Transform(Vec3(size, 0.f, 0.f), world);

	Vec3 y0 = Vec3::Transform(Vec3(0.f, -size, 0.f), world);
	Vec3 y1 = Vec3::Transform(Vec3(0.f, size, 0.f), world);

	Vec3 z0 = Vec3::Transform(Vec3(0.f, 0.f, -size), world);
	Vec3 z1 = Vec3::Transform(Vec3(0.f, 0.f, size), world);

	AddDebugLine(x0, x1, color);
	AddDebugLine(y0, y1, color);
	AddDebugLine(z0, z1, color);
}

void D3D12Debug::AddDebugArrow(const Mat4& world, float length, const Vec4& color) {
	Vec3 origin = Vec3::Transform(Vec3(0.f), world);

	// forward 방향
	Vec3 forward = Vec3::TransformNormal(Vec3(0.f, 0.f, 1.f), world);

	Vec3 tip = origin + forward * length;

	// 몸통
	AddDebugLine(origin, tip, color);
	forward.Normalize();

	// 화살촉 만들기 위해 right / up 필요
	Vec3 up = Vec3::TransformNormal(Vec3(0, 1, 0), world);
	up.Normalize();

	Vec3 right = forward.Cross(up);
	right.Normalize();

	const float headSize = length * 0.2f;

	Vec3 headBase = tip - forward * headSize;

	Vec3 wing1 = headBase + right * (headSize * 0.5f);
	Vec3 wing2 = headBase - right * (headSize * 0.5f);

	// 화살촉
	AddDebugLine(tip, wing1, color);
	AddDebugLine(tip, wing2, color);
}

void D3D12Debug::AddDebugBasis(const Mat4& world, float size) {
	Vec3 origin = Vec3::Transform(Vec3(0.f), world);

	Vec3 x = Vec3::Transform(Vec3(size, 0.f, 0.f), world);
	Vec3 y = Vec3::Transform(Vec3(0.f, size, 0.f), world);
	Vec3 z = Vec3::Transform(Vec3(0.f, 0.f, size), world);

	AddDebugLine(origin, x, Vec4(1.f, 0.f, 0.f, 1.f)); // X (red)
	AddDebugLine(origin, y, Vec4(0.f, 1.f, 0.f, 1.f)); // Y (green)
	AddDebugLine(origin, z, Vec4(0.f, 0.f, 1.f, 1.f)); // Z (blue)
}

void D3D12Debug::BuildReflectionProbeDebugLines(const ReflectionProbeDesc& desc) {
	if (!desc.Enabled) return;

	Vec4 mainColor;
	Vec4 blendColor;

	if (desc.BakeState == EProbeBakeState::E_Dirty) {
		mainColor = Vec4(1.f, 0.3f, 0.2f, 1.f);   // 빨강
		blendColor = Vec4(1.f, 0.6f, 0.4f, 1.f);
	}
	else {
		mainColor = desc.UseBoxProjection
			? Vec4(0.2f, 0.9f, 1.f, 1.f)          // 청록
			: Vec4(0.3f, 1.f, 0.3f, 1.f);         // 초록

		blendColor = desc.UseBoxProjection
			? Vec4(0.5f, 0.9f, 1.f, 1.f)
			: Vec4(0.6f, 1.f, 0.6f, 1.f);
	}

	switch (desc.Shape) {
	case EProbeShape::E_Box: {
		AddWireBox(desc.World, desc.BoxExtents, mainColor);

		if (desc.BlendDistance > 0.f) {
			Vec3 blendExtents = desc.BoxExtents + Vec3(desc.BlendDistance);
			AddWireBox(desc.World, blendExtents, blendColor);
		}

		break;
	}
	case EProbeShape::E_Sphere:	{
		AddWireSphere(desc.World, desc.Radius, mainColor);

		if (desc.BlendDistance > 0.f) 
			AddWireSphere(desc.World, desc.Radius + desc.BlendDistance, blendColor);

		break;
	}
	default:
		break;
	}

	AddDebugArrow(desc.World, 0.75f, Vec4(1.f, 1.f, 0.2f, 1.f)); // 노랑
	AddDebugBasis(desc.World, 0.5f);
}

bool D3D12Debug::DrawDebugLines(
	D3D12FrameResource* const pFrameResource
	, D3D12_VIEWPORT viewport, D3D12_RECT scissorRect
	, GpuResource* const pBackBuffer, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer
	, GpuResource* const pDepthBuffer, D3D12_CPU_DESCRIPTOR_HANDLE di_depthBuffer) {
	CheckReturn(mInitData.CommandObject->ResetDirectCommandList(
		pFrameResource->FrameCommandAllocator(),
		mPipelineStates[Debug::PipelineState::GP_DrawDebugLine].Get()));

	const auto CmdList = mInitData.CommandObject->GetDirectCommandList();
	CheckReturn(mpDescHeap->SetDescriptorHeap(CmdList));

	{
		CmdList->SetGraphicsRootSignature(mRootSignatures[Debug::RootSignature::GR_DrawDebugLine].Get());

		CmdList->RSSetViewports(1, &viewport);
		CmdList->RSSetScissorRects(1, &scissorRect);

		pBackBuffer->Transite(
			CmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
		pDepthBuffer->Transite(
			CmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		CmdList->OMSetRenderTargets(1, &ro_backBuffer, TRUE, &di_depthBuffer);

		CmdList->SetGraphicsRootConstantBufferView(
			Debug::RootSignature::DrawDebugLine::CB_Pass,
			pFrameResource->PassCB.CBAddress());

		if (!mDebugLineVertices.empty()) {
			D3D12_VERTEX_BUFFER_VIEW vbv{};
			vbv.BufferLocation = pFrameResource->DebugLineVB.Resource()->GetGPUVirtualAddress();
			vbv.StrideInBytes = sizeof(DebugLineVertex);
			vbv.SizeInBytes = static_cast<UINT>(sizeof(DebugLineVertex) * mDebugLineVertices.size());

			CmdList->IASetVertexBuffers(0, 1, &vbv);
			CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

			CmdList->DrawInstanced(static_cast<UINT>(mDebugLineVertices.size()), 1, 0, 0);
		}
	}

	CheckReturn(mInitData.CommandObject->ExecuteDirectCommandList());

	return true;
}