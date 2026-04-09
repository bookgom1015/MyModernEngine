#include "pch.h"
#include "CCollider.hpp"

CCollider::CCollider(EComponent::Type type, ECollider::Type colliderType)
    : Component(type)
    , mColliderType{ colliderType }
    , mOffset{ 0.f }
    , mbIsTrigger{ false } {}

CCollider::~CCollider() {}

bool CCollider::SaveToLevelFile(FILE* const pFile) {
	fwrite(&mColliderType, sizeof(mColliderType), 1, pFile);
    fwrite(&mOffset, sizeof(mOffset), 1, pFile);
	fwrite(&mbIsTrigger, sizeof(mbIsTrigger), 1, pFile);

    return true;
}

bool CCollider::LoadFromLevelFile(FILE* const pFile) {
    fread(&mColliderType, sizeof(mColliderType), 1, pFile);
    fread(&mOffset, sizeof(mOffset), 1, pFile);
    fread(&mbIsTrigger, sizeof(mbIsTrigger), 1, pFile);

    return true;
}