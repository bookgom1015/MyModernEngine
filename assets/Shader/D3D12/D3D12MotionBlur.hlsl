#ifndef __D3D12MOTIONBLUR_HLSL__
#define __D3D12MOTIONBLUR_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"

Texture2D<HDR_FORMAT>								gi_BackBuffer	: register(t0);
Texture2D<DepthStencilBuffer::DepthBufferFormat>	gi_DepthMap		: register(t1);
Texture2D<GBuffer::VelocityMapFormat>				gi_VelocityMap	: register(t2);

MotionBlur_Default_RootConstants(b0)

FitToScreenVertexOut

FitToScreenVertexShader

FitToScreenMeshShader

HDR_FORMAT PS(VertexOut pin) : SV_TARGET {
	float2 velocity = gi_VelocityMap.Sample(gsamPointWrap, pin.TexC);
	float3 colorSum = gi_BackBuffer.Sample(gsamPointWrap, pin.TexC).rgb;

	if (!GBuffer::IsValidVelocity(velocity)) velocity = (float2)0.f;
	if (all(velocity < 1e-6f)) return float4(colorSum, 1.f);

	velocity *= gIntensity;
	velocity = clamp(velocity, (float2)-gLimit,(float2)gLimit);
	
	const float centerDepth = gi_DepthMap.Sample(gsamDepthMap, pin.TexC);
	
	uint count = 1;
	float2 forward = pin.TexC;
	float2 inverse = pin.TexC;
	for (uint i = 0; i < gSampleCount; ++i) {
		forward += velocity;
		inverse -= velocity;
	
		if (centerDepth < gi_DepthMap.Sample(gsamDepthMap, forward) + gDepthBias) {
			colorSum += gi_BackBuffer.Sample(gsamLinearClamp, forward).rgb;
			++count;
		}
		if (centerDepth < gi_DepthMap.Sample(gsamDepthMap, inverse) + gDepthBias) {
			colorSum += gi_BackBuffer.Sample(gsamLinearClamp, inverse).rgb;
			++count;
		}
	}	
	colorSum /= count;

	return float4(colorSum, 1.f);
}

#endif // __D3D12MOTIONBLUR_HLSL__