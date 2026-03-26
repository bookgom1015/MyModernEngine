#ifndef __D3D12CONSTANTBUFFERS_H__
#define __D3D12CONSTANTBUFFERS_H__

#ifdef _HLSL
	#include "./../../include/LightData.h"
#else
	#include "LightData.h"
#endif

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
	Vec3	Albedo;
	FLOAT	__ConstantPad0__;

	FLOAT	Roughness;
	FLOAT	Metalness;
	FLOAT	__ConstantPad1__;
	FLOAT	__ConstantPad2__;

	Vec3	Specular;
	FLOAT	__ConstantPad3__;

	INT		AlbedoMapIndex;
	INT		NormalMapIndex;
	INT		AlphaMapIndex;
	INT		RoughnessMapIndex;

	INT		MetalnessMapIndex;
	INT		SpecularMapIndex;
	FLOAT	__ConstantPad4__;
	FLOAT	__ConstantPad5__;

	Mat4	MatTransform;
};

struct LightCB {
	UINT	LightCount;
	FLOAT	__ConstantPad0__;
	FLOAT	__ConstantPad1__;
	FLOAT	__ConstantPad2__;

	LightData Lights[MAX_LIGHT_COUNT];
};

struct GizmoCB {
	Mat4	UnitViewProj;
	Mat4	InvView;

	Vec2	ViewportSize;	// ex) (width, height)
	float	LineThickness;	// pixel 단위
	FLOAT	__ConstantPad0__;
};

#endif // __D3D12CONSTANTBUFFERS_H__