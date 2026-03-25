#ifndef __SHADERSTRUCTURES_HLSLI__
#define __SHADERSTRUCTURES_HLSLI__

struct Material {
	float4 Albedo;
	float3 FresnelR0;
	float  Roughness;
	float  Metalness;
};

#endif // __SHADERSTRUCTURES_HLSLI__