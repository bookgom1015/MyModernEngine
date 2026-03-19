#include "pch.h"
#include "CRenderComponent.hpp"

#include "AssetManager.hpp"
#include "LevelManager.hpp"

CRenderComponent::CRenderComponent(EComponent::Type type) : Component{ type } {}

CRenderComponent::CRenderComponent(const CRenderComponent& other) 
	: Component{ other }
	, mMesh{ other.mMesh }
	, mSharedMaterial{ other.mSharedMaterial } {
	if (other.mMaterial == other.mSharedMaterial)
		mMaterial = mSharedMaterial;
	else if (other.mDynamicMaterial != nullptr && other.mMaterial == other.mDynamicMaterial) 
		mMaterial = other.mMaterial;
}

CRenderComponent::~CRenderComponent() {}

bool CRenderComponent::Initialize() {
	CheckReturn(CreateMaterial());

	return true;
}

Ptr<AMaterial> CRenderComponent::CreateDynamicMaterial() {
	assert(LEVEL_MANAGER->GetLevelState() == ELevelState::E_Playing);

	if (mDynamicMaterial != nullptr) {
		mMaterial = mDynamicMaterial;
		return mDynamicMaterial;
	}
	else {
		mMaterial = mDynamicMaterial = mSharedMaterial->Clone();
		return mDynamicMaterial;
	}

	return nullptr;
}

bool CRenderComponent::SaveToLevelFile(FILE* const pFile) {
	SaveAssetRef(pFile, mMesh.Get());
	SaveAssetRef(pFile, mMaterial.Get());
	SaveAssetRef(pFile, mSharedMaterial.Get());

	return true;
}

bool CRenderComponent::LoadFromLevelFile(FILE* const pFile) {
	mMesh = LoadAssetRef<AMesh>(pFile);
	mMaterial = LoadAssetRef<AMaterial>(pFile);
	mSharedMaterial = LoadAssetRef<AMaterial>(pFile);

	return true;
}