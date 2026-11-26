#pragma once

namespace EProbeShape {
	enum Type {
		E_Sphere = 0,
		E_Box,
		Count
	};
}

namespace EProbeBakeState {
	enum Type {
		E_Unbaked = 0,
		E_Dirty,
		E_Baking,
		E_Ready,
		E_Failed,
		Count
	};
}

struct ReflectionProbeID {
	std::uint32_t Slot = UINT32_MAX;
	std::uint32_t Generation = 0;

	bool IsValid() const { return Slot != UINT32_MAX; }
};

struct ReflectionProbeDesc {
	EProbeShape::Type Shape = EProbeShape::E_Box;
	EProbeBakeState::Type BakeState = EProbeBakeState::E_Dirty;

	float Radius = 1.f;
	Vec3 BoxExtents = Vec3(1.f);

	Mat4 World = Identity4x4;

	int Priority = 0;
	float BlendDistance = 1.f;

	bool Enabled = false;
	bool UseBoxProjection = false;
};

struct ReflectionProbeSlot {
	ReflectionProbeDesc Desc;

	std::uint32_t Generation = 1;

	bool Alive = false;
};

struct ProbeSampleResult {
	std::uint32_t ProbeSlot = UINT32_MAX;   // 값이 유효하지 않으면 전역 환경을 사용합니다.

	float Blend = 1.f;

	bool UseGlobal = true;
};