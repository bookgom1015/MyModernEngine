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

	float3 Li = 0.f; // Ambient lights;

	[loop]
	for (uint i = 0; i < cbLight.LightCount; ++i) {
		const LightData light = cbLight.Lights[i];

		float3 direction = 0.f;
		float Ld = 0.f;
		float falloff = 1.f;

		if (light.Type == DirectionalLight) {
			direction = light.Direction;
		}
		else if (light.Type == TubeLight || light.Type == RectangleLight) {
			// Tube and rectangular light do not have shadow(depth) map for now, 
			//  so these can not calculate visibility.
			continue;
		}
		else {
			direction = PosW - light.Position;
			Ld = length(direction);
		}
		
		float visibility = 1.f;
		if (light.Type == PointLight) {
			visibility = CalcShadow(light, gi_ShadowMap, gsamShadow, PosW.xyz);

			falloff = CalcInverseSquareAttenuation(Ld, light.AttenuationRadius);
		}
		else if (light.Type == DirectionalLight || light.Type == SpotLight) {
			visibility = CalcShadow(light, gi_ShadowMap, gsamShadow, PosW.xyz);
		}

		const float PhaseFunction = VolumetricLight::HenyeyGreensteinPhaseFunction(direction, ToEyeW, gAnisotropicCoefficient);

		Li += visibility * light.Color * light.Intensity * falloff * PhaseFunction;
	}
	
	go_FrustumVolumeMap[DTid] = float4(Li * gUniformDensity, gUniformDensity);
}

#endif // __D3D12CALCULATESCATTERINGANDDENSITY_HLSL__