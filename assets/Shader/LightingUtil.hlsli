//***************************************************************************************
// LightingUtil.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// 셰이더 조명 계산 함수.
//***************************************************************************************

// [ 참고 자료 ]
//  - https://alextardif.com/arealights.html
//  - https://learnopengl.com/Guest-Articles/2022/Area-Lights

#ifndef __LIGHTINGUTIL_HLSLI__
#define __LIGHTINGUTIL_HLSLI__

#include "./../../include/LightData.h"
#include "./../../assets/Shader/BRDF.hlsli"

//-----------------------------------------------------------------------------------------------------------
//
// @param degrees : 도 단위 각도
//
// @return 라디안 단위 각도
//
//-----------------------------------------------------------------------------------------------------------
float DegToRad(in float degrees) {
    return degrees * DEG2RAD;
}

//-----------------------------------------------------------------------------------------------------------
//
// @param d : 광원까지의 거리
// @param attenRadius : 감쇠 반경
//
// @return 반경에서 0이 되는 선형 감쇠 계수
//
//-----------------------------------------------------------------------------------------------------------
float CalcLinearAttenuation(in float d, in float attenRadius) {
    return saturate((attenRadius - d) / (attenRadius));
}

//-----------------------------------------------------------------------------------------------------------
//
// @param d : 광원까지의 거리
// @param attenRadius : 감쇠 반경
//
// @return 반경 경계의 감쇠를 적용한 역제곱 조명 계수
//
//-----------------------------------------------------------------------------------------------------------
float CalcInverseSquareAttenuation(in float d, in float attenRadius) {
    const float X = pow(d / attenRadius, 4);
    const float numer = pow(saturate(1 - X), 2);
    const float denom = d * d + 1;

    return numer / denom;
}

//-----------------------------------------------------------------------------------------------------------
//
// @param pos : 직선의 시작점
// @param r : 직선의 방향
// @param dir : 평면 법선
// @param center : 평면 위의 기준점
//
// @return 직선과 평면의 교점
//
//-----------------------------------------------------------------------------------------------------------
float3 CalcPlaneIntersection(in float3 pos, in float3 r, in float3 dir, in float3 center) {
    return pos + r * (dot(dir, center - pos) / dot(dir, r));
}

//-----------------------------------------------------------------------------------------------------------
//
// 분모가 0에 가까운 경우와 광선 뒤쪽 교점을 걸러낸다.
//
// @param pos : 광선 시작점
// @param r : 광선 방향
// @param planeNormal : 평면 법선
// @param planePoint : 평면 위의 기준점
// @param outPoint : 교점 출력
//
// @return 유효한 교점이 있는지 여부
//
//-----------------------------------------------------------------------------------------------------------
bool CalcPlaneIntersectionSafe(
        in float3 pos,
        in float3 r,
        in float3 planeNormal,
        in float3 planePoint,
        out float3 outPoint) {
    const float EPS = 1e-4f;

    float denom = dot(planeNormal, r);
    if (abs(denom) < EPS) return false;

    float t = dot(planeNormal, planePoint - pos) / denom;
    if (t <= EPS) return false;

    outPoint = pos + r * t;
    
    return true;
}

//-----------------------------------------------------------------------------------------------------------
//
// @param a : 관측점에서 첫 번째 정점으로 향하는 벡터
// @param b : 관측점에서 두 번째 정점으로 향하는 벡터
// @param c : 관측점에서 세 번째 정점으로 향하는 벡터
//
// @return 삼각형이 차지하는 입체각의 크기
//
//-----------------------------------------------------------------------------------------------------------
float SolidAngleTriangle(float3 a, float3 b, float3 c) {
    // a, b, c는 관측점 P에서 각 정점으로 향하는 벡터다.
    float la = length(a), lb = length(b), lc = length(c);
    if (la < 1e-6f || lb < 1e-6f || lc < 1e-6f) return 0;

    float numerator = dot(a, cross(b, c));
    float denom = la * lb * lc + dot(a, b) * lc + dot(b, c) * la + dot(c, a) * lb;

    // atan2가 반환한 부호 있는 입체각에 절댓값을 취한다.
    return 2.f * atan2(numerator, denom);
}

//-----------------------------------------------------------------------------------------------------------
//
// 방향광의 표면 반사 기여도를 계산한다.
//
// @param light : 광원 데이터
// @param mat : 표면 재질
// @param normal : 표면 법선
// @param toEye : 시선 방향
//
// @return 광원의 RGB 조명 기여도
//
//-----------------------------------------------------------------------------------------------------------
float3 ComputeDirectionalLight(
        in LightData light, 
        in Material mat, 
        in float3 normal, 
        in float3 toEye) {
    const float3 L = -light.Direction;
    const float3 Li = light.Color * light.Intensity;

    const float NoL = max(dot(normal, L), 0);

    return CookTorrance(mat, Li, L, normal, toEye, NoL);
}

