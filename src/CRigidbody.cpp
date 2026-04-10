#include "pch.h"
#include "CRigidbody.hpp"

#include "PhysicsManager.hpp"

CRigidbody::CRigidbody() 
	: Component{ EComponent::E_Rigidbody }
	, mType{ ERigidbody::E_Static }
	, mInertiaDirty{ true }
	, mMass{ 1.f }
	, mInvMass{ 1.f }
	, mLinearVelocity{ 0.f }
	, mAngularVelocity{ 0.f }
	, mForceAccum{ 0.f }
	, mTorqueAccum{ 0.f }
	, mLocalInertia{ 0.f }
	, mLocalInvInertia{ 0.f }
	, mLinearDamping{ 0.01f }
	, mAngularDamping{ 0.05f }
	, mRestitution{ 0.f }
	, mFriction{ 0.85f }
	, mUseGravity{ true }
	, mIsTrigger{ false }
	, mConstraints{ ERigidbodyConstraint::E_None } {}

CRigidbody::~CRigidbody() {
	if (PHYSICS_MANAGER->IsRigidbodyRegistered(this))
		PHYSICS_MANAGER->UnregisterRigidbody(this);
}

bool CRigidbody::Initialize() {
	PHYSICS_MANAGER->RegisterRigidbody(this);

	return true;
}

bool CRigidbody::Begin() {
	return true;
}

bool CRigidbody::Update(float dt) {
	return true;
}

bool CRigidbody::FixedUpdate(float dt) {
	return true;
}

bool CRigidbody::LateUpdate(float dt) {
	return true;
}

bool CRigidbody::Final() {
	return true;
}

bool CRigidbody::OnLoaded() {
	if (PHYSICS_MANAGER->IsRigidbodyRegistered(this)) return true;
	PHYSICS_MANAGER->RegisterRigidbody(this);

	return true;
}

bool CRigidbody::OnUnloaded() {
	if (!PHYSICS_MANAGER->IsRigidbodyRegistered(this)) return true;
	PHYSICS_MANAGER->UnregisterRigidbody(this);

	return true;
}

bool CRigidbody::SaveToLevelFile(FILE* const pFile) {
	return true;
}

bool CRigidbody::LoadFromLevelFile(FILE* const pFile) {
	return true;
}

void CRigidbody::AddForce(const Vec3& force) {
	mForceAccum += force;
}

void CRigidbody::AddImpulse(const Vec3& impulse) {
	mLinearVelocity += impulse * mInvMass;
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
	const float i = 0.4f * mass * radius * radius; // 2/5 mr^2
	mLocalInertia = Vec3(i, i, i);
	mLocalInvInertia = Vec3(
		(i > 0.f) ? 1.f / i : 0.f,
		(i > 0.f) ? 1.f / i : 0.f,
		(i > 0.f) ? 1.f / i : 0.f
	);
}

void CRigidbody::SetCapsuleInertiaApprox(float mass, float radius, float halfSegment) {
	// 일단 근사치로 박스/원통 비슷하게 넣어도 충분
	const float h = halfSegment * 2.f;
	const float ix = (mass / 12.f) * (3.f * radius * radius + h * h);
	const float iz = ix;
	const float iy = 0.5f * mass * radius * radius;
	mLocalInertia = Vec3(ix, iy, iz);
	mLocalInvInertia = Vec3(
		(ix > 0.f) ? 1.f / ix : 0.f,
		(iy > 0.f) ? 1.f / iy : 0.f,
		(iz > 0.f) ? 1.f / iz : 0.f
	);
}

void CRigidbody::SetRigidbodyType(ERigidbody::Type type) noexcept {
	mType = type;
	mInvMass = (mType == ERigidbody::E_Dynamic) ? (1.f / std::max(mMass, 0.0001f)) : 0.f;
	MarkInertiaDirty();
}