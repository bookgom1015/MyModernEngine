#pragma once

namespace ECollider {
	enum Type {
		E_Box = 0,
		E_Sphere,
		E_Capsule,
		E_Mesh,
		Count
	};
}

struct DebugColliderShape {
	ECollider::Type Type;

	Mat4 World;

	Vec3 HalfExtents;   // Box
	float Radius;		// Sphere, Capsule
	float HalfSegment;	// Capsule

	UINT Flags = 0;
};
struct DebugLineVertex {
	Vec3 Position;
	Vec4 Color;
};

namespace EDebugColliderFlags {
	enum Flag {
		E_None		= 0,
		E_Collided	= 1 << 0,
		E_Trigger	= 1 << 1,
		E_Sleeping	= 1 << 2
	};
}
