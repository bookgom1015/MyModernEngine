#ifndef __D3D12_BOKEH_HLSL__
#define __D3D12_BOKEH_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"
#include "./../../assets/Shader/D3D12/D3D12DofSampling.hlsli"

DOF_Bokeh_RootConstants(b0)

Texture2D<HDR_FORMAT> gi_BackBuffer : register(t0);

FitToScreenVertexOut

FitToScreenVertexShader

FitToScreenMeshShader

// Values follow ShaderArgumentManager::_DOF::BokehShape.
static const uint BOKEH_HEXAGON = 1;
static const uint BOKEH_SQUARE = 2;
static const uint BOKEH_DIAMOND = 3;

float ApertureDistance(float2 offset) {
	const float2 p = abs(offset);
	switch (gBokehShape) {
	case BOKEH_HEXAGON:
		return max(p.y * 1.1547005f, p.x + p.y * 0.5773503f);
	case BOKEH_SQUARE:
		return max(p.x, p.y);
	case BOKEH_DIAMOND:
		return p.x + p.y;
	default:
		return length(p);
	}
}

float HighlightWeight(float3 color, float coc) {
	const float highlight = smoothstep(gThreshold, 1.f, abs(coc));
	if (highlight <= 0.f) return 1.f;

	const float luminance = max(0.f, dot(color, float3(0.2126f, 0.7152f, 0.0722f)));
	return lerp(1.f, pow(1.f + luminance / (1.f + luminance), gHighlightPower - 1.f), highlight);
}

HDR_FORMAT PS(VertexOut pin) : SV_TARGET {
	const int2 centerPixel = int2(pin.PosH.xy);
	const float4 center = gi_BackBuffer.Load(int3(centerPixel, 0));
	const float centerCoC = gi_CoCMap.Load(int3(centerPixel, 0));
	const float radius = abs(centerCoC) * gBokehRadius * gSampleCount;
	const float blend = DofBlurBlend(radius);
	if (blend <= 0.f) return center;

	const int sampleRadius = min((int)gSampleCount, (int)ceil(radius));
	const float2 sampleScale = float2(1.f, radius) / sampleRadius;
	const int2 textureSize = DofTextureSize();
	const float centerDepth = gProtectDepthEdges ? DofViewDepth(centerPixel) : 0.f;
	float weightSum = HighlightWeight(center.rgb, centerCoC);
	float3 colorSum = center.rgb * weightSum;

	[loop]
	for (int y = -sampleRadius; y <= sampleRadius; ++y) {
		[loop]
		for (int x = -sampleRadius; x <= sampleRadius; ++x) {
			const float2 offset = float2(x, y);
			const float apertureWeight = saturate((1.f - ApertureDistance(offset * sampleScale.x)) * sampleRadius + 0.5f);
			if (apertureWeight <= 0.f) continue;

			const int2 pixel = centerPixel + int2(round(offset * sampleScale.y));
			if (all(pixel == centerPixel) || any(pixel < 0) || any(pixel >= textureSize)) continue;

			const float sampleCoC = gi_CoCMap.Load(int3(pixel, 0));
			float surfaceWeight = 1.f;
			if (gProtectDepthEdges)
				surfaceWeight = DofSurfaceWeight(centerDepth, DofViewDepth(pixel), centerCoC, sampleCoC);
			if (surfaceWeight <= 0.f) continue;

			const float3 color = gi_BackBuffer.Load(int3(pixel, 0)).rgb;
			const float weight = apertureWeight * surfaceWeight * HighlightWeight(color, sampleCoC);
			colorSum += color * weight;
			weightSum += weight;
		}
	}

	return float4(lerp(center.rgb, colorSum / weightSum, blend), center.a);
}

#endif // __D3D12_BOKEH_HLSL__
