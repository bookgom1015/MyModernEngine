#include "pch.h"
#include "CRenderComponent.hpp"

#include "AssetManager.hpp"
#include "LevelManager.hpp"
#include "EditorManager.hpp"
#include RENDERER_HEADER

CRenderComponent::CRenderComponent(EComponent::Type type) : Component{ type } {}

CRenderComponent::CRenderComponent(const CRenderComponent& other) 
	: Component{ other }
	, mMesh{ other.mMesh } {
	mMaterialSlots.resize(other.mMaterialSlots.size());

	for (size_t i = 0, end = other.mMaterialSlots.size(); i < end; ++i) {
		const auto materialSlot = other.mMaterialSlots[i];

		if (materialSlot.Material == materialSlot.SharedMaterial)
			mMaterialSlots[i].Material = materialSlot.SharedMaterial;
		else if (materialSlot.DynamicMaterial != nullptr 
			&& materialSlot.Material == materialSlot.DynamicMaterial)
			CreateDynamicMaterial(i);


		mMaterialSlots[i].SharedMaterial = other.mMaterialSlots[i].SharedMaterial;
		mMaterialSlots[i].DynamicMaterial = other.mMaterialSlots[i].DynamicMaterial;
	}
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

bool CRenderComponent::CreateDynamicMaterial(size_t index) {
	if (mMaterialSlots.empty()) {
		LOG_WARNING("Mesh is not set. Dynamic material cannot be created.");
		return true;
	}

	if (mMaterialSlots[index].DynamicMaterial == nullptr) 
		mMaterialSlots[index].DynamicMaterial = mMaterialSlots[index].SharedMaterial->Clone();

	mMaterialSlots[index].Material = mMaterialSlots[index].DynamicMaterial;

	return true;
}

bool CRenderComponent::SetMesh(Ptr<AMesh> mesh) noexcept { 
	mMesh = mesh;

	if (!mMaterialSlots.empty()) 
		mMaterialSlots.clear();

	const auto primCount = mMesh->GetStaticPrimitiveCount()
		+ mMesh->GetSkinnedPrimitiveCount();
	mMaterialSlots.resize(primCount);

	for (size_t i = 0; i < primCount; ++i)
		mMaterialSlots[i].Material = mMaterialSlots[i].SharedMaterial
		= LOAD(AMaterial, L"Default Material");

	CheckReturn(OnMaterialChanged());
	CheckReturn(OnMeshChanged());

	return true;
}

void CRenderComponent::GetMaterials(std::vector<Ptr<AMaterial>>& outMaterials) const noexcept {
	for (size_t i = 0, end = mMaterialSlots.size(); i < end; ++i)
		outMaterials.push_back(mMaterialSlots[i].Material);
}

bool CRenderComponent::SetMaterial(size_t index, Ptr<AMaterial> material) noexcept {
	if (mMesh == nullptr) {
		LOG_WARNING("Mesh is not set. Material cannot be applied.");
		return true;
	}

	mMaterialSlots[index].Material = mMaterialSlots[index].SharedMaterial = material;
	CheckReturn(OnMaterialChanged());

	return true;
}

Vec3 CRenderComponent::GetAlbedo(size_t index) const {
	if (mMaterialSlots.empty() || mMaterialSlots[index].Material == nullptr) return Vec3(1.f);
	return mMaterialSlots[index].Material->GetAlbedo();
}

bool CRenderComponent::SetAlbedo(size_t index, Vec3 albedo) {
	if (LEVEL_MANAGER->GetCurrentLevelState() == ELevelState::E_Playing) 
		CheckReturn(CreateDynamicMaterial(index));

	mMaterialSlots[index].Material->SetAlbedo(albedo);

	return true;
}

float CRenderComponent::GetRoughness(size_t index) const {
	if (mMaterialSlots.empty() || mMaterialSlots[index].Material == nullptr) return 0.5f;
	return mMaterialSlots[index].Material->GetRoughness();
}

bool CRenderComponent::SetRoughness(size_t index, float roughness) {
	if (LEVEL_MANAGER->GetCurrentLevelState() == ELevelState::E_Playing)
		CheckReturn(CreateDynamicMaterial(index));

	mMaterialSlots[index].Material->SetRoughness(roughness);

	return true;
}

float CRenderComponent::GetMetalic(size_t index) const {
	if (mMaterialSlots.empty() || mMaterialSlots[index].Material == nullptr) return 0.f;
	return mMaterialSlots[index].Material->GetMetalic();
}

bool CRenderComponent::SetMetalic(size_t index, float metalic) {
	if (LEVEL_MANAGER->GetCurrentLevelState() == ELevelState::E_Playing)
		CheckReturn(CreateDynamicMaterial(index));

	mMaterialSlots[index].Material->SetMetalic(metalic);

	return true;
}

float CRenderComponent::GetSpecular(size_t index) const {
	if (mMaterialSlots.empty() || mMaterialSlots[index].Material == nullptr) return 0.f;
	return mMaterialSlots[index].Material->GetSpecular();
}

bool CRenderComponent::SetSpecular(size_t index, float specular) {
	if (LEVEL_MANAGER->GetCurrentLevelState() == ELevelState::E_Playing)
		CheckReturn(CreateDynamicMaterial(index));

	mMaterialSlots[index].Material->SetSpecular(specular);

	return true;
}

Ptr<ATexture> CRenderComponent::GetAlbedoMap(size_t index) const {
	if (mMaterialSlots.empty() || mMaterialSlots[index].Material == nullptr) return nullptr;
	return mMaterialSlots[index].Material->GetAlbedoMap();
}

bool CRenderComponent::SetAlbedoMap(size_t index, Ptr<ATexture> albedoMap) {
	if (LEVEL_MANAGER->GetCurrentLevelState() == ELevelState::E_Playing)
		CheckReturn(CreateDynamicMaterial(index));

	mMaterialSlots[index].Material->SetAlbedoMap(albedoMap);

	return true;
}

Ptr<ATexture> CRenderComponent::GetNormalMap(size_t index) const {
	if (mMaterialSlots.empty() || mMaterialSlots[0].Material == nullptr) return nullptr;
	return mMaterialSlots[0].Material->GetNormalMap();
}

bool CRenderComponent::SetNormalMap(size_t index, Ptr<ATexture> normalMap) {
	if (LEVEL_MANAGER->GetCurrentLevelState() == ELevelState::E_Playing)
		CheckReturn(CreateDynamicMaterial(index));

	mMaterialSlots[index].Material->SetNormalMap(normalMap);

	return true;
}

bool CRenderComponent::SaveToLevelFile(FILE* const pFile) {
	SaveAssetRef(pFile, mMesh.Get());

	for (size_t i = 0, end = mMesh.Get()->GetStaticPrimitiveCount(); i < end; ++i) {
		SaveAssetRef(pFile, mMaterialSlots[i].Material.Get());
		SaveAssetRef(pFile, mMaterialSlots[i].SharedMaterial.Get());
	}

	for (size_t i = 0, end = mMesh.Get()->GetSkinnedPrimitiveCount(); i < end; ++i) {
		SaveAssetRef(pFile, mMaterialSlots[i].Material.Get());
		SaveAssetRef(pFile, mMaterialSlots[i].SharedMaterial.Get());
	}

	return true;
}

bool CRenderComponent::LoadFromLevelFile(FILE* const pFile) {
	mMesh = LoadAssetRef<AMesh>(pFile);

	const auto primCount = mMesh.Get()->GetStaticPrimitiveCount() 
		+ mMesh.Get()->GetSkinnedPrimitiveCount();
	mMaterialSlots.resize(primCount);

	for (size_t i = 0; i < primCount; ++i) {
		mMaterialSlots[i].Material = LoadAssetRef<AMaterial>(pFile);
		mMaterialSlots[i].SharedMaterial = LoadAssetRef<AMaterial>(pFile);
	}

	return true;
}