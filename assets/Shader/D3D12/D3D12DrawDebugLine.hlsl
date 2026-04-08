#ifndef __D3D12DRAWDEBUGLINE_HLSL__
#define __D3D12DRAWDEBUGLINE_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"

ConstantBuffer<PassCB> cbPass : register(b0);

struct VertexIn {
    float3 PosL  : POSITION0;
    float4 Color : COLOR0;
};

struct VertexOut {
    float4 PosH  : SV_POSITION;
    float4 Color : COLOR;
};

VertexOut VS(in VertexIn vin) {
    VertexOut vout = (VertexOut)0;
    
    vout.PosH = mul(float4(vin.PosL, 1.0f), cbPass.ViewProj);
    vout.Color = vin.Color;
    
    return vout;
}

HDR_FORMAT PS(in VertexOut pin) : SV_TARGET {
    return pin.Color;
}

#endif // __D3D12DRAWDEBUGLINE_HLSL__