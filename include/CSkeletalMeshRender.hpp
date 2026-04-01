#pragma once

#include "CRenderComponent.hpp"

#include "ASkeleton.hpp"
#include "AAnimationClip.hpp"

class CSkeletalMeshRender : public CRenderComponent {
public:
	CSkeletalMeshRender();
	virtual ~CSkeletalMeshRender();

public:
	virtual bool Initialize() override;
	virtual bool Update(float dt) override;
	virtual bool Final() override;

	virtual bool CreateMaterial() override;

public:
	void SampleClip();
	void BuildGlobalPose();
	void BuildPalette();

public:
	CLONE(CSkeletalMeshRender);

	virtual bool SaveToLevelFile(FILE* const pFile) override;
	virtual bool LoadFromLevelFile(FILE* const pFile) override;

public:
	__forceinline Ptr<ASkeleton> GetSkeleton() const noexcept;
	void SetSkeleton(Ptr<ASkeleton> skeleton);

	__forceinline Ptr<AAnimationClip> GetAnimationClip() const noexcept;
	__forceinline void SetAnimationClip(Ptr<AAnimationClip> animClip) noexcept;

	__forceinline const std::vector<Mat4>& GetPalette() const noexcept;

private:
	int FindKeyframeIndex(const std::vector<float>& times, float t) const;

	Mat4 MakeLocalMatrix(const TransformTRS& trs) const;
	void BuildGlobalPoseRecursive(int nodeIndex, const Mat4& parentGlobal);

private:
	Ptr<ASkeleton> mSkeleton;

	const std::vector<NodeCPU>* mpNodes;
	const SkinCPU* mpSkin;
	Ptr<AAnimationClip> mAnimClip;

	float mCurrentTime;
	bool mbLoop;

	// node 전체 기준
	std::vector<TransformTRS> mNodeLocalPose;
	std::vector<Mat4> mNodeGlobalPose;

	// joint 기준 최종 결과
	std::vector<Mat4> mPalette;
};

#include "CSkeletalMeshRender.inl"