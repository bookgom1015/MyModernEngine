#include "pch.h"

#include <mutex>
#include <vector>
#include <iostream>

#include "TaskManager.hpp"

#include "Asset.hpp"
#include "ALevel.hpp"

#include "Components.hpp"

namespace {
    const char* const STR_FAIL_GET_PROC_INFO = "Failed to retrieve processor information.";
    const char* const STR_FAIL_GET_BUFF_SIZE = "Failed to get required buffer size.";
    const char* const STR_FAIL_GET_MEM_STAT = "Failed to get memory status";

    wchar_t LevelNameBuffer[255]{};    
}

void SaveWString(FILE* pFile, const std::wstring& string) {
    int Len = static_cast<int>(string.length());
    fwrite(&Len, sizeof(int), 1, pFile);
    fwrite(string.data(), sizeof(wchar_t), Len, pFile);
}

std::wstring LoadWString(FILE* pFile) {
    int Len{};
    fread(&Len, sizeof(int), 1, pFile);

    wchar_t buff[255]{};
    fread(buff, sizeof(wchar_t), Len, pFile);

    return buff;
}

void SaveAssetRef(FILE* pFile, Asset* pAsset) {
    // Asset 이 Null 인지 아닌지 저장
    bool IsNull = pAsset;
    fwrite(&IsNull, sizeof(bool), 1, pFile);

    // Asset 의 Key, RelativePath 저장
    if (nullptr != pAsset) {
        SaveWString(pFile, pAsset->GetKey());
        SaveWString(pFile, pAsset->GetRelativePath());
    }
}

std::string EAsset::AssetTypeToString(EAsset::Type type) {
    switch (type) {
    case EAsset::E_Mesh: return "Mesh";
    case EAsset::E_Material: return "Material";
    case EAsset::E_Texture: return "Texture";
    case EAsset::E_Level: return "Level";
    case EAsset::E_Sprite: return "Sprite";
    case EAsset::E_Skeleton: return "Skeleton";
    case EAsset::E_AnimationClip: return "AnimationClip";
	case EAsset::E_Sound: return "Sound";
    default: return "Unknown";
    }
}

namespace EComponent {
    std::string ComponentTypeToString(Type type) {
        switch (type) {
        case E_Transform: return "Transform";
        case E_Camera: return "Camera";
        case E_Light: return "Light";
        case E_MeshRender: return "MeshRender";
        case E_SkeletalMeshRender: return "SkeletalMeshRender";
        case E_SkySphereRender: return "SkySphereRender";
		case E_ReflectionProbe: return "ReflectionProbe";
		case E_BoxCollider: return "BoxCollider";
		case E_SphereCollider: return "SphereCollider";
		case E_CapsuleCollider: return "CapsuleCollider";
		case E_MeshCollider: return "MeshCollider";
		case E_Rigidbody: return "Rigidbody";
        default: assert(false && "Unknown component type");
			return "Unknown";
        }
    }

    Type ComponentStringToType(std::string name) {
        switch (HashString(name)) {
        case HashString("Transform"): return E_Transform;
		case HashString("Camera"): return E_Camera;		
		case HashString("Light"): return E_Light;
		case HashString("MeshRender"): return E_MeshRender;
		case HashString("SkeletalMeshRender"): return E_SkeletalMeshRender;
        case HashString("SkySphereRender"): return E_SkySphereRender;
		case HashString("ReflectionProbe"): return E_ReflectionProbe;
		case HashString("BoxCollider"): return E_BoxCollider;
		case HashString("SphereCollider"): return E_SphereCollider;
		case HashString("CapsuleCollider"): return E_CapsuleCollider;
		case HashString("MeshCollider"): return E_MeshCollider;
		case HashString("Rigidbody"): return E_Rigidbody;
		default: assert(false && "Unknown component type string");
            return static_cast<Type>(-1);
        }
    }

