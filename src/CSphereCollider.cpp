#include "pch.h"
#include "CSphereCollider.hpp"

#include "PhysicsManager.hpp"

CSphereCollider::CSphereCollider() 
	: CCollider(EComponent::E_SphereCollider, ECollider::E_Sphere)
	, mRadius{ 0.5f } {}

CSphereCollider::~CSphereCollider() {
	if (PHYSICS_MANAGER->IsColliderRegistered(this))
		PHYSICS_MANAGER->UnregisterCollider(this);
}

bool CSphereCollider::Initialize() {
	PHYSICS_MANAGER->RegisterCollider(this);

	return true;
}

bool CSphereCollider::Final() {
	return true;
}

bool CSphereCollider::OnLoaded() {
	if (PHYSICS_MANAGER->IsColliderRegistered(this)) return true;
	PHYSICS_MANAGER->RegisterCollider(this);

	return true;
}

bool CSphereCollider::OnUnloaded() {
	if (!PHYSICS_MANAGER->IsColliderRegistered(this)) return true;
	PHYSICS_MANAGER->UnregisterCollider(this);

	return true;
}

bool CSphereCollider::SaveToLevelFile(FILE* const pFile) {
	CheckReturn(CCollider::SaveToLevelFile(pFile));

	fwrite(&mRadius, sizeof(mRadius), 1, pFile);

	return true;
}

bool CSphereCollider::LoadFromLevelFile(FILE* const pFile) {
	CheckReturn(CCollider::LoadFromLevelFile(pFile));

	fread(&mRadius, sizeof(mRadius), 1, pFile);

	return true;
}