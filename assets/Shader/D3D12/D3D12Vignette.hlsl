#ifndef __D3D12VIGNETTE_HLSL__
#define __D3D12VIGNETTE_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"

Vignette_Default_RootConstants(b0)

Texture2D<HDR_FORMAT> gi_BackBuffer : register(t0);

FitToScreenVertexOut

FitToScreenVertexShader

FitToScreenMeshShader

HDR_FORMAT PS(in VertexOut pin) : SV_Target {
    float3 scene = gi_BackBuffer.SampleLevel(gsamLinearClamp, pin.TexC, 0).rgb;
    
    const float2 UV = pin.TexC * 2.f - 1.f;
    const float Vignette = 1.f - dot(UV, UV) * gStrength;

    scene *= saturate(Vignette);
    
    return float4(scene, 1.f);
}

#endif // __D3D12VIGNETTE_HLSL__