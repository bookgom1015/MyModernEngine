#include "pch.h"
#include "CSkeletalMeshRender.hpp"

#include "AssetManager.hpp"
#include "EditorManager.hpp"

CSkeletalMeshRender::CSkeletalMeshRender()
	: CRenderComponent(EComponent::E_SkeletalMeshRender)
	, mCurrentTime{}
	, mbLoop{ true } {}

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
    BuildPalette();

    return true;
}

bool CSkeletalMeshRender::Final() {
	CheckReturn(CRenderComponent::Final());

	return true;
}

bool CSkeletalMeshRender::CreateMaterial() {
	return true;
}

void CSkeletalMeshRender::SampleClip() {
    if (mpNodes == nullptr) return;

    const auto& nodes = *mpNodes;
    const size_t nodeCount = nodes.size();

    mNodeLocalPose.resize(nodeCount);

    // 1) 기본 pose로 초기화
    for (size_t i = 0; i < nodeCount; ++i) {
        const NodeCPU& src = nodes[i];

        TransformTRS dst;
        dst.Translation = src.Translation;
        dst.Rotation = src.Rotation;
        dst.Scale = src.Scale;

        mNodeLocalPose[i] = dst;
    }

    // clip asset 없으면 bind/default pose 유지
    if (mAnimClip == nullptr)
        return;

    const auto& clipAsset = mAnimClip.Get();
    const auto& animations = clipAsset->GetAnimations();

    if (animations.empty())
        return;

    // 현재 재생할 애니메이션 선택
    int animIndex = 0;
    if (animIndex < 0 || animIndex >= static_cast<int>(animations.size()))
        animIndex = 0;

    const AnimationCPU& animation = animations[animIndex];
    const auto& samplers = animation.Samplers;
    const auto& channels = animation.Channels;
    const float duration = animation.Duration;

    float t = 0.f;
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

        // 키 하나만 있으면 바로 적용
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
    }
}

void CSkeletalMeshRender::BuildGlobalPose() {
    if (mpNodes == nullptr) return;

    const auto& nodes = *mpNodes;
    const size_t nodeCount = nodes.size();

    mNodeGlobalPose.resize(nodeCount, Identity4x4);

    // 모든 root node에서 시작
    for (int i = 0; i < static_cast<int>(nodeCount); ++i) 
        if (nodes[i].Parent == -1)
            BuildGlobalPoseRecursive(i, Identity4x4);
}

void CSkeletalMeshRender::BuildPalette() {
    if (mpSkin == nullptr) return;

    const SkinCPU& skin = *mpSkin;
    const size_t jointCount = skin.Joints.size();

    mPalette.resize(jointCount, Identity4x4);

    for (size_t i = 0; i < jointCount; ++i) {
        const int nodeIndex = skin.Joints[i];

        if (nodeIndex < 0 || nodeIndex >= static_cast<int>(mNodeGlobalPose.size()))
            continue;

        const Mat4& global = mNodeGlobalPose[nodeIndex];
        const Mat4& invBind = skin.InverseBindMatrices[i];

        // 우선 row-vector 방식 가정
        mPalette[i] = global * invBind;
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

	mSkeleton = LoadAssetRef<ASkeleton>(pFile);
	mAnimClip = LoadAssetRef<AAnimationClip>(pFile);

	return true;
}

void CSkeletalMeshRender::SetSkeleton(Ptr<ASkeleton> skeleton) { 
    mSkeleton = skeleton; 
	mpNodes = (mSkeleton != nullptr) ? &mSkeleton->GetNodes() : nullptr;
	mpSkin = (mSkeleton != nullptr && !mSkeleton->GetSkins().empty()) ? &mSkeleton->GetSkins()[0] : nullptr;
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
	const Mat4 local = MakeLocalMatrix(mNodeLocalPose[nodeIndex]);

	// row-vector 스타일 가정
	mNodeGlobalPose[nodeIndex] = local * parentGlobal;

	const auto& node = (*mpNodes)[nodeIndex];
	for (int childIndex : node.Children)
		BuildGlobalPoseRecursive(childIndex, mNodeGlobalPose[nodeIndex]);
}