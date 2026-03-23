#ifndef __D3D12MATERIALDATA_H__
#define __D3D12MATERIALDATA_H__

struct D3D12MaterialData {
	INT AlbedoMapIndex		= -1;
	INT NormalMapIndex		= -1;
	INT AlphaMapIndex		= -1;
	INT RoughnessMapIndex	= -1;

	INT MetalnessMapIndex	= -1;
	INT SpecularMapIndex	= -1;
	INT __ConstantPaddding_0__;
	INT __ConstantPaddding_1__;

	Vec3 Albedo;
	FLOAT Roughness;

	Vec3 Specular;
	FLOAT Metalness;

	Mat4 MateiralTransform;
};

#endif // __D3D12MATERIALDATA_H__