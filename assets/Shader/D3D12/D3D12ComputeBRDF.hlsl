#ifndef __D3D12COMPUTEBRDF_HLSL__
#define __D3D12COMPUTEBRDF_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"
#include "./../../assets/Shader/D3D12/D3D12Samplers.hlsli"
#include "./../../assets/Shader/LightingUtil.hlsli"

ConstantBuffer<PassCB>  cbPass  : register(b0);
ConstantBuffer<LightCB> cbLight : register(b1);

BRDF_ComputeBRDF_RootConstants(b2)

Texture2D<GBuffer::AlbedoMapFormat>              gi_AlbedoMap             : register(t0);
Texture2D<GBuffer::NormalMapFormat>              gi_NormalMap             : register(t1);
Texture2D<DepthStencilBuffer::DepthBufferFormat> gi_DepthMap              : register(t2);
Texture2D<GBuffer::SpecularMapFormat>            gi_SpecularMap           : register(t3);
Texture2D<GBuffer::RoughnessMetalnessMapFormat>  gi_RoughnessMetalnessMap : register(t4);
Texture2D<GBuffer::PositionMapFormat>            gi_PositionMap           : register(t5);

FitToScreenVertexOut
FitToScreenVertexShader
FitToScreenMeshShader

HDR_FORMAT PS(in VertexOut pin) : SV_Target {
    const float4 PosW = gi_PositionMap.Sample(gsamLinearClamp, pin.TexC);
    if (!GBuffer::IsValidPosition(PosW)) return 0;

    const float4 Albedo = gi_AlbedoMap.Sample(gsamLinearClamp, pin.TexC);
    const float3 Specular = gi_SpecularMap.Sample(gsamLinearClamp, pin.TexC).rgb;
    const float2 RoughnessMetalness = gi_RoughnessMetalnessMap.Sample(gsamLinearClamp, pin.TexC);

    const float Roughness = saturate(RoughnessMetalness.r);
    const float Metalness = saturate(RoughnessMetalness.g);

    // normal 저장 포맷이 이미 [-1, 1] 이면 이대로
    const float3 NormalW = normalize(gi_NormalMap.Sample(gsamLinearClamp, pin.TexC).xyz);

    // dielectric F0를 spec map으로 조절하고 싶다면 유지
    //const float3 FresnelR0 = lerp(0.08f * Specular, Albedo.rgb, Metalness);
    const float3 FresnelR0 = lerp(0.08f, Albedo.rgb, Metalness);

    Material mat = { Albedo, FresnelR0, Roughness, Metalness };

    float shadowFactors[MAX_LIGHT_COUNT];
    [unroll]
    for (uint i = 0; i < MAX_LIGHT_COUNT; ++i) {
        shadowFactors[i] = 1.f;
    }

    const float3 ViewW = normalize(cbPass.EyePosW - PosW.xyz);
    const float3 Radiance = ComputeBRDF(cbLight.Lights, mat, PosW.xyz, NormalW, ViewW, shadowFactors, cbLight.LightCount);

    return float4(Radiance, 1.f);
}

#endif // __D3D12COMPUTEBRDF_HLSL__