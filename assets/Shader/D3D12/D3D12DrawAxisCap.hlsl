#ifndef __D3D12DRAWAXISLINE_HLSL__
#define __D3D12DRAWAXISLINE_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"

ConstantBuffer<GizmoCB> cbGizmo : register(b0);

struct VertexOut {
    float4 PosH : SV_POSITION;
    float2 UV   : TEXCOORD0; // 원 마스킹용
    uint InstID : INSTANCE_ID;
};

// 축 끝 위치
static const float3 gAxisEnd[3] = {
    float3(0.25f, 0.0f, 0.0f), // X
    float3(0.0f, 0.25f, 0.0f), // Y
    float3(0.0f, 0.0f, 0.25f) // Z
};

// quad용 로컬 좌표 (-1~1)
static const float2 gQuad[6] = {
    float2(-1.0f, -1.0f),
    float2(-1.0f, 1.0f),
    float2(1.0f, 1.0f),

    float2(-1.0f, -1.0f),
    float2(1.0f, 1.0f),
    float2(1.0f, -1.0f)
};

VertexOut VS(in uint vid : SV_VertexID, in uint instanceID : SV_InstanceID)
{
    VertexOut vout = (VertexOut) 0;

    float3 centerW = gAxisEnd[instanceID];
    float2 local = gQuad[vid];

    // 카메라 billboard basis
    // 보통 InvView의 0,1행(또는 열)을 right/up으로 사용
    // 네 매트릭스 저장 방식에 따라 row/column이 뒤집히면 아래 둘을 바꿔줘.
    float3 camRightW = normalize(cbGizmo.InvView[0].xyz);
    float3 camUpW = normalize(cbGizmo.InvView[1].xyz);

    // 원판 반지름 (월드 기준)
    float radius = 0.03f;

    float3 posW = centerW
                + camRightW * local.x * radius
                + camUpW * local.y * radius;

    vout.PosH = mul(float4(posW, 1.0f), cbGizmo.UnitViewProj);

    // local -1~1 -> UV 그대로 사용
    vout.UV = local;
    vout.InstID = instanceID;

    return vout;
}

float4 PS(VertexOut pin) : SV_Target {
    // 원 마스크
    float dist = dot(pin.UV, pin.UV);
    if (dist > 1.f) discard;

    // 가장자리 부드럽게 하고 싶으면 아래처럼 alpha 적용 가능
    // float alpha = saturate((1.0f - dist) * 8.0f);

    if (pin.InstID == 0)        // X
        return float4(1.f, 0.f, 0.f, 1.f);
    else if (pin.InstID == 1)   // Y
        return float4(0.f, 1.f, 0.f, 1.f);
    else                        // Z
        return float4(0.f, 0.f, 1.f, 1.f); 
}

#endif // __D3D12DRAWAXISLINE_HLSL__