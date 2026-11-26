#pragma once

#include "Core/minimum.h"

namespace ERenderObjectDomain {
	enum Type {
		E_Opaque,
		E_Masked,
		E_Transparent,
		E_SkySphere,
		Count
	};
}

namespace ERenderDomain = ERenderObjectDomain;

struct RenderMaterialDesc {
	Vec3 Albedo = Vec3(1.f);
	float Roughness = 0.5f;
	float Metalness = 0.f;
	float Specular = 0.f;
	std::wstring AlbedoMapKey;
	std::wstring NormalMapKey;
};

struct RenderObjectDesc {
	// 강체 애니메이션 노드 변환을 포함한 최종 프리미티브 행렬입니다.
	Mat4 World = Identity4x4;
	Mat4 PrevWorld = Identity4x4;

	RenderMaterialDesc Material;

	std::wstring EnvironmentMapKey;

	int SkinIndex = -1;
	std::vector<Mat4> BonePalette;
	std::vector<Mat4> PrevBonePalette;

	std::wstring StaticMeshKey;
	std::wstring SkinnedMeshKey;

	int SourcePrimitiveIndex = -1;
	int StaticPrimitiveIndex = -1;
	int SkinnedPrimitiveIndex = -1;
};