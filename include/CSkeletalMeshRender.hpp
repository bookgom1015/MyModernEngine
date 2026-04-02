#pragma once

#include "CRenderComponent.hpp"

#include "ASkeleton.hpp"
#include "AAnimationClip.hpp"

class CSkeletalMeshRender : public CRenderComponent {
public:
    CSkeletalMeshRender();
    CSkeletalMeshRender(const CSkeletalMeshRender& other);
    virtual ~CSkeletalMeshRender();

public:
    virtual bool Initialize() override;
    virtual bool Update(float dt) override;
    virtual bool Final() override;
    virtual bool SetMesh(Ptr<AMesh> mesh) override;

    virtual bool CreateMaterial() override;

public:
    CLONE(CSkeletalMeshRender);

    virtual bool SaveToLevelFile(FILE* const pFile) override;
    virtual bool LoadFromLevelFile(FILE* const pFile) override;

    void SetSkeleton(Ptr<ASkeleton> skeleton);
    bool SetAnimationClip(Ptr<AAnimationClip> clip);

    Ptr<ASkeleton> GetSkeleton() const { return mSkeleton; }
    Ptr<AAnimationClip> GetAnimationClip() const { return mAnimClip; }

    // skin별 palette 반환
    const std::vector<Mat4>& GetPalette(int skinIndex) const;
    bool HasPalette(int skinIndex) const;

    const Mat4& GetNodeGlobalPose(int nodeIndex) const;

private:
    void SampleClip();
    void BuildGlobalPose();
    void BuildGlobalPoseRecursive(int nodeIndex, const Mat4& parentGlobal);

    Mat4 MakeLocalMatrix(const TransformTRS& trs) const;
    int FindKeyframeIndex(const std::vector<float>& times, float t) const;

    // 단일 skin 검출 대신 mesh에서 사용 중인 skin들 수집
    std::vector<int> CollectSkinIndicesFromMesh() const;
    void RefreshSkinBinding();

    // skin별 palette 생성
    void BuildPalettes();
    void BuildPaletteForSkin(int skinIndex);

private:
    Ptr<ASkeleton> mSkeleton;
    Ptr<AAnimationClip> mAnimClip;

    float mCurrentTime = 0.f;
    bool mbLoop = true;

    const std::vector<NodeCPU>* mpNodes = nullptr;

    std::vector<TransformTRS> mNodeLocalPose;
    std::vector<Mat4> mNodeGlobalPose;

    std::vector<int> mUsedSkinIndices;
    std::unordered_map<int, std::vector<Mat4>> mPalettesBySkin;

    std::vector<Mat4> mNodeLocalMatrices;
};

//#include "CSkeletalMeshRender.inl"