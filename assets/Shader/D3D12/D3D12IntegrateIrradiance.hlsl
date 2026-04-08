#ifndef __D3D12INTEGRATEIRRADIANCE_HLSL__
#define __D3D12INTEGRATEIRRADIANCE_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"
#include "./../../assets/Shader/D3D12/D3D12Samplers.hlsli"
#include "./../../assets/Shader/BRDF.hlsli"

ConstantBuffer<PassCB> cbPass : register(b0);

BRDF_IntegrateIrradiance_RootConstants(b1)

Texture2D<SwapChain::HdrMapFormat>               gi_BackBuffer  : register(t0);
Texture2D<GBuffer::AlbedoMapFormat>              gi_AlbedoMap   : register(t1);
Texture2D<GBuffer::NormalMapFormat>              gi_NormalMap   : register(t2);
Texture2D<DepthStencilBuffer::DepthBufferFormat> gi_DepthMap    : register(t3);
Texture2D<GBuffer::RMSMapFormat>                 gi_RMSMap      : register(t4);
Texture2D<GBuffer::PositionMapFormat>            gi_PositionMap : register(t5);

Texture2D<EnvironmentManager::BrdfLutMapFormat>                  gi_BrdfLutMap                : register(t6);
TextureCube<EnvironmentManager::DiffuseIrradianceCubeMapFormat>  gi_DiffuseIrradianceCubeMap  : register(t7);
TextureCube<EnvironmentManager::SpecularIrradianceCubeMapFormat> gi_SpecularIrradianceCubeMap : register(t8);

TextureCube<EnvironmentManager::DiffuseIrradianceCubeMapFormat>  gi_DiffuseIrradianceCubeMaps[32]  : register(t0, space1);
TextureCube<EnvironmentManager::SpecularIrradianceCubeMapFormat> gi_SpecularIrradianceCubeMaps[32] : register(t32, space1);

struct VertexOut {
    float4 PosH : SV_Position;
    float2 TexC : TexCoord;
};

FitToScreenVertexShader

FitToScreenMeshShader

HDR_FORMAT PS(in VertexOut pin) : SV_Target {
    const float4 PosW = gi_PositionMap.Sample(gsamLinearClamp, pin.TexC);
    const float3 Radiance = gi_BackBuffer.Sample(gsamLinearClamp, pin.TexC).rgb;

    if (!GBuffer::IsValidPosition(PosW)) return float4(Radiance, 1.f);

    const float3 NormalW = normalize(gi_NormalMap.Sample(gsamLinearClamp, pin.TexC).xyz);

    const float4 Albedo = gi_AlbedoMap.Sample(gsamLinearClamp, pin.TexC);
    const float3 ViewW = normalize(cbPass.EyePosW - PosW.xyz);
    
    const float3 RoughnessMetalnessSpecualr = gi_RMSMap.Sample(gsamLinearClamp, pin.TexC);
    
    const float Roughness = saturate(RoughnessMetalnessSpecualr.r);
    const float Metalness = saturate(RoughnessMetalnessSpecualr.g);
    const float Specular = saturate(RoughnessMetalnessSpecualr.b);

    const float3 FresnelR0 = lerp((float3)0.08 * Specular, Albedo.rgb, Metalness);

    const float3 ToLightW = reflect(-ViewW, NormalW);

    //const float3 PrefilteredColor = gi_SpecularIrradianceCubeMap.SampleLevel(
    //    gsamLinearClamp, ToLightW, 
    //    Roughness * (float)(5 - 1)).rgb;
    const float3 PrefilteredColor = gi_SpecularIrradianceCubeMaps[0].SampleLevel(
        gsamLinearClamp, ToLightW, 
        Roughness * (float)(5 - 1)).rgb;

    const float NdotV = saturate(dot(NormalW, ViewW));
    
    const float3 kS = FresnelSchlickRoughness(saturate(dot(NormalW, ViewW)), FresnelR0, Roughness);
    float3 kD = 1.f - kS;
    kD *= (1.f - Metalness);
    
    const float2 Brdf = gi_BrdfLutMap.Sample(gsamLinearClamp, float2(NdotV, Roughness));
    const float3 SpecularBias = (kS * Brdf.x + Brdf.y);
    
    //const float4 Reflection = gi_ReflectionMap.Sample(gsamLinearClamp, pin.TexC);
    const float4 Reflection = (float4) 0.f;
    const float Alpha = Reflection.a;

    const float3 SpecularIrradiance = (1.f - Alpha) * PrefilteredColor + Alpha * Reflection.rgb;

    float ao = 1.f;
    //if (gAoEnabled) {
    //    const float AOValue = gi_AOMap.SampleLevel(gsamLinearClamp, pin.TexC, 0);
    //    if (AOValue != ShadingConvention::SSAO::InvalidAOValue) ao = AOValue;
    //}
                                                                                                                                                                                                                                                                                            
    //const float3 DiffuseIrradiance = gi_DiffuseIrradianceCubeMap.SampleLevel(gsamLinearClamp, NormalW, 0).rgb;
    const float3 DiffuseIrradiance = gi_DiffuseIrradianceCubeMaps[0].SampleLevel(gsamLinearClamp, NormalW, 0).rgb;
    const float3 AmbientDiffuse = kD * Albedo.rgb * DiffuseIrradiance * ao;
    const float3 AmbientSpecular = SpecularBias * SpecularIrradiance * ao;
    const float3 AmbientLight = AmbientDiffuse + AmbientSpecular;

    return float4(Radiance + AmbientLight, 1.f);
}

#endif // __D3D12INTEGRATEIRRADIANCE_HLSL__