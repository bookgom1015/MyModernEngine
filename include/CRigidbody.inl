#ifndef __CRIGIDBODY_INL__
#define __CRIGIDBODY_INL__

void CRigidbody::SetLinearVelocity(const Vec3& v) { mLinearVelocity = v; }

void CRigidbody::SetAngularVelocity(const Vec3& v) { mAngularVelocity = v; }

const Vec3& CRigidbody::GetLinearVelocity() const noexcept { return mLinearVelocity; }

const Vec3& CRigidbody::GetAngularVelocity() const noexcept { return mAngularVelocity; }

float CRigidbody::GetMass() const noexcept { return mMass; }

float CRigidbody::GetInvMass() const noexcept { return mInvMass; }

ERigidbody::Type CRigidbody::GetRigidbodyType() const noexcept { return mType; }

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

float CRigidbody::GetRestitution() const noexcept { return mRestitution; }

float CRigidbody::GetFriction() const noexcept { return mFriction; }

float CRigidbody::GetLinearDamping() const noexcept { return mLinearDamping; }

float CRigidbody::GetAngularDamping() const noexcept { return mAngularDamping; }

void CRigidbody::SetRestitution(float r) noexcept { mRestitution = std::clamp(r, 0.f, 1.f); }

void CRigidbody::SetFriction(float f) noexcept { mFriction = std::max(f, 0.f); }

void CRigidbody::SetLinearDamping(float d) noexcept { mLinearDamping = std::max(d, 0.f); }

void CRigidbody::SetAngularDamping(float d) noexcept { mAngularDamping = std::max(d, 0.f); }

void CRigidbody::SetLocalInertia(const Vec3& inertia) noexcept { mLocalInertia = inertia; }

void CRigidbody::SetLocalInvInertia(const Vec3& invInertia) noexcept { mLocalInvInertia = invInertia; }

const Vec3& CRigidbody::GetLocalInertia() const noexcept { return mLocalInertia; }

const Vec3& CRigidbody::GetLocalInvInertia() const noexcept { return mLocalInvInertia; }

void CRigidbody::MarkInertiaDirty() noexcept { mInertiaDirty = true; }

void CRigidbody::ClearInertiaDirty() noexcept { mInertiaDirty = false; }

bool CRigidbody::IsInertiaDirty() const noexcept { return mInertiaDirty; }

#endif // __CRIGIDBODY_INL__