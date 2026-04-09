#ifndef __CCOLLIDER_INL__
#define __CCOLLIDER_INL__

ECollider::Type CCollider::GetColliderType() const noexcept { return mColliderType; }

bool CCollider::IsTrigger() const noexcept { return mbIsTrigger; }

void CCollider::SetTrigger(bool enable) { mbIsTrigger = enable; }

Vec3 CCollider::GetOffset() const noexcept { return mOffset; }

void CCollider::SetOffset(const Vec3& offset) { mOffset = offset; }

#endif // __CCOLLIDER_INL__