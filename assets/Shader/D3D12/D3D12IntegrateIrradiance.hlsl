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

StructuredBuffer<ReflectionProbeMetaData> gi_ReflectionProbes   : register(t0, space2);

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

struct ProbeInfluenceResult {
    bool  Valid;
    float Weight;      // 최종 가중치
    float InnerFactor; // 중심에 가까울수록 1, 경계로 갈수록 0
    float Distance;    // 비교용
    uint  Priority;
    uint  IBLIndex;
};

struct SelectedProbeResult {
    bool  Found;
    uint  Count;
    uint  IBLIndex[4];
    float Weight[4];
};

struct ProbeCandidate {
    bool  Valid;
    uint  IBLIndex;
    uint  Priority;
    float Weight;
    float Distance;
};


FitToScreenVertexShader

FitToScreenMeshShader

float3 TransformPosition(float4x4 m, float3 p) {
    return mul(float4(p, 1.0f), m).xyz;
}

ProbeInfluenceResult EvaluateSphereProbe(ReflectionProbeMetaData probe, float3 worldPos) {
    ProbeInfluenceResult r = (ProbeInfluenceResult)0;

    float3 localPos = TransformPosition(probe.InvWorld, worldPos);
    float dist = length(localPos);

    if (dist > probe.Radius)
        return r;

    float blend = max(probe.BlendDistance, 1e-4f);

    // 경계 근처에서 부드럽게 감소
    float innerRadius = max(probe.Radius - blend, 0.0f);
    float innerFactor = 1.0f;

    if (dist > innerRadius) {
        float t = saturate((probe.Radius - dist) / blend);
        innerFactor = t;
    }

    r.Valid       = true;
    r.Weight      = innerFactor;
    r.InnerFactor = innerFactor;
    r.Distance    = dist;
    r.Priority    = probe.Priority;
    r.IBLIndex    = probe.IBLIndex;
    
    return r;
}

ProbeInfluenceResult EvaluateBoxProbe(ReflectionProbeMetaData probe, float3 worldPos) {
    ProbeInfluenceResult r = (ProbeInfluenceResult)0;

    float3 localPos = TransformPosition(probe.InvWorld, worldPos);
    float3 q = abs(localPos);

    float blend = max(probe.BlendDistance, 1e-4f);

    float3 inner = probe.BoxExtents;
    float3 outer = probe.BoxExtents + blend;

    // outer 바깥이면 영향 없음
    if (q.x > outer.x || q.y > outer.y || q.z > outer.z)
        return r;

    float weight = 1.0f;

    // inner 밖에 있는 축들만 감소
    float3 fade = (outer - q) / blend;
    fade = saturate(fade);

    if (q.x > inner.x) weight *= fade.x;
    if (q.y > inner.y) weight *= fade.y;
    if (q.z > inner.z) weight *= fade.z;

    float centerDist = length(localPos);

    r.Valid       = true;
    r.Weight      = weight;
    r.InnerFactor = weight;
    r.Distance    = centerDist;
    r.Priority    = probe.Priority;
    r.IBLIndex    = probe.IBLIndex;

    return r;
}

ProbeInfluenceResult EvaluateProbe(ReflectionProbeMetaData probe, float3 worldPos) {
    if (probe.Shape == EnvironmentManager::ProbeShape_Sphere)
        return EvaluateSphereProbe(probe, worldPos);
    else if (probe.Shape == EnvironmentManager::ProbeShape_Box)
        return EvaluateBoxProbe(probe, worldPos);

    ProbeInfluenceResult r = (ProbeInfluenceResult)0;
    return r;
}

bool IsBetterProbeCandidate(ProbeCandidate a, ProbeCandidate b) {
    if (!b.Valid) return true;

    if (a.Priority != b.Priority)
        return a.Priority > b.Priority;

    if (a.Weight != b.Weight)
        return a.Weight > b.Weight;

    return a.Distance < b.Distance;
}

void ResetProbeCandidate(out ProbeCandidate c) {
    c.Valid    = false;
    c.IBLIndex = 0;
    c.Priority = 0;
    c.Weight   = 0.0f;
    c.Distance = 1e30f;
}

void InsertProbeCandidate(inout ProbeCandidate top[4], ProbeCandidate c) {
    [unroll]
    for (uint i = 0; i < 4; ++i) {
        if (IsBetterProbeCandidate(c, top[i])) {
            [unroll]
            for (uint j = 3; j > i; --j) {
                top[j] = top[j - 1];
            }

            top[i] = c;
            break;
        }
    }
}

