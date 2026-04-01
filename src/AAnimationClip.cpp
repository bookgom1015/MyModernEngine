#include "pch.h"
#include "AAnimationClip.hpp"

#include "EditorManager.hpp"

namespace {
    void FinalizeAnimationDurations(std::vector<AnimationCPU>& animations) {
        for (auto& animation : animations) {
            float duration = 0.f;

            for (const auto& sampler : animation.Samplers)
                if (!sampler.InputTimes.empty())
                    duration = std::max(duration, sampler.InputTimes.back());

            // 빈 클립 방어
            animation.Duration = std::max(duration, 0.0001f);

            LOG_INFO(std::format(
                "Animation '{}' duration = {:.4f} sec",
                animation.Name,
                animation.Duration));
        }
    }
}

AAnimationClip::AAnimationClip()
    : Asset(EAsset::E_AnimationClip) {}

AAnimationClip::~AAnimationClip() {}

bool AAnimationClip::Load(const std::wstring& filePath) {
    GltfLoadResultCPU gltf{};
    CheckReturn(GltfLoader::LoadGltfCpu(WStrToStr(filePath), gltf));

    mAnimations = gltf.AnimationSet.Animations;
    FinalizeAnimationDurations(mAnimations);

    for (const auto& animation : mAnimations) {
        LOG_INFO(std::format(
            "Animation '{}' with {} channels and {} samplers. SkinIndex={}, Duration={:.4f}",
            animation.Name,
            animation.Channels.size(),
            animation.Samplers.size(),
            animation.SkinIndex,
            animation.Duration));
    }

    return true;
}

bool AAnimationClip::BuildFromGltf(const std::wstring& filePath, const GltfAnimationSetCPU& animSet) {
    mAnimations = animSet.Animations;
    FinalizeAnimationDurations(mAnimations);

    for (const auto& animation : mAnimations) {
        LOG_INFO(std::format(
            "Animation '{}' with {} channels and {} samplers. SkinIndex={}, Duration={:.4f}",
            animation.Name,
            animation.Channels.size(),
            animation.Samplers.size(),
            animation.SkinIndex,
            animation.Duration));
    }

    return true;
}