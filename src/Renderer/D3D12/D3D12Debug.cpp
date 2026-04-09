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

void D3D12Debug::AddWireSphere(
	const Mat4& world
	, float radius
	, const Vec4& color
	, UINT ringCount
	, UINT segmentCount) {
	if (radius <= 0.f) return;

	// 최소값 보정
	ringCount = std::max<UINT>(ringCount, 2);       // 위/아래 극 사이 최소 2단
	segmentCount = std::max<UINT>(segmentCount, 3); // 최소 삼각형

	const float pi = PI;
	const float twoPi = TwoPI;

	// ---------------------------------------
	// 1) Latitude rings (수평 링)
	// phi: 0 ~ PI
	// phi=0   -> top pole
	// phi=PI  -> bottom pole
	// 중간 링만 그림 (극점은 링 반지름이 0이라 제외)
	// ---------------------------------------
	for (UINT ring = 1; ring < ringCount; ++ring) {
		const float v = static_cast<float>(ring) / static_cast<float>(ringCount);
		const float phi = pi * v;

		const float y = radius * cosf(phi);
		const float ringRadius = radius * sinf(phi);

		for (UINT seg = 0; seg < segmentCount; ++seg) {
			const float u0 = static_cast<float>(seg) / static_cast<float>(segmentCount);
			const float u1 = static_cast<float>(seg + 1) / static_cast<float>(segmentCount);

			const float theta0 = twoPi * u0;
			const float theta1 = twoPi * u1;

			Vec3 p0(
				ringRadius * cosf(theta0),
				y,
				ringRadius * sinf(theta0)
			);

			Vec3 p1(
				ringRadius * cosf(theta1),
				y,
				ringRadius * sinf(theta1)
			);

			p0 = Vec3::Transform(p0, world);
			p1 = Vec3::Transform(p1, world);

			AddDebugLine(p0, p1, color);
		}
	}

	// ---------------------------------------
	// 2) Longitude lines (수직 경선)
	// theta 고정, phi를 따라 위->아래로 연결
	// ---------------------------------------
	for (UINT seg = 0; seg < segmentCount; ++seg) {
		const float u = static_cast<float>(seg) / static_cast<float>(segmentCount);
		const float theta = twoPi * u;

		for (UINT ring = 0; ring < ringCount; ++ring) {
			const float v0 = static_cast<float>(ring) / static_cast<float>(ringCount);
			const float v1 = static_cast<float>(ring + 1) / static_cast<float>(ringCount);

			const float phi0 = pi * v0;
			const float phi1 = pi * v1;

			Vec3 p0(
				radius * sinf(phi0) * cosf(theta),
				radius * cosf(phi0),
				radius * sinf(phi0) * sinf(theta)
			);

			Vec3 p1(
				radius * sinf(phi1) * cosf(theta),
				radius * cosf(phi1),
				radius * sinf(phi1) * sinf(theta)
			);

			p0 = Vec3::Transform(p0, world);
			p1 = Vec3::Transform(p1, world);

			AddDebugLine(p0, p1, color);
		}
	}
}

