#ifndef __D3D12MATERIALDATA_H__
#define __D3D12MATERIALDATA_H__

struct D3D12MaterialData {
	INT MaterialCBIndex;
	INT NumFramesDirty;
	INT AlbedoMapIndex;
	INT NormalMapIndex;

	INT AlphaMapIndex;
	INT RoughnessMapIndex;
	INT MetalnessMapIndex;
	INT SpecularMapIndex;

	Vec3 Albedo;
	FLOAT Roughness;

	Vec3 Specular;
	FLOAT Metalness;

	Mat4 MatTransform;
};

#endif // __D3D12MATERIALDATA_H__