//-----------------------------------------------------------------------------------------------------------
//
// 점광원의 거리 감쇠와 표면 반사를 계산한다.
//
// @param light : 광원 데이터
// @param mat : 표면 재질
// @param pos : 월드 공간 셰이딩 위치
// @param normal : 표면 법선
// @param toEye : 시선 방향
//
// @return 광원의 RGB 조명 기여도
//
//-----------------------------------------------------------------------------------------------------------
float3 ComputePointLight(
        in LightData light, 
        in Material mat, 
        in float3 pos, 
        in float3 normal, 
        in float3 toEye) {
    const float3 Ldisp = light.Position - pos;
    const float d = length(Ldisp);

    const float Falloff = CalcInverseSquareAttenuation(d, light.AttenuationRadius);
    const float3 Li = light.Color * light.Intensity * Falloff;

    const float3 r = reflect(-toEye, normal);
    const float3 CenterToRay = dot(Ldisp, r) * r - Ldisp;
    const float3 ClosestPoint = Ldisp + CenterToRay * saturate(light.Radius / length(CenterToRay));
    const float3 L = normalize(ClosestPoint);

    const float NoL = max(dot(normal, L), 0);
    
    return CookTorrance_GGXModified(mat, Li, L, normal, toEye, NoL, d, light.Radius);
}

//-----------------------------------------------------------------------------------------------------------
//
// 스포트라이트의 거리 및 각도 감쇠와 표면 반사를 계산한다.
//
// @param light : 광원 데이터
// @param mat : 표면 재질
// @param pos : 월드 공간 셰이딩 위치
// @param normal : 표면 법선
// @param toEye : 시선 방향
//
// @return 광원의 RGB 조명 기여도
//
//-----------------------------------------------------------------------------------------------------------
float3 ComputeSpotLight(
        in LightData light, 
        in Material mat, 
        in float3 pos, 
        in float3 normal, 
        in float3 toEye) {
    const float3 Ldisp = light.Position - pos;
    const float3 L = normalize(Ldisp);

    const float Theta = dot(-L, light.Direction);

    const float RadOuter = DegToRad(light.OuterConeAngle);
    const float CosOuter = cos(RadOuter);

    if (Theta < CosOuter) return 0;

    const float RadInner = DegToRad(light.InnerConeAngle);
    const float CosInner = cos(RadInner);

    const float Epsilon = CosInner - CosOuter;
    const float Factor = clamp((Theta - CosOuter) / Epsilon, 0, 1);

    const float d = length(Ldisp);
    const float Falloff = CalcInverseSquareAttenuation(d, light.AttenuationRadius);
    const float3 Li = light.Color * light.Intensity * Factor * Falloff;

    const float NoL = max(dot(normal, L), 0);

    return CookTorrance(mat, Li, L, normal, toEye, NoL);
}

//-----------------------------------------------------------------------------------------------------------
//
// 튜브 광원의 표면 반사 기여도를 근사한다.
//
// @param light : 광원 데이터
// @param mat : 표면 재질
// @param pos : 월드 공간 셰이딩 위치
// @param normal : 표면 법선
// @param toEye : 시선 방향
//
// @return 광원의 RGB 조명 기여도
//
//-----------------------------------------------------------------------------------------------------------
float3 ComputeTubeLight(
        in LightData light, 
        in Material mat, 
        in float3 pos, 
        in float3 normal, 
        in float3 toEye) {
    const float3 L0 = light.Position - pos;
    const float3 L1 = light.Position1 - pos;

    const float d0 = length(L0);
    const float d1 = length(L1);

    const float NoL0 = dot(normal, L0) / (2 * d0);
    const float NoL1 = dot(normal, L1) / (2 * d1);
    const float NoL = (2 * saturate(NoL0 + NoL1)) / (d0 * d1 + dot(L0, L1) + 2);

    const float3 r = reflect(-toEye, normal);

    const float3 Ldisp = L1 - L0;
    const float RoLdisp = dot(r, Ldisp);
    const float Ld = length(Ldisp);
    const float t = (dot(r, L0) * RoLdisp - dot(L0, Ldisp)) / (Ld * Ld - RoLdisp * RoLdisp);

    float3 closestPoint = L0 + Ldisp * saturate(t);
    const float3 CenterToRay = dot(closestPoint, r) * r - closestPoint;
    closestPoint = closestPoint + CenterToRay * saturate(light.Radius / length(CenterToRay));

    const float3 L = normalize(closestPoint);
    const float d = length(closestPoint);
		
    const float Falloff = CalcLinearAttenuation(d, light.AttenuationRadius);
    const float3 Li = light.Color * light.Intensity * Falloff;

    return CookTorrance_GGXModified(mat, Li, L, normal, toEye, NoL, d, light.Radius);
}

