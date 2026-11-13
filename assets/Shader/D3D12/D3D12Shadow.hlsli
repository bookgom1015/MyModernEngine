#ifndef __D3D12SHADOE_HLSLI__
#define __D3D12SHADOE_HLSLI__

//-----------------------------------------------------------------------------------------------------------
//
// @param light : 광원 데이터
// @param index : 행렬 슬롯 인덱스
//
// @return 슬롯의 뷰-투영 행렬 또는 범위 밖 인덱스에 대한 영행렬
//
//-----------------------------------------------------------------------------------------------------------
float4x4 GetViewProjMatrix(in LightData light, in uint index) {
    switch (index) {
        case 0: return light.Matrix0;
        case 1: return light.Matrix1;
        case 2: return light.Matrix2;
        case 3: return light.Matrix3;
        case 4: return light.Matrix4;
        case 5: return light.Matrix5;
        default: return (float4x4)0;
    }
}

//-----------------------------------------------------------------------------------------------------------
//
// 그림자 맵을 3×3 탭으로 비교 샘플링한다.
//
// @param depthMap : 그림자 깊이 배열 텍스처
// @param sampComp : 깊이 비교 샘플러
// @param texc : 중심 UV
// @param ddxy : 축별 샘플 간격
// @param depth : 비교할 깊이
// @param index : 그림자 맵 배열 슬라이스
//
// @return 아홉 탭의 평균 가시성
//
//-----------------------------------------------------------------------------------------------------------
float CalcShadowPCF3x3(
    in Texture2DArray<float> depthMap
    , in SamplerComparisonState sampComp
    , in float2 texc
    , in float2 ddxy
    , in float depth
    , in uint index) {    
    const int Radius = 1;
    const int Diameter = (2 * Radius) + 1;
    const float invCount = 1.f / (Diameter * Diameter);
    
    float sum = 0.f;
    
    [loop]
    for (int i = -Radius; i <= Radius; ++i) {
        [loop]
        for (int j = -Radius; j <= Radius; ++j) {
            float2 tap = texc + float2(i * ddxy.y, j * ddxy.x);    
            sum += depthMap.SampleCmpLevelZero(sampComp, float3(tap, index), depth);
        }
    }
    
    return sum * invCount;
}

//-----------------------------------------------------------------------------------------------------------
//
// 광원 유형에 맞게 월드 위치를 그림자 맵에 투영해 한 번 비교 샘플링한다.
//
// @param light : 광원 데이터
// @param depthMap : 그림자 깊이 배열 텍스처
// @param sampComp : 깊이 비교 샘플러
// @param posW : 월드 공간 셰이딩 위치
//
// @return 그림자 가시성 또는 지원하지 않는 광원 유형에 대해 1
//
//-----------------------------------------------------------------------------------------------------------
float CalcShadow(
    LightData light
    , in Texture2DArray<float> depthMap
    , in SamplerComparisonState sampComp
    , in float3 posW) {
    if (light.Type == DirectionalLight || light.Type == SpotLight) {
        float4 shadowPosH = mul(float4(posW, 1.f), light.Matrix1);
        shadowPosH /= shadowPosH.w;
        
        const float2 TexC = shadowPosH.xy;
        const float Depth = shadowPosH.z;
        
        return depthMap.SampleCmpLevelZero(sampComp, float3(TexC, light.BaseIndex), Depth);
    }
    else if (light.Type == PointLight) {
        const float3 Direction = posW.xyz - light.Position;
        const float3 Normalized = normalize(Direction);
        
        const uint FaceIndex = ShaderUtil::GetCubeFaceIndex(Direction);
        const float4x4 ViewProj = GetViewProjMatrix(light, FaceIndex);
                
        float4 shadowPosH = mul(float4(posW, 1.f), ViewProj);
        shadowPosH /= shadowPosH.w;        
        
        const float2 TexC = ShaderUtil::ConvertDirectionToUV(Normalized);
        const float Depth = shadowPosH.z;
        
        const uint Index = light.BaseIndex + FaceIndex;
        
        return depthMap.SampleCmpLevelZero(sampComp, float3(TexC, Index), Depth);
    }
    
    return 1.f;
}

//-----------------------------------------------------------------------------------------------------------
//
// 광원 유형에 맞게 그림자 맵 좌표를 구해 3×3 PCF를 적용한다.
//
// @param light : 광원 데이터
// @param depthMap : 그림자 깊이 배열 텍스처
// @param sampComp : 깊이 비교 샘플러
// @param posW : 월드 공간 셰이딩 위치
// @param ddxy : 축별 샘플 간격
//
// @return 필터링한 그림자 가시성 또는 지원하지 않는 광원 유형에 대해 1
//
//-----------------------------------------------------------------------------------------------------------
float CalcShadowPCF(
    LightData light
    , in Texture2DArray<float> depthMap
    , in SamplerComparisonState sampComp
    , in float3 posW
    , in float2 ddxy) {
    if (light.Type == DirectionalLight || light.Type == SpotLight) {
        float4 shadowPosH = mul(float4(posW, 1.f), light.Matrix1);
        shadowPosH /= shadowPosH.w;
        
        const float2 TexC = shadowPosH.xy;
        const float Depth = shadowPosH.z;
        
        return CalcShadowPCF3x3(depthMap, sampComp, TexC, ddxy, Depth, light.BaseIndex);
    }
    else if (light.Type == PointLight) {
        const float3 Direction = posW.xyz - light.Position;
        const float3 Normalized = normalize(Direction);
        
        const uint FaceIndex = ShaderUtil::GetCubeFaceIndex(Direction);
        const float4x4 ViewProj = GetViewProjMatrix(light, FaceIndex);
                
        float4 shadowPosH = mul(float4(posW, 1.f), ViewProj);
        shadowPosH /= shadowPosH.w;        
        
        const float2 TexC = ShaderUtil::ConvertDirectionToUV(Normalized);
        const float Depth = shadowPosH.z;
        
        const uint Index = light.BaseIndex + FaceIndex;
        
        return CalcShadowPCF3x3(depthMap, sampComp, TexC, ddxy, Depth, Index);
    }
    
    return 1.f;
}

#endif // __D3D12SHADOE_HLSLI__