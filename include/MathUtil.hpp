#pragma once

#include <SimpleMath.h>

using Vec2 = DirectX::SimpleMath::Vector2;
using Vec3 = DirectX::SimpleMath::Vector3;
using Vec4 = DirectX::SimpleMath::Vector4;

using Uint2 = DirectX::XMUINT2;
using Uint3 = DirectX::XMUINT3;
using Uint4 = DirectX::XMUINT4;

using Mat4 = DirectX::SimpleMath::Matrix;

namespace UnitVector {
	static const auto RightVector	= Vec3(1.f, 0.f, 0.f);
	static const auto UpVector		= Vec3(0.f, 1.f, 0.f);
	static const auto ForwardVector = Vec3(0.f, 0.f, 1.f);
	static const auto ZeroVector	= Vec3(0.f);
}

static const float PI = 3.14159265358979323846f;
static const float TwoPI = 2.f * PI;
static const float PITwo = PI / 2.f;

static const float RadToDeg = 180.f / PI;
static const float DegToRad = PI / 180.f;