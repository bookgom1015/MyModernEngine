#ifndef __LIGHTDATA_H__
#define __LIGHTDATA_H__

#define MAX_LIGHT_COUNT 32

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

	Vec3	Direction;			// directional/spot light 
	FLOAT	Radius;				// point/tube light 

	Vec3	Position;			// point/spot/tube/rectangle light 
	UINT	Type;

	Vec3	Position1;			// tube/rectangle light only (End Point)
	FLOAT	AttenuationRadius;	// point/spot light 

	Vec3	Position2;			// rectangle light only (End Point)
	FLOAT	InnerConeAngle;		// spot light only (degrees)

	Vec3	Position3;			// rectangle light only (End Point)
	FLOAT	OuterConeAngle;		// spot light only (degrees)

	Vec2	RectSize;			// rectangle light only
	UINT	BaseIndex;			 
	UINT	IndexStride;

	Vec3	AmbientColor;		// directional light only
	FLOAT	__ConstantPad0__;

	Vec3	Up;					// rentangle light only
	FLOAT	__ConstantPad1__;

	Vec3	Right;				// rentangle light only
	FLOAT	__ConstantPad2__;

	Vec3	Center;				// rentangle light only
	FLOAT	__ConstantPad3__;
};

#endif // __LIGHTDATA_H__