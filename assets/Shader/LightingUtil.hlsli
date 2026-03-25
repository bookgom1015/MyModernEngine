//***************************************************************************************
// LightingUtil.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Contains API for shader lighting.
//***************************************************************************************

// [ References ]
//  - https://alextardif.com/arealights.html
//  - https://learnopengl.com/Guest-Articles/2022/Area-Lights

#ifndef __LIGHTINGUTIL_HLSLI__
#define __LIGHTINGUTIL_HLSLI__

#include "./../../include/LightData.h"
#include "./../../assets/Shader/BRDF.hlsli"

float DegToRad(in float degrees) {
    return degrees * DEG2RAD;
}

float CalcLinearAttenuation(in float d, in float attenRadius) {
    return saturate((attenRadius - d) / (attenRadius));
}

float CalcInverseSquareAttenuation(in float d, in float attenRadius) {
    const float X = pow(d / attenRadius, 4);
    const float numer = pow(saturate(1 - X), 2);
    const float denom = d * d + 1;

    return numer / denom;
}

float3 CalcPlaneIntersection(in float3 pos, in float3 r, in float3 dir, in float3 center) {
    return pos + r * (dot(dir, center - pos) / dot(dir, r));
}

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

float SolidAngleTriangle(float3 a, float3 b, float3 c) {
    // a,b,c are vectors from P to vertices (A-P etc.)
    float la = length(a), lb = length(b), lc = length(c);
    if (la < 1e-6f || lb < 1e-6f || lc < 1e-6f) return 0;

    float numerator = dot(a, cross(b, c));
    float denom = la * lb * lc + dot(a, b) * lc + dot(b, c) * la + dot(c, a) * lb;

    // atan2 gives signed solid angle; abs() for magnitude
    return 2.f * atan2(numerator, denom);
}

///////////////////////////////////////////////////////////////////////

float SafeLength(in float3 v) {
    return max(length(v), FLT_EPSILON);
}

float3 SafeNormalize(in float3 v) {
    return v / SafeLength(v);
}

float SphereLightAlphaPrime(in float alpha, in float sourceRadius, in float distanceToLightCenter) {
    return saturate(alpha + sourceRadius / max(3.f * distanceToLightCenter, FLT_EPSILON));
}

float SphereLightNormalization(in float alpha, in float alphaPrime) {
    const float a = max(alpha, FLT_EPSILON);
    const float ap = max(alphaPrime, FLT_EPSILON);
    return (a * a) / (ap * ap);
}

float LineLightAlphaPrime(in float alpha, in float sourceRadius, in float distanceToClosestPoint) {
    return saturate(alpha + sourceRadius / max(3.f * distanceToClosestPoint, FLT_EPSILON));
}

float LineLightNormalization(in float alpha, in float alphaPrime) {
    const float a = max(alpha, FLT_EPSILON);
    const float ap = max(alphaPrime, FLT_EPSILON);
    return (a * a) / (ap * ap);
}

float RectLightAlphaPrime(in float alpha, in float sourceRadius, in float distanceToPlanePoint) {
    return saturate(alpha + sourceRadius / max(3.f * distanceToPlanePoint, FLT_EPSILON));
}

float RectLightNormalization(in float alpha, in float alphaPrime) {
    const float a = max(alpha, FLT_EPSILON);
    const float ap = max(alphaPrime, FLT_EPSILON);
    return (a * a) / (ap * ap);
}

float3 ClosestPointOnSphereToReflection(
        in float3 lightCenterToSurface,
        in float3 reflectionDir,
        in float sourceRadius) {
    const float proj = dot(lightCenterToSurface, reflectionDir);
    const float3 centerToRay = proj * reflectionDir - lightCenterToSurface;
    const float len = SafeLength(centerToRay);

    return lightCenterToSurface + centerToRay * saturate(sourceRadius / len);
}

float ClosestPointParameterOnSegmentToRay(
        in float3 P0,
        in float3 P1,
        in float3 rayDir) {
    const float3 lineVec = P1 - P0;
    const float lineLen2 = max(dot(lineVec, lineVec), FLT_EPSILON);
    const float rayLine = dot(rayDir, lineVec);

    const float denom = max(lineLen2 - rayLine * rayLine, FLT_EPSILON);
    const float t = (dot(rayDir, P0) * rayLine - dot(P0, lineVec)) / denom;

    return saturate(t);
}

