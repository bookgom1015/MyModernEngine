#pragma once

class GameObject;

void SaveWString(FILE* pFile, const std::wstring& string);
std::wstring LoadWString(FILE* pFile);

void SaveAssetRef(FILE* _File, class Asset* _Asset);

// 64비트 FNV-1a
constexpr uint64_t HashString(std::string_view str);
// wchar_t 버전입니다.
constexpr uint64_t HashWString(std::wstring_view str);

namespace EAsset {
	std::string AssetTypeToString(EAsset::Type type);
}

class Component;

namespace EComponent {
	std::string ComponentTypeToString(Type type);
	Type ComponentStringToType(std::string name);

	Component* GetComponent(Type type);
}

inline Hash HashCombine(Hash seed, Hash value);

void CreateGameObject(GameObject* obj, int layer);
void DestroyGameObject(GameObject* obj);

void ChangeLevel(const std::wstring& name);
void ChangeNewLevel(class ALevel* pNewLevel);
void ChangeLevelState(ELevelState::Type nextState);

decltype(auto) GetTimeStamp();
std::wstring MakeUniqueName(const std::wstring& name);

bool GetFile(const std::wstring& filePath, FILE*& pFile);

Vec3 CalcUpVector(const Vec3& dir);

Vec3 GltfToEngineVec3(const Vec3& v);
Mat4 GetRHtoLHMirrorMatrix();
Mat4 GltfToEngineMatrix(const Mat4& m);
Quat GltfToEngineQuat(const Quat& q);
TransformTRS GltfToEngineTRS(const TransformTRS& src);

Mat4 RemoveScaleFromMatrix(const Mat4& m);
Mat4 ExtractTRUniformScale(const Mat4& m);

bool DecomposeMatrixTRS(
	const Mat4& m,
	Vec3& outScale,
	Quat& outRot,
	Vec3& outTrans);
Mat4 MakeTRMatrix(const Vec3& translation, const Quat& rotation);
Mat4 MakeUniformScaleMatrix(float s);

float ExtractUniformScale(const Mat4& m);
Mat4 BuildStaticAttachmentWorld(
	const Mat4& nodeGlobal,
	const Mat4& assetRootWorld);

Vec3 ExtractWorldPosition(const Mat4& world);

#include "func.inl"