    Component* GetComponent(Type type) {
        switch (type) {
        case EComponent::E_Transform: return NEW CTransform;
        case EComponent::E_Camera: return new CCamera();
        case EComponent::E_Light: return new CLight();
        case EComponent::E_MeshRender: return new CMeshRender();
        case EComponent::E_SkeletalMeshRender: return new CSkeletalMeshRender();
		case EComponent::E_SkySphereRender: return new CSkySphereRender();
		case EComponent::E_ReflectionProbe: return new CReflectionProbe();
		case EComponent::E_BoxCollider: return new CBoxCollider();
		case EComponent::E_SphereCollider: return new CSphereCollider();
		case EComponent::E_CapsuleCollider: return new CCapsuleCollider();
		case EComponent::E_MeshCollider: return new CMeshCollider();
		case EComponent::E_Rigidbody: return new CRigidbody();
        default: return nullptr;
        }
    }
}

void CreateGameObject(GameObject* obj, int layer) {
    TaskInfo info{};

    info.Type = ETask::E_CreateObject;
    info.Param_0 = reinterpret_cast<DWORD_PTR>(obj);
    info.Param_1 = layer;

    TASK_MANAGER->AddTask(info);
}

void DestroyGameObject(GameObject* obj) {
    TaskInfo info{};

    info.Type = ETask::E_DestroyObject;
    info.Param_0 = reinterpret_cast<DWORD_PTR>(obj);

    TASK_MANAGER->AddTask(info);
}

void ChangeLevel(const std::wstring& name) {
    TaskInfo info{};
    
    wcscpy_s(LevelNameBuffer, 255, name.c_str());

    info.Type = ETask::E_ChangeLevel;
    info.Param_0 = reinterpret_cast<DWORD_PTR>(LevelNameBuffer);

    TASK_MANAGER->AddTask(info);
}

void ChangeNewLevel(ALevel* pNewLevel) {
    TaskInfo info{};

    info.Type = ETask::E_ChangeNewLevel;
    info.Param_0 = reinterpret_cast<DWORD_PTR>(pNewLevel);

    TASK_MANAGER->AddTask(info);
}

void ChangeLevelState(ELevelState::Type nextState) {
    TaskInfo info{};

    info.Type = ETask::E_ChangeLevelState;
    info.Param_0 = static_cast<DWORD_PTR>(nextState);

    TASK_MANAGER->AddTask(info);
}

decltype(auto) GetTimeStamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

std::wstring MakeUniqueName(const std::wstring& name) {
    return std::format(L"{}##{}", name, GetTimeStamp());
}

bool GetFile(const std::wstring& filePath, FILE*& pFile) {
    std::filesystem::path path(filePath);
    std::filesystem::path dir = path.parent_path();

    if (!dir.empty() && !std::filesystem::exists(dir))
        if (!std::filesystem::create_directories(dir))
			ReturnFalse(std::format("Failed to create directory: {}", dir.string()));
    
    if (_wfopen_s(&pFile, filePath.c_str(), L"wb") != 0)
        ReturnFalse("Failed to open file for writing");

    return true;
}

Vec3 CalcUpVector(const Vec3& dir) {
    auto up = UnitVector::UpVector;
    if (dir.z >= 0.0f) {
        const float dot = dir.Dot(up);;
        if (dot > 0.99f) up = UnitVector::BackwardVector;
        else if (dot < -0.99f) up = UnitVector::ForwardVector;
    }
    else {
        up = UnitVector::DownVector;

        const float dot = dir.Dot(up);
        if (dot > 0.99f) up = UnitVector::ForwardVector;
        else if (dot < -0.99f) up = UnitVector::BackwardVector;
    }

    auto right = up.Cross(dir);
    right.Normalize();

    up = dir.Cross(right);
    up.Normalize();

    return up;
}

Vec3 GltfToEngineVec3(const Vec3& v) {
    return Vec3(v.x, v.y, -v.z);
}

Mat4 GetRHtoLHMirrorMatrix() {
    return Mat4(
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, -1.f, 0.f,
        0.f, 0.f, 0.f, 1.f);
}

