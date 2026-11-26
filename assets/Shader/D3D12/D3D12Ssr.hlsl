#ifndef __D3D12_SSR_HLSL__
#define __D3D12_SSR_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"

ConstantBuffer<PassCB>                  cbPass  : register(b0);
ConstantBuffer<ScreenSpaceReflectionCB> cbSsr   : register(b1);

Texture2D<HDR_FORMAT>                               gi_BackBuffer   : register(t0);
Texture2D<GBuffer::PositionMapFormat>               gi_Position     : register(t1);
Texture2D<GBuffer::NormalMapFormat>                 gi_Normal       : register(t2);
Texture2D<DepthStencilBuffer::DepthBufferFormat>    gi_Depth        : register(t3);
Texture2D<GBuffer::RMSMapFormat>                    gi_RMS          : register(t4);

FitToScreenViewPosVertexOut

FitToScreenViewPosVertexShader(cbPass.InvProj)

FitToScreenViewPosMeshShader(cbPass.InvProj)

// 뷰 공간 레이를 원근 보정이 가능한 화면 공간 선분으로 변환한다.
struct SsrRay {
    float2 StartUv;
    float2 EndUv;
    float StartInvW;
    float EndInvW;
    float StartDepthOverW;
    float EndDepthOverW;
};

float2 ProjectToSsrUv(float4 clipPos) {
    // G-buffer를 그릴 때 사용한 TAA 지터를 동일하게 적용한다.
    const float2 ndc = clipPos.xy / clipPos.w + cbPass.JitteredOffset;
    return ndc * float2(0.5f, -0.5f) + 0.5f;
}

float GetSsrViewportExit(float start, float end) {
    if (end < 0.f)
        return start / (start - end);
    if (end > 1.f)
        return (1.f - start) / (end - start);

    return 1.f;
}

bool BuildSsrRay(float2 sourceUv, float3 startV, float3 directionV, float bias,
    inout float rayLength, out SsrRay ray) {
    ray = (SsrRay)0;

    // 근평면을 통과하는 레이도 카메라 앞에 남은 구간은 추적한다.
    const float nearZ = ShaderUtil::NdcDepthToViewDepth(0.f, cbPass.Proj);
    const float farZ = ShaderUtil::NdcDepthToViewDepth(1.f, cbPass.Proj);
    const float startZ = startV.z;
    if (startZ < nearZ || startZ >= farZ)
        return false;

    if (directionV.z < 0.f)
        rayLength = min(rayLength, (nearZ - startZ) / directionV.z);
    if (directionV.z > 0.f)
        rayLength = min(rayLength, (farZ - startZ) / directionV.z);
    if (rayLength <= 0.f)
        return false;

    const float3 endV = startV + directionV * rayLength;
    const float4 startH = mul(float4(startV, 1.f), cbPass.Proj);
    const float4 endH = mul(float4(endV, 1.f), cbPass.Proj);

    // SSR은 화면에 보이는 표면의 픽셀에서 출발한다.
    ray.StartUv = sourceUv;
    ray.EndUv = ProjectToSsrUv(endH);
    ray.StartInvW = rcp(startH.w);
    ray.EndInvW = rcp(endH.w);
    ray.StartDepthOverW = (startZ - bias) * ray.StartInvW;
    ray.EndDepthOverW = (endV.z - bias) * ray.EndInvW;

    const float exitT = min(GetSsrViewportExit(ray.StartUv.x, ray.EndUv.x),
                            GetSsrViewportExit(ray.StartUv.y, ray.EndUv.y));
    const float clippedInvW = lerp(ray.StartInvW, ray.EndInvW, exitT);
    rayLength *= exitT * ray.EndInvW / clippedInvW;
    ray.EndUv = lerp(ray.StartUv, ray.EndUv, exitT);
    ray.EndDepthOverW = lerp(ray.StartDepthOverW, ray.EndDepthOverW, exitT);
    ray.EndInvW = clippedInvW;

    return rayLength > 0.f;
}

float SampleSsrDepth(float2 uv) {
    return gi_Depth.SampleLevel(gsamPointClamp, uv, 0);
}

