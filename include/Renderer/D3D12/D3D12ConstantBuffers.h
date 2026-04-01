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

	UINT	BoneStartOffset;
	UINT	__ConstantPad0__;
	UINT	__ConstantPad1__;
	UINT	__ConstantPad2__;
};

struct MaterialCB {
	Vec3	Albedo;
	FLOAT	__ConstantPad0__;

	FLOAT	Roughness;
	FLOAT	Metalness;
	FLOAT	Specular;
	FLOAT	__ConstantPad1__;

	INT		AlbedoMapIndex;
	INT		NormalMapIndex;
	INT		AlphaMapIndex;
	INT		RoughnessMapIndex;

	INT		MetalnessMapIndex;
	INT		SpecularMapIndex;
	FLOAT	__ConstantPad2__;
	FLOAT	__ConstantPad3__;

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
	Mat4	View;
	Mat4	InvView;
	Mat4	Proj;
	Mat4	UnitViewProj;

	Vec2	ViewportSize;	// ex) (width, height)
	float	LineThickness;	// pixel 단위
	FLOAT	__ConstantPad0__;
};

#endif // __D3D12CONSTANTBUFFERS_H__