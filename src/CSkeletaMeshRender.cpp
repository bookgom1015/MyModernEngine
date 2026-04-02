#include "pch.h"
#include "CSkeletalMeshRender.hpp"

#include "AssetManager.hpp"
#include "EditorManager.hpp"

namespace {
    TransformTRS DecomposeNodeMatrix(const Mat4& matrix) {
        TransformTRS out{};

        Vec3 scale{};
        Quat rotation{};
        Vec3 translation{};

        if (DecomposeMatrixTRS(matrix, scale, rotation, translation)) {
            out.Translation = translation;
            out.Rotation = rotation;
            out.Rotation.Normalize();
            out.Scale = scale;
        }
        else {
            out.Translation = Vec3(matrix._41, matrix._42, matrix._43);
            out.Rotation = Quat(0.f, 0.f, 0.f, 1.f);
            out.Scale = Vec3(1.f, 1.f, 1.f);
        }

        return out;
    }
}

CSkeletalMeshRender::CSkeletalMeshRender()
    : CRenderComponent(EComponent::E_SkeletalMeshRender)
    , mCurrentTime{}
    , mbLoop{ true } {}

CSkeletalMeshRender::CSkeletalMeshRender(const CSkeletalMeshRender& other)
    : CRenderComponent{ other }
    , mSkeleton{ other.mSkeleton }
    , mAnimClip{ other.mAnimClip }
    , mCurrentTime{}
    , mbLoop{ other.mbLoop } {
    mpNodes = (mSkeleton != nullptr) ? &mSkeleton->GetNodes() : nullptr;
    RefreshSkinBinding();

    SampleClip();
    BuildGlobalPose();
    BuildPalettes();
}

CSkeletalMeshRender::~CSkeletalMeshRender() {}

bool CSkeletalMeshRender::Initialize() {
    CheckReturn(CRenderComponent::Initialize());

    return true;
}

bool CSkeletalMeshRender::Update(float dt) {
    if (mAnimClip != nullptr)
        mCurrentTime += dt;

    SampleClip();
    BuildGlobalPose();
    BuildPalettes();

    return true;
}

bool CSkeletalMeshRender::Final() {
    CheckReturn(CRenderComponent::Final());

    return true;
}

bool CSkeletalMeshRender::SetMesh(Ptr<AMesh> mesh) {
    CheckReturn(CRenderComponent::SetMesh(mesh));
    RefreshSkinBinding();

    SampleClip();
    BuildGlobalPose();
    BuildPalettes();

    return true;
}

bool CSkeletalMeshRender::CreateMaterial() { return true; }

bool CSkeletalMeshRender::SetAnimationClip(Ptr<AAnimationClip> clip) {
    mAnimClip = clip;

    SampleClip();
    BuildGlobalPose();
    BuildPalettes();

    return true;
}

const std::vector<Mat4>& CSkeletalMeshRender::GetPalette(int skinIndex) const {
    static const std::vector<Mat4> empty{};

    auto it = mPalettesBySkin.find(skinIndex);
    if (it == mPalettesBySkin.end())
        return empty;

    return it->second;
}

bool CSkeletalMeshRender::HasPalette(int skinIndex) const {
    auto it = mPalettesBySkin.find(skinIndex);
    return it != mPalettesBySkin.end() && !it->second.empty();
}

const Mat4& CSkeletalMeshRender::GetNodeGlobalPose(int nodeIndex) const {
    static const Mat4 identity = Identity4x4;

    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(mNodeGlobalPose.size()))
        return identity;

    return mNodeGlobalPose[nodeIndex];
}

