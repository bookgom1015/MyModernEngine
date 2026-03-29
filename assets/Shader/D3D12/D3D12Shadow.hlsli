#ifndef __D3D12SHADOE_HLSLI__
#define __D3D12SHADOE_HLSLI__

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
        
        //return depthMap.SampleCmpLevelZero(sampComp, float3(TexC, light.BaseIndex), Depth);
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
        
        //return depthMap.SampleCmpLevelZero(sampComp, float3(TexC, Index), Depth);
        return CalcShadowPCF3x3(depthMap, sampComp, TexC, ddxy, Depth, Index);
    }
    
    return 1.f;
}

#endif // __D3D12SHADOE_HLSLI__