SelectedProbeResult SelectReflectionProbes(float3 worldPos) {
    SelectedProbeResult outResult = (SelectedProbeResult)0;
    outResult.Found = false;
    outResult.Count = 0;

    ProbeCandidate top[4];
    [unroll]
    for (uint i = 0; i < 4; ++i) {
        ResetProbeCandidate(top[i]);
        outResult.IBLIndex[i] = 0;
        outResult.Weight[i]   = 0.f;
    }

    bool hasAnyValid = false;
    uint highestPriority = 0;

    [unroll]
    for (uint i = 0; i < 32; ++i) {
        ReflectionProbeMetaData probe = gi_ReflectionProbes[i];
        if ((probe.Flags & 1u) == 0) continue;

        ProbeInfluenceResult r = EvaluateProbe(probe, worldPos);
        if (!r.Valid || r.Weight <= 0.01f) continue;

        ProbeCandidate c;
        c.Valid    = true;
        c.IBLIndex = r.IBLIndex;
        c.Priority = r.Priority;
        c.Weight   = r.Weight;
        c.Distance = r.Distance;

        if (!hasAnyValid) {
            hasAnyValid = true;
            highestPriority = c.Priority;
            InsertProbeCandidate(top, c);
            continue;
        }

        if (c.Priority > highestPriority) {
            highestPriority = c.Priority;

            [unroll]
            for (uint k = 0; k < 4; ++k) {
                ResetProbeCandidate(top[k]);
            }

            InsertProbeCandidate(top, c);
            continue;
        }

        if (c.Priority < highestPriority)
            continue;

        InsertProbeCandidate(top, c);
    }

    float weightSum = 0.f;

    [unroll]
    for (uint i = 0; i < 4; ++i) {
        if (!top[i].Valid) continue;

        outResult.IBLIndex[outResult.Count] = top[i].IBLIndex;
        outResult.Weight[outResult.Count]   = top[i].Weight;
        weightSum += top[i].Weight;
        outResult.Count++;
    }

    if (outResult.Count > 0) {
        outResult.Found = true;

        if (weightSum > 1e-4f) {
            [unroll]
            for (uint i = 0; i < 4; ++i) {
                if (i >= outResult.Count) break;
                outResult.Weight[i] /= weightSum;
            }
        }
    }

    return outResult;
}

float3 SampleProbeDiffuse(uint iblIndex, float3 normalW) {
    return gi_DiffuseIrradianceCubeMaps[iblIndex].Sample(gsamLinearClamp, normalW).rgb;
}

float3 SampleProbeSpecular(uint iblIndex, float3 reflectDirW, float roughness, float maxMip) {
    float mip = roughness * (maxMip - 1);
    return gi_SpecularIrradianceCubeMaps[iblIndex].SampleLevel(gsamLinearClamp, reflectDirW, mip).rgb;
}

float3 SampleBlendedProbeDiffuse(SelectedProbeResult selected, float3 normalW, float3 globalDiffuse) {
    float3 diffuseIBL = 0.f;

    if (!selected.Found)
        return globalDiffuse;

    float probeWeightSum = 0.f;

    [unroll]
    for (uint i = 0; i < 4; ++i) {
        if (i >= selected.Count) break;

        const float w = selected.Weight[i];
        const uint iblIndex = selected.IBLIndex[i];

        diffuseIBL += SampleProbeDiffuse(iblIndex, normalW) * w;
        probeWeightSum += w;
    }

    // 혹시 정규화 후에도 총합이 1보다 작아지는 상황을 대비한 fallback
    return lerp(globalDiffuse, diffuseIBL, saturate(probeWeightSum));
}

float3 SampleBlendedProbeSpecular(SelectedProbeResult selected, float3 reflectDirW, float roughness, float maxMip, float3 globalSpecular) {
    float3 specularIBL = 0.f;

    if (!selected.Found)
        return globalSpecular;

    float probeWeightSum = 0.f;

    [unroll]
    for (uint i = 0; i < 4; ++i) {
        if (i >= selected.Count) break;

        const float w = selected.Weight[i];
        const uint iblIndex = selected.IBLIndex[i];

        specularIBL += SampleProbeSpecular(iblIndex, reflectDirW, roughness, maxMip) * w;
        probeWeightSum += w;
    }

    return lerp(globalSpecular, specularIBL, saturate(probeWeightSum));
}

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
    
    SelectedProbeResult selected = SelectReflectionProbes(PosW.xyz);

    //const float3 GlobalDiffuse = gi_DiffuseIrradianceCubeMap.Sample(gsamLinearClamp, NormalW).rgb;
    //const float3 GlobalSpecular = gi_SpecularIrradianceCubeMap.SampleLevel(gsamLinearClamp, ToLightW, Roughness * (float)(5 - 1)).rgb;
    const float3 GlobalDiffuse = (float3)0;
    const float3 GlobalSpecular = (float3)0;
    
    float3 diffuseIBL  = SampleBlendedProbeDiffuse(selected, NormalW, GlobalDiffuse);
    float3 specularIBL = SampleBlendedProbeSpecular(selected, ToLightW, Roughness, 5.0f, GlobalSpecular);
    
    const float NdotV = saturate(dot(NormalW, ViewW));
    
    const float3 kS = FresnelSchlickRoughness(saturate(dot(NormalW, ViewW)), FresnelR0, Roughness);
    float3 kD = 1.f - kS;
    kD *= (1.f - Metalness);
    
    const float2 Brdf = gi_BrdfLutMap.Sample(gsamLinearClamp, float2(NdotV, Roughness));
    const float3 SpecularBias = (kS * Brdf.x + Brdf.y);
    
    //const float4 Reflection = gi_ReflectionMap.Sample(gsamLinearClamp, pin.TexC);
    const float4 Reflection = (float4) 0.f;
    const float Alpha = Reflection.a;

    const float3 SpecularIrradiance = (1.f - Alpha) * specularIBL + Alpha * Reflection.rgb;

    float ao = 1.f;
    //if (gAoEnabled) {
    //    const float AOValue = gi_AOMap.SampleLevel(gsamLinearClamp, pin.TexC, 0);
    //    if (AOValue != ShadingConvention::SSAO::InvalidAOValue) ao = AOValue;
    //}
                                                                                                                    
    const float3 AmbientDiffuse = kD * Albedo.rgb * diffuseIBL * ao;
    const float3 AmbientSpecular = SpecularBias * SpecularIrradiance * ao;
    const float3 AmbientLight = AmbientDiffuse + AmbientSpecular;

    return float4(Radiance + AmbientLight, 1.f);
}

#endif // __D3D12INTEGRATEIRRADIANCE_HLSL__