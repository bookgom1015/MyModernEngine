#include "pch.h"
#include "CRigidbody.hpp"

CRigidbody::CRigidbody() 
	: Component{ EComponent::E_Rigidbody }
	, mType{ ERigidbody::E_Static }
	, mMass{ 1.f }
	, mInvMass{ 0.f }
	, mLinearVelocity{ 0.f }
	, mAngularVelocity{ 0.f }
	, mForceAccum{ 0.f }
	, mTorqueAccum{ 0.f }
	, mLinearDamping{ 0.01f }
	, mAngularDamping{ 0.05f }
	, mRestitution{ 0.f }
	, mFriction{ 0.5f }
	, mUseGravity{ true }
	, mIsTrigger{ false }
	, mConstraints{ ERigidbodyConstraint::E_None } {}

CRigidbody::~CRigidbody() {}

bool CRigidbody::Initialize() {
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