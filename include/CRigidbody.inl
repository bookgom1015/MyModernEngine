#ifndef __CRIGIDBODY_INL__
#define __CRIGIDBODY_INL__

void CRigidbody::SetLinearVelocity(const Vec3& v) { mLinearVelocity = v; }

void CRigidbody::SetAngularVelocity(const Vec3& v) { mAngularVelocity = v; }

const Vec3& CRigidbody::GetLinearVelocity() const noexcept { return mLinearVelocity; }

const Vec3& CRigidbody::GetAngularVelocity() const noexcept { return mAngularVelocity; }

float CRigidbody::GetMass() const noexcept { return mMass; }

float CRigidbody::GetInvMass() const noexcept { return mInvMass; }

void CRigidbody::SetMass(float mass) {
	mMass = mass;
	mInvMass = (mass != 0.f) ? 1.f / mass : 0.f;
}

void CRigidbody::SetUseGravity(bool enable) { mUseGravity = enable; }

void CRigidbody::SetIsTrigger(bool trigger) { mIsTrigger = trigger; }

void CRigidbody::SetType(ERigidbody::Type type) { mType = type; }

bool CRigidbody::IsDynamic() const noexcept { return mType == ERigidbody::E_Dynamic; }

bool CRigidbody::IsStatic() const noexcept { return mType == ERigidbody::E_Static; }

bool CRigidbody::IsKinematic() const noexcept { return mType == ERigidbody::E_Kinematic; }

bool CRigidbody::GetUseGravity() const noexcept { return mUseGravity; }

Vec3 CRigidbody::ConsumeForceAccum() noexcept {
	Vec3 force = mForceAccum;
	mForceAccum = Vec3(0.f);
	return force;
}

#endif // __CRIGIDBODY_INL__