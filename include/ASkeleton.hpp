#pragma once

#include "Asset.hpp"

#include "GltfLoader.hpp"

class ASkeleton : public Asset {
public:
    ASkeleton();
    virtual ~ASkeleton();

public:
    virtual bool Load(const std::wstring& filePath) override;
    bool BuildFromGltf(const std::wstring& filePath, const GltfSkeletonCPU& skeleton);

public:
    const std::vector<NodeCPU>& GetNodes() const noexcept { return mNodes; }
    const std::vector<int>& GetSceneRoots() const noexcept { return mSceneRoots; }
    const std::vector<SkinCPU>& GetSkins() const noexcept { return mSkins; }

private:
    std::vector<NodeCPU> mNodes;
    std::vector<int> mSceneRoots;
    std::vector<SkinCPU> mSkins;
};