//-----------------------------------------------------------------------------------------------------------
//
// 사각형 면광원의 입체각과 표면 반사를 계산한다.
//
// @param light : 광원 데이터
// @param mat : 표면 재질
// @param pos : 월드 공간 셰이딩 위치
// @param normal : 표면 법선
// @param toEye : 시선 방향
//
// @return 광원의 RGB 조명 기여도
//
//-----------------------------------------------------------------------------------------------------------
float3 ComputeRectLight(
        in LightData light, 
        in Material mat, 
        in float3 pos, 
        in float3 normal, 
        in float3 toEye) {
    if (light.RectSize.x < FLT_EPSILON || 
        light.RectSize.y < FLT_EPSILON || 
        dot(pos - light.Center, light.Direction) <= 0) return 0;
    
    const float3 L0 = light.Position - pos;
    const float3 L1 = light.Position1 - pos;
    const float3 L2 = light.Position2 - pos;
    const float3 L3 = light.Position3 - pos;

    const float3 n0 = normalize(cross(L0, L1));
    const float3 n1 = normalize(cross(L1, L2));
    const float3 n2 = normalize(cross(L2, L3));
    const float3 n3 = normalize(cross(L3, L0));

    const float G0 = acos(saturate(dot(-n0, n1)));
    const float G1 = acos(saturate(dot(-n1, n2)));
    const float G2 = acos(saturate(dot(-n2, n3)));            
    const float G3 = acos(saturate(dot(-n3, n0)));

    float omega = SolidAngleTriangle(L0, L1, L2) + SolidAngleTriangle(L0, L2, L3);
    omega = abs(omega);                    // 입체각의 크기
    omega = clamp(omega, 0.0f, 2.0f * PI); // 입체각을 [0, 2π] 범위로 제한한다.
    
    //const float SolidAngle = G0 + G1 + G2 + G3 - 2.f * 3.14159265359f;
    const float SolidAngle = omega;

    const float3 r = reflect(-toEye, normal);
    
    float3 intersectPoint;
    float3 nearestPoint;
    if (CalcPlaneIntersectionSafe(pos, r, light.Direction, light.Center, intersectPoint)) {
        const float3 IntersectionVector = intersectPoint - light.Center;
        const float2 IntersectPlanePoint = float2(dot(IntersectionVector, light.Right), dot(IntersectionVector, light.Up));
        const float2 Nearest2DPoint = float2(
            clamp(IntersectPlanePoint.x, -light.RectSize.x, light.RectSize.x), 
            clamp(IntersectPlanePoint.y, -light.RectSize.y, light.RectSize.y));
        
        nearestPoint = light.Center + (light.Right * Nearest2DPoint.x + light.Up * Nearest2DPoint.y);
    }
    else {
        nearestPoint = light.Center;
    }   
    
    const float d = distance(pos, nearestPoint);

    const float Area = (2 * light.RectSize.x) * (2 * light.RectSize.y);
    const float Falloff = CalcLinearAttenuation(d, light.AttenuationRadius);
    const float3 Li = light.Color * light.Intensity * Falloff / Area;

    const float3 L = normalize(nearestPoint - pos);
    
    const float NdotL = saturate(dot(normal, L));
    const float NoL = SolidAngle * NdotL;
    
    return CookTorrance(mat, Li, L, normal, toEye, NoL);
}

//-----------------------------------------------------------------------------------------------------------
//
// 광원 유형별 표면 반사와 그림자 계수를 적용해 조명을 합산한다.
//
// @param lights : 광원 배열
// @param mat : 표면 재질
// @param pos : 월드 공간 셰이딩 위치
// @param normal : 표면 법선
// @param toEye : 시선 방향
// @param shadowFactors : 광원별 그림자 가시성
// @param lightCount : 처리할 광원 수
//
// @return 광원별 RGB 조명 기여도의 합
//
//-----------------------------------------------------------------------------------------------------------
float3 ComputeBRDF(
        in LightData lights[MAX_LIGHT_COUNT], 
        in Material mat, 
        in float3 pos, 
        in float3 normal, 
        in float3 toEye, 
        in float shadowFactors[MAX_LIGHT_COUNT],
        in uint lightCount) {
    float3 result = 0;

	[loop]
    for (uint i = 0; i < lightCount; ++i) {
        LightData light = lights[i];
        
        const float factor = shadowFactors[i];
        
        if (light.Type == DirectionalLight)
            result += factor * ComputeDirectionalLight(light, mat, normal, toEye);
        else if (light.Type == PointLight)
            result += factor * ComputePointLight(light, mat, pos, normal, toEye);
        else if (light.Type == SpotLight)
            result += factor * ComputeSpotLight(light, mat, pos, normal, toEye);
        else if (light.Type == RectangleLight)
            result += ComputeRectLight(light, mat, pos, normal, toEye);
        else if (light.Type == TubeLight)
            result += factor * ComputeTubeLight(light, mat, pos, normal, toEye);
    }

    return result;
}

#endif // __LIGHTINGUTIL_HLSLI__