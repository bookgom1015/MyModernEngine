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
		CreateDynamicMaterial();
}

CRenderComponent::~CRenderComponent() {}

bool CRenderComponent::Initialize() {
	CheckReturn(CreateMaterial());

	return true;
}

bool CRenderComponent::Final() {

	return true;
}

bool CRenderComponent::OnMeshChanged() {
	return true;
}

bool CRenderComponent::OnMaterialChanged() {
	return true;
}

bool CRenderComponent::CreateDynamicMaterial() {
	if (mDynamicMaterial == nullptr) {
		mDynamicMaterial = mSharedMaterial->Clone();
	}

	mMaterial = mDynamicMaterial;

	return true;
}

bool CRenderComponent::SetMesh(Ptr<AMesh> mesh) noexcept { 
	mMesh = mesh;

	if (mMaterial == nullptr) {
		mMaterial = mSharedMaterial = LOAD(AMaterial, L"Default Material");

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

	mMaterial = mSharedMaterial = material; 
	CheckReturn(OnMaterialChanged());

	return true;
}

Vec3 CRenderComponent::GetAlbedo() const {
	if (mMaterial == nullptr) return Vec3(1.f);
	return mMaterial->GetAlbedo();
}

bool CRenderComponent::SetAlbedo(Vec3 albedo) {
	if (LEVEL_MANAGER->GetCurrentLevelState() == ELevelState::E_Playing) 
		CheckReturn(CreateDynamicMaterial());

	mMaterial->SetAlbedo(albedo);

	return true;
}

float CRenderComponent::GetRoughness() const {
	if (mMaterial == nullptr) return 0.5f;
	return mMaterial->GetRoughness();
}

bool CRenderComponent::SetRoughness(float roughness) {
	if (LEVEL_MANAGER->GetCurrentLevelState() == ELevelState::E_Playing)
		CheckReturn(CreateDynamicMaterial());

	mMaterial->SetRoughness(roughness);

	return true;
}

float CRenderComponent::GetMetalic() const {
	if (mMaterial == nullptr) return 0.f;
	return mMaterial->GetMetalic();
}

bool CRenderComponent::SetMetalic(float metalic) {
	if (LEVEL_MANAGER->GetCurrentLevelState() == ELevelState::E_Playing)
		CheckReturn(CreateDynamicMaterial());

	mMaterial->SetMetalic(metalic);

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

	return true;
}