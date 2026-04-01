#include "pch.h"
#include "AAnimationClip.hpp"

#include "EditorManager.hpp"

AAnimationClip::AAnimationClip()
    : Asset(EAsset::E_AnimationClip) {}

AAnimationClip::~AAnimationClip() {}

bool AAnimationClip::Load(const std::wstring& filePath) {
    GltfLoadResultCPU gltf{};
    CheckReturn(GltfLoader::LoadGltfCpu(WStrToStr(filePath), gltf));

    mAnimations = gltf.AnimationSet.Animations;

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