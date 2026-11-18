#ifndef __CCAPSULECOLLIDER_INL__
#define __CCAPSULECOLLIDER_INL__

float CCapsuleCollider::GetRadius() const noexcept { return mRadius; }

float CCapsuleCollider::GetHalfSegment() const noexcept { return mHalfSegment; }

ECapsuleAxis::Type CCapsuleCollider::GetAxis() const noexcept { return mAxis; }

void CCapsuleCollider::SetRadius(float radius) noexcept { mRadius = radius; }

void CCapsuleCollider::SetHalfSegment(float halfSegment) noexcept { mHalfSegment = halfSegment; }

void CCapsuleCollider::SetAxis(ECapsuleAxis::Type axis) noexcept { mAxis = axis; }

#endif // __CCAPSULECOLLIDER_INL__