#ifndef __D3D12CALCPARTICALDEPTHDERIVATIVE_HLSL__
#define __D3D12CALCPARTICALDEPTHDERIVATIVE_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"
#include "./../../assets/Shader/D3D12/D3D12Svgf.hlsli"

SVGF_CalcDepthPartialDerivative_RootConstants(b0)

Texture2D<DepthStencilBuffer::DepthBufferFormat>   gi_DepthMap                  : register(t0);
RWTexture2D<Svgf::DepthPartialDerivativeMapFormat> go_DepthPartialDerivativeMap : register(u0);

[numthreads(
    Svgf::ThreadGroup::Default::Width,
    Svgf::ThreadGroup::Default::Height,
    Svgf::ThreadGroup::Default::Depth)]
void CS(in uint2 DTid : SV_DispatchThreadID) {
    const float2 TexC = (DTid + 0.5) * gInvTexDim;

    const float Top = gi_DepthMap.SampleLevel(gsamPointClamp, TexC + float2(0, -gInvTexDim.y), 0);
    const float Bottom = gi_DepthMap.SampleLevel(gsamPointClamp, TexC + float2(0, gInvTexDim.y), 0);
    const float Left = gi_DepthMap.SampleLevel(gsamPointClamp, TexC + float2(-gInvTexDim.x, 0), 0);
    const float Right = gi_DepthMap.SampleLevel(gsamPointClamp, TexC + float2(gInvTexDim.x, 0), 0);
    const float Center = gi_DepthMap.SampleLevel(gsamPointClamp, TexC, 0);
    
    const float2 BackwardDiff = Center - float2(Left, Top);
    const float2 ForwardDiff = float2(Right, Bottom) - Center;

	// Calculates partial derivatives as the min of absolute backward and forward differences.

	// Find the absolute minimum of the backward and foward differences in each axis
    // while preserving the sign of the difference.
    const float2 Ddx = float2(BackwardDiff.x, ForwardDiff.x);
    const float2 Ddy = float2(BackwardDiff.y, ForwardDiff.y);

    const uint2 MinIndex = {
        Svgf::GetIndexOfValueClosestToReference(0, Ddx),
		Svgf::GetIndexOfValueClosestToReference(0, Ddy)
    };
    float2 ddxy = float2(Ddx[MinIndex.x], Ddy[MinIndex.y]);

	// Clamp ddxy to a reasonable value to avoid ddxy going over surface boundaries
	// on thin geometry and getting background/foreground blended together on blur.
    const float MaxDdxy = 1;
    const float2 _Sign = sign(ddxy);
    ddxy = _Sign * min(abs(ddxy), MaxDdxy);

    go_DepthPartialDerivativeMap[DTid] = ddxy;
}

#endif // __D3D12CALCPARTICALDEPTHDERIVATIVE_HLSL__