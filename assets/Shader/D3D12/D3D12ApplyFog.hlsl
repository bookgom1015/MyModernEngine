#ifndef __APPLYFOG_HLSL__
#define __APPLYFOG_HLSL__

#ifndef _HLSL
#define _HLSL
#endif

#ifndef _FIT_TO_SCREEN_COORD
#define _FIT_TO_SCREEN_COORD
#endif

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"

#include "./../../assets/Shader/D3D12/D3D12VolumetricLight.hlsli"

ConstantBuffer<PassCB> cbPass : register(b0);

VolumetricLight_ApplyFog_RootConstants(b1)

Texture2D<GBuffer::PositionMapFormat>				gi_PositionMap		: register(t0);
Texture3D<VolumetricLight::FrustumVolumeMapFormat>	gi_FrustumVolumeMap	: register(t1);

FitToScreenVertexOut

FitToScreenVertexShader

FitToScreenMeshShader

HDR_FORMAT PS(VertexOut pin) : SV_TARGET {
	uint3 dims;
	gi_FrustumVolumeMap.GetDimensions(dims.x, dims.y, dims.z);

	const float4 PosW = gi_PositionMap.SampleLevel(gsamLinearClamp, pin.TexC, 0);
	
	float4 posV = mul(PosW, cbPass.View);
	if (!GBuffer::IsValidPosition(PosW)) posV.z = dims.z - 1;
	
	const float NdcDepth = ShaderUtil::ViewDepthToNdcDepth(posV.z, gDepthExponent, gNearZ, gFarZ);	
	const float3 TexC = float3(pin.TexC, NdcDepth);

#ifdef TriCubicSampling
	const float4 ScatteringAndTransmittance = VolumetricLight::Tex3DTricubic(gi_FrustumVolumeMap, gsamLinearClamp, TexC, dims);
#else
	const float4 ScatteringAndTransmittance = gi_FrustumVolumeMap.SampleLevel(gsamLinearClamp, TexC, 0);
#endif
	const float3 ScatteringColor = ScatteringAndTransmittance.rgb;

	return float4(ScatteringColor, ScatteringAndTransmittance.a);
}

#endif // __APPLYFOG_HLSL__