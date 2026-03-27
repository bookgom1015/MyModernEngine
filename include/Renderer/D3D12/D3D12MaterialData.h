#ifndef __D3D12MATERIALDATA_H__
#define __D3D12MATERIALDATA_H__

struct D3D12MaterialData {
	INT MaterialCBIndex;

	Vec3 Albedo;

	FLOAT Roughness;
	FLOAT Metalness;
	FLOAT Specular;

	Mat4 MatTransform;
};

#endif // __D3D12MATERIALDATA_H__