void D3D12Debug::AddWireCapsule(const Mat4& world, float radius, float halfHeight, const Vec4& color, UINT segments) {
	if (segments < 4)
		segments = 4;

	const float step = TwoPI / static_cast<float>(segments);

	const float topY = halfHeight;
	const float bottomY = -halfHeight;

	// --------------------------------------------------
	// 1) Top / Bottom rings (XZ plane)
	// --------------------------------------------------
	for (UINT i = 0; i < segments; ++i) {
		const float a0 = step * static_cast<float>(i);
		const float a1 = step * static_cast<float>(i + 1);

		Vec3 top0(radius * cosf(a0), topY, radius * sinf(a0));
		Vec3 top1(radius * cosf(a1), topY, radius * sinf(a1));

		Vec3 bottom0(radius * cosf(a0), bottomY, radius * sinf(a0));
		Vec3 bottom1(radius * cosf(a1), bottomY, radius * sinf(a1));

		top0 = Vec3::Transform(top0, world);
		top1 = Vec3::Transform(top1, world);
		bottom0 = Vec3::Transform(bottom0, world);
		bottom1 = Vec3::Transform(bottom1, world);

		AddDebugLine(top0, top1, color);
		AddDebugLine(bottom0, bottom1, color);
	}

	// --------------------------------------------------
	// 2) Side lines
	// --------------------------------------------------
	{
		Vec3 p[8] = {
			Vec3(radius, topY,    0.f),
			Vec3(radius, bottomY, 0.f),

			Vec3(-radius, topY,    0.f),
			Vec3(-radius, bottomY, 0.f),

			Vec3(0.f, topY,     radius),
			Vec3(0.f, bottomY,  radius),

			Vec3(0.f, topY,    -radius),
			Vec3(0.f, bottomY, -radius),
		};

		for (Vec3& v : p)
			v = Vec3::Transform(v, world);

		AddDebugLine(p[0], p[1], color);
		AddDebugLine(p[2], p[3], color);
		AddDebugLine(p[4], p[5], color);
		AddDebugLine(p[6], p[7], color);
	}

	// --------------------------------------------------
	// 3) Hemispheres
	//    두 개의 수직 단면(XY, YZ)에서 반원들을 그림
	// --------------------------------------------------
	const float hemiStep = PI / static_cast<float>(segments);

	// XY plane arcs (z = 0)
	for (UINT i = 0; i < segments; ++i) {
		const float a0 = hemiStep * static_cast<float>(i);
		const float a1 = hemiStep * static_cast<float>(i + 1);

		// top hemisphere
		Vec3 t0(radius * cosf(a0), topY + radius * sinf(a0), 0.f);
		Vec3 t1(radius * cosf(a1), topY + radius * sinf(a1), 0.f);

		// bottom hemisphere
		Vec3 b0(radius * cosf(a0), bottomY - radius * sinf(a0), 0.f);
		Vec3 b1(radius * cosf(a1), bottomY - radius * sinf(a1), 0.f);

		t0 = Vec3::Transform(t0, world);
		t1 = Vec3::Transform(t1, world);
		b0 = Vec3::Transform(b0, world);
		b1 = Vec3::Transform(b1, world);

		AddDebugLine(t0, t1, color);
		AddDebugLine(b0, b1, color);

		// 반대편 XY plane arc
		Vec3 t0b(-radius * cosf(a0), topY + radius * sinf(a0), 0.f);
		Vec3 t1b(-radius * cosf(a1), topY + radius * sinf(a1), 0.f);

		Vec3 b0b(-radius * cosf(a0), bottomY - radius * sinf(a0), 0.f);
		Vec3 b1b(-radius * cosf(a1), bottomY - radius * sinf(a1), 0.f);

		t0b = Vec3::Transform(t0b, world);
		t1b = Vec3::Transform(t1b, world);
		b0b = Vec3::Transform(b0b, world);
		b1b = Vec3::Transform(b1b, world);

		AddDebugLine(t0b, t1b, color);
		AddDebugLine(b0b, b1b, color);
	}

	// YZ plane arcs (x = 0)
	for (UINT i = 0; i < segments; ++i) {
		const float a0 = hemiStep * static_cast<float>(i);
		const float a1 = hemiStep * static_cast<float>(i + 1);

		// top hemisphere
		Vec3 t0(0.f, topY + radius * sinf(a0), radius * cosf(a0));
		Vec3 t1(0.f, topY + radius * sinf(a1), radius * cosf(a1));

		// bottom hemisphere
		Vec3 b0(0.f, bottomY - radius * sinf(a0), radius * cosf(a0));
		Vec3 b1(0.f, bottomY - radius * sinf(a1), radius * cosf(a1));

		t0 = Vec3::Transform(t0, world);
		t1 = Vec3::Transform(t1, world);
		b0 = Vec3::Transform(b0, world);
		b1 = Vec3::Transform(b1, world);

		AddDebugLine(t0, t1, color);
		AddDebugLine(b0, b1, color);

		// 반대편 YZ plane arc
		Vec3 t0b(0.f, topY + radius * sinf(a0), -radius * cosf(a0));
		Vec3 t1b(0.f, topY + radius * sinf(a1), -radius * cosf(a1));

		Vec3 b0b(0.f, bottomY - radius * sinf(a0), -radius * cosf(a0));
		Vec3 b1b(0.f, bottomY - radius * sinf(a1), -radius * cosf(a1));

		t0b = Vec3::Transform(t0b, world);
		t1b = Vec3::Transform(t1b, world);
		b0b = Vec3::Transform(b0b, world);
		b1b = Vec3::Transform(b1b, world);

		AddDebugLine(t0b, t1b, color);
		AddDebugLine(b0b, b1b, color);
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