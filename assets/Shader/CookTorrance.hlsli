#ifndef __COOKTORRANCE_HLSLI__
#define __COOKTORRANCE_HLSLI__

//-----------------------------------------------------------------------------------------------------------
//
// Cook–Torrance 정반사와 램버트 확산 반사를 합산한다.
//
// @param mat : 표면 재질
// @param Li : 입사광 RGB
// @param L : 광원 방향
// @param N : 표면 법선
// @param V : 시선 방향
// @param NoL : 법선과 광원 방향의 내적
//
// @return 입사광과 코사인 항을 반영한 RGB 조명 기여도
//
//-----------------------------------------------------------------------------------------------------------
float3 CookTorrance(in Material mat, in float3 Li, in float3 L, in float3 N, in float3 V, in float NoL) {
    const float3 H = normalize(V + L);
    const float Roughness = mat.Roughness;

    const float NDF = DistributionGGX(N, H, Roughness);
    const float G = GeometrySmith(N, V, L, Roughness);
    const float3 F = FresnelSchlick(saturate(dot(H, V)), mat.FresnelR0);

    const float3 Diffuse = mat.Albedo.rgb / PI;

    const float3 Numerator = NDF * G * F;
    const float Denominator = 4.f * max(dot(N, V), 0.f) * NoL + 0.0001f;
    const float3 Specular = Numerator / Denominator;

    const float3 kS = F;
    float3 kD = 1.f - kS;
    kD *= (1.f - mat.Metalness);

    return (kD * Diffuse + Specular) * Li * NoL;
}

//-----------------------------------------------------------------------------------------------------------
//
// 광원 크기와 거리로 보정한 GGX 항을 사용해 조명 기여도를 계산한다.
//
// @param mat : 표면 재질
// @param Li : 입사광 RGB
// @param L : 광원 방향
// @param N : 표면 법선
// @param V : 시선 방향
// @param NoL : 법선과 광원 방향의 내적
// @param d : 광원까지의 거리
// @param radius : 광원 반지름
//
// @return 입사광과 코사인 항을 반영한 RGB 조명 기여도
//
//-----------------------------------------------------------------------------------------------------------
float3 CookTorrance_GGXModified(in Material mat, in float3 Li, in float3 L, in float3 N, in float3 V, in float NoL, in float d, in float radius) {
    const float3 H = normalize(V + L);
    const float Roughness = mat.Roughness;

    const float NDF = DistributionGGX_Modified(N, H, Roughness, d, radius);
    const float G = GeometrySmith(N, V, L, Roughness);
    const float3 F = FresnelSchlick(saturate(dot(H, V)), mat.FresnelR0);

    const float3 Diffuse = mat.Albedo.rgb / PI;

    const float3 Numerator = NDF * G * F;
    const float Denominator = 4.f * max(dot(N, V), 0.f) * NoL + 0.0001f;
    const float3 Specular = Numerator / Denominator;

    const float3 kS = F;
    float3 kD = 1.f - kS;
    kD *= (1.f - mat.Metalness);

    return (kD * Diffuse + Specular) * Li * NoL;
}

#endif // __COOKTORRANCE_HLSLI__