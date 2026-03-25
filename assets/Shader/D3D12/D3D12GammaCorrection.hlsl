#ifndef __D3D12GAMMACORRECTION_HLSL__
#define __D3D12GAMMACORRECTION_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"

GammaCorrection_Default_RootConstants(b0)

Texture2D<SDR_FORMAT> gi_BackBuffer : register(t0);

FitToScreenVertexOut

FitToScreenVertexShader

FitToScreenMeshShader

SDR_FORMAT PS(in VertexOut pin) : SV_Target {
    const float3 Color = gi_BackBuffer.SampleLevel(gsamPointClamp, pin.TexC, 0).rgb;
    const float3 ColorCorrected = pow(Color, 1.f / gGamma);
    
    return float4(ColorCorrected, 1.f);
}

#endif // __D3D12GAMMACORRECTION_HLSL__