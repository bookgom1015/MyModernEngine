#pragma once

#include "Component.hpp"

#include "AMesh.hpp"
#include "AMaterial.hpp"

class CRenderComponent : public Component {
private:
	struct MaterialSlot	{
		Ptr<AMaterial> Material;
		Ptr<AMaterial> SharedMaterial;
		Ptr<AMaterial> DynamicMaterial;
	};

public:
	CRenderComponent(EComponent::Type type);
	CRenderComponent(const CRenderComponent& other);
	virtual ~CRenderComponent();

public:
	virtual bool Initialize() override;	
	virtual bool Final() override;

	virtual bool CreateMaterial() = 0;

public:
	bool CreateDynamicMaterial(size_t index);

public:
	__forceinline Ptr<AMesh> GetMesh() const noexcept;
	bool SetMesh(Ptr<AMesh> mesh) noexcept;

	__forceinline Ptr<AMaterial> GetMaterial(size_t index) const noexcept;
	void GetMaterials(std::vector<Ptr<AMaterial>>& outMaterials) const noexcept;
	bool SetMaterial(size_t index, Ptr<AMaterial> material) noexcept;

	Vec3 GetAlbedo(size_t index) const;
	bool SetAlbedo(size_t index, Vec3 albedo);

	float GetRoughness(size_t index) const;
	bool SetRoughness(size_t index, float roughness);

	float GetMetalic(size_t index) const;
	bool SetMetalic(size_t index, float metalic);

	float GetSpecular(size_t index) const;
	bool SetSpecular(size_t index, float specular);

	Ptr<ATexture> GetAlbedoMap(size_t index) const;
	bool SetAlbedoMap(size_t index, Ptr<ATexture> albedoMap);

	Ptr<ATexture> GetNormalMap(size_t index) const;
	bool SetNormalMap(size_t index, Ptr<ATexture> normalMap);

public:
	virtual bool SaveToLevelFile(FILE* const pFile) override;
	virtual bool LoadFromLevelFile(FILE* const pFile) override;

private:
	Ptr<AMesh> mMesh;

	std::vector<MaterialSlot> mMaterialSlots;

};

#include "CRenderComponent.inl"