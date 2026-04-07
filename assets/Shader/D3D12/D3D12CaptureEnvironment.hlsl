#ifndef __D3D12CAPTUREENVIRONMENT_HLSL__
#define __D3D12CAPTUREENVIRONMENT_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"
#include "./../../assets/Shader/LightingUtil.hlsli"

#include "./../../assets/Shader/D3D12/D3D12Shadow.hlsli"

ConstantBuffer<PassCB>          cbPass          : register(b0);
ConstantBuffer<ProjectToCubeCB> cbProjectToCube : register(b1);
ConstantBuffer<LightCB>         cbLight         : register(b2);
ConstantBuffer<ObjectCB>        cbObject        : register(b3);
ConstantBuffer<MaterialCB>      cbMaterial      : register(b4);

EnvironmentManager_CaptureEnvironment_RootConstants(b5)

Texture2DArray<float>           gi_ShadowMap    : register(t0);

Texture2D<float4>               gi_AlbedoMap    : register(t0, space1);
Texture2D<float4>               gi_NormalMap    : register(t1, space1);

VERTEX_IN

struct VertexOut {
    float3 PosL        : POSITION0;
    float3 NormalL     : NORMAL0;
    float4 TangentL    : TANGENT0;
    float2 TexC        : TEXCOORD;
};

struct GeoOut {
    float4 PosH     : SV_Position;
    float3 PosW     : POSITION0;
    float3 NormalW  : NORMAL0;   
    float4 TangentW : TANGENT0;
    float2 TexC     : TEXCOORD0;
    uint ArrayIndex : SV_RenderTargetArrayIndex;
};

VertexOut VS(in VertexIn vin) {
    VertexOut vout = (VertexOut)0;

    vout.PosL     = vin.PosL;
    vout.NormalL  = vin.NormalL;
    vout.TangentL = vin.TangentL;
    vout.TexC     = vin.TexC;

    return vout;
}

[maxvertexcount(18)]
void GS(triangle VertexOut gin[3], inout TriangleStream<GeoOut> triStream) {
    [unroll]
    for (uint face = 0; face < 6; ++face) {
        GeoOut gout[3];

        [unroll]
        for (uint i = 0; i < 3; ++i) {            
            float3 posW = mul(float4(gin[i].PosL, 1.f), cbObject.World).xyz;
            float3 normalW = mul(gin[i].NormalL, (float3x3)cbObject.World);
            float4 tangentW = float4(mul(gin[i].TangentL.xyz, (float3x3)cbObject.World), gin[i].TangentL.w);

            gout[i].PosW       = posW;
            gout[i].NormalW    = normalW;
            gout[i].TangentW   = tangentW;
            gout[i].ArrayIndex = face;
            gout[i].TexC       = gin[i].TexC;
            
            float4 posV = mul(float4(posW, 1.f), cbProjectToCube.Views[face]);
            gout[i].PosH = mul(posV, cbProjectToCube.Proj);
        }

        triStream.Append(gout[0]);
        triStream.Append(gout[1]);
        triStream.Append(gout[2]);
        triStream.RestartStrip();
    }
}

HDR_FORMAT PS(in GeoOut pin) : SV_Target {
    // 필요하면 알파 테스트 / 디더링 / 거리 기반 clip을 여기에 추가

    // -----------------------------
    // Material inputs
    // -----------------------------
    float4 albedo = float4(cbMaterial.Albedo, 1.0f);

    float2 texc = pin.TexC;
    texc.y = 1.0f - texc.y;

    if (gHasAlbedoMap) 
        albedo *= gi_AlbedoMap.SampleLevel(gsamLinearClamp, texc, 0);

    float roughness = saturate(cbMaterial.Roughness);
    float metalness = saturate(cbMaterial.Metalness);
    float specular  = saturate(cbMaterial.Specular);

    // 필요하면 RMS map도 여기서 직접 샘플링 가능
    // 예:
    // if (gHasRMSMap)
    // {
    //     float3 rms = gi_RMSMap.SampleLevel(gsamLinearClamp, texc, 0).rgb;
    //     roughness *= saturate(rms.r);
    //     metalness *= saturate(rms.g);
    //     specular  *= saturate(rms.b);
    // }

    // -----------------------------
    // Normal
    // -----------------------------
    float3 normalW = normalize(pin.NormalW);

    if (gHasNormalMap) {
        float3 normalTS = gi_NormalMap.SampleLevel(gsamLinearClamp, texc, 0).xyz;
        normalTS = normalTS * 2.0f - 1.0f;
        normalTS.y = -normalTS.y;

        float3 N = normalize(pin.NormalW);
        float3 T = normalize(pin.TangentW.xyz);
        T = normalize(T - N * dot(T, N));
        float3 B = cross(N, T) * pin.TangentW.w;

        float3x3 TBN = float3x3(T, B, N);

        normalW = normalize(mul(normalTS, TBN));
    }

    // -----------------------------
    // F0 / Material
    // -----------------------------
    const float3 FresnelR0 = lerp((float3)0.08f * specular, albedo.rgb, metalness);
    Material mat = { albedo, FresnelR0, roughness, metalness };

    // -----------------------------
    // Shadows
    // -----------------------------
    float shadowFactors[MAX_LIGHT_COUNT];
    [unroll]
    for (uint i = 0; i < MAX_LIGHT_COUNT; ++i)
        shadowFactors[i] = 1.0f;

    [unroll]
    for (uint i = 0; i < MAX_LIGHT_COUNT; ++i) {
        const LightData light = cbLight.Lights[i];
        shadowFactors[i] = CalcShadowPCF(light, gi_ShadowMap, gsamShadow, pin.PosW.xyz, gInvTexDim);
    }

    // -----------------------------
    // Lighting
    // -----------------------------
    const float3 viewW = normalize(cbPass.EyePosW - pin.PosW.xyz);
    const float3 radiance = ComputeBRDF(
        cbLight.Lights, mat, pin.PosW.xyz, normalW, viewW, shadowFactors, cbLight.LightCount);

    return float4(radiance, 1.f);
}

#endif // __D3D12CAPTUREENVIRONMENT_HLSL__