void CSkeletalMeshRender::SampleClip() {
    if (mpNodes == nullptr)
        return;

    const auto& nodes = *mpNodes;
    const size_t nodeCount = nodes.size();

    mNodeLocalPose.resize(nodeCount);
    mNodeLocalMatrices.resize(nodeCount);

    // 1) 기본 pose / local matrix 초기화
    for (size_t i = 0; i < nodeCount; ++i) {
        const NodeCPU& src = nodes[i];

        if (src.HasMatrix) {
            mNodeLocalMatrices[i] = src.LocalMatrix;
            mNodeLocalPose[i] = DecomposeNodeMatrix(src.LocalMatrix);
        }
        else {
            TransformTRS dst{};
            dst.Translation = src.Translation;
            dst.Rotation = Quat(src.Rotation.x, src.Rotation.y, src.Rotation.z, src.Rotation.w);
            dst.Rotation.Normalize();
            dst.Scale = src.Scale;

            mNodeLocalPose[i] = dst;
            mNodeLocalMatrices[i] = MakeLocalMatrix(dst);
        }
    }

    // clip asset 없으면 bind/default pose 유지
    if (mAnimClip == nullptr)
        return;

    const auto& clipAsset = mAnimClip.Get();
    const auto& animations = clipAsset->GetAnimations();

    if (animations.empty())
        return;

    int animIndex = 0;
    if (animIndex < 0 || animIndex >= static_cast<int>(animations.size()))
        animIndex = 0;

    const AnimationCPU& animation = animations[animIndex];
    const auto& samplers = animation.Samplers;
    const auto& channels = animation.Channels;
    const float duration = animation.Duration;

    float t = mCurrentTime;
    if (duration > 0.f) {
        if (mbLoop) {
            t = std::fmod(t, duration);
            if (t < 0.f)
                t += duration;
        }
        else {
            t = std::clamp(t, 0.f, duration);
        }
    }
    else {
        t = 0.f;
    }

    // 2) 애니메이션 채널 적용
    for (const AnimationChannelCPU& channel : channels) {
        if (channel.SamplerIndex < 0 || channel.SamplerIndex >= static_cast<int>(samplers.size()))
            continue;

        if (channel.TargetNodeIndex < 0 || channel.TargetNodeIndex >= static_cast<int>(nodeCount))
            continue;

        const AnimationSamplerCPU& sampler = samplers[channel.SamplerIndex];
        const auto& times = sampler.InputTimes;
        const auto& values = sampler.OutputsValues;

        if (times.empty() || values.empty())
            continue;

        TransformTRS& local = mNodeLocalPose[channel.TargetNodeIndex];

        if (times.size() == 1 || values.size() == 1) {
            const Vec4& v = values[0];

            switch (channel.Path) {
            case AnimationPathCPU::Translation:
                local.Translation = Vec3(v.x, v.y, v.z);
                break;

            case AnimationPathCPU::Rotation:
                local.Rotation = Quat(v.x, v.y, v.z, v.w);
                local.Rotation.Normalize();
                break;

            case AnimationPathCPU::Scale:
                local.Scale = Vec3(v.x, v.y, v.z);
                break;

            default:
                break;
            }

            mNodeLocalMatrices[channel.TargetNodeIndex] = MakeLocalMatrix(local);
            continue;
        }

        const int k = FindKeyframeIndex(times, t);
        if (k < 0)
            continue;

        const int k0 = k;
        const int k1 = std::min(k + 1, static_cast<int>(times.size()) - 1);

        const float t0 = times[k0];
        const float t1 = times[k1];

        float alpha = 0.f;
        if (t1 > t0)
            alpha = (t - t0) / (t1 - t0);

        alpha = std::clamp(alpha, 0.f, 1.f);

        const Vec4& v0 = values[k0];
        const Vec4& v1 = values[k1];

        switch (channel.Path) {
        case AnimationPathCPU::Translation: {
            Vec3 a(v0.x, v0.y, v0.z);
            Vec3 b(v1.x, v1.y, v1.z);
            local.Translation = Vec3::Lerp(a, b, alpha);
            break;
        }

        case AnimationPathCPU::Rotation: {
            Quat q0(v0.x, v0.y, v0.z, v0.w);
            q0.Normalize();

            Quat q1(v1.x, v1.y, v1.z, v1.w);
            q1.Normalize();

            local.Rotation = Quat::Slerp(q0, q1, alpha);
            local.Rotation.Normalize();
            break;
        }

        case AnimationPathCPU::Scale: {
            Vec3 a(v0.x, v0.y, v0.z);
            Vec3 b(v1.x, v1.y, v1.z);
            local.Scale = Vec3::Lerp(a, b, alpha);
            break;
        }

        default:
            break;
        }

        mNodeLocalMatrices[channel.TargetNodeIndex] = MakeLocalMatrix(local);
    }
}

void CSkeletalMeshRender::BuildGlobalPose() {
    if (mpNodes == nullptr)
        return;

    const auto& nodes = *mpNodes;
    mNodeGlobalPose.assign(nodes.size(), Identity4x4);

    // parent가 없는 루트들부터 시작
    for (int i = 0; i < (int)nodes.size(); ++i) {
        if (nodes[i].ParentIndex == -1)
            BuildGlobalPoseRecursive(i, Identity4x4);
    }
}

bool CSkeletalMeshRender::SaveToLevelFile(FILE* const pFile) {
    CRenderComponent::SaveToLevelFile(pFile);

    SaveAssetRef(pFile, mSkeleton.Get());
    SaveAssetRef(pFile, mAnimClip.Get());

    return true;
}

