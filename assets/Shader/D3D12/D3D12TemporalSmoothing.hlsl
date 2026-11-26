#ifndef __D3D12TEMPORALSMOOTHING_HLSL__
#define __D3D12TEMPORALSMOOTHING_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"

EyeAdaption_TemporalSmoothing_RootConstants(b0) 

RWStructuredBuffer<EyeAdaption::Result> gi_AvgLogLum : register(u0);
RWTexture2D<float> go_Exposure                       : register(u1);
RWStructuredBuffer<float> gio_PrevLum                : register(u2);

[numthreads(1, 1, 1)]
void CS(in uint2 DTid : SV_DispatchThreadID) {
    const float TargetLum = gi_AvgLogLum[0].AvgLogLum;
    const float PrevLum = gio_PrevLum[0];
                                                                                                                                                                                                                                                                                                
    const float Delta = TargetLum - PrevLum;
    const float Glare = saturate((Delta - 1.f) / 3.f);
    
    const float UpSpeed = lerp(gUpSpeed, gGlareUpSpeed, Glare);

    const float k = (TargetLum > PrevLum) ? UpSpeed : gDownSpeed;
    const float Alpha = 1.f - exp(-k * gDeltaTime);

    const float Smoothed = lerp(PrevLum, TargetLum, Alpha);

    // 로그 휘도를 선형 노출 배율로 바꿔 DLSS와 톤매핑이 같은 값을 읽도록 합니다.
    go_Exposure[uint2(0, 0)] = exp2(-Smoothed) * gMiddleGrayKey;
    gio_PrevLum[0] = Smoothed;
}

#endif // __D3D12TEMPORALSMOOTHING_HLSL__
