#ifndef __D3D12_CALCFOCALDISTANCE_HLSL__
#define __D3D12_CALCFOCALDISTANCE_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"

ConstantBuffer<PassCB> cbPass : register(b0);

Texture2D<GBuffer::PositionMapFormat> gi_PositionMap : register(t0);

RWStructuredBuffer<float> gio_FocalDistanceBuffer : register(u0);

groupshared float FocalDistanceSet[8 * 8];

[numthreads(
    Dof::ThreadGroup::Default::Width,
    Dof::ThreadGroup::Default::Height,
    Dof::ThreadGroup::Default::Depth)]
void CS(in uint2 DTid : SV_DispatchThreadID) {
    uint2 size;
    gi_PositionMap.GetDimensions(size.x, size.y);
    
    const uint2 HalfSize = size * 0.5f;
    const uint2 Index = HalfSize + (DTid - 4);
    
    const float4 PosW = gi_PositionMap[Index];
    const float4 PosV = mul(PosW, cbPass.View);
    
    FocalDistanceSet[DTid.x + DTid.y * 8] = PosV.z;
    
    GroupMemoryBarrierWithGroupSync();

    if (all(DTid == 0)) {
        const float PrevFocalDist = gio_FocalDistanceBuffer[0];
        
        float sum = 0.f;
        for (uint i = 0; i < 64; ++i)
            sum += FocalDistanceSet[i];
        const float Result = sum / 64.f;
        
        gio_FocalDistanceBuffer[0] = lerp(PrevFocalDist, Result, 0.02f);
    }
}

#endif // __D3D12_CALCFOCALDISTANCE_HLSL__