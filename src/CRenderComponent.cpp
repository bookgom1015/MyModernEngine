#include "pch.h"
#include "CRenderComponent.hpp"

#include "AssetManager.hpp"
#include "LevelManager.hpp"
#include "EditorManager.hpp"
#include RENDERER_HEADER

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

bool CRenderComponent::Final() {
	if (GetMesh() != nullptr && Transform()->IsChanged()) {
		CheckReturn(RENDERER->UpdateRenderItemTransform(GetOwner()->GetName(), Transform()));

		Transform()->ReflectedChanges();
	}

	return true;
}

bool CRenderComponent::OnMeshChanged() {
	return true;
}

bool CRenderComponent::OnMaterialChanged() {
	return true;
}

Ptr<AMaterial> CRenderComponent::CreateDynamicMaterial() {
	assert(LEVEL_MANAGER->GetCurrentLevelState() == ELevelState::E_Playing);

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

bool CRenderComponent::SetMesh(Ptr<AMesh> mesh) noexcept { 
	mMesh = mesh;

	CheckReturn(RENDERER->RegisterRenderItem(GetOwner()->GetName(), GetMesh()->GetKey(), L""));

	if (mMaterial == nullptr) {
		mMaterial = LOAD(AMaterial, L"Default Material");
		CheckReturn(OnMaterialChanged());
	}

	CheckReturn(OnMeshChanged());

	return true;
}

bool CRenderComponent::SetMaterial(Ptr<AMaterial> material) noexcept {
	if (mMesh == nullptr) {
		LOG_WARNING("Mesh is not set. Material cannot be applied.");
		return true;
	}

	mMaterial = material; 
	CheckReturn(OnMaterialChanged());

	return true;
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

	if (GetMesh() != nullptr) 
		CheckReturn(RENDERER->RegisterRenderItem(GetOwner()->GetName(), GetMesh()->GetKey(), L""));

	return true;
}