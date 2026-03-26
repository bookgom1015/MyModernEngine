#ifndef __D3D12DRAWAXISLINE_HLSL__
#define __D3D12DRAWAXISLINE_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"

ConstantBuffer<GizmoCB> cbGizmo : register(b0);

struct VertexOut {
	float4 PosH   : SV_POSITION;
	uint   InstID : INSTANCE_ID;
};

float3 GetAxisStart(uint instanceID) {
    return float3(0.0f, 0.0f, 0.0f);
}

float3 GetAxisEnd(uint instanceID) {
    if (instanceID == 0)
        return float3(1.f, 0.0f, 0.0f); // X
    else if (instanceID == 1)
        return float3(0.0f, 1.f, 0.0f); // Y
    else
        return float3(0.0f, 0.0f, 1.f); // Z
}

VertexOut VS(in uint vid : SV_VertexID, in uint instanceID : SV_InstanceID) {
    VertexOut vout = (VertexOut)0;

    float3 startW = GetAxisStart(instanceID);
    float3 endW = GetAxisEnd(instanceID);

    float4 startH = mul(float4(startW, 1.0f), cbGizmo.UnitViewProj);
    float4 endH = mul(float4(endW, 1.0f), cbGizmo.UnitViewProj);

    // NDC 좌표
    float2 startNdc = startH.xy / startH.w;
    float2 endNdc = endH.xy / endH.w;

    float2 dir = endNdc - startNdc;
    float len = length(dir);

    // 길이가 너무 짧으면 안전 처리
    if (len < 1e-6f) {
        dir = float2(1.f, 0.f);
        len = 1.f;
    }

    dir /= len;

    // 선에 수직인 방향
    float2 perp = float2(-dir.y, dir.x);

    // pixel -> NDC 변환
    // NDC는 [-1,1] 이라서 화면 절반 기준으로 2/size
    float2 pixelToNdc = float2(2.f / cbGizmo.ViewportSize.x,
                               2.f / cbGizmo.ViewportSize.y);

    float2 offsetNdc = perp * (cbGizmo.LineThickness * 0.5f) * pixelToNdc;

    // 6개 정점으로 quad 구성
    float2 outNdc;
    float outW;

    switch (vid) {
        case 0:
            outNdc = startNdc - offsetNdc;
            outW = startH.w;
            break;
        case 1:
            outNdc = startNdc + offsetNdc;
            outW = startH.w;
            break;
        case 2:
            outNdc = endNdc - offsetNdc;
            outW = endH.w;
            break;
        case 3:
            outNdc = endNdc - offsetNdc;
            outW = endH.w;
            break;
        case 4:
            outNdc = startNdc + offsetNdc;
            outW = startH.w;
            break;
        default:
            outNdc = endNdc + offsetNdc;
            outW = endH.w;
            break;
    }

    vout.PosH = float4(outNdc * outW, lerp(startH.z / startH.w, endH.z / endH.w, 0.5f) * outW, outW);
    vout.InstID = instanceID;

    return vout;
}

float4 PS(in VertexOut pin) : SV_Target {
    if (pin.InstID == 0)
        return float4(1.f, 0.f, 0.f, 1.f);
    else if (pin.InstID == 1)
        return float4(0.f, 1.f, 0.f, 1.f);
    else
        return float4(0.f, 0.f, 1.f, 1.f);
}

#endif // __D3D12DRAWAXISLINE_HLSL__