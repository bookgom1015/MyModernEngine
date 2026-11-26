#ifndef __D3D12BLENDBLOOMWITHDOWNSAMPLED_HLSL__
#define __D3D12BLENDBLOOMWITHDOWNSAMPLED_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"

Bloom_BlendBloomWithDownSampled_RootConstants(b0)

Texture2D<Bloom::HighlightMapFormat>    gi_LowerScaleMap    : register(t0);
RWTexture2D<Bloom::HighlightMapFormat>  gio_HigherScaleMap  : register(u0);

[numthreads(
    Bloom::ThreadGroup::Default::Width,
    Bloom::ThreadGroup::Default::Height,
    Bloom::ThreadGroup::Default::Depth)]
void CS(in uint2 DTid : SV_DispatchThreadID) {
    const float2 TexC = ((float2)DTid + 0.5f) * gInvTexDim;

    const float4 LowSample = gi_LowerScaleMap.SampleLevel(gsamLinearClamp, TexC, 0);
    const float4 HighSample = gio_HigherScaleMap[DTid];
    
    gio_HigherScaleMap[DTid] = lerp(HighSample, LowSample, 0.5f);
}

#endif // __D3D12BLENDBLOOMWITHDOWNSAMPLED_HLSL__