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
static const float3 gAxisEnd[4] = {
    float3(0.f, 0.f, 0.f),  // Origin
    float3(1.f, 0.f, 0.f),  // X
    float3(0.f, 1.f, 0.f),  // Y
    float3(0.f, 0.f, 1.f) // Z
};

// quad용 로컬 좌표 (-1~1)
static const float2 gQuad[6] = {
    float2(-1.f, -1.f),
    float2(-1.f,  1.f),
    float2( 1.f,  1.f),

    float2(-1.f, -1.f),
    float2( 1.f,  1.f),
    float2( 1.f, -1.f)
};

VertexOut VS(in uint vid : SV_VertexID, in uint instanceID : SV_InstanceID) {
    VertexOut vout = (VertexOut)0;

    float3 centerW = gAxisEnd[instanceID] * 0.9f;
    float2 uv = gQuad[vid]; // -1 ~ 1 그대로 유지

    float3 posW = centerW;
    
    float4 posV = mul(float4(posW, 1.f), cbGizmo.View);
    posV.xy += uv * 0.15f; // 화면에서 보이는 크기 고정
    posV.z -= 0.5f;
    
    float4 posH = mul(posV, cbGizmo.Proj);
    
    vout.PosH = posH;
    vout.UV = uv;
    vout.InstID = instanceID;

    return vout;
}

float4 PS(VertexOut pin) : SV_Target {
    float dist = dot(pin.UV, pin.UV);
    if (dist > 1.f) discard;
    
    // 가장자리 부드럽게 하고 싶으면 아래처럼 alpha 적용 가능
    // float alpha = saturate((1.0f - dist) * 8.0f);

    if (pin.InstID == 0)
        return float4(1.f, 1.f, 1.f, 1.f);
    else if (pin.InstID == 1)
        return float4(1.f, 0.f, 0.f, 1.f);
    else if (pin.InstID == 2)
        return float4(0.f, 1.f, 0.f, 1.f);
    else
        return float4(0.f, 0.f, 1.f, 1.f);
}

#endif // __D3D12DRAWAXISLINE_HLSL__