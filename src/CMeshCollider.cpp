#include "pch.h"
#include "CMeshCollider.hpp"

#include "AssetManager.hpp"

CMeshCollider::CMeshCollider() 
	: CCollider(EComponent::E_MeshCollider, ECollider::E_Mesh)
	, mMeshCollisionType{ EMeshCollision::E_Triangle }
	, mMesh{} {}

CMeshCollider::~CMeshCollider() {}

bool CMeshCollider::Final() {
	return true;
}

bool CMeshCollider::SaveToLevelFile(FILE* const pFile) {
	CheckReturn(CCollider::SaveToLevelFile(pFile));

	fwrite(&mMeshCollisionType, sizeof(mMeshCollisionType), 1, pFile);

	SaveAssetRef(pFile, mMesh.Get());

	return true;
}

bool CMeshCollider::LoadFromLevelFile(FILE* const pFile) {
	CheckReturn(CCollider::LoadFromLevelFile(pFile));

	fread(&mMeshCollisionType, sizeof(mMeshCollisionType), 1, pFile);

	mMesh = LoadAssetRef<AMesh>(pFile);

	return true;
}