bool CSkeletalMeshRender::LoadFromLevelFile(FILE* const pFile) {
    CRenderComponent::LoadFromLevelFile(pFile);

    auto skeleton = LoadAssetRef<ASkeleton>(pFile);
    SetSkeleton(skeleton);

    auto clip = LoadAssetRef<AAnimationClip>(pFile);
    SetAnimationClip(clip);

    return true;
}

void CSkeletalMeshRender::SetSkeleton(Ptr<ASkeleton> skeleton) {
    mSkeleton = skeleton;
    mpNodes = (mSkeleton != nullptr) ? &mSkeleton->GetNodes() : nullptr;

    RefreshSkinBinding();

    SampleClip();
    BuildGlobalPose();
    BuildPalettes();
}

int CSkeletalMeshRender::FindKeyframeIndex(const std::vector<float>& times, float t) const {
    if (times.empty()) return -1;
    if (times.size() == 1 || t <= times.front()) return 0;

    for (int i = 0, end = static_cast<int>(times.size()) - 1; i < end; ++i)
        if (t >= times[i] && t <= times[i + 1])
            return i;

    return static_cast<int>(times.size()) - 2;
}

Mat4 CSkeletalMeshRender::MakeLocalMatrix(const TransformTRS& trs) const {
    return Mat4::CreateScale(trs.Scale)
        * Mat4::CreateFromQuaternion(trs.Rotation)
        * Mat4::CreateTranslation(trs.Translation);
}

void CSkeletalMeshRender::BuildGlobalPoseRecursive(int nodeIndex, const Mat4& parentGlobal) {
    const auto& nodes = *mpNodes;
    const auto& node = nodes[nodeIndex];

    const Mat4& local = mNodeLocalMatrices[nodeIndex];

    Mat4 global = local * parentGlobal;

    mNodeGlobalPose[nodeIndex] = global;

    for (int childIndex : node.Children)
        BuildGlobalPoseRecursive(childIndex, global);
}

std::vector<int> CSkeletalMeshRender::CollectSkinIndicesFromMesh() const {
    std::vector<int> result;

    auto mesh = GetMesh();
    if (mesh == nullptr)
        return result;

    const auto& prims = mesh->GetMeshPrimitives();
    for (const auto& prim : prims) {
        if (prim.VertexType != EVertex::E_Skinned)
            continue;

        if (prim.SkinIndex < 0)
            continue;

        if (std::find(result.begin(), result.end(), prim.SkinIndex) == result.end())
            result.push_back(prim.SkinIndex);
    }

    return result;
}

void CSkeletalMeshRender::RefreshSkinBinding() {
    mUsedSkinIndices.clear();
    mPalettesBySkin.clear();

    if (mSkeleton == nullptr || GetMesh() == nullptr)
        return;

    const auto& skins = mSkeleton->GetSkins();
    auto used = CollectSkinIndicesFromMesh();

    for (int skinIndex : used) {
        if (skinIndex < 0 || skinIndex >= static_cast<int>(skins.size())) {
            LOG_WARNING(std::format("Invalid skin index {} for current mesh.", skinIndex));
            continue;
        }

        mUsedSkinIndices.push_back(skinIndex);
    }
}

void CSkeletalMeshRender::BuildPalettes() {
    mPalettesBySkin.clear();

    if (mSkeleton == nullptr || mpNodes == nullptr)
        return;

    for (int skinIndex : mUsedSkinIndices)
        BuildPaletteForSkin(skinIndex);
}

void CSkeletalMeshRender::BuildPaletteForSkin(int skinIndex) {
    if (mSkeleton == nullptr)
        return;

    const auto& skins = mSkeleton->GetSkins();
    if (skinIndex < 0 || skinIndex >= static_cast<int>(skins.size()))
        return;

    const SkinCPU& skin = skins[skinIndex];
    const size_t jointCount = skin.Joints.size();

    auto& palette = mPalettesBySkin[skinIndex];
    palette.assign(jointCount, Identity4x4);

    for (size_t i = 0; i < jointCount; ++i) {
        const int nodeIndex = skin.Joints[i];
        if (nodeIndex < 0 || nodeIndex >= static_cast<int>(mNodeGlobalPose.size()))
            continue;

        const Mat4& global = mNodeGlobalPose[nodeIndex];
        const Mat4& invBind = skin.InverseBindMatrices[i];

        palette[i] = invBind * global;
    }
}