float3 ClosestPointOnSegmentToReflection(
        in float3 P0,
        in float3 P1,
        in float3 reflectionDir,
        in float sourceRadius) {
    const float t = ClosestPointParameterOnSegmentToRay(P0, P1, reflectionDir);
    float3 pointOnLine = lerp(P0, P1, t);

    const float proj = dot(pointOnLine, reflectionDir);
    const float3 centerToRay = proj * reflectionDir - pointOnLine;
    const float len = SafeLength(centerToRay);

    return pointOnLine + centerToRay * saturate(sourceRadius / len);
}

float3 ClosestPointOnRectToReflection(
        in float3 pos,
        in float3 reflectionDir,
        in float3 rectCenter,
        in float3 rectNormal,
        in float3 rectRight,
        in float3 rectUp,
        in float2 rectHalfSize) {
    float3 intersectPoint;
    if (CalcPlaneIntersectionSafe(pos, reflectionDir, rectNormal, rectCenter, intersectPoint)) {
        const float3 v = intersectPoint - rectCenter;

        const float x = clamp(dot(v, rectRight), -rectHalfSize.x, rectHalfSize.x);
        const float y = clamp(dot(v, rectUp),    -rectHalfSize.y, rectHalfSize.y);

        return rectCenter + rectRight * x + rectUp * y;
    }

    return rectCenter;
}

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

float3 ComputePointLight(
        in LightData light, 
        in Material mat, 
        in float3 pos, 
        in float3 normal, 
        in float3 toEye) {
    const float3 toCenter = light.Position - pos;
    const float d = length(toCenter);
    if (d <= FLT_EPSILON) return 0;

    const float3 Lcenter = toCenter / d;

    const float Falloff = CalcInverseSquareAttenuation(d, light.AttenuationRadius);
    const float3 Li = light.Color * light.Intensity * Falloff;

    const float NoL_diff = max(dot(normal, Lcenter), 0.f);

    const float3 R = reflect(-toEye, normal);
    const float3 repPoint = ClosestPointOnSphereToReflection(toCenter, R, light.Radius);
    const float3 Lspec = SafeNormalize(repPoint);
    const float NoL_spec = max(dot(normal, Lspec), 0.f);

    const float Roughness = mat.Roughness;
    const float alpha = max(Roughness * Roughness, FLT_EPSILON);
    const float alphaPrime = SphereLightAlphaPrime(alpha, light.Radius, d);
    const float normFactor = SphereLightNormalization(alpha, alphaPrime);

    const float3 Hdiff = normalize(toEye + Lcenter);
    const float3 Fdiff = FresnelSchlick(saturate(dot(Hdiff, toEye)), mat.FresnelR0);
    const float3 diffuse = CookTorranceDiffuse(mat, Fdiff, NoL_diff);

    float3 specular = 0.f;
    if (NoL_spec > 0.f) {
        specular = CookTorranceSpecular(mat, Lspec, normal, toEye, NoL_spec);
        specular *= NoL_spec;
        specular *= normFactor;
    }

    return (diffuse + specular) * Li;
}

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
    const float Factor = clamp((Theta - CosOuter) / max(Epsilon, FLT_EPSILON), 0, 1);

    const float d = length(Ldisp);
    const float Falloff = CalcInverseSquareAttenuation(d, light.AttenuationRadius);
    const float3 Li = light.Color * light.Intensity * Factor * Falloff;

    const float NoL = max(dot(normal, L), 0);

    return CookTorrance(mat, Li, L, normal, toEye, NoL);
}

