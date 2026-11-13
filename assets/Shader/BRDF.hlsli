
#ifndef __BRDF_HLSLI__
#define __BRDF_HLSLI__

#include "./../../assets/Shader/ConstantValues.hlsli"

//-----------------------------------------------------------------------------------------------------------
//
// GGX 법선 분포 항을 계산한다.
//
// @param N : 표면 법선
// @param H : 반각 벡터
// @param roughness : 표면 거칠기
//
// @return Trowbridge–Reitz GGX 법선 분포 값
//
//-----------------------------------------------------------------------------------------------------------
float DistributionGGX(in float3 N, in float3 H, in float roughness) {
    const float a = roughness * roughness;
    const float a2 = a * a;
    const float NdotH = max(dot(N, H), 0.f);
    const float NdotH2 = NdotH * NdotH;

    const float num = a2;
    float denom = (NdotH2 * (a2 - 1.f) + 1.f);
    denom = PI * denom * denom;

    return num / denom;
}

//-----------------------------------------------------------------------------------------------------------
//
// 광원의 거리와 반지름으로 GGX 분포의 분자를 보정한다.
//
// @param N : 표면 법선
// @param H : 반각 벡터
// @param roughness : 표면 거칠기
// @param d : 광원까지의 거리
// @param radius : 광원 반지름
//
// @return 보정된 GGX 법선 분포 값
//
//-----------------------------------------------------------------------------------------------------------
float DistributionGGX_Modified(in float3 N, in float3 H, in float roughness, in float d, in float radius) {
    const float a = roughness * roughness;
    const float a_ = saturate(radius / (2.f * d) + a);
    const float a2 = a * a;
    const float a_2 = a_ * a_;
    const float NdotH = max(dot(N, H), 0.f);
    const float NdotH2 = NdotH * NdotH;

    const float num = a2 * a_2;
    float denom = (NdotH2 * (a2 - 1.f) + 1.f);
    denom = PI * denom * denom;

    return num / denom;
}

//-----------------------------------------------------------------------------------------------------------
//
// 직접광용 Schlick–GGX 기하 항을 계산한다.
//
// @param NdotV : 법선과 방향 벡터의 내적
// @param roughness : 표면 거칠기
//
// @return 한 방향의 가시성 항
//
//-----------------------------------------------------------------------------------------------------------
float GeometryShlickGGX(in float NdotV, in float roughness) {
    const float a = (roughness + 1.f);
    const float k = (a * a) / 8.f;

    const float num = NdotV;
    const float denom = NdotV * (1.f - k) + k;

    return num / denom;
}

//-----------------------------------------------------------------------------------------------------------
//
// 이미지 기반 조명용 Schlick–GGX 기하 항을 계산한다.
//
// @param NdotV : 법선과 방향 벡터의 내적
// @param roughness : 표면 거칠기
//
// @return 한 방향의 가시성 항
//
//-----------------------------------------------------------------------------------------------------------
float GeometryShlickGGX_IBL(in float NdotV, in float roughness) {
    const float a = roughness;
    const float k = (a * a) / 2.f;

    const float num = NdotV;
    const float denom = NdotV * (1.f - k) + k;

    return num / denom;
}

