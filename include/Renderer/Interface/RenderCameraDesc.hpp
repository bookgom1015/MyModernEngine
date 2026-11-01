#pragma once

namespace EProjection {
	enum Type {
		E_Perspective,
		E_Orthographic
	};
}

struct RenderCameraDesc {
	float FovY;
	float NearZ;
	float FarZ;

	Vec3 Position;
	Vec3 Direction;
	Vec3 Up;
	
	Mat4 ViewMatrix;
	Mat4 ProjMatrix;

	Mat4 UnitViewMatrix;
	Mat4 OrthoProjMatrix;

	EProjection::Type ProjType;
};