#ifndef __MATERIALDATA_H__
#define __MATERIALDATA_H__

struct MaterialData {
	UINT AlbedoMapIndex{ UINT_MAX };

	Vec3 Albedo;
	Vec3 Specular;
	float Roughness;
	float Metalic;
};

#endif // __MATERIALDATA_H__
