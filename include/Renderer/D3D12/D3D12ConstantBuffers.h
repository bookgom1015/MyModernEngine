#ifndef __D3D12CONSTANTBUFFERS_H__
#define __D3D12CONSTANTBUFFERS_H__

struct PassCB {
	Mat4	View;
	Mat4	InvView;
	Mat4	Proj;
	Mat4	InvProj;
	Mat4	ViewProj;
	Mat4	InvViewProj;
	Mat4	PrevViewProj;
	Mat4	ViewProjTex;

	Vec3	EyePosW;
	FLOAT	__ConstantPad0__;

	Vec2	JitteredOffset;
	FLOAT	__ConstantPad1__;
	FLOAT	__ConstantPad2__;
};

struct ObjectCB {
	Mat4	World;
	Mat4	PrevWorld;
	Mat4	TexTransform;
	Vec4	Center;
	Vec4	Extents;
};

struct MaterialCB {
	Vec4	Albedo;

	FLOAT	Roughness;
	FLOAT	Metalness;
	FLOAT	__ConstantPad0__;
	FLOAT	__ConstantPad1__;

	Vec3	Specular;
	FLOAT	__ConstantPad2__;

	INT		AlbedoMapIndex;
	INT		NormalMapIndex;
	INT		AlphaMapIndex;
	INT		RoughnessMapIndex;

	INT		MetalnessMapIndex;
	INT		SpecularMapIndex;
	FLOAT	__ConstantPad3__;
	FLOAT	__ConstantPad4__;

	Mat4	MaterialTransform;
};

#endif // __D3D12CONSTANTBUFFERS_H__