#pragma once

constexpr ERigidbody::Type CRigidbody::GetRigidbodyType() const noexcept { return mType; }

bool CRigidbody::IsDynamic() const noexcept {	return mType == ERigidbody::E_Dynamic; }

float CRigidbody::GetMass() const noexcept { return mMass; }

float CRigidbody::GetInvMass() const noexcept { return mInvMass; }

const Vec3& CRigidbody::GetLinearVelocity() const noexcept { return mLinearVelocity; }

const Vec3& CRigidbody::GetAngularVelocity() const noexcept { return mAngularVelocity; }

const Vec3& CRigidbody::GetLocalInertia() const noexcept { return mLocalInertia; }

const Vec3& CRigidbody::GetLocalInvInertia() const noexcept { return mLocalInvInertia; }

float CRigidbody::GetLinearDamping() const noexcept { return mLinearDamping; }

float CRigidbody::GetAngularDamping() const noexcept { return mAngularDamping; }

float CRigidbody::GetRestitution() const noexcept { return mRestitution; }

float CRigidbody::GetFriction() const noexcept { return mFriction; }

bool CRigidbody::GetUseGravity() const noexcept { return mUseGravity; }

bool CRigidbody::IsTrigger() const noexcept { return mIsTrigger; }

ERigidbodyConstraint::Type CRigidbody::GetConstraints() const noexcept { return mConstraints; }

bool CRigidbody::IsInertiaDirty() const noexcept { return mInertiaDirty; }

Vec3 CRigidbody::ConsumeForceAccum() noexcept {
	Vec3 f = mForceAccum;
	mForceAccum = Vec3(0.f);
	return f;
}

void CRigidbody::SetMass(float mass) {
	mMass = std::max(mass, 0.0001f);

	if (mType == ERigidbody::E_Dynamic)
		mInvMass = 1.f / mMass;
	else
		mInvMass = 0.f;

	MarkInertiaDirty();
}

void CRigidbody::SetLinearVelocity(const Vec3& v) noexcept { mLinearVelocity = v; }

void CRigidbody::SetAngularVelocity(const Vec3& v) noexcept { mAngularVelocity = v; }

void CRigidbody::SetLocalInertia(const Vec3& v) noexcept { mLocalInertia = v; }

void CRigidbody::SetLocalInvInertia(const Vec3& v) noexcept { mLocalInvInertia = v; }

void CRigidbody::SetLinearDamping(float v) noexcept { mLinearDamping = std::max(0.f, v); }

void CRigidbody::SetAngularDamping(float v) noexcept { mAngularDamping = std::max(0.f, v); }

void CRigidbody::SetRestitution(float v) noexcept { mRestitution = std::clamp(v, 0.f, 1.f); }

void CRigidbody::SetFriction(float v) noexcept { mFriction = std::max(0.f, v); }

void CRigidbody::SetUseGravity(bool v) noexcept { mUseGravity = v; }

void CRigidbody::SetTrigger(bool v) noexcept { mIsTrigger = v; }

void CRigidbody::SetConstraints(ERigidbodyConstraint::Type v) noexcept { mConstraints = v; }

void CRigidbody::MarkInertiaDirty() noexcept { mInertiaDirty = true; }

void CRigidbody::ClearInertiaDirty() noexcept { mInertiaDirty = false; }

void CRigidbody::SetType(ERigidbody::Type type) { SetRigidbodyType(type); }

void CRigidbody::SetSleeping(bool sleeping) noexcept { mSleeping = sleeping; }

bool CRigidbody::IsSleeping() const noexcept { return mSleeping; }

void CRigidbody::SetSleepTimer(float timer) noexcept { mSleepTimer = timer; }

float CRigidbody::GetSleepTimer() const noexcept { return mSleepTimer; }