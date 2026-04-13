#include "pch.h"
#include "CRigidbody.hpp"

#include "PhysicsManager.hpp"

#include "GameObject.hpp"

CRigidbody::CRigidbody()
	: Component{ EComponent::E_Rigidbody }
	, mType{ ERigidbody::E_Static }
	, mInertiaDirty{ true }
	, mMass{ 1.f }
	, mInvMass{ 0.f }
	, mLinearVelocity{ 0.f }
	, mAngularVelocity{ 0.f }
	, mForceAccum{ 0.f }
	, mTorqueAccum{ 0.f }
	, mLocalInertia{ 0.f }
	, mLocalInvInertia{ 0.f }
	, mLinearDamping{ 0.03f }
	, mAngularDamping{ 0.18f }
	, mRestitution{ 0.f }
	, mFriction{ 0.8f }
	, mUseGravity{ true }
	, mIsTrigger{ false }
	, mConstraints{ ERigidbodyConstraint::E_None }
	, mSleeping{ false }
	, mSleepTimer{ 0.f } {}

CRigidbody::~CRigidbody() {
	if (PHYSICS_MANAGER->IsRigidbodyRegistered(this)) {
		PHYSICS_MANAGER->UnregisterRigidbody(this);
	}
}

bool CRigidbody::Initialize() {
	if (GetOwner()->GetLayer() != -1)
		PHYSICS_MANAGER->RegisterRigidbody(this);

	return true;
}

bool CRigidbody::Begin() { return true; }
bool CRigidbody::Update(float dt) { UNREFERENCED_PARAMETER(dt); return true; }
bool CRigidbody::FixedUpdate(float dt) { UNREFERENCED_PARAMETER(dt); return true; }
bool CRigidbody::LateUpdate(float dt) { UNREFERENCED_PARAMETER(dt); return true; }
bool CRigidbody::Final() { return true; }

bool CRigidbody::OnLoaded() {
	if (!PHYSICS_MANAGER->IsRigidbodyRegistered(this)) {
		PHYSICS_MANAGER->RegisterRigidbody(this);
	}
	return true;
}

bool CRigidbody::OnUnloaded() {
	if (PHYSICS_MANAGER->IsRigidbodyRegistered(this)) {
		PHYSICS_MANAGER->UnregisterRigidbody(this);
	}
	return true;
}

bool CRigidbody::SaveToLevelFile(FILE* const pFile) {
	fwrite(&mType, sizeof(mType), 1, pFile);
	fwrite(&mMass, sizeof(mMass), 1, pFile);
	fwrite(&mLinearVelocity, sizeof(mLinearVelocity), 1, pFile);
	fwrite(&mAngularVelocity, sizeof(mAngularVelocity), 1, pFile);
	fwrite(&mLinearDamping, sizeof(mLinearDamping), 1, pFile);
	fwrite(&mAngularDamping, sizeof(mAngularDamping), 1, pFile);
	fwrite(&mRestitution, sizeof(mRestitution), 1, pFile);
	fwrite(&mFriction, sizeof(mFriction), 1, pFile);
	fwrite(&mUseGravity, sizeof(mUseGravity), 1, pFile);
	fwrite(&mIsTrigger, sizeof(mIsTrigger), 1, pFile);
	fwrite(&mConstraints, sizeof(mConstraints), 1, pFile);

	return true;
}

bool CRigidbody::LoadFromLevelFile(FILE* const pFile) {
	fread(&mType, sizeof(mType), 1, pFile);
	fread(&mMass, sizeof(mMass), 1, pFile);
	fread(&mLinearVelocity, sizeof(mLinearVelocity), 1, pFile);
	fread(&mAngularVelocity, sizeof(mAngularVelocity), 1, pFile);
	fread(&mLinearDamping, sizeof(mLinearDamping), 1, pFile);
	fread(&mAngularDamping, sizeof(mAngularDamping), 1, pFile);
	fread(&mRestitution, sizeof(mRestitution), 1, pFile);
	fread(&mFriction, sizeof(mFriction), 1, pFile);
	fread(&mUseGravity, sizeof(mUseGravity), 1, pFile);
	fread(&mIsTrigger, sizeof(mIsTrigger), 1, pFile);
	fread(&mConstraints, sizeof(mConstraints), 1, pFile);

	mMass = std::max(mMass, 0.0001f);
	mInvMass = (mType == ERigidbody::E_Dynamic) ? (1.f / mMass) : 0.f;
	mForceAccum = Vec3(0.f);
	mTorqueAccum = Vec3(0.f);
	mSleeping = false;
	mSleepTimer = 0.f;
	mInertiaDirty = true;

	return true;
}

void CRigidbody::AddForce(const Vec3& force) {
	mForceAccum += force;
}

void CRigidbody::AddImpulse(const Vec3& impulse) {
	if (mInvMass > 0.f) {
		mLinearVelocity += impulse * mInvMass;
	}
}

Vec3 CRigidbody::ConsumeTorqueAccum() noexcept {
	Vec3 t = mTorqueAccum;
	mTorqueAccum = Vec3(0.f);
	return t;
}

void CRigidbody::AddTorque(const Vec3& t) noexcept {
	mTorqueAccum += t;
}

void CRigidbody::SetBoxInertia(float mass, const Vec3& fullExtents) {
	const float x = fullExtents.x;
	const float y = fullExtents.y;
	const float z = fullExtents.z;
	mLocalInertia.x = (mass / 12.f) * (y * y + z * z);
	mLocalInertia.y = (mass / 12.f) * (x * x + z * z);
	mLocalInertia.z = (mass / 12.f) * (x * x + y * y);
	mLocalInvInertia.x = (mLocalInertia.x > 0.f) ? 1.f / mLocalInertia.x : 0.f;
	mLocalInvInertia.y = (mLocalInertia.y > 0.f) ? 1.f / mLocalInertia.y : 0.f;
	mLocalInvInertia.z = (mLocalInertia.z > 0.f) ? 1.f / mLocalInertia.z : 0.f;
}

void CRigidbody::SetSphereInertia(float mass, float radius) {
	const float i = 0.4f * mass * radius * radius;
	mLocalInertia = Vec3(i, i, i);
	mLocalInvInertia = Vec3((i > 0.f) ? 1.f / i : 0.f);
}

void CRigidbody::SetCapsuleInertiaApprox(float mass, float radius, float halfSegment) {
	const float h = halfSegment * 2.f;
	const float ix = (mass / 12.f) * (3.f * radius * radius + h * h);
	const float iz = ix;
	const float iy = 0.5f * mass * radius * radius;
	mLocalInertia = Vec3(ix, iy, iz);
	mLocalInvInertia = Vec3(
		(ix > 0.f) ? 1.f / ix : 0.f,
		(iy > 0.f) ? 1.f / iy : 0.f,
		(iz > 0.f) ? 1.f / iz : 0.f);
}

void CRigidbody::SetRigidbodyType(ERigidbody::Type type) noexcept {
	mType = type;
	mInvMass = (mType == ERigidbody::E_Dynamic) ? (1.f / std::max(mMass, 0.0001f)) : 0.f;
	MarkInertiaDirty();
}
