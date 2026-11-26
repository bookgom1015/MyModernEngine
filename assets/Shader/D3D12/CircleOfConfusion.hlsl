#ifndef __D3D12_CIRCLEOFCONFUSION_HLSL__
#define __D3D12_CIRCLEOFCONFUSION_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"

ConstantBuffer<PassCB> cbPass : register(b0);

DOF_CircleOfConfusion_RootConstants(b1)

Texture2D<DepthStencilBuffer::DepthBufferFormat> gi_DepthMap : register(t0);

RWStructuredBuffer<float> gi_FocalDistanceBuffer        : register(u0);
RWTexture2D<Dof::CircleOfConfusionMapFormat> go_CoCMap  : register(u1);

[numthreads(
    Dof::ThreadGroup::Default::Width,
    Dof::ThreadGroup::Default::Height,
    Dof::ThreadGroup::Default::Depth)]
void CS(in uint2 DTid : SV_DispatchThreadID) {
    const float ZDepth = gi_DepthMap[DTid];
    const float DepthV = ShaderUtil::NdcDepthToViewDepth(ZDepth, cbPass.Proj);
    
    const float FocalDist = gi_FocalDistanceBuffer[0];
    const float Diff = DepthV - FocalDist;
    const float CoC = clamp(Diff / gFocusRange, -1.f, 1.f);
    
    go_CoCMap[DTid] = CoC;
}

#endif // __D3D12_CIRCLEOFCONFUSION_HLSL__