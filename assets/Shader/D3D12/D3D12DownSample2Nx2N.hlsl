#ifndef __D3D12DOWNSAMPLE2NX2N_HLSL__
#define __D3D12DOWNSAMPLE2NX2N_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef KERNEL_RADIUS
#define KERNEL_RADIUS 1
#endif

static const int KERNEL_SIZE = (KERNEL_RADIUS * 2);

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"

TextureScaler_DownSample2Nx2N_RootConstants(b0)

Texture2D<HDR_FORMAT>   gi_InputMap  : register(t0);
RWTexture2D<HDR_FORMAT> go_OutputMap : register(u0);

[numthreads(
    TextureScaler::ThreadGroup::DownSample2Nx2N::Width,
    TextureScaler::ThreadGroup::DownSample2Nx2N::Height,
    TextureScaler::ThreadGroup::DownSample2Nx2N::Depth)]
void CS(in uint2 DTid : SV_DispatchThreadID) {
    const uint2 DstIndex = DTid;    
    if (any(DstIndex >= gDstTexDim)) return;
    
    const uint2 SrcCenter = DstIndex * 2;
    
    const int2 BaseIndex = int2(SrcCenter) - int2(2, 2);
    
    HDR_FORMAT valueSum = 0.f;
    float weightSum = 0.f;
    
    [unroll]
    for (int y = 0; y < KERNEL_SIZE; ++y) {
        const int SrcY = clamp(BaseIndex.y + y, 0, int(gSrcTexDim.y) - 1);
        
        [unroll]
        for(int x = 0; x < KERNEL_SIZE; ++x) {
            const int SrcX = clamp(BaseIndex.x + x, 0, int(gSrcTexDim.x) - 1);
            
            const HDR_FORMAT Color = gi_InputMap.Load(int3(SrcX, SrcY, 0));
            valueSum += Color;
            weightSum += 1.f;
        }
    }
    
    go_OutputMap[DTid] = valueSum / weightSum;
}

#endif // __D3D12DOWNSAMPLE2NX2N_HLSL__