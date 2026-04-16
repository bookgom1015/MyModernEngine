#ifndef __D3D12REFLECTIONPROBE_HLSL__
#define __D3D12REFLECTIONPROBE_HLSL__

struct ProbeInfluenceResult {
    bool  Valid;
    float Weight;      // raw influence
    float InnerFactor;
    float Distance;
    uint  Priority;
    uint  IBLIndex;
    uint  ProbeIndex;
};

struct SelectedProbeResult {
    bool  Found;
    uint  Count;
    uint  IBLIndex[4];
    uint  ProbeIndex[4];
    float Weight[4];          // normalized weights for probe blending
    float TotalRawWeight;     // global fallback blend factor
};

struct ProbeCandidate {
    bool  Valid;
    uint  IBLIndex;
    uint  ProbeIndex;
    uint  Priority;
    float Weight;
    float Distance;
};

float3 GetProbeCenterWS(ReflectionProbeMetaData probe) {
    return mul(float4(0.f, 0.f, 0.f, 1.f), probe.World).xyz;
}

float3x3 GetProbeRotationOnly(float4x4 world) {
    float3x3 rot = (float3x3)world;

    rot[0] = normalize(rot[0]);
    rot[1] = normalize(rot[1]);
    rot[2] = normalize(rot[2]);

    return rot;
}

float3 ComputeBoxProjectedReflectDir(
    ReflectionProbeMetaData probe,
    float3 worldPos,
    float3 reflectDirW)
{
    // world -> probe local
    float3 localPos = ShaderUtil::TransformPosition(probe.InvWorld, worldPos);
    float3 localDir = ShaderUtil::SafeNormalize(
        ShaderUtil::TransformDirection(probe.InvWorld, reflectDirW));

    const float3 boxMin = -probe.BoxExtents;
    const float3 boxMax =  probe.BoxExtents;

    float3 invDir;
    invDir.x = (abs(localDir.x) > 1e-6f) ? rcp(localDir.x) : 1e30f;
    invDir.y = (abs(localDir.y) > 1e-6f) ? rcp(localDir.y) : 1e30f;
    invDir.z = (abs(localDir.z) > 1e-6f) ? rcp(localDir.z) : 1e30f;

    float3 tMin3 = (boxMin - localPos) * invDir;
    float3 tMax3 = (boxMax - localPos) * invDir;

    float3 t1 = min(tMin3, tMax3);
    float3 t2 = max(tMin3, tMax3);

    // assume shading point is inside the probe influence volume
    float tExit = min(t2.x, min(t2.y, t2.z));
    tExit = max(tExit, 0.f);

    float3 hitLocal = localPos + localDir * tExit;

    // IMPORTANT:
    // cubemap lookup direction should not be distorted by probe scale.
    float3x3 probeRot = GetProbeRotationOnly(probe.World);
    float3 sampleDirW = mul(hitLocal, probeRot);

    return ShaderUtil::SafeNormalize(sampleDirW);
}

ProbeInfluenceResult EvaluateSphereProbe(
    ReflectionProbeMetaData probe,
    uint probeIndex,
    float3 worldPos)
{
    ProbeInfluenceResult r = (ProbeInfluenceResult)0;

    float3 localPos = ShaderUtil::TransformPosition(probe.InvWorld, worldPos);
    float dist = length(localPos);

    if (dist > probe.Radius)
        return r;

    float blend = max(probe.BlendDistance, 1e-4f);

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
    r.ProbeIndex  = probeIndex;

    return r;
}

ProbeInfluenceResult EvaluateBoxProbe(
    ReflectionProbeMetaData probe,
    uint probeIndex,
    float3 worldPos)
{
    ProbeInfluenceResult r = (ProbeInfluenceResult)0;

    float3 localPos = ShaderUtil::TransformPosition(probe.InvWorld, worldPos);
    float3 q = abs(localPos);

    float blend = max(probe.BlendDistance, 1e-4f);

    // NOTE:
    // this assumes probe.BoxExtents is in probe local space.
    float3 inner = probe.BoxExtents;
    float3 outer = probe.BoxExtents + blend;

    if (q.x > outer.x || q.y > outer.y || q.z > outer.z)
        return r;

    float weight = 1.0f;

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
    r.ProbeIndex  = probeIndex;

    return r;
}

