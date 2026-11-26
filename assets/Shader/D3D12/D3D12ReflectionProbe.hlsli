#ifndef __D3D12REFLECTIONPROBE_HLSL__
#define __D3D12REFLECTIONPROBE_HLSL__

struct ProbeInfluenceResult {
    bool  Valid;
    float Weight;      // 정규화 전 영향도
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
    float Weight[4];          // 프로브 블렌딩에 사용할 정규화된 가중치
    float TotalRawWeight;     // 전역 환경맵 블렌딩에 사용할 정규화 전 가중치 합
};

struct ProbeCandidate {
    bool  Valid;
    uint  IBLIndex;
    uint  ProbeIndex;
    uint  Priority;
    float Weight;
    float Distance;
};

//-----------------------------------------------------------------------------------------------------------
//
// @param probe : 반사 프로브 메타데이터
//
// @return 반사 프로브 중심의 월드 좌표
//
//-----------------------------------------------------------------------------------------------------------
float3 GetProbeCenterWS(ReflectionProbeMetaData probe) {
    return mul(float4(0.f, 0.f, 0.f, 1.f), probe.World).xyz;
}

//-----------------------------------------------------------------------------------------------------------
//
// 월드 행렬의 기저 벡터를 정규화해 스케일을 제거한다.
//
// @param world : 프로브 월드 행렬
//
// @return 정규화된 3×3 회전 행렬
//
//-----------------------------------------------------------------------------------------------------------
float3x3 GetProbeRotationOnly(float4x4 world) {
    float3x3 rot = (float3x3)world;

    rot[0] = normalize(rot[0]);
    rot[1] = normalize(rot[1]);
    rot[2] = normalize(rot[2]);

    return rot;
}

//-----------------------------------------------------------------------------------------------------------
//
// 박스 면과 반사 광선의 교차점으로 큐브맵 샘플 방향을 보정한다.
//
// @param probe : 박스 프로브 메타데이터
// @param worldPos : 영향 영역 내부의 월드 공간 셰이딩 위치
// @param reflectDirW : 월드 공간 반사 방향
//
// @return 스케일이 제거된 월드 공간 샘플 방향
//
//-----------------------------------------------------------------------------------------------------------
float3 ComputeBoxProjectedReflectDir(
    ReflectionProbeMetaData probe,
    float3 worldPos,
    float3 reflectDirW)
{
    // 월드 공간에서 프로브 로컬 공간으로 변환한다.
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

    // 셰이딩 위치는 프로브 영향 영역 안에 있다고 가정한다.
    float tExit = min(t2.x, min(t2.y, t2.z));
    tExit = max(tExit, 0.f);

    float3 hitLocal = localPos + localDir * tExit;

    // 큐브맵 샘플 방향에는 프로브 스케일을 적용하지 않는다.
    float3x3 probeRot = GetProbeRotationOnly(probe.World);
    float3 sampleDirW = mul(hitLocal, probeRot);

    return ShaderUtil::SafeNormalize(sampleDirW);
}

