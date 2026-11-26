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

	Vec3 HalfExtents;   // 박스
	float Radius;		// 구와 캡슐
	float HalfSegment;	// 캡슐

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
