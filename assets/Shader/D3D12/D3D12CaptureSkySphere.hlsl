#ifndef __D3D12CAPTURESKYSPHERE_HLSL__
#define __D3D12CAPTURESKYSPHERE_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"

ConstantBuffer<PassCB>          cbPass          : register(b0);
ConstantBuffer<ProjectToCubeCB> cbProjectToCube : register(b1);
ConstantBuffer<ObjectCB>        cbObject        : register(b2);

TextureCube<EnvironmentManager::EnvironmentCubeMapFormat> gi_EnvCubeMap : register(t0);

VERTEX_IN

struct VertexOut {
    float3 PosL : POSITION0;
};

struct GeoOut {
    float4 PosH     : SV_Position;
    float3 SampleDir : POSITION0;
    uint   ArrayIndex : SV_RenderTargetArrayIndex;
};

VertexOut VS(in VertexIn vin) {
    VertexOut vout = (VertexOut)0;
    vout.PosL = vin.PosL;
    return vout;
}

[maxvertexcount(18)]
void GS(triangle VertexOut gin[3], inout TriangleStream<GeoOut> triStream) {
    [unroll]
    for (uint face = 0; face < 6; ++face) {
        GeoOut gout[3];

        [unroll]
        for (uint i = 0; i < 3; ++i) {
            float3 posL = gin[i].PosL;

            // sky lookup direction
            gout[i].SampleDir = posL;

            // sky sphere를 캡처 위치 기준으로 이동
            // 권장: cbProjectToCube에 CapturePosW를 넣어서 사용
            float3 capturePosW = cbPass.EyePosW;

            float4 posW = mul(float4(posL, 1.f), cbObject.World);
            posW.xyz += capturePosW;

            float4 posV = mul(posW, cbProjectToCube.Views[face]);

            // far plane에 고정
            gout[i].PosH = mul(posV, cbProjectToCube.Proj).xyww;
            gout[i].ArrayIndex = face;
        }

        triStream.Append(gout[0]);
        triStream.Append(gout[1]);
        triStream.Append(gout[2]);
        triStream.RestartStrip();
    }
}

HDR_FORMAT PS(in GeoOut pin) : SV_Target {
    return gi_EnvCubeMap.SampleLevel(gsamAnisotropicClamp, normalize(pin.SampleDir), 0);
}

#endif // __D3D12CAPTURESKYSPHERE_HLSL__