#ifndef __CSKELETALMESHRENDER_INL__
#define __CSKELETALMESHRENDER_INL__

Ptr<ASkeleton> CSkeletalMeshRender::GetSkeleton() const noexcept { return mSkeleton; }

Ptr<AAnimationClip> CSkeletalMeshRender::GetAnimationClip() const noexcept { return mAnimClip; }

void CSkeletalMeshRender::SetAnimationClip(Ptr<AAnimationClip> animClip) noexcept { mAnimClip = animClip; }

const std::vector<Mat4>& CSkeletalMeshRender::GetPalette() const noexcept { return mPalette; }

#endif // __CSKELETALMESHRENDER_INL__