float3 SampleSsrRadiance(float2 hitUv, uint2 size, float radius, float thickness) {
    const float3 center = gi_BackBuffer.SampleLevel(gsamLinearClamp, hitUv, 0).rgb;
    if (radius < 1.f)
        return center;

    const float hitDepth = ShaderUtil::NdcDepthToViewDepth(SampleSsrDepth(hitUv), cbPass.Proj);
    const float depthTolerance = max(thickness, hitDepth * 0.02f);
    const float2 texelSize = rcp(float2(size));
    // 원판 내부의 두 반경에서 입사광을 샘플링한다.
    static const float2 offsets[8] = {
        float2(0.5f, 0.f), float2(-0.5f, 0.f), float2(0.f, 0.5f), float2(0.f, -0.5f),
        float2(0.7071f, 0.7071f), float2(-0.7071f, 0.7071f),
        float2(0.7071f, -0.7071f), float2(-0.7071f, -0.7071f)
    };

    float3 radiance = center;
    float weightSum = 1.f;
    [unroll]
    for (uint i = 0; i < 8; ++i) {
        const float2 sampleUv = hitUv + offsets[i] * radius * texelSize;
        if (any(sampleUv < 0.f) || any(sampleUv >= 1.f))
            continue;

        const float sampleDepth = SampleSsrDepth(sampleUv);
        if (sampleDepth == DepthStencilBuffer::InvalidDepthValue)
            continue;

        const float sampleViewDepth = ShaderUtil::NdcDepthToViewDepth(sampleDepth, cbPass.Proj);
        // 깊이 경계 너머의 색이 반사에 섞이는 것을 줄인다.
        const float weight = saturate(1.f - abs(sampleViewDepth - hitDepth) / depthTolerance);
        radiance += weight * gi_BackBuffer.SampleLevel(gsamPointClamp, sampleUv, 0).rgb;
        weightSum += weight;
    }

    return lerp(center, radiance / weightSum, smoothstep(1.f, 2.f, radius));
}

float SampleSsrDepthDelta(SsrRay ray, float t, int2 sourcePixel, uint2 depthSize) {
    const float2 uv = lerp(ray.StartUv, ray.EndUv, t);
    const int2 pixel = min(int2(uv * depthSize), int2(depthSize) - 1);
    if (all(pixel == sourcePixel))
        return -1.f;

    const float sceneDepth = gi_Depth.Load(int3(pixel, 0));
    if (sceneDepth == DepthStencilBuffer::InvalidDepthValue)
        return -1.f;

    // 화면 공간에서는 z/w와 1/w를 보간한 뒤 나누어 뷰 깊이를 복원한다.
    const float rayDepth = lerp(ray.StartDepthOverW, ray.EndDepthOverW, t) /
                           lerp(ray.StartInvW, ray.EndInvW, t);
    return rayDepth - ShaderUtil::NdcDepthToViewDepth(sceneDepth, cbPass.Proj);
}