//-----------------------------------------------------------------------------------------------------------
//
// 직접광의 입사 방향과 시선 방향에 대한 기하 항을 결합한다.
//
// @param N : 표면 법선
// @param V : 시선 방향
// @param L : 광원 방향
// @param roughness : 표면 거칠기
//
// @return Smith 기하 항
//
//-----------------------------------------------------------------------------------------------------------
float GeometrySmith(in float3 N, in float3 V, in float3 L, in float roughness) {
    const float NdotV = max(dot(N, V), 0.f);
    const float NdotL = max(dot(N, L), 0.f);

    const float ggx1 = GeometryShlickGGX(NdotV, roughness);
    const float ggx2 = GeometryShlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

//-----------------------------------------------------------------------------------------------------------
//
// 이미지 기반 조명용 기하 항을 결합한다.
//
// @param N : 표면 법선
// @param V : 시선 방향
// @param L : 광원 방향
// @param roughness : 표면 거칠기
//
// @return Smith 기하 항
//
//-----------------------------------------------------------------------------------------------------------
float GeometrySmith_IBL(in float3 N, in float3 V, in float3 L, in float roughness) {
    const float NdotV = max(dot(N, V), 0.f);
    const float NdotL = max(dot(N, L), 0.f);

    const float ggx1 = GeometryShlickGGX_IBL(NdotV, roughness);
    const float ggx2 = GeometryShlickGGX_IBL(NdotL, roughness);

    return ggx1 * ggx2;
}

//-----------------------------------------------------------------------------------------------------------
//
// Schlick 근사로 프레넬 반사율을 구한다.
//
// @param cos : 입사각의 코사인
// @param F0 : 수직 입사 반사율
//
// @return RGB 프레넬 반사율
//
//-----------------------------------------------------------------------------------------------------------
float3 FresnelSchlick(in float cos, in float3 F0) {
    return F0 + (1.f - F0) * pow(max((1.f - cos), 0.f), 5.f);
}

//-----------------------------------------------------------------------------------------------------------
//
// 거칠기를 반영한 Schlick 프레넬 근사를 계산한다.
//
// @param cos : 입사각의 코사인
// @param F0 : 수직 입사 반사율
// @param roughness : 표면 거칠기
//
// @return RGB 프레넬 반사율
//
//-----------------------------------------------------------------------------------------------------------
float3 FresnelSchlickRoughness(in float cos, in float3 F0, in float roughness) {
    return F0 + (max((float3) (1.f - roughness), F0) - F0) * pow(max(1.f - cos, 0.f), 5.f);
}

//-----------------------------------------------------------------------------------------------------------
//
// 32비트 정수의 비트 순서를 뒤집어 Van der Corput 수열 값을 구한다.
//
// @param bits : 수열 인덱스
//
// @return 2진 기수 역함수 값
//
//-----------------------------------------------------------------------------------------------------------
float RadicalInverse_VdC(in uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

//-----------------------------------------------------------------------------------------------------------
//
// Hammersley 저불일치 점 집합에서 표본을 구한다.
//
// @param i : 표본 인덱스
// @param N : 전체 표본 수
//
// @return 2D 표본 좌표
//
//-----------------------------------------------------------------------------------------------------------
float2 Hammersley(in uint i, in uint N) {
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

//-----------------------------------------------------------------------------------------------------------
//
// GGX 분포에서 반각 벡터를 중요도 샘플링한다.
//
// @param Xi : 단위 정사각형의 2D 표본
// @param N : 월드 공간 표면 법선
// @param roughness : 표면 거칠기
//
// @return 정규화된 월드 공간 반각 벡터
//
//-----------------------------------------------------------------------------------------------------------
float3 ImportanceSampleGGX(in float2 Xi, in float3 N, in float roughness) {
    const float a = roughness * roughness;

    const float phi = 2.f * PI * Xi.x;
    const float cosTheta = sqrt((1.f - Xi.y) / (1.f + (a * a - 1.f) * Xi.y));
    const float sinTheta = sqrt(1.f - cosTheta * cosTheta);

	// 구면 좌표를 직교 좌표로 변환한다.
    float3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

	// 접선 공간의 표본 벡터를 월드 공간으로 변환한다.
    const float3 up = abs(N.z) < 0.999f ? float3(0.f, 0.f, 1.f) : float3(1.f, 0.f, 0.f);
    const float3 tangent = normalize(cross(up, N));
    const float3 bitangent = cross(N, tangent);

    const float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

#include "./../../assets/Shader/CookTorrance.hlsli"

#endif // __BRDF_HLSLI__
