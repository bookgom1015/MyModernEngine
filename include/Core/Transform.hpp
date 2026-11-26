#pragma once

struct TransformTRS {
	Vec3 Translation = Vec3(0.f);
	Quat Rotation = Quat(0.f, 0.f, 0.f, 1.f);
	Vec3 Scale = Vec3(1.f);
};