//-----------------------------------------------------------------------------------------------------------
//
// 반지름 안의 지점에 대해 경계까지의 거리로 가중치를 계산한다.
//
// @param probe : 구형 프로브 메타데이터
// @param probeIndex : 프로브 인덱스
// @param worldPos : 월드 공간 셰이딩 위치
//
// @return 구형 프로브의 영향 정보
//
//-----------------------------------------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------------------------------------
//
// 블렌딩 영역을 포함한 로컬 박스에서 축별 감쇠를 결합한다.
//
// @param probe : 박스 프로브 메타데이터
// @param probeIndex : 프로브 인덱스
// @param worldPos : 월드 공간 셰이딩 위치
//
// @return 박스 프로브의 영향 정보
//
//-----------------------------------------------------------------------------------------------------------
ProbeInfluenceResult EvaluateBoxProbe(
    ReflectionProbeMetaData probe,
    uint probeIndex,
    float3 worldPos)
{
    ProbeInfluenceResult r = (ProbeInfluenceResult)0;

    float3 localPos = ShaderUtil::TransformPosition(probe.InvWorld, worldPos);
    float3 q = abs(localPos);

    float blend = max(probe.BlendDistance, 1e-4f);

    // probe.BoxExtents는 프로브 로컬 공간의 반길이다.
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

//-----------------------------------------------------------------------------------------------------------
//
// 프로브 형태에 맞는 영향도 계산을 호출한다.
//
// @param probe : 반사 프로브 메타데이터
// @param probeIndex : 프로브 인덱스
// @param worldPos : 월드 공간 셰이딩 위치
//
// @return 프로브의 유효 여부와 가중치 등 영향 정보
//
//-----------------------------------------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------------------------------------
//
// 유효성, 우선순위, 가중치, 거리 순으로 후보를 비교한다.
//
// @param a : 비교할 후보
// @param b : 기준 후보
//
// @return 후보 a가 후보 b보다 먼저 선택되는지 여부
//
//-----------------------------------------------------------------------------------------------------------
bool IsBetterProbeCandidate(ProbeCandidate a, ProbeCandidate b) {
    if (!b.Valid) return true;

    if (a.Priority != b.Priority)
        return a.Priority > b.Priority;

    if (a.Weight != b.Weight)
        return a.Weight > b.Weight;

    return a.Distance < b.Distance;
}

//-----------------------------------------------------------------------------------------------------------
//
// @param c : 무효 후보로 초기화할 출력 변수
//
//-----------------------------------------------------------------------------------------------------------
void ResetProbeCandidate(out ProbeCandidate c) {
    c.Valid      = false;
    c.IBLIndex   = 0;
    c.ProbeIndex = 0;
    c.Priority   = 0;
    c.Weight     = 0.0f;
    c.Distance   = 1e30f;
}

//-----------------------------------------------------------------------------------------------------------
//
// 정렬 기준에 따라 후보를 삽입하고 최대 네 개를 유지한다.
//
// @param top : 정렬된 상위 후보 배열
// @param c : 삽입할 후보
//
//-----------------------------------------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------------------------------------
//
// 월드 위치에 영향을 주는 반사 프로브를 선택하고 블렌딩 가중치를 계산한다.
//
// @param worldPos : 월드 공간 셰이딩 위치
//
// @return 선택된 프로브와 블렌딩 가중치
//
//-----------------------------------------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------------------------------------
//
// @param iblIndex : IBL 텍스처 인덱스
// @param normalW : 월드 공간 표면 법선
//
// @return 확산 조사량 샘플의 RGB
//
//-----------------------------------------------------------------------------------------------------------
float3 SampleProbeDiffuse(uint iblIndex, float3 normalW) {
    return gi_DiffuseIrradianceCubeMaps[iblIndex].Sample(gsamLinearClamp, normalW).rgb;
}

//-----------------------------------------------------------------------------------------------------------
//
// 거칠기에 맞는 밉에서 프로브의 정반사 큐브맵을 샘플링한다.
//
// @param probe : 반사 프로브 메타데이터
// @param worldPos : 월드 공간 셰이딩 위치
// @param reflectDirW : 월드 공간 반사 방향
// @param roughness : 표면 거칠기
// @param maxMip : 최대 밉 레벨
//
// @return 정반사 환경맵 샘플의 RGB
//
//-----------------------------------------------------------------------------------------------------------
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
    return gi_SpecularIrradianceCubeMaps[probe.IBLIndex].SampleLevel(gsamLinearClamp, sampleDir, mip).rgb;
}

//-----------------------------------------------------------------------------------------------------------
//
// 선택된 프로브의 확산 조사량과 전역 환경맵 기여도를 혼합한다.
//
// @param selected : 선택된 프로브와 가중치
// @param normalW : 월드 공간 표면 법선
// @param globalDiffuse : 전역 확산 조사량
//
// @return 블렌딩한 확산 조사량 RGB
//
//-----------------------------------------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------------------------------------
//
// 선택된 프로브의 정반사 샘플과 전역 환경맵 기여도를 혼합한다.
//
// @param selected : 선택된 프로브와 가중치
// @param worldPos : 월드 공간 셰이딩 위치
// @param reflectDirW : 월드 공간 반사 방향
// @param roughness : 표면 거칠기
// @param maxMip : 최대 밉 레벨
// @param globalSpecular : 전역 정반사 샘플
//
// @return 블렌딩한 정반사 환경맵 RGB
//
//-----------------------------------------------------------------------------------------------------------
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