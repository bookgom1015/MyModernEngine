#ifndef __D3D12SSAO_HLSL__
#define __D3D12SSAO_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"
#include "./../../assets/Shader/D3D12/D3D12AmbientOcclusion.hlsli"

ConstantBuffer<AmbientOcclusionCB>          cbAO                : register(b0);

SSAO_Default_RootConstants(b1);

Texture2D<GBuffer::NormalDepthMapFormat>    gi_NormalDepthMap   : register(t0);
Texture2D<GBuffer::PositionMapFormat>       gi_PositionMap      : register(t1);

RWTexture2D<Ssao::AOCoefficientMapFormat>   go_AOMap            : register(u0);
RWTexture2D<Svgf::RayHitDistanceMapFormat>  go_RayHitDistMap    : register(u1);

#define MaxSampleCount 14

[numthreads(
    Ssao::ThreadGroup::Default::Width,
    Ssao::ThreadGroup::Default::Height,
    Ssao::ThreadGroup::Default::Depth)]
void CS(in uint2 DTid : SV_DispatchThreadID) {
    const float2 TexC = (DTid + 0.5f) * gInvTexDim;

    const float4 PosW = gi_PositionMap.SampleLevel(gsamPointClamp, TexC, 0);
    if (!GBuffer::IsValidPosition(PosW)) {
        go_AOMap[DTid] = Ssao::InvalidAOValue;
        return;
    }

    const uint NormalDepth = gi_NormalDepthMap[DTid];

    float3 normalW;
    float dump;
    ValuePackaging::DecodeNormalDepth(NormalDepth, normalW, dump);

    // 🔹 Pixel 기반 stable seed
    uint seed = Random::InitRand(
        DTid.x + DTid.y * cbAO.TextureDim.x,
        cbAO.FrameCount
    );

    float occlusionSum = 0.f;

    float weights[MaxSampleCount];
    float dists[MaxSampleCount];
    float weightSum = 0.f;

    [loop]
    for (uint i = 0; i < cbAO.SampleCount; ++i) {
        // 🔹 샘플별 seed (stable + frame variation)
        seed = Random::InitRand(seed, i);

        // cosine-weighted hemisphere sampling
        const float3 dir = Random::CosHemisphereSample(seed, normalW);

        // 🔹 radius (biased sampling)
        float r = Random::Random01inclusive(seed);
        r = r * r; // near 집중 (optional, quality ↑)
        const float radius = r * cbAO.OcclusionRadius;

        const float3 samplePos = PosW.xyz + dir * radius;

        const float4 samplePosV = mul(float4(samplePos, 1.f), cbAO.View);

        float4 projPos = mul(samplePosV, cbAO.ProjTex);
        projPos /= projPos.w;

        const float4 PosW_ = gi_PositionMap.SampleLevel(gsamPointClamp, projPos.xy, 0);
        if (!GBuffer::IsValidPosition(PosW_))
            continue;

        const float dist = distance(PosW.xyz, PosW_.xyz);
        const float3 v = normalize(PosW_.xyz - PosW.xyz);

        const float NdotV = max(dot(normalW, v), 0.f);

        const float occlusion =
            NdotV *
            AmbientOcclusion::OcclusionFunction(
                dist,
                cbAO.SurfaceEpsilon,
                cbAO.OcclusionFadeStart,
                cbAO.OcclusionFadeEnd
            );

        weights[i] = occlusion;
        dists[i] = dist;

        weightSum += occlusion;
        occlusionSum += occlusion;
    }

    float dist = 100.f;

    if (weightSum > 1e-6) {
        dist = 0.f;

        [loop]
        for (uint i = 0; i < cbAO.SampleCount; ++i)
            dist += weights[i] / weightSum * dists[i];
    }

    occlusionSum /= cbAO.SampleCount;

    const float access = 1.f - occlusionSum;

    go_AOMap[DTid] = saturate(pow(access, cbAO.OcclusionStrength));
    go_RayHitDistMap[DTid] = dist;
}

#endif // __D3D12SSAO_HLSL__