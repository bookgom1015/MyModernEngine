#ifndef __CBOXCOLLIDER_INL__
#define __CBOXCOLLIDER_INL__

const Vec3& CBoxCollider::GetHalfExtents() const noexcept { return mHalfExtents; }

void CBoxCollider::SetHalfExtents(const Vec3& halfExtents) noexcept { mHalfExtents = halfExtents; }

#endif // __CBOXCOLLIDER_INL__