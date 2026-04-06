#pragma once

struct Processor {
	std::wstring Name;

	BOOL SupportMMX = FALSE;
	BOOL SupportSSE = FALSE;
	BOOL SupportSSE2 = FALSE;
	BOOL SupportSSE3 = FALSE;
	BOOL SupportSSSE3 = FALSE;
	BOOL SupportSSE4_1 = FALSE;
	BOOL SupportSSE4_2 = FALSE;
	BOOL SupportAVX = FALSE;
	BOOL SupportAVX2 = FALSE;
	BOOL SupportAVX512F = FALSE;  // AVX-512 Foundation
	BOOL SupportAVX512DQ = FALSE; // AVX-512 Doubleword & Quadword
	BOOL SupportAVX512BW = FALSE; // AVX-512 Byte & Word

	UINT64 Physical = 0;
	UINT64 Logical = 0;

	UINT64 TotalPhysicalMemory = 0;
	UINT64 AvailablePhysicalMemory = 0;
	UINT64 TotalVirtualMemory = 0;
	UINT64 AvailableVirtualMemory = 0;
};

struct TaskInfo {
	ETask::Type	Type;
	DWORD_PTR	Param_0;
	DWORD_PTR	Param_1;
	DWORD_PTR	Param_2;
};

struct ScriptInfo {
	std::string Name;
	ScriptFactory Factory;
};

struct Primitive {
	UINT BaseVertexLocation;
	UINT VertexCount;

	UINT StartIndexLocation;
	UINT IndexCount;
};

struct RenderObject {
	class GameObject* Object;
	int SourcePrimitiveIndex = -1;
	int StaticPrimitiveIndex = -1;
	int SkinnedPrimitiveIndex = -1;
};

struct TransformTRS {
	Vec3 Translation = Vec3(0.f);
	Quat Rotation = Quat(0.f, 0.f, 0.f, 1.f);
	Vec3 Scale = Vec3(1.f);
};

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

	int Priority = 0;
	float BlendDistance = 1.f;

	bool Enabled = false;
	bool UseBoxProjection = false;
};

struct ReflectionProbeSlot {
	ReflectionProbeDesc Desc{};
	uint32_t Generation = 1;
	bool Alive = false;
};