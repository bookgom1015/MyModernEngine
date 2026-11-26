#ifndef __D3D12_BOKEHBLURNXN_HLSL__
#define __D3D12_BOKEHBLURNXN_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"
#include "./../../assets/Shader/D3D12/D3D12DofSampling.hlsli"

DOF_BokehBlur3x3_RootConstants(b0)

Texture2D<HDR_FORMAT> gi_InputMap : register(t0);

FitToScreenVertexOut

FitToScreenVertexShader

FitToScreenMeshShader

HDR_FORMAT PS(VertexOut pin) : SV_TARGET {
	const int2 centerPixel = int2(pin.PosH.xy);
	const float4 center = gi_InputMap.Load(int3(centerPixel, 0));
	const float centerCoC = gi_CoCMap.Load(int3(centerPixel, 0));
	const float radius = abs(centerCoC) * gMaxBlurRadius;
	// 선택한 조리개 모양을 유지하면서 성긴 샘플링으로 생긴 밝기 차이를 완화합니다.
	const float blend = 0.35f * DofBlurBlend(radius);
	if (blend <= 0.f) return center;

	const int2 textureSize = DofTextureSize();
	const float centerDepth = gProtectDepthEdges ? DofViewDepth(centerPixel) : 0.f;
	float3 colorSum = center.rgb;
	float weightSum = 1.f;

	[unroll]
	for (int y = -1; y <= 1; ++y) {
		[unroll]
		for (int x = -1; x <= 1; ++x) {
			if (x == 0 && y == 0) continue;

			const int2 pixel = centerPixel + int2(x, y);
			if (any(pixel < 0) || any(pixel >= textureSize)) continue;

			float surfaceWeight = 1.f;
			if (gProtectDepthEdges) {
				const float sampleCoC = gi_CoCMap.Load(int3(pixel, 0));
				surfaceWeight = DofSurfaceWeight(centerDepth, DofViewDepth(pixel), centerCoC, sampleCoC);
			}
			const float spatialWeight = (x == 0 || y == 0) ? 0.5f : 0.25f;
			const float weight = surfaceWeight * spatialWeight;
			colorSum += gi_InputMap.Load(int3(pixel, 0)).rgb * weight;
			weightSum += weight;
		}
	}

	return float4(lerp(center.rgb, colorSum / weightSum, blend), center.a);
}

#endif // __D3D12_BOKEHBLURNXN_HLSL__