ProbeInfluenceResult EvaluateProbe(
    ReflectionProbeMetaData probe,
    uint probeIndex,
    float3 worldPos)
{
    if (probe.Shape == EnvironmentManager::ProbeShape_Sphere)
        return EvaluateSphereProbe(probe, probeIndex, worldPos);
    else if (probe.Shape == EnvironmentManager::ProbeShape_Box)
        return EvaluateBoxProbe(probe, probeIndex, worldPos);

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
    c.Valid      = false;
    c.IBLIndex   = 0;
    c.ProbeIndex = 0;
    c.Priority   = 0;
    c.Weight     = 0.0f;
    c.Distance   = 1e30f;
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
    outResult.TotalRawWeight = 0.f;

    ProbeCandidate top[4];
    [unroll]
    for (uint i = 0; i < 4; ++i) {
        ResetProbeCandidate(top[i]);
        outResult.IBLIndex[i]   = 0;
        outResult.ProbeIndex[i] = 0;
        outResult.Weight[i]     = 0.f;
    }

    bool hasAnyValid = false;
    uint highestPriority = 0;

    [unroll]
    for (uint i = 0; i < 32; ++i) {
        ReflectionProbeMetaData probe = gi_ReflectionProbes[i];
        if ((probe.Flags & 1u) == 0) continue;

        ProbeInfluenceResult r = EvaluateProbe(probe, i, worldPos);
        if (!r.Valid || r.Weight <= 0.01f) continue;

        ProbeCandidate c;
        c.Valid      = true;
        c.IBLIndex   = r.IBLIndex;
        c.ProbeIndex = r.ProbeIndex;
        c.Priority   = r.Priority;
        c.Weight     = r.Weight;
        c.Distance   = r.Distance;

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

    float rawWeightSum = 0.f;

    [unroll]
    for (uint i = 0; i < 4; ++i) {
        if (!top[i].Valid) continue;

        outResult.IBLIndex[outResult.Count]   = top[i].IBLIndex;
        outResult.ProbeIndex[outResult.Count] = top[i].ProbeIndex;
        outResult.Weight[outResult.Count]     = top[i].Weight;
        rawWeightSum += top[i].Weight;
        outResult.Count++;
    }

    if (outResult.Count > 0) {
        outResult.Found = true;
        outResult.TotalRawWeight = saturate(rawWeightSum);

        if (rawWeightSum > 1e-4f) {
            [unroll]
            for (uint i = 0; i < 4; ++i) {
                if (i >= outResult.Count) break;
                outResult.Weight[i] /= rawWeightSum;
            }
        }
    }

    return outResult;
}

float3 SampleProbeDiffuse(uint iblIndex, float3 normalW) {
    return gi_DiffuseIrradianceCubeMaps[iblIndex].Sample(gsamLinearClamp, normalW).rgb;
}

float3 SampleProbeSpecular(
    ReflectionProbeMetaData probe,
    float3 worldPos,
    float3 reflectDirW,
    float roughness,
    float maxMip)
{
    float3 sampleDir = reflectDirW;

    if (probe.Shape == EnvironmentManager::ProbeShape_Box &&
        probe.UseBoxProjection != 0)
    {
        sampleDir = ComputeBoxProjectedReflectDir(probe, worldPos, reflectDirW);
    }

    float mip = roughness * (maxMip - 1);
    return gi_SpecularIrradianceCubeMaps[probe.IBLIndex]
        .SampleLevel(gsamLinearClamp, sampleDir, mip).rgb;
}

float3 SampleBlendedProbeDiffuse(
    SelectedProbeResult selected,
    float3 normalW,
    float3 globalDiffuse)
{
    float3 diffuseIBL = 0.f;

    if (!selected.Found)
        return globalDiffuse;

    [unroll]
    for (uint i = 0; i < 4; ++i) {
        if (i >= selected.Count) break;

        const float w = selected.Weight[i];
        const uint iblIndex = selected.IBLIndex[i];

        diffuseIBL += SampleProbeDiffuse(iblIndex, normalW) * w;
    }

    return lerp(globalDiffuse, diffuseIBL, selected.TotalRawWeight);
}

float3 SampleBlendedProbeSpecular(
    SelectedProbeResult selected,
    float3 worldPos,
    float3 reflectDirW,
    float roughness,
    float maxMip,
    float3 globalSpecular)
{
    float3 specularIBL = 0.f;

    if (!selected.Found)
        return globalSpecular;

    [unroll]
    for (uint i = 0; i < 4; ++i) {
        if (i >= selected.Count) break;

        const float w = selected.Weight[i];
        const uint probeIndex = selected.ProbeIndex[i];

        ReflectionProbeMetaData probe = gi_ReflectionProbes[probeIndex];
        float3 probeSpecular = SampleProbeSpecular(
            probe,
            worldPos,
            reflectDirW,
            roughness,
            maxMip);

        specularIBL += probeSpecular * w;
    }

    return lerp(globalSpecular, specularIBL, selected.TotalRawWeight);
}

#endif // __D3D12REFLECTIONPROBE_HLSL__