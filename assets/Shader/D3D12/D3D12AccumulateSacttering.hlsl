#ifndef __D3D12ACCUMULATESCATTERING_HLSL__
#define __D3D12ACCUMULATESCATTERING_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"
#include "./../../assets/Shader/LightingUtil.hlsli"

#include "./../../assets/Shader/D3D12/D3D12Shadow.hlsli"
#include "./../../assets/Shader/D3D12/D3D12VolumetricLight.hlsli"

VolumetricLight_AccumulateScattering_RootConstants(b0)

RWTexture3D<VolumetricLight::FrustumVolumeMapFormat> gio_FrustumVolumeMap : register(u0);

[numthreads(
	VolumetricLight::ThreadGroup::AccumulateScattering::Width, 
	VolumetricLight::ThreadGroup::AccumulateScattering::Height, 
	VolumetricLight::ThreadGroup::AccumulateScattering::Depth)]
void CS(in uint3 DTid : SV_DispatchThreadID) {
	uint3 dims;
	gio_FrustumVolumeMap.GetDimensions(dims.x, dims.y, dims.z);
	if (any(DTid >= dims)) return;

	float4 accum = float4(0.f, 0.f, 0.f, 1.f);
	uint3 pos = uint3(DTid.xy, 0.f);

	[loop]
	for (uint z = 0; z < dims.z; ++z) {
		pos.z = z;
		const float4 Slice = gio_FrustumVolumeMap[pos];
		const float Tickness = VolumetricLight::SliceTickness(
			(float)z / dims.z, gDepthExponent, gNearZ, gFarZ, dims.z);

		accum = VolumetricLight::ScatterStep(
			accum.rgb, accum.a, Slice.rgb, Slice.a, Tickness, gDensityScale);

		gio_FrustumVolumeMap[pos] = accum;
	}
}

#endif // __D3D12ACCUMULATESCATTERING_HLSL__