float3 ComputeTubeLight(
        in LightData light, 
        in Material mat, 
        in float3 pos, 
        in float3 normal, 
        in float3 toEye) {
    const float3 P0 = light.Position - pos;
    const float3 P1 = light.Position1 - pos;

    const float d0 = length(P0);
    const float d1 = length(P1);

    const float NoL0 = dot(normal, P0) / max(2.f * d0, FLT_EPSILON);
    const float NoL1 = dot(normal, P1) / max(2.f * d1, FLT_EPSILON);
    const float NoL_diff = (2.f * saturate(NoL0 + NoL1)) / max(d0 * d1 + dot(P0, P1) + 2.f, FLT_EPSILON);

    const float3 closestLinePoint = lerp(P0, P1, 0.5f);
    const float dLine = length(closestLinePoint);

    const float Falloff = CalcLinearAttenuation(dLine, light.AttenuationRadius);
    const float3 Li = light.Color * light.Intensity * Falloff;

    const float3 R = reflect(-toEye, normal);
    const float3 repPoint = ClosestPointOnSegmentToReflection(P0, P1, R, light.Radius);
    const float dSpec = length(repPoint);

    const float3 Lspec = SafeNormalize(repPoint);
    const float NoL_spec = max(dot(normal, Lspec), 0.f);

    const float Roughness = mat.Roughness;
    const float alpha = max(Roughness * Roughness, FLT_EPSILON);
    const float alphaPrime = LineLightAlphaPrime(alpha, light.Radius, dSpec);
    const float normFactor = LineLightNormalization(alpha, alphaPrime);

    const float3 Ldiff = SafeNormalize(closestLinePoint);
    const float3 Hdiff = normalize(toEye + Ldiff);
    const float3 Fdiff = FresnelSchlick(saturate(dot(Hdiff, toEye)), mat.FresnelR0);
    const float3 diffuse = CookTorranceDiffuse(mat, Fdiff, NoL_diff);

    float3 specular = 0.f;
    if (NoL_spec > 0.f) {
        specular = CookTorranceSpecular(mat, Lspec, normal, toEye, NoL_spec);
        specular *= NoL_spec;
        specular *= normFactor;
    }

    return (diffuse + specular) * Li;
}

float3 ComputeRectLight(
        in LightData light, 
        in Material mat, 
        in float3 pos, 
        in float3 normal, 
        in float3 toEye) {
    if (light.RectSize.x < FLT_EPSILON || 
        light.RectSize.y < FLT_EPSILON || 
        dot(pos - light.Center, light.Direction) <= 0) return 0;
    
    const float3 L0 = light.Position  - pos;
    const float3 L1 = light.Position1 - pos;
    const float3 L2 = light.Position2 - pos;
    const float3 L3 = light.Position3 - pos;

    float omega = SolidAngleTriangle(L0, L1, L2) + SolidAngleTriangle(L0, L2, L3);
    omega = abs(omega);
    omega = clamp(omega, 0.0f, 2.0f * PI);

    const float3 R = reflect(-toEye, normal);
    const float3 nearestPoint = ClosestPointOnRectToReflection(
        pos,
        R,
        light.Center,
        light.Direction,
        light.Right,
        light.Up,
        light.RectSize);

    const float3 rectCenterToPos = light.Center - pos;
    const float dCenter = length(rectCenterToPos);
    const float dSpec = distance(pos, nearestPoint);

    const float Area = (2.f * light.RectSize.x) * (2.f * light.RectSize.y);
    const float Falloff = CalcLinearAttenuation(dCenter, light.AttenuationRadius);
    const float3 Li = light.Color * light.Intensity * Falloff / max(Area, FLT_EPSILON);

    const float3 Ldiff = SafeNormalize(rectCenterToPos);
    const float NdotL = saturate(dot(normal, Ldiff));
    const float NoL_diff = omega * NdotL;

    const float3 Lspec = SafeNormalize(nearestPoint - pos);
    const float NoL_spec = max(dot(normal, Lspec), 0.f);

    const float Roughness = mat.Roughness;
    const float alpha = max(Roughness * Roughness, FLT_EPSILON);

    // 대략적인 source radius 근사
    const float sourceRadius = 0.5f * length(float2(light.RectSize.x, light.RectSize.y));
    const float alphaPrime = RectLightAlphaPrime(alpha, sourceRadius, dSpec);
    const float normFactor = RectLightNormalization(alpha, alphaPrime);

    const float3 Hdiff = normalize(toEye + Ldiff);
    const float3 Fdiff = FresnelSchlick(saturate(dot(Hdiff, toEye)), mat.FresnelR0);
    const float3 diffuse = CookTorranceDiffuse(mat, Fdiff, NoL_diff);

    float3 specular = 0.f;
    if (NoL_spec > 0.f) {
        specular = CookTorranceSpecular(mat, Lspec, normal, toEye, NoL_spec);
        specular *= NoL_spec;
        specular *= normFactor;
        specular *= omega;
    }

    return (diffuse + specular) * Li;
}

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
        else if (light.Type == TubeLight)
            result += factor * ComputeTubeLight(light, mat, pos, normal, toEye);
        else if (light.Type == RectangleLight)
            result += ComputeRectLight(light, mat, pos, normal, toEye);
    }

    return result;
}

#endif // __LIGHTINGUTIL_HLSLI__