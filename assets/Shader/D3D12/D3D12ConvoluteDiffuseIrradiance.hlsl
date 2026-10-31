#ifndef __D3D12CONVOLUTESPECULARIRRADIANCE_HLSL__
#define __D3D12CONVOLUTESPECULARIRRADIANCE_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _CUBE_COORD
#define _CUBE_COORD
#endif 

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"

ConstantBuffer<ProjectToCubeCB> cbProjectToCube : register(b0);

TextureCube<HDR_FORMAT> gi_EnvCubeMap : register(t0);

struct VertexOut {
    float3 PosL : POSITION;
};

struct GeoOut {
    float4 PosH : SV_Position;
    float3 PosL : POSITION;
    uint ArrayIndex : SV_RenderTargetArrayIndex;
};

FitToCubeVertexShader

FitToCubeGeometryShader(cbProjectToCube)

float3 ConvoluteIrradiance(in float3 pos) {
    // 월드 벡터는 원점에서 WorldPos 방향으로 정렬된 접평면의 법선역할을 합니다.
    // 이 법선을 기준으로 환경에서 들어오는 모든 복사휘도(radiance)를 계산합니다.
    // 그 결과는 -Normal 방향에서 들어오는 빛의 복사휘도이며,
    // PBR 셰이더에서는 이를 사용해 복사조도(irradiance)를 샘플링합니다.
    const float3 N = normalize(pos);

    float3 irradiance = (float3) 0.f;

    float3 up = float3(0.f, 1.f, 0.f);
    const float3 Right = normalize(cross(up, N));
    up = normalize(cross(N, Right));

    uint numSamples = 0;
    const float SampleDelta = 0.0075f;
    for (float phi = 0.f, phi_end = 2.f * PI; phi < phi_end; phi += SampleDelta) {
        for (float theta = 0.f, theta_end = 0.5f * PI; theta < theta_end; theta += SampleDelta) {
            const float CosTheta = cos(theta);
            const float SinTheta = sin(theta);

            // 구면 좌표계에서 카르테시안 좌표계로 변환 (접선 공간에서)
            const float3 TangentSample = float3(SinTheta * cos(phi), SinTheta * sin(phi), CosTheta);
            // 접선 공간에서 월드 공간으로 변환
            const float3 SampleVec = TangentSample.x * Right + TangentSample.y * up + TangentSample.z * N;

            float4 gammaDecoded = gi_EnvCubeMap.Sample(gsamLinearClamp, SampleVec);
            irradiance += gammaDecoded.rgb * CosTheta * SinTheta;
            ++numSamples;
        }
    }
    
    irradiance = PI * irradiance * (1.f / (float) numSamples);

    return irradiance;
}

HDR_FORMAT PS(in GeoOut pin) : SV_Target {
    const float3 Irradiance = ConvoluteIrradiance(pin.PosL);
    
    return float4(Irradiance, 1.f);
}

#endif // __D3D12CONVOLUTESPECULARIRRADIANCE_HLSL__