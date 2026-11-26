#ifndef __D3D12CALCULATESCATTERINGANDDENSITY_HLSL__
#define __D3D12CALCULATESCATTERINGANDDENSITY_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"
#include "./../../assets/Shader/LightingUtil.hlsli"

#include "./../../assets/Shader/D3D12/D3D12Shadow.hlsli"
#include "./../../assets/Shader/D3D12/D3D12VolumetricLight.hlsli"

ConstantBuffer<PassCB>	cbPass	: register(b0);
ConstantBuffer<LightCB> cbLight	: register(b1);

VolumetricLight_CalculateScatteringAndDensity_RootConstants(b2)

Texture2DArray<float> gi_ShadowMap : register(t0);

RWTexture3D<VolumetricLight::FrustumVolumeMapFormat> go_FrustumVolumeMap : register(u0);

[numthreads(
	VolumetricLight::ThreadGroup::CalculateScatteringAndDensity::Width, 
	VolumetricLight::ThreadGroup::CalculateScatteringAndDensity::Height, 
	VolumetricLight::ThreadGroup::CalculateScatteringAndDensity::Depth)]
void CS(in uint3 DTid : SV_DispatchThreadId) {
	uint3 dims;
	go_FrustumVolumeMap.GetDimensions(dims.x, dims.y, dims.z);
	if (any(DTid >= dims)) return;
	
	const uint Idx = Random::Hash3D(DTid) + gFrameCount;
	const float Jitter = Random::HaltonSequence[Idx % MAX_HALTON_SEQUENCE].z - 0.5f;

	const float3 PosW = ShaderUtil::ThreadIdToWorldPosition(
		float3((float2)DTid.xy, (float)DTid.z + Jitter), 
		dims, gDepthExponent, gNearZ, gFarZ, cbPass.InvView, cbPass.InvProj);

	const float3 ToEyeW = normalize(cbPass.EyePosW - PosW);

	float3 Li = 0.f;

	[loop]
	for (uint i = 0; i < cbLight.LightCount; ++i) {
		const LightData light = cbLight.Lights[i];

		float3 LightToPos = 0.f;
		float3 L = 0.f;          // light -> sample 방향이 아니라, sample -> light 방향으로 맞춰서 쓸 것
		float Ld = 0.f;
		float falloff = 1.f;
		float visibility = 1.f;

		if (light.Type == DirectionalLight) {
			// directional은 광원이 무한대에 있으므로 sample -> light 방향은 -light.Direction
			L = normalize(-light.Direction);
		}
		else if (light.Type == TubeLight || light.Type == RectangleLight) {
			// 아직 shadow(depth) map 미지원
			continue;
		}
		else {
			LightToPos = PosW - light.Position;
			Ld = length(LightToPos);

			if (Ld <= 1e-5f)
				continue;

			// sample -> light 방향
			L = -LightToPos / Ld;
		}
		
		if (light.Type == PointLight) {
			visibility = CalcShadow(light, gi_ShadowMap, gsamShadow, PosW.xyz);
			falloff = CalcInverseSquareAttenuation(Ld, light.AttenuationRadius);
		}
		else if (light.Type == SpotLight) {
			visibility = CalcShadow(light, gi_ShadowMap, gsamShadow, PosW.xyz);
			falloff = CalcInverseSquareAttenuation(Ld, light.AttenuationRadius);

			// spotlight cone attenuation
			// L             : sample -> light
			// -L            : light -> sample
			// light.Direction : spotlight가 비추는 방향 (light -> forward)
			const float3 LightForward = normalize(light.Direction);
			const float cosTheta = dot(-L, LightForward);

			const float outerCos = cos(light.OuterConeAngle * DEG2RAD);

			// inner cone 값이 없다면 일단 outer 기준 hard cut
			const float spotAtten = step(outerCos, cosTheta);

			visibility *= spotAtten;
			falloff *= spotAtten;
		}
		else if (light.Type == DirectionalLight) {
			visibility = CalcShadow(light, gi_ShadowMap, gsamShadow, PosW.xyz);
		}

		const float PhaseFunction =
			VolumetricLight::HenyeyGreensteinPhaseFunction(L, ToEyeW, gAnisotropicCoefficient);

		Li += visibility * light.Color * light.Intensity * falloff * PhaseFunction;
	}
	
	go_FrustumVolumeMap[DTid] = float4(Li * gUniformDensity, gUniformDensity);
}

#endif // __D3D12CALCULATESCATTERINGANDDENSITY_HLSL__