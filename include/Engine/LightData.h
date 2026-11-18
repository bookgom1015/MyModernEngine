#ifndef __LIGHTDATA_H__
#define __LIGHTDATA_H__

#ifndef _HLSL
#include "Core/minimum.h"

namespace ELight {
	enum Type {
		E_Directional,
		E_Point,
		E_Spot,
		E_Tube,
		E_Rectangle,
		Count
	};
}
#endif

#define MAX_LIGHT_COUNT 16
#define MAX_TEX_PER_LIGHT 6
#define MAX_LIGHT_TEX_COUNT (MAX_LIGHT_COUNT * MAX_TEX_PER_LIGHT)

#ifdef _HLSL
static UINT DirectionalLight	= 0;
static UINT PointLight			= 1;
static UINT SpotLight			= 2;
static UINT TubeLight			= 3;
static UINT RectangleLight		= 4;
#else
static UINT DirectionalLight	= ELight::E_Directional;
static UINT PointLight			= ELight::E_Point;
static UINT SpotLight			= ELight::E_Spot;
static UINT TubeLight			= ELight::E_Tube;
static UINT RectangleLight		= ELight::E_Rectangle;
#endif

struct LightData {
	Mat4	Matrix0;
	Mat4	Matrix1;
	Mat4	Matrix2;
	Mat4	Matrix3;
	Mat4	Matrix4;
	Mat4	Matrix5;

	Vec3	Color;
	FLOAT	Intensity;

	Vec3	Direction;			// 방향광과 스포트라이트에 사용합니다.
	FLOAT	Radius;				// 점광원과 튜브 광원에 사용합니다.

	Vec3	Position;			// 점광원, 스포트라이트, 튜브 광원, 직사각형 광원에 사용합니다.
	UINT	Type;

	Vec3	Position1;			// 튜브 광원과 직사각형 광원에 사용하는 끝점입니다.
	FLOAT	AttenuationRadius;	// 점광원과 스포트라이트에 사용합니다.

	Vec3	Position2;			// 직사각형 광원에 사용하는 끝점입니다.
	FLOAT	InnerConeAngle;		// 스포트라이트에만 사용합니다. 단위는 도입니다.

	Vec3	Position3;			// 직사각형 광원에 사용하는 끝점입니다.
	FLOAT	OuterConeAngle;		// 스포트라이트에만 사용합니다. 단위는 도입니다.

	Vec2	RectSize;			// 직사각형 광원에만 사용합니다.
	UINT	BaseIndex;			 
	UINT	IndexStride;

	Vec3	AmbientColor;		// 방향광에만 사용합니다.
	FLOAT	__ConstantPad0__;

	Vec3	Up;					// 직사각형 광원에만 사용합니다.
	FLOAT	__ConstantPad1__;

	Vec3	Right;				// 직사각형 광원에만 사용합니다.
	FLOAT	__ConstantPad2__;

	Vec3	Center;				// 직사각형 광원에만 사용합니다.
	FLOAT	__ConstantPad3__;
};

#endif // __LIGHTDATA_H__