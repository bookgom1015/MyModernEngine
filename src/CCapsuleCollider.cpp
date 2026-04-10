#include "pch.h"
#include "CCapsuleCollider.hpp"

#include "PhysicsManager.hpp"

CCapsuleCollider::CCapsuleCollider() 
	: CCollider(EComponent::E_CapsuleCollider, ECollider::E_Capsule)
	, mRadius{ 0.5f }
	, mHalfSegment{ 0.5f }
	, mAxis{ ECapsuleAxis::E_YAxis } {}

CCapsuleCollider::~CCapsuleCollider() {
	if (PHYSICS_MANAGER->IsColliderRegistered(this))
		PHYSICS_MANAGER->UnregisterCollider(this);
}

bool CCapsuleCollider::Initialize() {
	PHYSICS_MANAGER->RegisterCollider(this);

	return true;
}

bool CCapsuleCollider::Final() {
	return true;
}

bool CCapsuleCollider::OnLoaded() {
	if (!PHYSICS_MANAGER->IsColliderRegistered(this))
		PHYSICS_MANAGER->RegisterCollider(this);

	return true;
}

bool CCapsuleCollider::OnUnloaded() {
	if (PHYSICS_MANAGER->IsColliderRegistered(this))
		PHYSICS_MANAGER->UnregisterCollider(this);

	return true;
}

bool CCapsuleCollider::SaveToLevelFile(FILE* const pFile) {
	CheckReturn(CCollider::SaveToLevelFile(pFile));

	fwrite(&mRadius, sizeof(mRadius), 1, pFile);
	fwrite(&mHalfSegment, sizeof(mHalfSegment), 1, pFile);
	fwrite(&mAxis, sizeof(mAxis), 1, pFile);

	return true;
}

bool CCapsuleCollider::LoadFromLevelFile(FILE* const pFile) {
	CheckReturn(CCollider::LoadFromLevelFile(pFile));

	fread(&mRadius, sizeof(mRadius), 1, pFile);
	fread(&mHalfSegment, sizeof(mHalfSegment), 1, pFile);
	fread(&mAxis, sizeof(mAxis), 1, pFile);

	return true;
}