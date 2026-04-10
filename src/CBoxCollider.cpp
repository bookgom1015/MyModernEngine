#include "pch.h"
#include "CBoxCollider.hpp"

#include "PhysicsManager.hpp"

CBoxCollider::CBoxCollider() 
	: CCollider{ EComponent::E_BoxCollider, ECollider::E_Box }
	, mHalfExtents{ 0.5f } {}

CBoxCollider::~CBoxCollider() {
	if (PHYSICS_MANAGER->IsColliderRegistered(this))
		PHYSICS_MANAGER->UnregisterCollider(this);
}

bool CBoxCollider::Initialize() {
	PHYSICS_MANAGER->RegisterCollider(this);

	return true;
}

bool CBoxCollider::Final() {
	return true;
}

bool CBoxCollider::OnLoaded() {
	if (!PHYSICS_MANAGER->IsColliderRegistered(this))
		PHYSICS_MANAGER->RegisterCollider(this);

	return true;
}

bool CBoxCollider::OnUnloaded() {
	if (PHYSICS_MANAGER->IsColliderRegistered(this))
		PHYSICS_MANAGER->UnregisterCollider(this);

	return true;
}

bool CBoxCollider::SaveToLevelFile(FILE* const pFile) {
	CheckReturn(CCollider::SaveToLevelFile(pFile));

	fwrite(&mHalfExtents, sizeof(mHalfExtents), 1, pFile);

	return true;
}

bool CBoxCollider::LoadFromLevelFile(FILE* const pFile) {
	CheckReturn(CCollider::LoadFromLevelFile(pFile));

	fread(&mHalfExtents, sizeof(mHalfExtents), 1, pFile);

	return true;
}