HDR_FORMAT TraceSsr(float2 sourceUv, float3 posV, bool viewSpace, float rayLength, float thickness) {
    if (rayLength <= 0.f || thickness <= 0.f)
        return 0;

    const float3 normalW = gi_Normal.SampleLevel(gsamPointClamp, sourceUv, 0).xyz;
    const float3 normalV = mul(normalW, (float3x3)cbPass.View);
    if (dot(normalV, normalV) <= 1e-6f)
        return 0;

    const float3 directionV = reflect(normalize(posV), normalize(normalV));
    const float bias = min(0.05f, thickness * 0.1f);
    SsrRay ray;
    if (!BuildSsrRay(sourceUv, posV, directionV, bias, rayLength, ray))
        return 0;

    uint2 depthSize;
    gi_Depth.GetDimensions(depthSize.x, depthSize.y);
    const float2 pixelSpan = abs(ray.EndUv - ray.StartUv) * depthSize;
    const float dominantSpan = max(pixelSpan.x, pixelSpan.y);
    if (dominantSpan < 1.f)
        return 0;

    const float resolution = saturate(cbSsr.Resolution);
    if (!viewSpace && resolution <= 0.f)
        return 0;

    // 뷰 경로는 월드 단위, 화면 경로는 주축의 픽셀 간격을 기준으로 전진한다.
    const float sampleCount = viewSpace ? rayLength / cbSsr.RayLength : dominantSpan * resolution;
    const uint maxSteps = viewSpace ? uint(cbSsr.StepCount) : 64u;
    const uint stepCount = uint(min(ceil(sampleCount), float(maxSteps)));
    const int2 sourcePixel = int2(sourceUv * depthSize);

    float previousT = 0.f;
    float previousDelta = 0.f;
    [loop]
    for (uint i = 1; i <= stepCount; ++i) {
        float t = float(i) / stepCount;
        if (viewSpace) {
            // 뷰 공간 균등 간격을 투영된 선분의 보간 계수로 바꾼다.
            t = t * ray.StartInvW / lerp(ray.EndInvW, ray.StartInvW, t);
        }

        const float delta = SampleSsrDepthDelta(ray, t, sourcePixel, depthSize);
        if (previousDelta <= 0.f && delta > 0.f) {
            float frontT = previousT;
            float backT = t;
            float backDelta = delta;

            // 앞/뒤 구간을 유지하면서 표면 뒤쪽에서 가장 가까운 샘플을 찾는다.
            [loop]
            for (int j = 0; j < cbSsr.BackStepCount; ++j) {
                const float middleT = (frontT + backT) * 0.5f;
                const float middleDelta = SampleSsrDepthDelta(ray, middleT, sourcePixel, depthSize);
                if (middleDelta > 0.f) {
                    backT = middleT;
                    backDelta = middleDelta;
                } else {
                    frontT = middleT;
                }
            }

            // 두께 검사는 정제된 교차점에 적용해 깊이 불연속에서의 오검출을 줄인다.
            if (backDelta <= thickness) {
                const float2 hitUv = lerp(ray.StartUv, ray.EndUv, backT);
                const float roughness = saturate(gi_RMS.SampleLevel(gsamPointClamp, sourceUv, 0).r);
                const float hitInvW = lerp(ray.StartInvW, ray.EndInvW, backT);
                const float hitDistance = rayLength * backT * ray.EndInvW / hitInvW;
                // 거칠기와 히트 거리로 반사 원뿔의 폭을 근사하고 픽셀 반경으로 투영한다.
                const float coneRadius = hitDistance * roughness * roughness * 0.25f;
                const float pixelsPerViewUnit = 0.5f * depthSize.y * abs(cbPass.Proj[1][1]) * hitInvW;
                const float filterRadius = min(coneRadius * pixelsPerViewUnit, 24.f);
                const float3 color = SampleSsrRadiance(hitUv, depthSize, filterRadius, thickness);

                const float2 edgeDistance = min(hitUv, 1.f - hitUv);
                const float confidence = smoothstep(0.f, 0.05f, min(edgeDistance.x, edgeDistance.y));
                return float4(color, confidence);
            }
        }

        previousT = t;
        previousDelta = delta;
    }

    return 0;
}

HDR_FORMAT PS_View(in VertexOut pin) : SV_Target {
    if (cbSsr.RayLength <= 0.f || cbSsr.StepCount <= 0)
        return 0;

    const float depth = SampleSsrDepth(pin.TexC);
    if (depth == DepthStencilBuffer::InvalidDepthValue)
        return 0;

    // 깊이 버퍼의 좌표에서 TAA 지터를 제거한 뒤 뷰 공간 위치를 복원한다.
    const float2 ndc = pin.TexC * float2(2.f, -2.f) + float2(-1.f, 1.f);
    const float4 posH = mul(float4(ndc - cbPass.JitteredOffset, depth, 1.f), cbPass.InvProj);
    const float3 posV = posH.xyz / posH.w;
    if (posV.z > cbSsr.MaxDistance)
        return 0;

    return TraceSsr(pin.TexC, posV, true, cbSsr.RayLength * cbSsr.StepCount, cbSsr.DepthThreshold);
}

HDR_FORMAT PS_Screen(in VertexOut pin) : SV_Target {
    const float4 posW = gi_Position.SampleLevel(gsamPointClamp, pin.TexC, 0);
    if (!GBuffer::IsValidPosition(posW))
        return 0;

    const float3 posV = mul(posW, cbPass.View).xyz;
    
    return TraceSsr(pin.TexC, posV, false, cbSsr.MaxDistance, cbSsr.Thickness);
}

#endif // __D3D12_SSR_HLSL__
