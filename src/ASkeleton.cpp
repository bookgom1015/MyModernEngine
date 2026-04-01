#include "pch.h"
#include "ASkeleton.hpp"

#include "EditorManager.hpp"

ASkeleton::ASkeleton()
    : Asset(EAsset::E_Skeleton) {}

ASkeleton::~ASkeleton() {}

bool ASkeleton::Load(const std::wstring& filePath) {
    GltfLoadResultCPU gltf{};
    CheckReturn(GltfLoader::LoadGltfCpu(WStrToStr(filePath), gltf));

    mNodes = gltf.Skeleton.Nodes;
    mSceneRoots = gltf.Skeleton.SceneRoots;
    mSkins = gltf.Skeleton.Skins;

    LOG_INFO(std::format("Loaded skeleton asset '{}' with {} nodes and {} skins.",
        WStrToStr(filePath),
        mNodes.size(),
        mSkins.size()));

    return true;
}

bool ASkeleton::BuildFromGltf(const std::wstring& filePath, const GltfSkeletonCPU& skeleton) {
    mNodes = skeleton.Nodes;
    mSceneRoots = skeleton.SceneRoots;
    mSkins = skeleton.Skins;

    LOG_INFO(std::format("Loaded skeleton asset '{}' with {} nodes and {} skins.",
        WStrToStr(filePath),
        mNodes.size(),
        mSkins.size()));

    return true;
}