Mat4 GltfToEngineMatrix(const Mat4& m) {
    const Mat4 C = GetRHtoLHMirrorMatrix();
    return C * m * C;
}

Quat GltfToEngineQuat(const Quat& q) {
    Mat4 r = Mat4::CreateFromQuaternion(q);
    r = GltfToEngineMatrix(r);

    Quat out;
    out = Quat::CreateFromRotationMatrix(r);
    out.Normalize();
    return out;
}

TransformTRS GltfToEngineTRS(const TransformTRS& src) {
    TransformTRS dst;
    dst.Translation = GltfToEngineVec3(src.Translation);
    dst.Rotation = GltfToEngineQuat(src.Rotation);
    dst.Scale = src.Scale;
    return dst;
}

Mat4 RemoveScaleFromMatrix(const Mat4& m) {
    using namespace DirectX;

    XMVECTOR scale;
    XMVECTOR rot;
    XMVECTOR trans;

    if (!XMMatrixDecompose(&scale, &rot, &trans, m))
        return m;

    XMMATRIX R = XMMatrixRotationQuaternion(rot);
    XMMATRIX T = XMMatrixTranslationFromVector(trans);

    // scale 제외
    return R * T;
}

Mat4 ExtractTRUniformScale(const Mat4& m) {
    using namespace DirectX;

    XMVECTOR s, r, t;
    if (!XMMatrixDecompose(&s, &r, &t, m))
        return m;

    Vec3 scale;
    XMStoreFloat3(&scale, s);

    float uniform = (scale.x + scale.y + scale.z) / 3.f;

    XMMATRIX S = XMMatrixScaling(uniform, uniform, uniform);
    XMMATRIX R = XMMatrixRotationQuaternion(r);
    XMMATRIX T = XMMatrixTranslationFromVector(t);

    return S * R * T;
}

bool DecomposeMatrixTRS(
    const Mat4& m
    , Vec3& outScale
    , Quat& outRot
    , Vec3& outTrans) {
    using namespace DirectX;

    XMVECTOR s, r, t;
    if (!XMMatrixDecompose(&s, &r, &t, m))
        return false;

    XMStoreFloat3(&outScale, s);
    XMStoreFloat4(&outRot, r);
    XMStoreFloat3(&outTrans, t);
    return true;
}

Mat4 MakeTRMatrix(const Vec3& translation, const Quat& rotation) {
    using namespace DirectX;

    const Mat4 R = XMMatrixRotationQuaternion(XMLoadFloat4(&rotation));
    const Mat4 T = XMMatrixTranslation(translation.x, translation.y, translation.z);

    return R * T;
}

Mat4 MakeUniformScaleMatrix(float s) {
    using namespace DirectX;
    return XMMatrixScaling(s, s, s);
}

float ExtractUniformScale(const Mat4& m) {
    Vec3 scale{};
    Quat rot{};
    Vec3 trans{};

    if (!DecomposeMatrixTRS(m, scale, rot, trans))
        return 1.f;

    return (scale.x + scale.y + scale.z) / 3.f;
}

Mat4 BuildStaticAttachmentWorld(const Mat4& nodeGlobal , const Mat4& assetRootWorld) {
    Vec3 nodeScale{};
    Quat nodeRot{};
    Vec3 nodeTrans{};

    if (!DecomposeMatrixTRS(nodeGlobal, nodeScale, nodeRot, nodeTrans))
        return nodeGlobal;

    // 1) attachment는 위치/회전만 사용
    Mat4 nodeTR = MakeTRMatrix(nodeTrans, nodeRot);

    // 2) asset root 쪽에서 uniform scale만 추출
    float uniformScale = ExtractUniformScale(assetRootWorld);
    Mat4 uniformS = MakeUniformScaleMatrix(uniformScale);

    // 네 규약 기준으로 둘 다 시험 가능하지만
    // 지금은 이쪽부터 추천
    return uniformS * nodeTR;
}

Vec3 ExtractWorldPosition(const Mat4& world) {
    return Vec3(world.m[3][0], world.m[3][1], world.m[3][2]);
}