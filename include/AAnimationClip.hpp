#pragma once

#include "Asset.hpp"
#include "GltfLoader.hpp"

class AAnimationClip : public Asset {
public:
    AAnimationClip();
    virtual ~AAnimationClip();

public:
    virtual bool Load(const std::wstring& filePath) override;
    bool BuildFromGltf(const std::wstring& filePath, const GltfAnimationSetCPU& animSet);

public:
    const std::vector<AnimationCPU>& GetAnimations() const noexcept { return mAnimations; }

    const std::vector<AnimationSamplerCPU>& GetSamplers() const noexcept { return mSamplers; }
    const std::vector<AnimationChannelCPU>& GetChannels() const noexcept { return mChannels; }

    float GetDuration() const noexcept { return mDuration; }

private:
    std::vector<AnimationCPU> mAnimations;

    std::vector<AnimationSamplerCPU> mSamplers;
    std::vector<AnimationChannelCPU> mChannels;

    float mDuration = 0.f;
};